#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "WifiConnect.h"
#include "OTA.h"
#include "WifiConfig.h"
#include "MqttConfig.h"
#include "MqttClient.h"
#include "MqttTemplates.h"

// === DHT11 ===
#include "DHT.h"
#define DHTPIN   2   // <--- broche DATA du DHT11 (à adapter si besoin)
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);
unsigned long lastDHTMillis = 0;
const unsigned long DHT_INTERVAL = 3000; // 30 s entre 2 mesures


/* ====== LED ====== */
#define LED_PIN                 4
#define LED_COUNT               26
#define BLUE_LEVEL               120
#define FRAME_DELAY               30
#define MAX_GLOBAL_BRIGHTNESS   255
Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


/* ====== ETAT LAMPE ====== */
struct {
  uint8_t r = 255, g = 160, b = 20;
  uint8_t brightness = 0;     // 0..255
  bool on = false;
  enum Effect { NONE, RAINBOW, COLOR_WIPE } effect = NONE;
} lamp;

uint32_t effectTicker = 0;
uint16_t rainbowHue = 0;
uint16_t wipeIndex = 0;

/* ====== WIFI / MQTT ====== */
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

/* ====== LEDS ====== */
static inline uint8_t clampBrightness(uint8_t b) {
  return (b > MAX_GLOBAL_BRIGHTNESS) ? MAX_GLOBAL_BRIGHTNESS : b;
}

void ledsInit() {
  leds.begin();
  leds.show(); // éteint
}

void ledsSetAll(uint8_t r, uint8_t g, uint8_t b, uint8_t globalBrightness) {
  leds.setBrightness(globalBrightness);
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    leds.setPixelColor(i, r, g, b);
  }
  leds.show();
}

void bootBlueBounce() {
  leds.clear();
  leds.show();

  for (int i = 0; i < LED_COUNT; i++) {
    leds.clear();
    leds.setPixelColor(i, leds.Color(lamp.r, lamp.g, lamp.b));
    leds.show();
    delay(FRAME_DELAY);
  }

  for (int i = LED_COUNT - 2; i > 0; i--) {
    leds.clear();
    leds.setPixelColor(i, leds.Color(lamp.r, lamp.g, lamp.b));
    leds.show();
    delay(FRAME_DELAY);
  }

  leds.clear();
  leds.show();
}

void applyStrip() {
  if (!lamp.on) {
    bootBlueBounce();  
    ledsSetAll(0, 0, 0, 0);
    return;
  }
  ledsSetAll(lamp.r, lamp.g, lamp.b, clampBrightness(lamp.brightness));
}



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
    if (e == "none")           lamp.effect = decltype(lamp)::NONE;
    else if (e == "rainbow")   { lamp.effect = decltype(lamp)::RAINBOW;    rainbowHue = 0; }
    else if (e == "color_wipe"){ lamp.effect = decltype(lamp)::COLOR_WIPE; wipeIndex  = 0; }
  }

  applyStrip();

  // Nouveau : on utilise le template
  mqttPublishLampState(
    lamp.on,
    lamp.brightness,
    lamp.r, lamp.g, lamp.b,
    static_cast<LampEffect>(lamp.effect)
  );
}


/* ====== EFFETS ====== */
void loopEffects() {
  uint32_t now = millis();
  if (now - effectTicker < 20) return;   // ~50 FPS max
  effectTicker = now;

  if (!lamp.on) return;

  switch (lamp.effect) {
    case decltype(lamp)::RAINBOW: {
      for (uint16_t i = 0; i < LED_COUNT; i++) {
        uint16_t h = (rainbowHue + i * (65535UL / LED_COUNT)) & 0xFFFF;
        uint8_t r = (uint8_t)(sin(0.024 * (h +   0)) * 127 + 128);
        uint8_t g = (uint8_t)(sin(0.024 * (h + 8000)) * 127 + 128);
        uint8_t b = (uint8_t)(sin(0.024 * (h +16000)) * 127 + 128);
        leds.setPixelColor(i, r, g, b);
      }
      leds.setBrightness(clampBrightness(lamp.brightness));
      leds.show();
      rainbowHue += 256;
    } break;

    case decltype(lamp)::COLOR_WIPE: {
      leds.setPixelColor(wipeIndex % LED_COUNT, lamp.r, lamp.g, lamp.b);
      leds.setBrightness(clampBrightness(lamp.brightness));
      leds.show();
      wipeIndex++;
      if (wipeIndex >= LED_COUNT) lamp.effect = decltype(lamp)::NONE;
    } break;

    default: break;
  }
}


/* ====== SETUP / LOOP ====== */
void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) { delay(10); }
  Serial.println("\n[BOOT] Lampe RGB (ESP32-C3)");

  ledsInit();
  bootBlueBounce();

  // === DHT11 ===
  dht.begin();

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
      // On (re)annonce tout au moment de la connexion
      mqttPublishDiscovery();

      mqttPublishLampState(
        lamp.on,
        lamp.brightness,
        lamp.r, lamp.g, lamp.b,
        static_cast<LampEffect>(lamp.effect)
      );
      // TODO est ce important d'envoyer ces infos ici alors que la boucle suivante se charge déjà d'envoyer...
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      mqttPublishDht(t, h);

      applyStrip();
    }
  }

  mqttLoop();
  loopOTA();
  loopEffects();

  // === DHT11 : lecture périodique ===
  unsigned long now = millis();
  if (now - lastDHTMillis > DHT_INTERVAL) {
    lastDHTMillis = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    mqttPublishDht(t, h);
  }
}
