#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "display_config.h"
#include "flir_uvc.h"

static LGFX lcd;

#define DISP_W 320
#define DISP_H 170
static uint16_t frame_rgb[FLIR_FRAME_W * FLIR_FRAME_H];

// Variable pour le message d'état à afficher
static char status_message[64] = "Connecte le FLIR One via OTG...";
static int status_y = 40;
static uint16_t status_color = TFT_WHITE;
static unsigned long last_update = 0;
static bool flir_connected = false;

// Fonction pour afficher les messages d'état sur l'écran
void display_status(const char* msg, uint16_t color = TFT_WHITE) {
  // Effacer l'ancienne ligne
  lcd.fillRect(0, status_y, DISP_W, 20, TFT_BLACK);
  
  // Afficher le nouveau message
  strncpy(status_message, msg, sizeof(status_message) - 1);
  status_color = color;
  
  lcd.setTextColor(color, TFT_BLACK);
  lcd.setTextSize(1);
  lcd.drawString(status_message, 10, status_y);
}

// Fonction pour logger et afficher sur l'écran
void log_screen(const char* msg, uint16_t color = TFT_WHITE) {
  Serial.println(msg);
  display_status(msg, color);
  delay(100);
}

void onFlirFrame(const uint8_t* data, size_t len) {
  // Afficher l'image thermique
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
  
  // Afficher un indicateur en bas à droite
  if (!flir_connected) {
    flir_connected = true;
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    lcd.setTextSize(1);
    lcd.drawString("OK", 290, 155);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n[MAIN] === FLIR Bridge Starting ===");
  
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  delay(200);
  
  Serial.println("[MAIN] Initializing LCD...");
  lcd.init();
  lcd.setRotation(1);
  lcd.setBrightness(255);
  
  // Afficher les couleurs de test
  lcd.fillScreen(TFT_RED);
  delay(300);
  lcd.fillScreen(TFT_GREEN);
  delay(300);
  lcd.fillScreen(TFT_BLUE);
  delay(300);
  
  // Écran noir et titre
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.setTextSize(2);
  lcd.drawString("FLIR Bridge v1", 10, 10);
  
  log_screen("Init USB...", TFT_YELLOW);
  delay(500);
  
  flir_uvc_init(onFlirFrame);
  
  log_screen("En attente du FLIR One...", TFT_WHITE);
  Serial.println("[MAIN] Setup complete");
}

void loop() {
  flir_uvc_task();
  
  // Mettre à jour le statut de connexion toutes les 2 secondes
  if (millis() - last_update > 2000) {
    last_update = millis();
    
    if (flir_uvc_connected()) {
      if (!flir_connected) {
        log_screen("FLIR One connecte!", TFT_GREEN);
        flir_connected = true;
      }
    } else {
      if (flir_connected) {
        log_screen("FLIR One deconnecte!", TFT_RED);
        flir_connected = false;
      }
    }
  }
  
  delay(1);
}
