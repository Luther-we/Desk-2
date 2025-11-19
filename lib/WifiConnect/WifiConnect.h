#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "WifiConfig.h" 

// À appeler dans setup()
void wifiSetup(const WifiConfig& cfg);

// À appeler dans loop()
void wifiLoop();

// Pour savoir si on est connecté
bool wifiIsConnected();
