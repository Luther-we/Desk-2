#pragma once
#include <Arduino.h>
#include "DHT.h"

// Type de capteur environnement (DHT, etc.)
struct EnvSensorConfig {
  uint8_t       pin;
  uint8_t       dhtType;    // DHT11, DHT22, ...
  unsigned long intervalMs; // période entre 2 mesures MQTT
};

void envSensorSetup(const EnvSensorConfig& cfg);

void envSensorLoop();

void envSensorPublishOnceOnConnect();

// GET
float envSensorGetLastTemperature();
float envSensorGetLastHumidity();
