#include "LedController.h"
#include "LedAnimations.h"

static LampConfig     g_cfg;
static LampState*     g_state = nullptr;
static Adafruit_NeoPixel g_strip;

static uint32_t effectTicker = 0;
static uint16_t rainbowHue   = 0;
static uint16_t wipeIndex    = 0;

static inline uint8_t clampBrightness(uint8_t b) {
  return (b > g_cfg.maxGlobalBrightness) ? g_cfg.maxGlobalBrightness : b;
}

Adafruit_NeoPixel& lampStrip() {
  return g_strip;
}

void lampSetup(const LampConfig& cfg, LampState* state) {
  g_cfg   = cfg;
  g_state = state;

  g_strip.updateLength(g_cfg.ledCount);
  g_strip.setPin(g_cfg.pin);
  g_strip.begin();
  g_strip.show(); // éteint

  effectTicker = millis();
  rainbowHue   = 0;
  wipeIndex    = 0;

  Serial.printf("[Lamp] Init strip (pin=%u, count=%u)\n",
                g_cfg.pin, g_cfg.ledCount);
}

static void setAll(uint8_t r, uint8_t g, uint8_t b, uint8_t globalBrightness) {
  g_strip.setBrightness(globalBrightness);
  for (uint16_t i = 0; i < g_cfg.ledCount; i++) {
    g_strip.setPixelColor(i, r, g, b);
  }
  g_strip.show();
}

void lampApply() {
  if (!g_state) return;

  if (!g_state->on) {
    // Animation de boot quand la lampe est éteinte
    bootBlueBounce(g_strip, *g_state, g_cfg.frameDelayMs);
    setAll(0, 0, 0, 0);
    return;
  }

  setAll(g_state->r, g_state->g, g_state->b,
         clampBrightness(g_state->brightness));
}

void lampLoopEffects() {
  if (!g_state) return;

  uint32_t now = millis();
  if (now - effectTicker < 20) return; // ~50 FPS max
  effectTicker = now;

  if (!g_state->on) return;

  switch (g_state->effect) {
    case LampEffect::RAINBOW: {
      for (uint16_t i = 0; i < g_cfg.ledCount; i++) {
        uint16_t h = (rainbowHue + i * (65535UL / g_cfg.ledCount)) & 0xFFFF;
        uint8_t r = (uint8_t)(sin(0.024 * (h +    0)) * 127 + 128);
        uint8_t g = (uint8_t)(sin(0.024 * (h +  8000)) * 127 + 128);
        uint8_t b = (uint8_t)(sin(0.024 * (h + 16000)) * 127 + 128);
        g_strip.setPixelColor(i, r, g, b);
      }
      g_strip.setBrightness(clampBrightness(g_state->brightness));
      g_strip.show();
      rainbowHue += 256;
    } break;

    case LampEffect::COLOR_WIPE: {
      g_strip.setPixelColor(wipeIndex % g_cfg.ledCount,
                            g_state->r, g_state->g, g_state->b);
      g_strip.setBrightness(clampBrightness(g_state->brightness));
      g_strip.show();
      wipeIndex++;
      if (wipeIndex >= g_cfg.ledCount) {
        g_state->effect = LampEffect::NONE;
      }
    } break;

    case LampEffect::NONE:
    default:
      break;
  }
}
