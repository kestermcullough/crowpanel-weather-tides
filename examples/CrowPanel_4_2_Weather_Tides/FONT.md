# Font Notes

## Current Preference

For whole-display typography, current preference from on-device tests:

1. X11 8x13
2. ProFont
3. Courier
4. Helvetica as the default/fallback reference

X11 9x15 looked promising in the framebuffer gallery but was too large on the physical display. X11 8x13 is the current favorite, but it probably needs mixed larger-font treatment for important numeric areas. t0 terminal looked worse on-device.

## Current Test Build

The normal firmware is currently mapped to ProFont for a comparison pass:

- `UI_FONT_08`: `u8g2_font_profont11_tf`
- `UI_FONT_10`: `u8g2_font_profont12_tf`
- `UI_FONT_12`: `u8g2_font_profont15_tf`
- `UI_FONT_14`: `u8g2_font_profont22_tf`
- `UI_FONT_24`: `u8g2_font_profont29_tf`

Switching between families is centralized in `FontGalleryFont()` / `SetUIFont()`.

## X11 Size Options

U8g2 bitmap fonts are not continuously scalable. To make X11 smaller, choose a smaller fixed bitmap font rather than trying to scale `9x15`.

Available X11-style candidates in the bundled U8g2 font file include:

- `u8g2_font_5x7_tf`
- `u8g2_font_5x8_tf`
- `u8g2_font_6x10_tf`
- `u8g2_font_6x12_tf`
- `u8g2_font_6x13_tf`
- `u8g2_font_7x13_tf`
- `u8g2_font_8x13_tf`
- `u8g2_font_9x15_tf`

A better X11 pass would probably map small UI to `5x7` or `6x10`, body text to `6x12`/`6x13`, and only use `7x13` or `8x13` for emphasized numbers. Using `9x15` everywhere is too large.

With the current X11 8x13 pass, the normal/body UI looks good, but larger values need separate treatment. The main current temperature, humidity, tide value, and possibly forecast-box temperatures should not simply use the same `8x13` face as body labels. Test either larger X11 variants in isolated places (`9x15`, `8x13B`) or a mixed numeric font such as ProFont/Courier for those fields.

## Alignment Issue

This dovetails with the broader text-alignment cleanup. Current `drawString()` measures text with Adafruit GFX `getTextBounds()` but renders with U8g2. That means centering and baselines can drift depending on the font family.

Before finalizing fonts, test a U8g2-native text helper that uses U8g2 width/ascent/descent metrics for:

- current temperature and humidity
- forecast boxes
- tide labels
- graph titles and axis labels
- heading/date/time text

Do this behind a compile-time test flag and compare framebuffer captures before making it the default, because almost every label depends on this helper.
