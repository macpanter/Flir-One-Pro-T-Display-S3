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
    lcd.width() / (float)FLIR_FRAME_W,
    lcd.height() / (float)FLIR_FRAME_H,
    FLIR_FRAME_W,
    FLIR_FRAME_H,
    frame_rgb
  );
}

void setup() {
  Serial.begin(115200);
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  delay(200);
  lcd.init();
  lcd.setBrightness(255);

  // Test chaque rotation — note laquelle affiche le texte horizontal
  for (int r = 0; r < 4; r++) {
    lcd.setRotation(r);
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    lcd.setTextSize(2);
    lcd.setCursor(5, 5);
    lcd.printf("ROT %d  %dx%d", r, lcd.width(), lcd.height());
    lcd.setCursor(5, 30);
    lcd.println("FLIR Bridge");
    delay(2000);
  }

  // Garde la bonne rotation — change le chiffre selon ce que tu vois
  lcd.setRotation(1);
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.setTextSize(2);
  lcd.setCursor(10, 10);
  lcd.println("FLIR Bridge");
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(1);
  lcd.setCursor(10, 40);
  lcd.println("Connecte FLIR via OTG...");

  flir_uvc_init(onFlirFrame);
}

void loop() {
  flir_uvc_task();
  delay(1);
}
