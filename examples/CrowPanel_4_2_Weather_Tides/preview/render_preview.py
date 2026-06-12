#!/usr/bin/env python3
"""Render a 1-bit desktop preview of the CrowPanel 4.2 weather layout."""

from __future__ import annotations

import argparse
import json
import math
import re
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


SCREEN_WIDTH = 400
SCREEN_HEIGHT = 300
BLACK = 0
WHITE = 255

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_FIXTURE = Path(__file__).with_name("fixture.json")
DEFAULT_OUTPUT = Path(__file__).with_name("software-render.png")
DEFAULT_RAW_OUTPUT = Path(__file__).with_name("software-render-raw-1bit.png")
LOCAL_TIDES = ROOT / "local_tides.h"

FONT_REGULAR = Path("/System/Library/Fonts/Supplemental/Arial.ttf")
FONT_BOLD = Path("/System/Library/Fonts/Supplemental/Arial Bold.ttf")


def load_font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    font_path = FONT_BOLD if bold else FONT_REGULAR
    if font_path.exists():
        return ImageFont.truetype(str(font_path), size)
    return ImageFont.load_default()


FONTS = {
    "B08": load_font(9, True),
    "R08": load_font(9, False),
    "B10": load_font(11, True),
    "R10": load_font(11, False),
    "B12": load_font(13, True),
    "B14": load_font(17, True),
    "B18": load_font(22, True),
    "B24": load_font(30, True),
}


class Canvas:
    def __init__(self) -> None:
        self.image = Image.new("L", (SCREEN_WIDTH, SCREEN_HEIGHT), WHITE)
        self.draw = ImageDraw.Draw(self.image)

    def line(self, xy, width: int = 1) -> None:
        self.draw.line(xy, fill=BLACK, width=width)

    def rect(self, x: int, y: int, w: int, h: int, fill: int | None = None) -> None:
        self.draw.rectangle((x, y, x + w, y + h), outline=BLACK, fill=fill)

    def circle(self, x: int, y: int, r: int, fill: int | None = None, width: int = 1) -> None:
        self.draw.ellipse((x - r, y - r, x + r, y + r), outline=BLACK, fill=fill, width=width)

    def polygon(self, points, fill: int = BLACK) -> None:
        self.draw.polygon(points, fill=fill)

    def text(self, x: int, y: int, value: str, font_key: str = "B08", align: str = "left") -> None:
        font = FONTS[font_key]
        bbox = self.draw.textbbox((0, 0), value, font=font)
        w = bbox[2] - bbox[0]
        if align == "center":
            x -= w // 2
        elif align == "right":
            x -= w
        self.draw.text((x, y), value, font=font, fill=BLACK)

    def fast_hline(self, x: int, y: int, w: int) -> None:
        self.line((x, y, x + w, y))


def parse_tide_samples(path: Path) -> list[tuple[float, float]]:
    text = path.read_text()
    match = re.search(r"LocalTideSamples\[[^\]]*\]\s*=\s*\{(?P<body>.*?)\};", text, re.S)
    if not match:
        return []
    samples: list[tuple[float, float]] = []
    for hour, height in re.findall(r"\{\s*([0-9.]+)\s*,\s*([0-9.]+)\s*\}", match.group("body")):
        samples.append((float(hour), float(height)))
    return samples


def tide_height_at_hour(samples: list[tuple[float, float]], hour: float) -> float:
    if not samples:
        return 0.0
    if hour <= samples[0][0]:
        return samples[0][1]
    for idx in range(1, len(samples)):
        prev_hour, prev_height = samples[idx - 1]
        next_hour, next_height = samples[idx]
        if hour <= next_hour:
            span = next_hour - prev_hour
            if span <= 0:
                return next_height
            ratio = (hour - prev_hour) / span
            return prev_height + ratio * (next_height - prev_height)
    return samples[-1][1]


