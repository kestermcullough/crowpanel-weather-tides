#pragma once

#include <Arduino.h>

namespace CrowPanelEPD {

static const uint8_t PWR  = 7;
static const uint8_t PWR2 = 41;
static const uint8_t BUSY = 48;
static const uint8_t CS   = 45;
static const uint8_t RST  = 47;
static const uint8_t DC   = 46;
static const uint8_t SCK  = 12;
static const uint8_t MOSI = 11;

static const int16_t WIDTH = 400;
static const int16_t HEIGHT = 300;
static const uint32_t FRAME_BYTES = (WIDTH * HEIGHT) / 8;

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

inline void writeByte(uint8_t value) {
  digitalWrite(CS, LOW);
  for (uint8_t bit = 0; bit < 8; bit++) {
    digitalWrite(SCK, LOW);
    digitalWrite(MOSI, (value & 0x80) ? HIGH : LOW);
    delayMicroseconds(1);
    digitalWrite(SCK, HIGH);
    delayMicroseconds(1);
    value <<= 1;
  }
  digitalWrite(CS, HIGH);
}

inline void command(uint8_t value) {
  digitalWrite(DC, LOW);
  writeByte(value);
  digitalWrite(DC, HIGH);
}

inline void data(uint8_t value) {
  digitalWrite(DC, HIGH);
  writeByte(value);
}

inline void configurePins() {
  pinMode(PWR, OUTPUT);
  pinMode(PWR2, OUTPUT);
  pinMode(SCK, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(RST, OUTPUT);
  pinMode(DC, OUTPUT);
  pinMode(CS, OUTPUT);
  pinMode(BUSY, INPUT);

  digitalWrite(CS, HIGH);
  digitalWrite(DC, HIGH);
  digitalWrite(SCK, LOW);
  digitalWrite(MOSI, LOW);
  digitalWrite(RST, HIGH);
}

inline void powerOn() {
  configurePins();
  digitalWrite(PWR, LOW);
  digitalWrite(PWR2, LOW);
  delay(50);
  Serial.println("Powering CrowPanel e-paper via GPIO7 and GPIO41");
  digitalWrite(PWR2, HIGH);
  digitalWrite(PWR, HIGH);
  delay(500);
}

inline void powerOff() {
  pinMode(PWR, OUTPUT);
  pinMode(PWR2, OUTPUT);
  digitalWrite(PWR, LOW);
  digitalWrite(PWR2, LOW);
}

inline bool waitReady(const char* label, uint32_t timeout_ms) {
  const uint32_t start = millis();
  while (digitalRead(BUSY) == HIGH) {
    if (millis() - start > timeout_ms) {
      Serial.printf("TIMEOUT waiting for %s after %lu ms; BUSY=%d\r\n",
                    label, millis() - start, digitalRead(BUSY));
      return false;
    }
    delay(25);
  }
  Serial.printf("CrowPanel EPD ready after %s in %lu ms\r\n", label, millis() - start);
  return true;
}

inline void reset() {
  digitalWrite(RST, HIGH);
  delay(10);
  digitalWrite(RST, LOW);
  delay(100);
  digitalWrite(RST, HIGH);
  delay(100);
}

inline void initPanel() {
  reset();
  Serial.printf("CrowPanel EPD BUSY after reset = %d\r\n", digitalRead(BUSY));

  command(0x00);
  data(0x3F);
  data(0x4D);

  command(0x01);
  data(0x03);
  data(0x10);
  data(0x3F);
  data(0x3F);
  data(0x03);

  command(0x06);
  data(0x96);
  data(0x96);
  data(0x29);

  command(0x30);
  data(0x09);

  command(0x61);
  data(0x01);
  data(0x90);
  data(0x01);
  data(0x2C);

  command(0x82);
  data(0x05);

  command(0x50);
  data(0x97);

  command(0x60);
  data(0x22);

  command(0xE3);
  data(0x88);
}

inline void begin() {
  powerOn();
  initPanel();
}

inline void writeLut(uint8_t lut_command, const uint8_t* values, size_t count) {
  command(lut_command);
  for (size_t i = 0; i < count; i++) {
    data(values[i]);
  }
}

inline void writeGlobalRefreshLut() {
  writeLut(0x20, LUT_R20_GC, sizeof(LUT_R20_GC));
  writeLut(0x21, LUT_R21_GC, sizeof(LUT_R21_GC));
  writeLut(0x22, LUT_R22_GC, sizeof(LUT_R22_GC));
  writeLut(0x23, LUT_R23_GC, sizeof(LUT_R23_GC));
  writeLut(0x24, LUT_R24_GC, sizeof(LUT_R24_GC));
}

inline bool refresh(const uint8_t* frame) {
  Serial.printf("Sending CrowPanel framebuffer (%lu bytes)\r\n", FRAME_BYTES);

  command(0x10);
  for (uint32_t i = 0; i < FRAME_BYTES; i++) {
    data(0xFF);
  }

  command(0x50);
  data(0xD7);

  command(0x13);
  for (uint32_t i = 0; i < FRAME_BYTES; i++) {
    data(frame[i]);
  }

  writeGlobalRefreshLut();
  command(0x17);
  data(0xA5);

  delay(100);
  const bool ready = waitReady("full refresh", 45000);
  delay(8000);
  return ready;
}

inline void sleep() {
  command(0x07);
  data(0xA5);
  delay(50);
}

} // namespace CrowPanelEPD
