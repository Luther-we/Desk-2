#pragma once
#include <Arduino.h>
#include <WiFi.h>

struct WifiConfig {
  const char* ssid;
  const char* password;
  IPAddress   localIP;
  IPAddress   gateway;
  IPAddress   subnet;
  IPAddress   dns1;
  IPAddress   dns2;
  const char* hostname;   // peut être nullptr si tu ne veux pas de hostname
};

// Cette config sera définie dans WifiConfig.cpp
extern const WifiConfig WIFI_CONFIG;
