#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "display_config.h"
#include "flir_uvc.h"

static LGFX lcd;

#define DISP_W 320
#define DISP_H 170
static uint16_t frame_rgb[FLIR_FRAME_W * FLIR_FRAME_H];

void onFlirFrame(const uint8_t* data, size_t len) {
  yuyv_to_rgb565(data, frame_rgb, FLIR_FRAME_W, FLIR_FRAME_H);
  lcd.pushImageRotateZoom(
    (float)(DISP_W / 2),
    (float)(DISP_H / 2),
    (float)(FLIR_FRAME_W / 2),
    (float)(FLIR_FRAME_H / 2),
    0.0f,
    DISP_W / (float)FLIR_FRAME_W,
    DISP_H / (float)FLIR_FRAME_H,
    FLIR_FRAME_W,
    FLIR_FRAME_H,
    frame_rgb
  );
}

void setup() {
  Serial.begin(115200);
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(220);
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.setTextSize(2);
  lcd.drawString("FLIR Bridge", 10, 10);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(1);
  lcd.drawString("Connecte le FLIR One via OTG...", 10, 40);
  flir_uvc_init(onFlirFrame);
  Serial.println("[MAIN] Pret. Branche le FLIR One.");
}

void loop() {
  flir_uvc_task();
  delay(1);
}
