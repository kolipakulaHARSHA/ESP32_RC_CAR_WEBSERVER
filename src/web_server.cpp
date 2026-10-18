#include "web_server.h"

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#include "config.h"
#include "drive_control.h"

namespace web {
namespace {

AsyncWebServer server(config::kHttpPort);
AsyncWebSocket ws(config::kWebSocketPath);

void handleMessage(AsyncWebSocketClient* client, void* arg, const uint8_t* data, size_t length) {
  const auto* info = static_cast<const AwsFrameInfo*>(arg);

  // Only accept complete, single-frame text messages.
  const bool isSingleFrame = info->final && info->index == 0 && info->len == length;
  if (!isSingleFrame || info->opcode != WS_TEXT) {
    return;
  }

  drive::Command command;
  if (!drive::parseCommand(data, length, command)) {
    Serial.printf("WS client #%lu: ignored unknown command (%u bytes)\n",
                  static_cast<unsigned long>(client->id()), static_cast<unsigned>(length));
    return;
  }

  drive::apply(command);
}

void onWebSocketEvent(AsyncWebSocket* /*server*/, AsyncWebSocketClient* client, AwsEventType type,
                      void* arg, uint8_t* data, size_t length) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WS client #%lu connected from %s\n", static_cast<unsigned long>(client->id()),
                    client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WS client #%lu disconnected\n", static_cast<unsigned long>(client->id()));
      break;
    case WS_EVT_DATA:
      handleMessage(client, arg, data, length);
      break;
    default:
      break;
  }
}

}  // namespace

bool begin() {
  const bool fsMounted = LittleFS.begin();

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  if (fsMounted) {
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  } else {
    Serial.println("ERROR: LittleFS mount failed. Upload the web UI with: pio run -t uploadfs");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send(503, "text/plain", "Web UI files missing. Run: pio run -t uploadfs");
    });
  }

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  return fsMounted;
}

void loop() {
  ws.cleanupClients();
}

}  // namespace web
