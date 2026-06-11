# CrowPanel 4.2 Display Diagnostic

This is a hardware bring-up sketch for the Elecrow CrowPanel ESP32-S3 4.2-inch e-paper display.

It intentionally avoids GxEPD2 and uses Elecrow's newer "green circular sticker" command sequence with a 1-bit Adafruit GFX canvas. Use it before flashing the full weather/tide sketch to confirm:

- USB serial upload and reset work.
- Display power control works on GPIO7 and GPIO41.
- E-paper pins are correct: SCK 12, MOSI 11, CS 45, DC 46, RST 47, BUSY 48.
- A full-screen 400x300 framebuffer can be sent to the panel.

## Run

```sh
../../.venv-platformio/bin/platformio run -t upload
```

Then monitor at 115200 baud. A successful serial run reaches:

```text
Display diagnostic complete
Panel power off; image should persist if refresh succeeded
```

The screen should show a bordered "CrowPanel 4.2 / Elecrow green-driver diag" test page.