def tide_rising(samples: list[tuple[int, float]], hour: float) -> bool:
    return tide_height_at_hour(samples, min(hour + 0.5, 24)) >= tide_height_at_hour(samples, hour)


def wind_direction(degrees: float) -> str:
    names = ["N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"]
    return names[int((degrees / 22.5) + 0.5) % 16]


def draw_heading(c: Canvas, data: dict) -> None:
    c.text(4, 1, data["time"], "B08")
    c.text(SCREEN_WIDTH // 2, 1, data["city"], "B08", "center")
    c.text(SCREEN_WIDTH - 2, 1, data["date"], "R08", "right")
    draw_battery(c, 65, 12, data.get("battery", 100))
    c.line((0, 12, SCREEN_WIDTH, 12))


def draw_battery(c: Canvas, x: int, y: int, percentage: int) -> None:
    c.rect(x + 15, y - 12, 19, 10)
    c.rect(x + 34, y - 10, 2, 5, BLACK)
    fill_w = int(15 * max(0, min(100, percentage)) / 100.0)
    if fill_w:
        c.rect(x + 17, y - 10, fill_w, 6, BLACK)
    c.text(x + 65, y - 11, f"{percentage}%", "B08", "right")


def draw_wind_section(c: Canvas, x: int, y: int, angle: float, speed: float, radius: int, units: str) -> None:
    arrow(c, x, y, radius - 7, angle, 12, 18)
    c.line((0, 15, 0, y + radius + 30))
    c.circle(x, y, radius)
    c.circle(x, y, radius + 1)
    c.circle(x, y, int(radius * 0.7))
    for a in [i * 22.5 for i in range(16)]:
        dxo = radius * math.cos(math.radians(a - 90))
        dyo = radius * math.sin(math.radians(a - 90))
        dxi = dxo * 0.9
        dyi = dyo * 0.9
        c.line((x + dxo, y + dyo, x + dxi, y + dyi))
        dxo2 = dxo * 0.7
        dyo2 = dyo * 0.7
        c.line((x + dxo2, y + dyo2, x + dxo2 * 0.9, y + dyo2 * 0.9))
    c.text(x, y - radius - 12, "N", "B08", "center")
    c.text(x, y + radius + 5, "S", "B08", "center")
    c.text(x - radius - 10, y - 5, "W", "B08", "center")
    c.text(x + radius + 8, y - 5, "E", "B08", "center")
    c.text(x - 2, y - 24, wind_direction(angle), "B08", "center")
    c.text(x + 3, y + 8, f"{angle:.0f}°", "B08", "center")
    c.text(x + 3, y - 8, f"{speed:.0f}", "B18", "center")
    c.text(x + 3, y + 15, "m/s" if units == "M" else "mph", "B08", "center")


def arrow(c: Canvas, x: int, y: int, size: int, angle: float, width: int, length: int) -> None:
    dx = (size + 28) * math.cos(math.radians(angle - 90)) + x
    dy = (size + 28) * math.sin(math.radians(angle - 90)) + y
    base = [(0, length), (width / 2, width / 2), (-width / 2, width / 2)]
    theta = math.radians(angle)
    points = []
    for px, py in base:
        points.append((px * math.cos(theta) - py * math.sin(theta) + dx, py * math.cos(theta) + px * math.sin(theta) + dy))
    c.polygon(points)


def draw_weather_icon(c: Canvas, x: int, y: int, icon: str, large: bool) -> None:
    scale = 2 if large else 1
    if icon.startswith("01"):
        draw_sun(c, x, y, 12 * scale)
    elif icon.startswith("10") or icon.startswith("09"):
        draw_cloud(c, x, y, scale)
        for offset in (-10, 0, 10):
            c.line((x + offset, y + 16 * scale, x + offset - 4, y + 24 * scale))
    elif icon.startswith("13"):
        draw_cloud(c, x, y, scale)
        c.text(x, y + 10 * scale, "*", "B12", "center")
    else:
        draw_cloud(c, x, y, scale)


def draw_sun(c: Canvas, x: int, y: int, r: int) -> None:
    c.circle(x, y, r, fill=None, width=2)
    for a in range(0, 360, 45):
        sx = x + int((r + 4) * math.cos(math.radians(a)))
        sy = y + int((r + 4) * math.sin(math.radians(a)))
        ex = x + int((r + 12) * math.cos(math.radians(a)))
        ey = y + int((r + 12) * math.sin(math.radians(a)))
        c.line((sx, sy, ex, ey))


def draw_cloud(c: Canvas, x: int, y: int, scale: int) -> None:
    s = 8 * scale
    c.circle(x - s, y, s, fill=WHITE, width=2)
    c.circle(x, y - s // 2, int(s * 1.2), fill=WHITE, width=2)
    c.circle(x + s, y, s, fill=WHITE, width=2)
    c.rect(x - 2 * s, y, 4 * s, s, WHITE)
    c.line((x - 2 * s, y + s, x + 2 * s, y + s), width=2)


def draw_main_weather(c: Canvas, data: dict, tide_samples: list[tuple[int, float]]) -> None:
    w = data["weather"]
    units = data["units"]
    x, y = 172, 70
    draw_wind_section(c, x - 115, y - 3, w["wind_dir"], w["wind_speed"], 40, units)
    draw_weather_icon(c, x + 5, y - 5, w["icon"], True)
    draw_current_tide_status(c, x - 120, y + 58, data, tide_samples)
    c.text(x - 25, y + 37, f"{w['temperature']:.0f}°{'C' if units == 'M' else 'F'}", "B14", "center")
    c.text(x - 15, y + 57, f"{w['high']:.0f}° | {w['low']:.0f}°", "B12", "center")
    c.text(x + 37, y + 37, f"{w['humidity']:.0f}%", "B12", "center")
    c.text(x + 39, y + 57, "RH", "B10", "center")
    c.rect(0, y + 68, 232, 48)
    draw_tide_24h_graph(c, 0, y + 68, 232, 48, data, tide_samples)


def draw_current_tide_status(c: Canvas, x: int, y: int, data: dict, tide_samples: list[tuple[int, float]]) -> None:
    hour = data["current_hour"] + data["current_min"] / 60
    height = tide_height_at_hour(tide_samples, hour)
    c.text(x - 4, y - 4, f"{height:.1f}ft", "B12", "center")
    draw_tide_arrow(c, x + 36, y + 5, tide_rising(tide_samples, hour))


def draw_tide_arrow(c: Canvas, x: int, y: int, rising: bool) -> None:
    if rising:
        c.polygon([(x, y - 6), (x - 4, y + 1), (x + 4, y + 1)])
        c.rect(x - 1, y + 1, 3, 6, BLACK)
    else:
        c.polygon([(x, y + 6), (x - 4, y - 1), (x + 4, y - 1)])
        c.rect(x - 1, y - 7, 3, 6, BLACK)


def draw_tide_24h_graph(c: Canvas, x: int, y: int, w: int, h: int, data: dict, tide_samples: list[tuple[int, float]]) -> None:
    if not tide_samples:
        c.text(x + w // 2, y + h // 2 - 5, "No tide data", "B08", "center")
        return
    heights = [height for _, height in tide_samples]
    min_h, max_h = min(heights), max(heights)
    if max_h - min_h < 0.1:
        max_h += 0.1
        min_h -= 0.1
    graph_x, graph_y = x + 8, y + 13
    graph_w, graph_h = w - 16, h - 24
    hour_now = data["current_hour"] + data["current_min"] / 60
    c.text(x + 6, y + 2, "24h Tide", "B08")
    c.text(x + w - 6, y + 2, f"{tide_height_at_hour(tide_samples, hour_now):.1f}ft", "B08", "right")
    c.rect(graph_x, graph_y, graph_w, graph_h)
    points = []
    for hour in range(25):
        height = tide_height_at_hour(tide_samples, hour)
        px = graph_x + int(hour * graph_w / 24)
        py = graph_y + graph_h - 2 - int((height - min_h) * (graph_h - 4) / (max_h - min_h))
        points.append((px, py))
    for p1, p2 in zip(points, points[1:]):
        c.line((*p1, *p2))
    for hour in range(0, 25, 6):
        tx = graph_x + int(hour * graph_w / 24)
        c.line((tx, graph_y + graph_h, tx, graph_y + graph_h + 3))
        c.text(tx, graph_y + graph_h + 3, str(hour), "R08", "center")
    now_x = graph_x + int(data["current_hour"] * graph_w / 24)
    c.line((now_x, graph_y, now_x, graph_y + graph_h))


def draw_forecast_section(c: Canvas, data: dict) -> None:
    x, y = 233, 15
    for idx, daily in enumerate(data["daily"][:3]):
        draw_forecast_box(c, x + idx * 56, y, daily)
    c.line((0, y + 172, SCREEN_WIDTH, y + 172))
    hourly = data["hourly"]
    units = data["units"]
    draw_graph(c, 30, 221, SCREEN_WIDTH // 4, SCREEN_HEIGHT // 5, 0, 30, "Wind (m/s)" if units == "M" else "Wind (mph)", hourly["wind"], True, False)
    draw_graph(c, 158, 221, SCREEN_WIDTH // 4, SCREEN_HEIGHT // 5, 10, 30, "Temperature (°F)" if units == "I" else "Temperature (°C)", hourly["temperature"], True, False)
    draw_graph(c, 288, 221, SCREEN_WIDTH // 4, SCREEN_HEIGHT // 5, 0, 30, "Rainfall (in)" if units == "I" else "Rainfall (mm)", hourly["rain"], True, True)


def draw_forecast_box(c: Canvas, x: int, y: int, daily: dict) -> None:
    c.rect(x, y, 55, 65)
    c.line((x + 1, y + 13, x + 54, y + 13))
    c.text(x + 31, y + 3, daily["time"], "B08", "center")
    draw_weather_icon(c, x + 28, y + 35, daily["icon"], False)
    c.text(x + 41, y + 52, f"{daily['high']:.0f}° / {daily['low']:.0f}°", "B08", "center")


def draw_precipitation(c: Canvas, data: dict) -> None:
    x, y = 233, 82
    w = data["weather"]
    c.rect(x, y - 1, 167, 56)
    if w["rain_next"] > 0.005:
        c.text(x + 5, y + 15, f"{w['rain_next']:.2f}{'in' if data['units'] == 'I' else 'mm'}", "B10")
    else:
        c.text(x + 5, y + 15, "0.00in" if data["units"] == "I" else "0.00mm", "B10")
    if w["visibility"] > 0:
        visibility = f"{w['visibility']:.0f}M" if data["units"] == "M" else f"{w['visibility']:.1f}mi"
        draw_visibility(c, 335, 100, visibility)
    if w["cloud_cover"] > 0:
        draw_cloud_cover(c, 350, 125, w["cloud_cover"])


def draw_cloud_cover(c: Canvas, x: int, y: int, cover: int) -> None:
    draw_cloud(c, x, y, 1)
    c.text(x + 15, y - 5, f"{cover}%", "B08")


def draw_visibility(c: Canvas, x: int, y: int, value: str) -> None:
    y -= 3
    c.draw.arc((x - 10, y - 8, x + 10, y + 8), 200, 340, fill=BLACK, width=1)
    c.draw.arc((x - 10, y - 2, x + 10, y + 14), 20, 160, fill=BLACK, width=1)
    c.circle(x, y, 2, BLACK)
    c.text(x + 12, y - 5, value, "B08")


def draw_astronomy(c: Canvas, data: dict) -> None:
    x, y = 233, 74
    w = data["weather"]
    c.rect(x, y + 64, 167, 48)
    c.text(x + 7, y + 70, f"{w['sunrise']} Sunrise", "B08")
    c.text(x + 7, y + 85, f"{w['sunset']} Sunset", "B08")
    c.text(x + 7, y + 100, w["moon_phase"], "B08")
    draw_moon(c, x + 105, y + 50)


def draw_moon(c: Canvas, x: int, y: int) -> None:
    diameter = 38
    cx, cy = x + diameter - 1, y + diameter
    c.circle(cx, cy, diameter // 2 + 1, BLACK)
    c.circle(cx + 7, cy, diameter // 2 + 1, WHITE)
    c.circle(cx, cy, diameter // 2 + 1)


def draw_graph(c: Canvas, x: int, y: int, w: int, h: int, y_min: float, y_max: float, title: str, data: list[float], autoscale: bool, bars: bool) -> None:
    readings = len(data)
    if autoscale and readings > 1:
        y_max = round(max(data[1:]) + 0.5)
        y_min = round(min(data[1:]))
        if y_max == y_min:
            y_max += 1
            y_min -= 1
    c.rect(x, y, w + 3, h + 2)
    c.text(x + w // 2, y - 13, title, "B08", "center")
    last_x = x
    last_y = y + int((y_max - max(y_min, min(y_max, data[1]))) / (y_max - y_min) * h)
    for idx, value in enumerate(data):
        value = max(y_min, min(y_max, value))
        y2 = y + int((y_max - value) / (y_max - y_min) * h) + 1
        if bars:
            x2 = x + idx * (w // readings) + 2
            c.rect(x2, y2, max(1, (w // readings) - 2), y + h - y2 + 2, BLACK)
        else:
            x2 = x + int(idx * w / (readings - 1)) + 1
            c.line((last_x, last_y, x2, y2))
        last_x, last_y = x2, y2
    for spacing in range(6):
        gy = y + int(h * spacing / 5)
        if spacing < 5:
            for j in range(15):
                c.fast_hline(x + 3 + int(j * w / 15), gy, max(1, w // 30))
        label = y_max - (y_max - y_min) / 5 * spacing
        c.text(x - 3, gy - 5, f"{label:.1f}" if abs(label) < 5 else f"{label:.0f}", "R08", "right")
    for idx in range(3):
        c.text(15 + x + w // 3 * idx, y + h + 3, str(idx), "R08")
    c.text(x + w // 2, y + h + 10, "(Days)", "R08", "center")


def render(data: dict, tide_samples: list[tuple[int, float]]) -> Image.Image:
    c = Canvas()
    draw_heading(c, data)
    draw_main_weather(c, data, tide_samples)
    draw_forecast_section(c, data)
    draw_precipitation(c, data)
    draw_astronomy(c, data)
    return c.image.convert("1")


def main() -> None:
    parser = argparse.ArgumentParser(description="Render a 1-bit preview of the CrowPanel weather display.")
    parser.add_argument("--fixture", type=Path, default=DEFAULT_FIXTURE)
    parser.add_argument("--tides", type=Path, default=LOCAL_TIDES)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--raw-output", type=Path, default=DEFAULT_RAW_OUTPUT)
    parser.add_argument("--scale", type=int, default=2)
    args = parser.parse_args()

    data = json.loads(args.fixture.read_text())
    tide_samples = parse_tide_samples(args.tides)
    raw = render(data, tide_samples)
    args.raw_output.parent.mkdir(parents=True, exist_ok=True)
    raw.save(args.raw_output)
    scaled = raw.resize((SCREEN_WIDTH * args.scale, SCREEN_HEIGHT * args.scale), Image.Resampling.NEAREST)
    scaled.save(args.output)
    print(f"wrote {args.raw_output}")
    print(f"wrote {args.output}")


if __name__ == "__main__":
    main()
