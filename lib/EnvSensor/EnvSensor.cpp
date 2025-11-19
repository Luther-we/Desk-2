#include "EnvSensor.h"
#include "DHT.h"
#include "MqttTemplates.h"
#include "WifiConnect.h"
#include "MqttClient.h"

static EnvSensorConfig g_cfg;
static bool            g_initialized = false;

// On utilise un pointeur pour pouvoir instancier DHT avec la config
static DHT* dhtPtr = nullptr;

static unsigned long lastDHTMillis = 0;
static float lastTemp = NAN;
static float lastHum  = NAN;

static void readAndPublish() {
  if (!dhtPtr) return;

  float h = dhtPtr->readHumidity();
  float t = dhtPtr->readTemperature(); // °C

  if (isnan(h) || isnan(t)) {
    Serial.println("[EnvSensor] Erreur de lecture DHT");
    return;
  }

  lastTemp = t;
  lastHum  = h;

  mqttPublishDht(t, h);
}

void envSensorSetup(const EnvSensorConfig& cfg) {
  g_cfg = cfg;
  g_initialized = true;

  if (dhtPtr) {
    delete dhtPtr;
    dhtPtr = nullptr;
  }

  dhtPtr = new DHT(g_cfg.pin, g_cfg.dhtType);
  dhtPtr->begin();

  lastDHTMillis = millis();

  Serial.printf("[EnvSensor] Init DHT (pin=%u, type=%u, interval=%lu ms)\n",
                g_cfg.pin, g_cfg.dhtType, g_cfg.intervalMs);
}

void envSensorLoop() {
  if (!g_initialized || !dhtPtr) return;

  if (!wifiIsConnected() || !mqttClient().connected()) {
    return; // on ne lit/publie que si tout est connecté
  }

  unsigned long now = millis();
  if (now - lastDHTMillis > g_cfg.intervalMs) {
    lastDHTMillis = now;
    readAndPublish();
  }
}

void envSensorPublishOnceOnConnect() {
  if (!g_initialized || !dhtPtr) return;
  if (!wifiIsConnected() || !mqttClient().connected()) return;

  readAndPublish();
}

float envSensorGetLastTemperature() {
  return lastTemp;
}

float envSensorGetLastHumidity() {
  return lastHum;
}
