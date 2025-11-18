#include <ArduinoOTA.h>

void setupOTA() {
  ArduinoOTA.setHostname("esp-button");
  ArduinoOTA
    .onStart([]() {
      Serial.println("[OTA] Start");
    })
    .onEnd([]() {
      Serial.println("\n[OTA] End");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("[OTA] Progress: %u%%\r", (progress * 100) / total);
    })
    .onError([](ota_error_t error) {
      Serial.printf("[OTA] Error[%u]\n", error);
    });

  ArduinoOTA.begin();
  Serial.println("[OTA] Ready");
}

void loopOTA() {
  ArduinoOTA.handle();
}