#include "wifi_station.h"

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"

#if defined(WOKWI_SIMULATION)
#define ACTIVE_SSID     "Wokwi-GUEST"
#define ACTIVE_PASSWORD ""
#else
#if __has_include("secrets.h")
#include "secrets.h"
#define ACTIVE_SSID     WIFI_SSID
#define ACTIVE_PASSWORD WIFI_PASSWORD
#else
#error "include/secrets.h not found. Copy include/secrets.example.h to include/secrets.h and set your Wi-Fi credentials."
#endif
#endif

namespace net {

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ACTIVE_SSID, ACTIVE_PASSWORD);

  Serial.printf("[WIFI] Connecting to \"%s\"", ACTIVE_SSID);
  uint32_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(config::kWifiRetryDelayMs);
    Serial.print('.');
    attempts++;
    if (attempts % 15 == 0) {
      Serial.printf("\n[WIFI] Still attempting connection to \"%s\"...", ACTIVE_SSID);
    }
  }

  Serial.printf("\n[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[WIFI] Open http://%s/ in your browser\n", WiFi.localIP().toString().c_str());
}

}  // namespace net
