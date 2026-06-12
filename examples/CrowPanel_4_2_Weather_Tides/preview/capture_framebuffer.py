#!/usr/bin/env python3
import argparse
import struct
import re
import sys
import time
import zlib
from pathlib import Path

import serial


ROOT = Path(__file__).resolve().parents[1]
LOCAL_CREDENTIALS = ROOT / "owm_credentials_local.h"


def load_redactions() -> list[tuple[str, str]]:
    if not LOCAL_CREDENTIALS.exists():
        return []
    text = LOCAL_CREDENTIALS.read_text(errors="replace")
    redactions: list[tuple[str, str]] = []
    for name in ("ssid", "password", "apikey"):
        match = re.search(
            rf'(?:const char\*?\s+|String\s+){name}\s*(?:\[\])?\s*=\s*"([^"]*)"',
            text,
        )
        if match and match.group(1):
            redactions.append((match.group(1), f"<{name.upper()}_REDACTED>"))
    return redactions


def scrub(line: str, redactions: list[tuple[str, str]]) -> str:
    for value, replacement in redactions:
        line = line.replace(value, replacement)
    return line


def parse_begin(line: str) -> dict[str, str]:
    metadata: dict[str, str] = {}
    for key, value in re.findall(r"(\w+)=([^\s]+)", line):
        metadata[key] = value
    return metadata


def framebuffer_to_pixels(data: bytes, width: int, height: int) -> list[bytearray]:
    row_bytes = (width + 7) // 8
    expected = row_bytes * height
    if len(data) != expected:
        raise ValueError(f"expected {expected} framebuffer bytes, got {len(data)}")

    rows: list[bytearray] = []
    for y in range(height):
        row = bytearray(width)
        row_start = y * row_bytes
        for x in range(width):
            byte = data[row_start + x // 8]
            bit = (byte >> (7 - (x & 7))) & 1
            row[x] = 255 if bit else 0
        rows.append(row)
    return rows


def write_png(path: Path, rows: list[bytearray], width: int, height: int) -> None:
    def chunk(kind: bytes, payload: bytes) -> bytes:
        crc = zlib.crc32(kind + payload) & 0xFFFFFFFF
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", crc)

    raw = b"".join(b"\x00" + bytes(row) for row in rows)
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 0, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    path.write_bytes(png)


def scale_rows(rows: list[bytearray], scale: int) -> list[bytearray]:
    if scale <= 1:
        return rows
    scaled: list[bytearray] = []
    for row in rows:
        wide = bytearray()
        for pixel in row:
            wide.extend([pixel] * scale)
        for _ in range(scale):
            scaled.append(bytearray(wide))
    return scaled


def capture_all(port: str, baud: int, timeout: float, reset: bool, quiet: bool) -> list[tuple[dict[str, str], bytes]]:
    redactions = load_redactions()
    captures: list[tuple[dict[str, str], bytes]] = []
    with serial.Serial(port, baud, timeout=0.25) as ser:
        if reset:
            ser.dtr = False
            ser.rts = True
            time.sleep(0.12)
            ser.rts = False
            time.sleep(0.25)

        deadline = time.time() + timeout
        metadata: dict[str, str] | None = None
        hex_lines: list[str] = []
        collecting = False
        buffer = b""

        while time.time() < deadline:
            chunk = ser.read(512)
            if not chunk:
                time.sleep(0.05)
                continue
            buffer += chunk
            while b"\n" in buffer:
                raw_line, buffer = buffer.split(b"\n", 1)
                line = raw_line.decode("utf-8", "replace").strip()
                if not line:
                    continue
                if line.startswith("FB_DUMP_BEGIN"):
                    metadata = parse_begin(line)
                    hex_lines = []
                    collecting = True
                    if not quiet:
                        print(line)
                    continue
                if line.startswith("FB_GALLERY_END") and captures:
                    if not quiet:
                        print(line)
                    return captures
                if line.startswith("FB_DUMP_END") and collecting:
                    if not quiet:
                        print(line)
                    captures.append((metadata or {}, bytes.fromhex("".join(hex_lines))))
                    metadata = None
                    hex_lines = []
                    collecting = False
                    continue
                if collecting:
                    if re.fullmatch(r"[0-9A-Fa-f]+", line):
                        hex_lines.append(line)
                    continue
                if not quiet:
                    print(scrub(line, redactions))

    if captures:
        return captures
    raise TimeoutError(f"no complete FB_DUMP block received from {port} within {timeout:.0f}s")


def capture(port: str, baud: int, timeout: float, reset: bool, quiet: bool) -> tuple[dict[str, str], bytes]:
    return capture_all(port, baud, timeout, reset, quiet)[0]


def safe_label(label: str) -> str:
    label = re.sub(r"[^A-Za-z0-9._-]+", "-", label.strip())
    return label or "framebuffer"


def main() -> None:
    parser = argparse.ArgumentParser(description="Capture a CrowPanel GFXcanvas1 framebuffer dump over serial.")
    parser.add_argument("--port", default="/dev/cu.usbserial-110")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=120)
    parser.add_argument("--output", type=Path, default=ROOT / "preview" / "framebuffer-capture.png")
    parser.add_argument("--scale", type=int, default=2)
    parser.add_argument("--all", action="store_true", help="capture every FB_DUMP block before timeout and write one PNG per label")
    parser.add_argument("--no-reset", action="store_true")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()

    captures = capture_all(args.port, args.baud, args.timeout, not args.no_reset, args.quiet) if args.all else [capture(args.port, args.baud, args.timeout, not args.no_reset, args.quiet)]
    for index, (metadata, data) in enumerate(captures):
        width = int(metadata.get("width", "400"))
        height = int(metadata.get("height", "300"))
        rows = framebuffer_to_pixels(data, width, height)

        if args.all:
            label = safe_label(metadata.get("label", f"framebuffer-{index + 1:02d}"))
            output = args.output.with_name(f"{args.output.stem}-{index + 1:02d}-{label}{args.output.suffix}")
        else:
            output = args.output

        raw_output = output.with_name(output.stem + "-raw-1bit" + output.suffix)
        write_png(raw_output, rows, width, height)
        write_png(output, scale_rows(rows, args.scale), width * args.scale, height * args.scale)

        print(f"wrote {raw_output}")
        print(f"wrote {output}")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"capture failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
