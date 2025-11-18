#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "OTA.h"

// === DHT11 ===
#include "DHT.h"
#define DHTPIN   2   // <--- broche DATA du DHT11 (à adapter si besoin)
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);
unsigned long lastDHTMillis = 0;
const unsigned long DHT_INTERVAL = 3000; // 30 s entre 2 mesures

/* ====== CONFIG WIFI ====== */
static const char* WIFI_SSID   = "Livebox-1714";
static const char* WIFI_PASS   = "47PnSkZz7GtYv4QG7m";

/* ===== CONFIG IP STATIQUE ===== */
IPAddress local_IP(192, 168, 1, 19);   // <-- Choisis ici ton XX
IPAddress gateway(192,168,1,1);     // Routeur
IPAddress subnet(255,255,255,0);
IPAddress dns1(1, 1, 1, 1);
IPAddress dns2(8, 8, 8, 8);

static const char* MQTT_HOST   = "192.168.1.31"; // IP ou hostname du broker
static const uint16_t MQTT_PORT= 1883;
static const char* MQTT_USER   = "mqttuser";
static const char* MQTT_PASSWD = "aloha22";

#define ESP_WIFI_NAME          "Desk #2"
#define DEVICE_NAME            "lampe_rgb1"
#define FRIENDLY_NAME          "Desk #2"

/* ====== LED ====== */
#define LED_PIN                 4
#define LED_COUNT               26
#define BLUE_LEVEL               120
#define FRAME_DELAY               30
#define MAX_GLOBAL_BRIGHTNESS   255
Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

/* ====== TOPICS MQTT ====== */
String t_base   = String(DEVICE_NAME) + "/";
String t_cmd    = t_base + "light/set";
String t_state  = t_base + "light/state";
String t_avail  = t_base + "status";
String t_disc   = "homeassistant/light/" + String(DEVICE_NAME) + "/config";

// === DHT11 : topics capteurs ===
String t_temp_state = t_base + "sensor/temperature";
String t_hum_state  = t_base + "sensor/humidity";
String t_temp_disc  = "homeassistant/sensor/" + String(DEVICE_NAME) + "_temperature/config";
String t_hum_disc   = "homeassistant/sensor/" + String(DEVICE_NAME) + "_humidity/config";

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

/* ====== MQTT PUBLISH ====== */
void publishState() {
  JsonDocument doc;
  doc["state"] = lamp.on ? "ON" : "OFF";
  doc["brightness"] = lamp.brightness;

  JsonObject color = doc["color"].to<JsonObject>();
  color["r"] = lamp.r;
  color["g"] = lamp.g;
  color["b"] = lamp.b;

  switch (lamp.effect) {
    case decltype(lamp)::NONE:       break;
    case decltype(lamp)::RAINBOW:    doc["effect"] = "rainbow"; break;
    case decltype(lamp)::COLOR_WIPE: doc["effect"] = "color_wipe"; break;
  }

  char buf[256];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(t_state.c_str(), (const uint8_t*)buf, (unsigned int)n, true);
}

// === DHT11 : publication des mesures ===
void publishDHTState() {
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // °C

  if (isnan(h) || isnan(t)) {
    Serial.println("[DHT] Erreur de lecture");
    return;
  }

  Serial.print("[DHT] T = ");
  Serial.print(t);
  Serial.print(" °C, H = ");
  Serial.print(h);
  Serial.println(" %");

  char buf[16];

  dtostrf(t, 2, 2, buf);
  mqtt.publish(t_temp_state.c_str(), buf, true); // retain

  dtostrf(h, 2, 2, buf);
  mqtt.publish(t_hum_state.c_str(), buf, true);  // retain
}

