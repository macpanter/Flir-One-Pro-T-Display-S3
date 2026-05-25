#include "flir_uvc.h"
#include <Arduino.h>
#include "usb/usb_host.h"

static flir_frame_cb_t  s_cb        = nullptr;
static usb_host_client_handle_t s_client = nullptr;
static usb_device_handle_t      s_dev    = nullptr;
static usb_transfer_t*          s_xfer   = nullptr;
static bool s_connected = false;
static uint8_t s_buf[FLIR_FRAME_BYTES];

static void bulk_cb(usb_transfer_t* xfer) {
  if (xfer->status == USB_TRANSFER_STATUS_COMPLETED && xfer->actual_num_bytes > 0) {
    size_t len = xfer->actual_num_bytes;
    Serial.printf("[FLIR] Recu %d bytes\n", len);
    if (len > sizeof(s_buf)) len = sizeof(s_buf);
    memcpy(s_buf, xfer->data_buffer, len);
    // Accept frame sizes within 95% of expected size to handle minor variations
    if (s_cb && len >= (FLIR_FRAME_BYTES * 95 / 100)) {
      Serial.println("[FLIR] Frame OK, call callback");
      s_cb(s_buf, len);
    } else {
      Serial.printf("[FLIR] Frame size mismatch: got %d, expected ~%d\n", len, FLIR_FRAME_BYTES);
    }
  } else {
    Serial.printf("[FLIR] Transfer error: status=%d, len=%d\n", xfer->status, xfer->actual_num_bytes);
  }
  if (s_connected) usb_host_transfer_submit(xfer);
}

static void client_event_cb(const usb_host_client_event_msg_t* msg, void* arg) {
  if (msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
    Serial.println("[FLIR] NEW DEVICE DETECTED");
    usb_device_handle_t dev;
    if (usb_host_device_open(s_client, msg->new_dev.address, &dev) != ESP_OK) {
      Serial.println("[FLIR] Erreur: usb_host_device_open");
      return;
    }
    const usb_device_desc_t* dd;
    usb_host_get_device_descriptor(dev, &dd);
    Serial.printf("[FLIR] VID=0x%04X PID=0x%04X\n", dd->idVendor, dd->idProduct);
    
    if (dd->idVendor != FLIR_VID) {
      Serial.printf("[FLIR] Mauvais VID (attendu 0x%04X)\n", FLIR_VID);
      usb_host_device_close(s_client, dev);
      return;
    }
    
    s_dev = dev;
    
    // Essayer de clamer l'interface 1
    esp_err_t ret = usb_host_interface_claim(s_client, s_dev, 1, 1);
    Serial.printf("[FLIR] usb_host_interface_claim ret=%d\n", ret);
    
    const usb_config_desc_t* cd;
    usb_host_get_active_config_descriptor(s_dev, &cd);
    
    Serial.printf("[FLIR] Config descriptor: wTotalLength=%d\n", cd->wTotalLength);
    
    uint8_t ep = 0;
    int off = 0;
    auto dp = (const usb_standard_desc_t*)cd;
    
    while (off < cd->wTotalLength) {
      Serial.printf("[FLIR] Descriptor at offset %d: type=0x%02X len=%d\n", 
                    off, dp->bDescriptorType, dp->bLength);
      
      if (dp->bDescriptorType == USB_B_DESCRIPTOR_TYPE_ENDPOINT) {
        auto epd = (const usb_ep_desc_t*)dp;
        Serial.printf("[FLIR]   ENDPOINT: addr=0x%02X attr=0x%02X\n", 
                      epd->bEndpointAddress, epd->bmAttributes);
        
        // Chercher endpoint IN de type BULK
        if ((epd->bEndpointAddress & 0x80) &&
            ((epd->bmAttributes & 0x03) == USB_BM_ATTRIBUTES_XFER_BULK)) {
          ep = epd->bEndpointAddress;
          Serial.printf("[FLIR] BULK IN endpoint trouve: 0x%02X\n", ep);
          break;
        }
      }
      
      off += dp->bLength;
      dp = (const usb_standard_desc_t*)((uint8_t*)dp + dp->bLength);
    }
    
    if (!ep) {
      Serial.println("[FLIR] ERREUR: Aucun endpoint BULK IN trouve!");
      usb_host_device_close(s_client, s_dev);
      s_dev = nullptr;
      return;
    }
    
    usb_host_transfer_alloc(FLIR_FRAME_BYTES + 64, 0, &s_xfer);
    s_xfer->device_handle    = s_dev;
    s_xfer->bEndpointAddress = ep;
    s_xfer->callback         = bulk_cb;
    s_xfer->context          = nullptr;
    s_xfer->num_bytes        = FLIR_FRAME_BYTES;
    s_connected = true;
    usb_host_transfer_submit(s_xfer);
    Serial.println("[FLIR] Streaming demarre");
    
  } else if (msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
    Serial.println("[FLIR] DEVICE DISCONNECTED");
    s_connected = false;
    if (s_xfer) { usb_host_transfer_free(s_xfer); s_xfer = nullptr; }
    if (s_dev)  { usb_host_device_close(s_client, s_dev); s_dev = nullptr; }
  }
}

static void usb_task(void* arg) {
  Serial.println("[USB] Task started");
  while (true) {
    uint32_t f;
    usb_host_lib_handle_events(portMAX_DELAY, &f);
    if (f & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) usb_host_device_free_all();
  }
}

void flir_uvc_init(flir_frame_cb_t cb) {
  Serial.println("[FLIR] Initializing FLIR USB...");
  s_cb = cb;
  usb_host_config_t cfg = { .intr_flags = ESP_INTR_FLAG_LEVEL1 };
  usb_host_install(&cfg);
  Serial.println("[USB] Host installed");
  
  xTaskCreatePinnedToCore(usb_task, "usb_host", 4096, nullptr, 5, nullptr, 0);
  
  usb_host_client_config_t cc = {
    .is_synchronous    = false,
    .max_num_event_msg = 5,
    .async = { .client_event_callback = client_event_cb, .callback_arg = nullptr }
  };
  usb_host_client_register(&cc, &s_client);
  Serial.println("[USB] Client registered");
}

void flir_uvc_task() { if (s_client) usb_host_client_handle_events(s_client, 0); }
bool flir_uvc_connected() { return s_connected; }
