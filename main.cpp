#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "display_config.h"

static LGFX lcd;

#define DISP_W 320
#define DISP_H 170

// Variables de debug
static char debug_lines[5][64];
static int debug_line_idx = 0;

void add_debug(const char* text) {
  Serial.println(text);
  
  // Effacer la ligne la plus ancienne et décaler
  if (debug_line_idx >= 5) {
    for (int i = 0; i < 4; i++) {
      strcpy(debug_lines[i], debug_lines[i + 1]);
    }
    debug_line_idx = 4;
  }
  
  strncpy(debug_lines[debug_line_idx], text, sizeof(debug_lines[0]) - 1);
  debug_line_idx++;
  
  // Afficher à l'écran
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_CYAN, TFT_BLACK);
  lcd.setTextSize(2);
  lcd.drawString("FLIR Bridge", 10, 10);
  
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextSize(1);
  for (int i = 0; i < debug_line_idx; i++) {
    lcd.drawString(debug_lines[i], 10, 40 + (i * 15));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== FLIR Bridge - USB Test ===");
  
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  delay(200);
  
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
  
  add_debug("[1] LCD OK");
  delay(500);
  
  add_debug("[2] USB init...");
  delay(500);
  
  // Initialiser l'USB Host
  usb_host_config_t cfg = { .intr_flags = ESP_INTR_FLAG_LEVEL1 };
  usb_host_install(&cfg);
  
  add_debug("[3] USB Host installed");
  delay(500);
  
  // Créer la tâche USB
  xTaskCreatePinnedToCore([](void* arg) {
    while (true) {
      uint32_t f;
      usb_host_lib_handle_events(portMAX_DELAY, &f);
      if (f & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) usb_host_device_free_all();
    }
  }, "usb_task", 4096, nullptr, 5, nullptr, 0);
  
  add_debug("[4] USB Task created");
  delay(500);
  
  // Enregistrer le client USB
  usb_host_client_handle_t client = nullptr;
  usb_host_client_config_t cc = {
    .is_synchronous    = false,
    .max_num_event_msg = 5,
    .async = { 
      .client_event_callback = [](const usb_host_client_event_msg_t* msg, void* arg) {
        if (msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
          add_debug("[USB] DEVICE DETECTED!");
          usb_device_handle_t dev;
          if (usb_host_device_open((usb_host_client_handle_t)arg, msg->new_dev.address, &dev) == ESP_OK) {
            const usb_device_desc_t* dd;
            usb_host_get_device_descriptor(dev, &dd);
            char buf[64];
            sprintf(buf, "VID=0x%04X PID=0x%04X", dd->idVendor, dd->idProduct);
            add_debug(buf);
            usb_host_device_close((usb_host_client_handle_t)arg, dev);
          }
        }
      },
      .callback_arg = (void*)client 
    }
  };
  usb_host_client_register(&cc, &client);
  
  add_debug("[5] USB Client ready!");
  add_debug("Attendez device...");
}

void loop() {
  delay(1000);
}
