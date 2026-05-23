#pragma once
#include <stdint.h>
#include <stddef.h>
#define FLIR_VID        0x09CB
#define FLIR_PID_GEN3   0x1996
#define FLIR_FRAME_W    160
#define FLIR_FRAME_H    120
#define FLIR_FRAME_BYTES (FLIR_FRAME_W * FLIR_FRAME_H * 2)
typedef void (*flir_frame_cb_t)(const uint8_t* data, size_t len);
void flir_uvc_init(flir_frame_cb_t cb);
void flir_uvc_task();
bool flir_uvc_connected();
