#include "MqttConfig.h"

// Tu pourras passer ça en secrets PlatformIO plus tard
#ifndef MQTT_HOST
  #define MQTT_HOST   "192.168.1.31"
#endif

#ifndef MQTT_PORT
  #define MQTT_PORT   1883
#endif

#ifndef MQTT_USER
  #define MQTT_USER   "mqttuser"
#endif

#ifndef MQTT_PASSWD
  #define MQTT_PASSWD "aloha22"
#endif

// Identité
const char* const DEVICE_NAME   = "lampe_rgb1";
const char* const FRIENDLY_NAME = "Desk #2";

// Topics
String t_base   = String(DEVICE_NAME) + "/";
String t_cmd    = t_base + "light/set";
String t_state  = t_base + "light/state";
String t_avail  = t_base + "status";
String t_disc   = "homeassistant/light/" + String(DEVICE_NAME) + "/config";

String t_temp_state = t_base + "sensor/temperature";
String t_hum_state  = t_base + "sensor/humidity";
String t_temp_disc  = "homeassistant/sensor/" + String(DEVICE_NAME) + "_temperature/config";
String t_hum_disc   = "homeassistant/sensor/" + String(DEVICE_NAME) + "_humidity/config";

const MqttConfig MQTT_CONFIG = {
  .host     = MQTT_HOST,
  .port     = MQTT_PORT,
  .user     = MQTT_USER,
  .password = MQTT_PASSWD
};
