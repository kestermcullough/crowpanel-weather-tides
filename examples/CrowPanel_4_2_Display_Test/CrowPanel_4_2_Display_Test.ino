#include <Arduino.h>
#include <Adafruit_GFX.h>

static const uint8_t EPD_PWR  = 7;
static const uint8_t EPD_PWR2 = 41;
static const uint8_t EPD_BUSY = 48;
static const uint8_t EPD_CS   = 45;
static const uint8_t EPD_RST  = 47;
static const uint8_t EPD_DC   = 46;
static const uint8_t EPD_SCK  = 12;
static const uint8_t EPD_MOSI = 11;

static const int16_t EPD_W = 400;
static const int16_t EPD_H = 300;
static const uint32_t FRAME_BYTES = (EPD_W * EPD_H) / 8;

GFXcanvas1 canvas(EPD_W, EPD_H);

static const uint8_t LUT_R20_GC[] = {
  0x01, 0x14, 0x0A, 0x14, 0x00, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t LUT_R21_GC[] = {
  0x01, 0x54, 0x0A, 0x94, 0x00, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t LUT_R22_GC[] = {
  0x01, 0x54, 0x0A, 0x94, 0x00, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t LUT_R23_GC[] = {
  0x01, 0x94, 0x0A, 0x54, 0x00, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t LUT_R24_GC[] = {
  0x01, 0x94, 0x0A, 0x54, 0x00, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static void epdWriteByte(uint8_t value) {
  digitalWrite(EPD_CS, LOW);
  for (uint8_t bit = 0; bit < 8; bit++) {
    digitalWrite(EPD_SCK, LOW);
    digitalWrite(EPD_MOSI, (value & 0x80) ? HIGH : LOW);
    delayMicroseconds(1);
    digitalWrite(EPD_SCK, HIGH);
    delayMicroseconds(1);
    value <<= 1;
  }
  digitalWrite(EPD_CS, HIGH);
}

static void epdCommand(uint8_t command) {
  digitalWrite(EPD_DC, LOW);
  epdWriteByte(command);
  digitalWrite(EPD_DC, HIGH);
}

static void epdData(uint8_t data) {
  digitalWrite(EPD_DC, HIGH);
  epdWriteByte(data);
}

static bool waitReady(const char* label, uint32_t timeout_ms) {
  const uint32_t start = millis();
  Serial.printf("BUSY before %-12s = %d\r\n", label, digitalRead(EPD_BUSY));
  while (digitalRead(EPD_BUSY) == HIGH) {
    if (millis() - start > timeout_ms) {
      Serial.printf("TIMEOUT waiting for %-12s after %lu ms; BUSY=%d\r\n",
                    label, millis() - start, digitalRead(EPD_BUSY));
      return false;
    }
    delay(25);
  }
  Serial.printf("Ready after %-12s in %lu ms\r\n", label, millis() - start);
  return true;
}

static void resetPanel() {
  Serial.println("Hardware reset");
  digitalWrite(EPD_RST, HIGH);
  delay(10);
  digitalWrite(EPD_RST, LOW);
  delay(100);
  digitalWrite(EPD_RST, HIGH);
  delay(100);
}

static void writeLut(uint8_t command, const uint8_t* values, size_t count) {
  epdCommand(command);
  for (size_t i = 0; i < count; i++) {
    epdData(values[i]);
  }
}

static void writeGlobalRefreshLut() {
  Serial.println("Writing green-sticker global-refresh LUT");
  writeLut(0x20, LUT_R20_GC, sizeof(LUT_R20_GC));
  writeLut(0x21, LUT_R21_GC, sizeof(LUT_R21_GC));
  writeLut(0x22, LUT_R22_GC, sizeof(LUT_R22_GC));
  writeLut(0x23, LUT_R23_GC, sizeof(LUT_R23_GC));
  writeLut(0x24, LUT_R24_GC, sizeof(LUT_R24_GC));
}

static bool initPanel() {
  resetPanel();
  Serial.printf("BUSY after reset = %d\r\n", digitalRead(EPD_BUSY));

  Serial.println("Green-sticker panel init");
  epdCommand(0x00);
  epdData(0x3F);
  epdData(0x4D);

  epdCommand(0x01);
  epdData(0x03);
  epdData(0x10);
  epdData(0x3F);
  epdData(0x3F);
  epdData(0x03);

  epdCommand(0x06);
  epdData(0x96);
  epdData(0x96);
  epdData(0x29);

  epdCommand(0x30);
  epdData(0x09);

  epdCommand(0x61);
  epdData(0x01);
  epdData(0x90);
  epdData(0x01);
  epdData(0x2C);

  epdCommand(0x82);
  epdData(0x05);

  epdCommand(0x50);
  epdData(0x97);

  epdCommand(0x60);
  epdData(0x22);

  epdCommand(0xE3);
  epdData(0x88);

  return true;
}

static void drawPattern() {
  canvas.fillScreen(1);
  canvas.drawRect(0, 0, EPD_W, EPD_H, 0);
  canvas.drawRect(8, 8, EPD_W - 16, EPD_H - 16, 0);

  canvas.setTextColor(0);
  canvas.setTextSize(3);
  canvas.setCursor(28, 32);
  canvas.print("CrowPanel 4.2");
  canvas.setTextSize(2);
  canvas.setCursor(28, 72);
  canvas.print("Elecrow green-driver diag");

  canvas.drawFastHLine(24, 112, 352, 0);
  canvas.drawFastVLine(200, 126, 132, 0);

  canvas.setTextSize(1);
  canvas.setCursor(30, 132);
  canvas.print("PWR 7/41  SCK 12  MOSI 11");
  canvas.setCursor(30, 150);
  canvas.print("CS 45  DC 46  RST 47  BUSY 48");
  canvas.setCursor(30, 174);
  canvas.print("If visible: panel power, pins,");
  canvas.setCursor(30, 190);
  canvas.print("bit-banged SPI, refresh OK.");

  canvas.fillRect(222, 132, 36, 36, 0);
  canvas.drawRect(274, 132, 54, 36, 0);
  canvas.fillCircle(252, 216, 22, 0);
  canvas.drawCircle(306, 216, 22, 0);
  canvas.drawLine(222, 254, 356, 132, 0);
  canvas.drawLine(222, 132, 356, 254, 0);

  for (int x = 24; x < 376; x += 16) {
    if ((x / 16) & 1) {
      canvas.fillRect(x, 268, 8, 16, 0);
    } else {
      canvas.drawRect(x, 268, 8, 16, 0);
    }
  }

  canvas.setCursor(28, 280);
  canvas.print("Built ");
  canvas.print(__DATE__);
  canvas.print(" ");
  canvas.print(__TIME__);
}

static void sendFrame(const uint8_t* frame) {
  Serial.printf("Sending %lu bytes to old/new RAM\r\n", FRAME_BYTES);
  epdCommand(0x10);
  for (uint32_t i = 0; i < FRAME_BYTES; i++) {
    epdData(0xFF);
  }

  epdCommand(0x50);
  epdData(0xD7);

  epdCommand(0x13);
  for (uint32_t i = 0; i < FRAME_BYTES; i++) {
    epdData(frame[i]);
  }
}

static bool refreshPanel() {
  Serial.println("Triggering full refresh");
  writeGlobalRefreshLut();
  epdCommand(0x17);
  epdData(0xA5);
  delay(100);
  return waitReady("full refresh", 45000);
}

static void sleepPanel() {
  Serial.println("Panel sleep");
  epdCommand(0x07);
  epdData(0xA5);
  delay(50);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("CrowPanel 4.2 Elecrow-style display diagnostic");

  pinMode(EPD_PWR, OUTPUT);
  pinMode(EPD_PWR2, OUTPUT);
  digitalWrite(EPD_PWR, LOW);
  digitalWrite(EPD_PWR2, LOW);

  pinMode(EPD_SCK, OUTPUT);
  pinMode(EPD_MOSI, OUTPUT);
  pinMode(EPD_RST, OUTPUT);
  pinMode(EPD_DC, OUTPUT);
  pinMode(EPD_CS, OUTPUT);
  pinMode(EPD_BUSY, INPUT);

  digitalWrite(EPD_CS, HIGH);
  digitalWrite(EPD_DC, HIGH);
  digitalWrite(EPD_SCK, LOW);
  digitalWrite(EPD_MOSI, LOW);
  digitalWrite(EPD_RST, HIGH);

  Serial.printf("BUSY with power off = %d\r\n", digitalRead(EPD_BUSY));
  Serial.println("Powering panel on via GPIO7 and GPIO41");
  digitalWrite(EPD_PWR2, HIGH);
  digitalWrite(EPD_PWR, HIGH);
  delay(500);
  Serial.printf("BUSY after power on = %d\r\n", digitalRead(EPD_BUSY));

  bool ok = initPanel();
  if (ok) {
    drawPattern();
    sendFrame(canvas.getBuffer());
    ok = refreshPanel();
  }

  if (ok) {
    Serial.println("Display diagnostic complete");
    Serial.println("Holding panel power for 8 seconds before sleep");
    delay(8000);
  } else {
    Serial.println("Display diagnostic failed before completion");
  }

  sleepPanel();
  digitalWrite(EPD_PWR, LOW);
  digitalWrite(EPD_PWR2, LOW);
  Serial.println("Panel power off; image should persist if refresh succeeded");
}

void loop() {
}
