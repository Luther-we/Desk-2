#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedTypes.h"


// Etat partagé de la lampe
struct LampState {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t brightness; // 0..255
  bool    on;
  LampEffect effect;
};

// Configuration hardware de la lampe
struct LampConfig {
  uint8_t  pin;                 // ex: LED_PIN
  uint16_t ledCount;            // ex: 26
  uint8_t  maxGlobalBrightness; // clamp brightness
  uint16_t frameDelayMs;        // pour certaines anims
};

// Initialisation : à appeler dans setup()
void lampSetup(const LampConfig& cfg, LampState* state);

// Appliquer l'état courant sur le strip (ON/OFF + couleur)
// -> appelle l’animation de boot si off.
void lampApply();

// Boucle d’effets (arc-en-ciel, color_wipe...)
void lampLoopEffects();

// Accès au strip si besoin dans d’autres modules (optionnel)
Adafruit_NeoPixel& lampStrip();
