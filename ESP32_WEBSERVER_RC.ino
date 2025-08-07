#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

const char* ssid = "OnePlus";
const char* password = "12345689";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

#define IN1 33
#define IN2 25
#define IN3 26
#define IN4 27

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "up") == 0) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    } else if (strcmp((char*)data, "down") == 0) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
    } else if (strcmp((char*)data, "right") == 0) {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
    } else if (strcmp((char*)data, "left") == 0) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    } else if (strcmp((char*)data, "off") == 0) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println(WiFi.localIP());

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", 
    "<!DOCTYPE html><html>"
    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<link rel=\"icon\" href=\"data:,\">"
    "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; background: black; color: white;}"
    ".button { background-color: red; border: none; color: white; padding: 16px 40px; border-radius: 12px;"
    "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; user-select: none;}"
    ".button2 {background-color: #555555;}"
    ".left {float: left; width: 50%; text-align: center;}"
    ".right {float: right; width: 50%; text-align: center; margin-top: 50px;}</style></head>"
    "<body><h1>ESP32 RC CAR</h1>"
    "<div class=\"left\">"
    "<p><button class=\"button\" onmousedown=\"ws.send('up')\" onmouseup=\"ws.send('off')\" ontouchstart=\"ws.send('up')\" ontouchend=\"ws.send('off')\">&#8593;</button></p>"
    "<p><button class=\"button\" onmousedown=\"ws.send('down')\" onmouseup=\"ws.send('off')\" ontouchstart=\"ws.send('down')\" ontouchend=\"ws.send('off')\">&#8595;</button></p>"
    "</div>"
    "<div class=\"right\">"
    "<p><button class=\"button\" onmousedown=\"ws.send('left')\" onmouseup=\"ws.send('off')\" ontouchstart=\"ws.send('left')\" ontouchend=\"ws.send('off')\">&#8592;</button>"
    "<button class=\"button\" onmousedown=\"ws.send('right')\" onmouseup=\"ws.send('off')\" ontouchstart=\"ws.send('right')\" ontouchend=\"ws.send('off')\">&#8594;</button></p>"
    "</div>"
    "<script>var ws = new WebSocket('ws://' + location.hostname + '/ws');</script>"
    "</body></html>");
  });

  server.begin();
}

void loop() {
  ws.cleanupClients();
}