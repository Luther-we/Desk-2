#include "MqttTemplates.h"
#include <ArduinoJson.h>
#include "MqttClient.h"
#include "MqttConfig.h"

void mqttPublishLampState(bool on,
                          uint8_t brightness,
                          uint8_t r, uint8_t g, uint8_t b,
                          LampEffect effect)
{
  JsonDocument doc;
  doc["state"]      = on ? "ON" : "OFF";
  doc["brightness"] = brightness;

  JsonObject color = doc["color"].to<JsonObject>();
  color["r"] = r;
  color["g"] = g;
  color["b"] = b;

  switch (effect) {
    case LampEffect::NONE:       break;
    case LampEffect::RAINBOW:    doc["effect"] = "rainbow"; break;
    case LampEffect::COLOR_WIPE: doc["effect"] = "color_wipe"; break;
  }

  char buf[256];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  mqttClient().publish(t_state.c_str(), (const uint8_t*)buf, (unsigned int)n, true);
}

void mqttPublishDht(float temperature, float humidity) {
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("[DHT] Valeurs invalides, publish annule");
    return;
  }

  Serial.print("[DHT] T = ");
  Serial.print(temperature);
  Serial.print(" °C, H = ");
  Serial.print(humidity);
  Serial.println(" %");

  char buf[16];

  dtostrf(temperature, 2, 2, buf);
  mqttClient().publish(t_temp_state.c_str(), buf, true);

  dtostrf(humidity, 2, 2, buf);
  mqttClient().publish(t_hum_state.c_str(), buf, true);
}

void mqttPublishDiscovery() {
  // Light
  {
    JsonDocument doc;

    doc["name"]    = FRIENDLY_NAME;
    doc["uniq_id"] = String(DEVICE_NAME) + "_light";
    doc["obj_id"]  = DEVICE_NAME;

    doc["cmd_t"]   = t_cmd;
    doc["stat_t"]  = t_state;

    doc["schema"]     = "json";
    doc["brightness"] = true;

    doc["color_mode"] = true;
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
    mqttClient().publish(t_disc.c_str(), (const uint8_t*)buf, (unsigned int)n, true);
    Serial.printf("[DISCOVERY] LIGHT %s\n", buf);
  }

  // Temp sensor
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
    mqttClient().publish(t_temp_disc.c_str(), (const uint8_t*)buf, (unsigned int)n, true);
    Serial.printf("[DISCOVERY] TEMP %s\n", buf);
  }

  // Hum sensor
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
    mqttClient().publish(t_hum_disc.c_str(), (const uint8_t*)buf, (unsigned int)n, true);
    Serial.printf("[DISCOVERY] HUM %s\n", buf);
  }
}
