#pragma once
#include <Arduino.h>

struct MqttConfig {
  const char* host;
  uint16_t    port;
  const char* user;
  const char* password;
};

// Config globale
extern const MqttConfig MQTT_CONFIG;

// Identité du device
extern const char* const DEVICE_NAME;
extern const char* const FRIENDLY_NAME;

// Topics MQTT globaux
extern String t_base;
extern String t_cmd;
extern String t_state;
extern String t_avail;
extern String t_disc;
extern String t_temp_state;
extern String t_hum_state;
extern String t_temp_disc;
extern String t_hum_disc;
