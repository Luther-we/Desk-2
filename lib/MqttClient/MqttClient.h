#pragma once
#include <Arduino.h>
#include <PubSubClient.h>

// Initialise le client (host, port, buffer, keepalive…)
void mqttSetup();

// À appeler dans loop()
void mqttLoop();

// Assure la connexion MQTT (bloquant comme ton code actuel).
// Retourne true si une nouvelle connexion vient d'être établie.
bool mqttEnsureConnected();

// Accès au client pour setCallback / subscribe / autres
PubSubClient& mqttClient();
