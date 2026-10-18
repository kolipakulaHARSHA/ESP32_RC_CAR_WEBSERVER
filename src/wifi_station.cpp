#include "wifi_station.h"

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "include/secrets.h not found. Copy include/secrets.example.h to include/secrets.h and set your Wi-Fi credentials."
#endif

namespace net {

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("Connecting to Wi-Fi \"%s\"", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(config::kWifiRetryDelayMs);
    Serial.print('.');
  }

  Serial.printf("\nConnected. Control page: http://%s/\n", WiFi.localIP().toString().c_str());
}

}  // namespace net
