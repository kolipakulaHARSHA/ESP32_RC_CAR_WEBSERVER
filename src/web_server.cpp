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

// Embedded fallback page served if LittleFS filesystem is not uploaded yet.
const char kFallbackIndexHtml[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 RC Car (Tier 1)</title>
  <style>
    * { box-sizing: border-box; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: #0f172a; color: #f8fafc; margin: 0; padding: 20px; display: flex; flex-direction: column; align-items: center; min-height: 100vh; user-select: none; }
    h1 { margin: 0 0 10px 0; font-size: 1.6rem; letter-spacing: 1px; color: #38bdf8; }
    .badge-bar { display: flex; gap: 10px; margin-bottom: 20px; }
    .badge { padding: 4px 12px; border-radius: 999px; font-size: 0.85rem; font-weight: 600; text-transform: uppercase; }
    .badge-conn { background: #334155; color: #94a3b8; }
    .badge-conn.ok { background: #166534; color: #86efac; }
    .badge-state { background: #1e293b; color: #cbd5e1; border: 1px solid #475569; }
    .badge-state.failsafe { background: #7f1d1d; color: #fca5a5; border-color: #ef4444; }
    .badge-state.driving { background: #0369a1; color: #7dd3fc; }
    .controls { display: grid; grid-template-columns: repeat(3, 80px); grid-template-rows: repeat(3, 80px); gap: 12px; margin: 15px 0; }
    .btn { background: #1e293b; color: #38bdf8; border: 2px solid #38bdf8; border-radius: 16px; font-size: 2rem; display: flex; align-items: center; justify-content: center; cursor: pointer; touch-action: manipulation; transition: background 0.1s, transform 0.1s; }
    .btn:active, .btn.active { background: #38bdf8; color: #0f172a; transform: scale(0.95); }
    .btn-up { grid-column: 2; grid-row: 1; }
    .btn-left { grid-column: 1; grid-row: 2; }
    .btn-down { grid-column: 2; grid-row: 3; }
    .btn-right { grid-column: 3; grid-row: 2; }
    .speed-panel { margin-top: 15px; width: 260px; text-align: center; }
    .speed-label { font-size: 0.9rem; color: #94a3b8; margin-bottom: 6px; }
    input[type=range] { width: 100%; accent-color: #38bdf8; }
  </style>
</head>
<body>
  <h1>ESP32 RC CAR &bull; TIER 1</h1>
  <div class="badge-bar">
    <span id="conn-badge" class="badge badge-conn">Connecting...</span>
    <span id="state-badge" class="badge badge-state">IDLE</span>
  </div>
  <div class="controls">
    <button class="btn btn-up" data-cmd="up">&uarr;</button>
    <button class="btn btn-left" data-cmd="left">&larr;</button>
    <button class="btn btn-right" data-cmd="right">&rarr;</button>
    <button class="btn btn-down" data-cmd="down">&darr;</button>
  </div>
  <div class="speed-panel">
    <div class="speed-label">PWM Speed: <span id="spd-val">200</span> / 255</div>
    <input type="range" id="spd-slider" min="80" max="255" value="200">
  </div>
  <script>
    let ws = null, activeCmd = null, hbTimer = null;
    const connEl = document.getElementById('conn-badge');
    const stateEl = document.getElementById('state-badge');
    const spdEl = document.getElementById('spd-val');
    const slider = document.getElementById('spd-slider');

    function connect() {
      ws = new WebSocket('ws://' + location.host + '/ws');
      ws.onopen = () => {
        connEl.textContent = 'CONNECTED';
        connEl.classList.add('ok');
        startHeartbeat();
      };
      ws.onclose = () => {
        connEl.textContent = 'DISCONNECTED';
        connEl.classList.remove('ok');
        stateEl.textContent = 'DISCONNECTED';
        stateEl.className = 'badge badge-state failsafe';
        stopHeartbeat();
        setTimeout(connect, 1000);
      };
      ws.onmessage = (e) => {
        if (e.data.startsWith('state:')) {
          const s = e.data.substring(6);
          stateEl.textContent = s;
          stateEl.className = 'badge badge-state' + (s === 'FAILSAFE' ? ' failsafe' : (s === 'DRIVING' ? ' driving' : ''));
        }
      };
    }
    function send(msg) { if (ws && ws.readyState === WebSocket.OPEN) ws.send(msg); }
    function startHeartbeat() {
      clearInterval(hbTimer);
      hbTimer = setInterval(() => {
        if (activeCmd) { send(activeCmd); }
        else { send('hb'); }
      }, 100);
    }
    function stopHeartbeat() { clearInterval(hbTimer); }

    document.querySelectorAll('[data-cmd]').forEach(btn => {
      const cmd = btn.dataset.cmd;
      btn.addEventListener('pointerdown', (e) => {
        e.preventDefault();
        btn.setPointerCapture(e.pointerId);
        btn.classList.add('active');
        activeCmd = cmd;
        send(cmd);
      });
      btn.addEventListener('lostpointercapture', () => {
        btn.classList.remove('active');
        if (activeCmd === cmd) activeCmd = null;
        send('off');
      });
      btn.addEventListener('contextmenu', e => e.preventDefault());
    });
    window.addEventListener('blur', () => { activeCmd = null; send('off'); });
    slider.addEventListener('input', () => {
      spdEl.textContent = slider.value;
      send('speed:' + slider.value);
    });
    connect();
  </script>
</body>
</html>
)rawliteral";

void handleMessage(AsyncWebSocketClient* client, void* arg, const uint8_t* data, size_t length) {
  const auto* info = static_cast<const AwsFrameInfo*>(arg);

  const bool isSingleFrame = info->final && info->index == 0 && info->len == length;
  if (!isSingleFrame || info->opcode != WS_TEXT) {
    return;
  }

  drive::DriveMessage msg;
  if (!drive::parseCommand(data, length, msg)) {
    Serial.printf("[WS] Client #%lu: ignored unparsed packet (%u bytes)\n",
                  static_cast<unsigned long>(client->id()), static_cast<unsigned>(length));
    return;
  }

  drive::postMessage(msg);
}

void onWebSocketEvent(AsyncWebSocket* /*server*/, AsyncWebSocketClient* client, AwsEventType type,
                      void* arg, uint8_t* data, size_t length) {
  switch (type) {
    case WS_EVT_CONNECT: {
      Serial.printf("[WS] Client #%lu connected from %s\n",
                    static_cast<unsigned long>(client->id()),
                    client->remoteIP().toString().c_str());
      drive::DriveMessage msg{};
      msg.type = drive::MessageType::ClientConnected;
      msg.timestampMs = millis();
      drive::postMessage(msg);

      char buf[32];
      snprintf(buf, sizeof(buf), "state:%s", drive::stateToString(drive::getState()));
      client->text(buf);
      break;
    }
    case WS_EVT_DISCONNECT: {
      Serial.printf("[WS] Client #%lu disconnected\n", static_cast<unsigned long>(client->id()));
      drive::DriveMessage msg{};
      msg.type = drive::MessageType::ClientDisconnected;
      msg.timestampMs = millis();
      drive::postMessage(msg);
      break;
    }
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

  if (fsMounted && LittleFS.exists("/index.html")) {
    Serial.println("[HTTP] Serving web UI from LittleFS flash storage.");
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  } else {
    Serial.println("[HTTP] Serving built-in Tier 1 web UI (LittleFS not found or empty).");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send(200, "text/html", kFallbackIndexHtml);
    });
  }

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  return true;
}

void loop() {
  ws.cleanupClients();

  // Broadcast current vehicle state to connected clients periodically
  static uint32_t lastBroadcast = 0;
  static drive::VehicleState lastReportedState = drive::VehicleState::Idle;
  const uint32_t now = millis();
  const drive::VehicleState curState = drive::getState();

  if (ws.count() > 0 && (curState != lastReportedState || now - lastBroadcast > 500)) {
    lastBroadcast = now;
    lastReportedState = curState;
    char buf[32];
    snprintf(buf, sizeof(buf), "state:%s", drive::stateToString(curState));
    ws.textAll(buf);
  }
}

}  // namespace web
