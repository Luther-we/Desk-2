#include "WifiConnect.h"

static WifiConfig   g_cfg;
static bool         g_initialized = false;
static unsigned long g_lastAttempt = 0;
static const unsigned long RECONNECT_INTERVAL = 15000; // 15 s

bool wifiIsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void wifiSetup(const WifiConfig& cfg) {
  g_cfg = cfg;
  g_initialized = true;

  Serial.println("[WiFi] Initialisation...");

  Serial.println("[WiFi] Configuration IP statique...");
  if (!WiFi.config(g_cfg.localIP, g_cfg.gateway, g_cfg.subnet, g_cfg.dns1, g_cfg.dns2)) {
    Serial.println("[WiFi] ERREUR config IP !");
  }

  WiFi.mode(WIFI_STA);

#if defined(ESP32)
  if (g_cfg.hostname && strlen(g_cfg.hostname) > 0) {
    WiFi.setHostname(g_cfg.hostname);
  }
#endif

  Serial.printf("[WiFi] Connexion a %s ...\n", g_cfg.ssid);
  WiFi.begin(g_cfg.ssid, g_cfg.password);

  g_lastAttempt = millis();
}

void wifiLoop() {
  if (!g_initialized) return;

  wl_status_t s = WiFi.status();

  if (s == WL_CONNECTED) {
    // On logue une seule fois au moment où ça se connecte
    static bool alreadyLogged = false;
    if (!alreadyLogged) {
      alreadyLogged = true;
      Serial.printf("[WiFi] CONNECTE - IP: %s\n",
                    WiFi.localIP().toString().c_str());
    }
    return;
  }

  // Ici : pas connecté
  static bool firstDisconnectLog = true;
  if (firstDisconnectLog) {
    Serial.println("[WiFi] Deconnecte, tentative de reconnexion...");
    firstDisconnectLog = false;
  }

  unsigned long now = millis();
  if (now - g_lastAttempt > RECONNECT_INTERVAL) {
    Serial.printf("[WiFi] Nouvelle tentative vers %s ...\n", g_cfg.ssid);
    WiFi.disconnect();
    WiFi.begin(g_cfg.ssid, g_cfg.password);
    g_lastAttempt = now;
  }
}
