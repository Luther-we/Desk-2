#include "WifiConfig.h"

// Macros venant de l'env (déjà définies via build_flags)
#ifndef WIFI_SSID
  #define WIFI_SSID ""
#endif

#ifndef WIFI_PASS
  #define WIFI_PASS ""
#endif

// IP statique
static IPAddress local_IP(192, 168, 1, 19);
static IPAddress gateway (192, 168, 1, 1);
static IPAddress subnet  (255, 255, 255, 0);
static IPAddress dns1    (1, 1, 1, 1);
static IPAddress dns2    (8, 8, 8, 8);

static const char* ESP_WIFI_NAME = "Desk #2";

const WifiConfig WIFI_CONFIG = {
  .ssid     = WIFI_SSID,
  .password = WIFI_PASS,
  .localIP  = local_IP,
  .gateway  = gateway,
  .subnet   = subnet,
  .dns1     = dns1,
  .dns2     = dns2,
  .hostname = ESP_WIFI_NAME
};
