#include "effects.h"
#include "ws2812.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "esp_random.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---- module state ----
static int    N = 0;
static rgb_t* buf = NULL;
static effect_t cur = EFFECT_STATIC;
static uint32_t frame = 0;

// colors
static rgb_t static_c = {255,255,255};      // "last static color"
static rgb_t c1 = {255,0,128}, c2 = {0,255,128}, c3 = {64,64,255};

// ---- helpers ----
static inline uint8_t clampi(int v){ return (v<0)?0:((v>255)?255:v); }

static rgb_t hsv(float h, float s, float v){
  float r,g,b;
  int i=(int)floorf(h*6.f);
  float f=h*6.f-i;
  float p=v*(1.f-s), q=v*(1.f-f*s), t=v*(1.f-(1.f-f)*s);
  switch(i%6){
    case 0:r=v;g=t;b=p;break; case 1:r=q;g=v;b=p;break;
    case 2:r=p;g=v;b=t;break; case 3:r=p;g=q;b=v;break;
    case 4:r=t;g=p;b=v;break; default:r=v;g=p;b=q;break;
  }
  rgb_t R={(uint8_t)(r*255),(uint8_t)(g*255),(uint8_t)(b*255)}; return R;
}

// ---- API ----
void effects_init(int led_count, int gpio){
  (void)gpio; // ws2812_init handled by caller (main)
  N = (led_count > 0) ? led_count : 0;
  if (buf) { free(buf); buf = NULL; }
  if (N) buf = (rgb_t*)calloc(N, sizeof(rgb_t));
  frame = 0;
}

void effects_set_static_color(rgb_t color){
  static_c = color;       // remember last static color
}

void effects_set_static(rgb_t color){
  static_c = color;
  cur = EFFECT_STATIC;
  frame = 0;
}

void effects_set_effect(effect_t effect){
  if (effect < 0 || effect >= EFFECT_MAX) return;
  cur = effect;
  frame = 0;
}

void effects_configure_spacey(rgb_t a, rgb_t b, rgb_t d){
  c1=a; c2=b; c3=d;
}

// ------- getters -------
rgb_t effects_get_static(void){ return static_c; }
effect_t effects_get_effect(void){ return cur; }

// ---- per-frame generators (no blocking) ----
static void draw_static(void){
  for (int i=0;i<N;i++) buf[i]=static_c;
}

static void draw_rainbow(void){
  for (int i=0;i<N;i++){
    float hue = fmodf((float)i/(float)N + frame*0.003f, 1.f);
    buf[i]=hsv(hue, 1.f, 0.4f);
  }
}

static void draw_theater(void){
  for (int i=0;i<N;i++){
    buf[i] = (((i+frame)%3)==0) ? c1 : (rgb_t){0,0,0};
  }
}

static void draw_fire(void){
  for (int i=0;i<N;i++){
    int base = 160 + (int)(esp_random()%96);
    int g = base*2/3;
    buf[i]=(rgb_t){clampi(base),clampi(g),0};
  }
}

static void draw_wave(void){
  for (int i=0;i<N;i++){
    float v=(sinf((i+frame)*0.10f)+1.f)*0.5f;
    buf[i]=(rgb_t){(uint8_t)(c1.r*v),(uint8_t)(c1.g*v),(uint8_t)(c1.b*v)};
  }
}

static void draw_twinkle(void){
  for(int i=0;i<N;i++){ buf[i].r/=2; buf[i].g/=2; buf[i].b/=2; }
  for(int k=0;k<5;k++){ int j=(N>0)? (int)(esp_random()%N) : 0; if (N) buf[j]=c1; }
}

static void draw_color_wipe(void){
  int idx = (N>0) ? (frame % N) : 0;
  for (int i=0;i<N;i++) buf[i]=(i<=idx)?c1:(rgb_t){0,0,0};
}

static void draw_strobe(void){
  rgb_t x = (frame&1)?(rgb_t){0,0,0}:c1;
  for(int i=0;i<N;i++) buf[i]=x;
}

static void draw_breathing(void){
  float v = (sinf(frame*0.05f)+1.f)*0.5f;
  for (int i=0;i<N;i++){
    buf[i]=(rgb_t){(uint8_t)(c1.r*v),(uint8_t)(c1.g*v),(uint8_t)(c1.b*v)};
  }
}

static void draw_comet(void){
  if (N<=0) return;
  int pos = frame % N;
  for (int i=0;i<N;i++) buf[i]=(rgb_t){0,0,0};
  for (int t=0;t<12;t++){
    int idx=(pos-t+N)%N;
    float fade = 1.f - t/12.f;
    buf[idx]=(rgb_t){(uint8_t)(c1.r*fade),(uint8_t)(c1.g*fade),(uint8_t)(c1.b*fade)};
  }
}

static void draw_sparkle(void){
  for (int i=0;i<N;i++){
    if ((esp_random()%32)==0) buf[i]=c1; else { buf[i].r/=2; buf[i].g/=2; buf[i].b/=2; }
  }
}

static void draw_spacey(void){
  for (int i=0;i<N;i++){
    float ph  = (float)(i+frame) / 50.f;
    float w1 = (sinf(ph)+1.f)*0.5f;
    float w2 = (sinf(ph+2.094f)+1.f)*0.5f; // +120°
    float w3 = (sinf(ph+4.188f)+1.f)*0.5f; // +240°
    float sum = w1+w2+w3; if (sum <= 0.f) sum = 1.f; w1/=sum; w2/=sum; w3/=sum;
    buf[i].r = (uint8_t)(c1.r*w1 + c2.r*w2 + c3.r*w3);
    buf[i].g = (uint8_t)(c1.g*w1 + c2.g*w2 + c3.g*w3);
    buf[i].b = (uint8_t)(c1.b*w1 + c2.b*w2 + c3.b*w3);
  }
}

void effects_update(void){
  if (!buf || N<=0) return;

  switch(cur){
    case EFFECT_STATIC:           draw_static(); break;
    case EFFECT_RAINBOW:          draw_rainbow(); break;
    case EFFECT_THEATER_CHASE:    draw_theater(); break;
    case EFFECT_FIRE:             draw_fire(); break;
    case EFFECT_WAVE:             draw_wave(); break;
    case EFFECT_TWINKLE:          draw_twinkle(); break;
    case EFFECT_COLOR_WIPE:       draw_color_wipe(); break;
    case EFFECT_STROBE:           draw_strobe(); break;
    case EFFECT_BREATHING:        draw_breathing(); break;
    case EFFECT_COMET:            draw_comet(); break;
    case EFFECT_SPARKLE:          draw_sparkle(); break;
    case EFFECT_SPACEY_GRADIENT:  draw_spacey(); break;
    default: break;
  }
  ws2812_set_colors(N, buf);
  frame++;
}