void publishDiscovery() {
  // === Discovery pour la lampe (light) ===
  {
    JsonDocument doc;

    doc["name"]         = FRIENDLY_NAME;
    doc["uniq_id"]      = String(DEVICE_NAME) + "_light";
    doc["obj_id"]       = DEVICE_NAME;

    doc["cmd_t"]        = t_cmd;
    doc["stat_t"]       = t_state;

    doc["schema"]       = "json";
    doc["brightness"]   = true;

    doc["color_mode"]   = true;
    JsonArray modes = doc["supported_color_modes"].to<JsonArray>();
    modes.add("rgb");

    doc["avty_t"]       = t_avail;
    doc["pl_avail"]     = "online";
    doc["pl_not_avail"] = "offline";

    JsonObject dev = doc["device"].to<JsonObject>();
    dev["name"] = FRIENDLY_NAME;
    dev["mf"]   = "DIY";
    dev["mdl"]  = "ESP32 C3 RGB Lamp";
    JsonArray ids = dev["ids"].to<JsonArray>();
    ids.add(DEVICE_NAME);

    char buf[512];
    size_t n = serializeJson(doc, buf, sizeof(buf));
    mqtt.publish(t_disc.c_str(), (const uint8_t*)buf, (unsigned int)n, true);
    Serial.printf("[DISCOVERY] LIGHT %s\n", buf);
  }

  // === Discovery pour capteur température ===
  {
    JsonDocument doc;

    doc["name"]    = String(FRIENDLY_NAME) + " Température";
    doc["uniq_id"] = String(DEVICE_NAME) + "_temperature";
    doc["obj_id"]  = String(DEVICE_NAME) + "_temperature";

    doc["stat_t"]  = t_temp_state;
    doc["unit_of_measurement"] = "°C";
    doc["device_class"]        = "temperature";
    doc["state_class"]         = "measurement";

    doc["avty_t"]       = t_avail;
    doc["pl_avail"]     = "online";
    doc["pl_not_avail"] = "offline";

    JsonObject dev = doc["device"].to<JsonObject>();
    dev["name"] = FRIENDLY_NAME;
    dev["mf"]   = "DIY";
    dev["mdl"]  = "ESP32 C3 RGB Lamp";
    JsonArray ids = dev["ids"].to<JsonArray>();
    ids.add(DEVICE_NAME);

    char buf[512];
    size_t n = serializeJson(doc, buf, sizeof(buf));
    mqtt.publish(t_temp_disc.c_str(), (const uint8_t*)buf, (unsigned int)n, true);
    Serial.printf("[DISCOVERY] TEMP %s\n", buf);
  }

  // === Discovery pour capteur humidité ===
  {
    JsonDocument doc;

    doc["name"]    = String(FRIENDLY_NAME) + " Humidité";
    doc["uniq_id"] = String(DEVICE_NAME) + "_humidity";
    doc["obj_id"]  = String(DEVICE_NAME) + "_humidity";

    doc["stat_t"]  = t_hum_state;
    doc["unit_of_measurement"] = "%";
    doc["device_class"]        = "humidity";
    doc["state_class"]         = "measurement";

    doc["avty_t"]       = t_avail;
    doc["pl_avail"]     = "online";
    doc["pl_not_avail"] = "offline";

    JsonObject dev = doc["device"].to<JsonObject>();
    dev["name"] = FRIENDLY_NAME;
    dev["mf"]   = "DIY";
    dev["mdl"]  = "ESP32 C3 RGB Lamp";
    JsonArray ids = dev["ids"].to<JsonArray>();
    ids.add(DEVICE_NAME);

    char buf[512];
    size_t n = serializeJson(doc, buf, sizeof(buf));
    mqtt.publish(t_hum_disc.c_str(), (const uint8_t*)buf, (unsigned int)n, true);
    Serial.printf("[DISCOVERY] HUM %s\n", buf);
  }
}

/* ====== MQTT RX ====== */
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
  publishState();
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

/* ====== WIFI / MQTT ====== */
void wifiConnect() {
  Serial.println("[WiFi] Configuration IP statique...");
  if (!WiFi.config(local_IP, gateway, subnet, dns1, dns2)) {
    Serial.println("[WiFi] ERREUR config IP !");
  }
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  Serial.printf("[WiFi] Connexion a %s ...\n", WIFI_SSID);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    Serial.print(".");
    delay(250);
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] IP: %s\n",
              WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[WiFi] ECHEC (timeout)");
  }
}

void mqttEnsure() {
  while (!mqtt.connected()) {
    String clientId = String(DEVICE_NAME) + "_" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.printf("[MQTT] Connexion a %s:%u ...\n", MQTT_HOST, MQTT_PORT);
    bool ok = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWD,
                           t_avail.c_str(), 0, true, "offline");
    if (ok) {
      Serial.println("[MQTT] Connecte !");
      mqtt.publish(t_avail.c_str(), "online", true);
      Serial.printf("[MQTT] Subscribed to %s\n", t_cmd.c_str());
      mqtt.subscribe(t_cmd.c_str());

      publishDiscovery();
      publishState();
      publishDHTState();   // push un premier état des capteurs
      applyStrip();
    } else {
      Serial.printf("[MQTT] Echec (state=%d), retry...\n", mqtt.state());
      delay(1500);
    }
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

  wifiConnect();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(15);

  setupOTA();

}

void loop() {
    if (WiFi.status() != WL_CONNECTED) wifiConnect();
  if (!mqtt.connected()) mqttEnsure();
  mqtt.loop();

  loopOTA();

  loopEffects();

  // === DHT11 : lecture périodique ===
  unsigned long now = millis();
  if (now - lastDHTMillis > DHT_INTERVAL) {
    lastDHTMillis = now;
    publishDHTState();
  }
}
