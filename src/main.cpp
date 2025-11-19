#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WifiConnect.h"
#include "OTA.h"
#include "WifiConfig.h"
#include "MqttConfig.h"
#include "MqttClient.h"
#include "MqttTemplates.h"
#include "EnvSensor.h" 
#include "LedController.h"
#include "LedAnimations.h"
#include "LedTypes.h"


#define DH11_SENSOR_PIN       2
#define LED_PIN                 4
#define LED_COUNT               26
#define BLUE_LEVEL               120
#define FRAME_DELAY               30
#define MAX_GLOBAL_BRIGHTNESS   255

// Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const EnvSensorConfig ENV_SENSOR_CONFIG = {
  .pin        = DH11_SENSOR_PIN,
  .dhtType    = DHT11,
  .intervalMs = 30000
};

const LampConfig LAMP_CONFIG = {
  .pin                 = LED_PIN,
  .ledCount            = LED_COUNT,
  .maxGlobalBrightness = MAX_GLOBAL_BRIGHTNESS,
  .frameDelayMs        = FRAME_DELAY
};

LampState lamp = {
  .r          = 255,
  .g          = 160,
  .b          = 20,
  .brightness = 100,
  .on         = false,
  .effect     = LampEffect::NONE
};


/* ====== WIFI / MQTT ====== */
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("[MQTT] RX topic=%s len=%u : ", topic, length);
  for (unsigned int i = 0; i < length; i++) Serial.write(payload[i]);
  Serial.println();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) {
    Serial.printf("[MQTT] JSON error: %s\n", err.c_str());
    return;
  }

  if (doc["state"].is<const char*>()) {
    lamp.on = (strcasecmp(doc["state"].as<const char*>(), "ON") == 0);
  }

  if (doc["brightness"].is<int>()) {
    int b = doc["brightness"].as<int>();
    lamp.brightness = (uint8_t) constrain(b, 0, 255);
  }

  if (doc["color"].is<JsonObject>()) {
    JsonObject c = doc["color"].as<JsonObject>();
    if (c["r"].is<int>()) lamp.r = (uint8_t)c["r"].as<int>();
    if (c["g"].is<int>()) lamp.g = (uint8_t)c["g"].as<int>();
    if (c["b"].is<int>()) lamp.b = (uint8_t)c["b"].as<int>();
  }

  if (doc["effect"].is<const char*>()) {
    String e = doc["effect"].as<const char*>();
    if (e == "none")           lamp.effect = LampEffect::NONE;
    else if (e == "rainbow")   { lamp.effect = LampEffect::RAINBOW;    /* reset si besoin */ }
    else if (e == "color_wipe"){ lamp.effect = LampEffect::COLOR_WIPE; /* reset si besoin */ }
  }

  lampApply();

  mqttPublishLampState(
    lamp.on,
    lamp.brightness,
    lamp.r, lamp.g, lamp.b,
    lamp.effect           // LampEffect, à adapter dans MqttTemplates
  );
}


void onConnectPublish() {
  mqttPublishDiscovery();

  mqttPublishLampState(
    lamp.on,
    lamp.brightness,
    lamp.r, lamp.g, lamp.b,
    lamp.effect
  );

  // Publier une mesure tout de suite à la connexion MQTT
  envSensorPublishOnceOnConnect();
}


void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) { delay(10); }
  Serial.println("\n[BOOT] Lampe RGB (ESP32-C3)");

  lampSetup(LAMP_CONFIG, &lamp);
  lampApply();
  bootBlueBounce(lampStrip(),  // si tu veux stric. appel ici
                 lamp,
                 FRAME_DELAY);

  // === DHT11 ===
   envSensorSetup(ENV_SENSOR_CONFIG);

  Serial.printf("[DEBUG] WIFI_CONFIG.ssid = '%s'\n", WIFI_CONFIG.ssid);
  wifiSetup(WIFI_CONFIG);
  mqttSetup();
  mqttClient().setCallback(mqttCallback); 

  setupOTA();

}

void loop() {
    wifiLoop();

  if (wifiIsConnected()) {
    bool justConnected = mqttEnsureConnected();
    if (justConnected) {
      onConnectPublish();
      lampApply();   
    }
  }

  mqttLoop();
  loopOTA();
  lampLoopEffects();
  envSensorLoop();
}
