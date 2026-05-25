#pragma once
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_Parallel8 _bus;
  lgfx::Light_PWM _light;

public:
  LGFX() {
    // ==================== BUS PARALLEL 8 ====================
    auto bcfg = _bus.config();
    bcfg.port = 0;
    bcfg.freq_write = 20000000;     // 20MHz est stable
    bcfg.pin_wr = 8;
    bcfg.pin_rd = 9;
    bcfg.pin_rs = 7;
    bcfg.pin_d0 = 39;
    bcfg.pin_d1 = 40;
    bcfg.pin_d2 = 41;
    bcfg.pin_d3 = 42;
    bcfg.pin_d4 = 45;
    bcfg.pin_d5 = 46;
    bcfg.pin_d6 = 47;
    bcfg.pin_d7 = 48;
    _bus.config(bcfg);
    _panel.setBus(&_bus);

    // ==================== PANEL CONFIG ====================
    auto pcfg = _panel.config();
    pcfg.pin_cs   = 6;
    pcfg.pin_rst  = 5;
    pcfg.pin_busy = -1;

    pcfg.panel_width   = 320;
    pcfg.panel_height  = 170;
    pcfg.memory_width  = 320;
    pcfg.memory_height = 170;

    pcfg.offset_x = 0;
    pcfg.offset_y = 28;           // ← Valeur que tu utilises (bonne base)
    pcfg.offset_rotation = 1;     // 1 = 90° (le plus courant sur ce modèle)

    pcfg.invert       = true;
    pcfg.readable     = false;
    pcfg.bus_shared   = false;
    pcfg.dummy        = 0x00;     // Ajout utile
    pcfg.write_depth  = 16;       // RGB565

    _panel.config(pcfg);

    // ==================== BACKLIGHT ====================
    auto lcfg = _light.config();
    lcfg.pin_bl = 38;
    lcfg.invert = false;
    lcfg.freq = 44100;
    lcfg.pwm_channel = 0;
    _light.config(lcfg);
    _panel.setLight(&_light);

    setPanel(&_panel);
  }
};

inline void yuyv_to_rgb565(const uint8_t* yuyv, uint16_t* out, int w, int h) {
  for (int i = 0; i < (w * h) / 2; i++) {
    int y0=yuyv[i*4+0], u=yuyv[i*4+1], y1=yuyv[i*4+2], v=yuyv[i*4+3];
    auto cl=[](int x){return x<0?0:(x>255?255:x);};
    auto cvt=[&](int y, uint16_t& o){
      int r=cl(y+1.402f*(v-128));
      int g=cl(y-0.344f*(u-128)-0.714f*(v-128));
      int b=cl(y+1.772f*(u-128));
      o=((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3);
    };
    cvt(y0, out[i*2]);
    cvt(y1, out[i*2+1]);
  }
}
