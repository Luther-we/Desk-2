#pragma once
#include <stdint.h>

// On redéfinit un enum simple compatible avec ton lamp.effect
enum class LampEffect : uint8_t { NONE = 0, RAINBOW = 1, COLOR_WIPE = 2 };

// Publie l'état de la lampe sur t_state
void mqttPublishLampState(bool on,
                          uint8_t brightness,
                          uint8_t r, uint8_t g, uint8_t b,
                          LampEffect effect);

// Publie température + humidité sur les topics capteurs
void mqttPublishDht(float temperature, float humidity);

// Publie la discovery Home Assistant (light + temp + hum)
void mqttPublishDiscovery();
