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
    (float)(lcd.width() / 2),
    (float)(lcd.height() / 2),
    (float)(FLIR_FRAME_W / 2),
    (float)(FLIR_FRAME_H / 2),
    0.0f,
    lcd.width()  / (float)FLIR_FRAME_W,
    lcd.height() / (float)FLIR_FRAME_H,
    FLIR_FRAME_W,
    FLIR_FRAME_H,
    frame_rgb
  );
  // Affiche confirmation sur écran
  lcd.setCursor(10, 60);
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd.println("FRAME RECU !");
}

void setup() {
  Serial.begin(115200);
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  delay(200);
  lcd.init();
  lcd.setRotation(3);
  lcd.setBrightness(255);
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.setTextSize(2);
  lcd.setCursor(10, 10);
  lcd.println("FLIR Bridge");
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(1);
  lcd.setCursor(10, 40);
  lcd.println("Connecte le FLIR One via OTG...");

  // Affiche statut USB sur écran
  lcd.setCursor(10, 60);
  lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
  lcd.println("Init USB host...");

  flir_uvc_init(onFlirFrame);

  lcd.setCursor(10, 60);
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd.println("USB host OK - attente FLIR");
}

void loop() {
  flir_uvc_task();
  delay(1);
}
