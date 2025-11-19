#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LedController.h"

// Animation de boot : "bille" qui parcourt la barre
void bootBlueBounce(Adafruit_NeoPixel& strip,
                    const LampState&    state,
                    uint16_t            frameDelayMs);
