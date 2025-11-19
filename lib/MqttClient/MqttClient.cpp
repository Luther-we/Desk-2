#include <WiFi.h>
#include "MqttClient.h"
#include "MqttConfig.h"

static WiFiClient   g_wifiClient;
static PubSubClient g_mqtt(g_wifiClient);

PubSubClient& mqttClient() {
  return g_mqtt;
}

void mqttSetup() {
  g_mqtt.setServer(MQTT_CONFIG.host, MQTT_CONFIG.port);
  g_mqtt.setBufferSize(512);
  g_mqtt.setKeepAlive(30);
  g_mqtt.setSocketTimeout(15);
}

// Retourne true si on vient de se (re)connecter
bool mqttEnsureConnected() {
  if (g_mqtt.connected()) return false;

  while (!g_mqtt.connected()) {
    String clientId = String(DEVICE_NAME) + "_" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.printf("[MQTT] Connexion a %s:%u ...\n", MQTT_CONFIG.host, MQTT_CONFIG.port);

    bool ok = g_mqtt.connect(
      clientId.c_str(),
      MQTT_CONFIG.user,
      MQTT_CONFIG.password,
      t_avail.c_str(),   // Last Will topic
      0,
      true,
      "offline"
    );

    if (ok) {
      Serial.println("[MQTT] Connecte !");
      g_mqtt.publish(t_avail.c_str(), "online", true);
      Serial.printf("[MQTT] Subscribed to %s\n", t_cmd.c_str());
      g_mqtt.subscribe(t_cmd.c_str());
      return true;
    } else {
      Serial.printf("[MQTT] Echec (state=%d), retry...\n", g_mqtt.state());
      delay(1500);
    }
  }
  return true;
}

void mqttLoop() {
  g_mqtt.loop();
}
