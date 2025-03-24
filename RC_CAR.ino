#include <WiFi.h>
#include <WebSocketsClient.h>

const char* ssid = "id";
const char* password = "pass";
const char* websocket_server = "ws://your_server_ip:8080";

WebSocketsClient webSocket;

void setup() {
    const int max = 32767;
    const int min = -32768;
    int drive = 5;
    int input = 0;

  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  webSocket.begin(websocket_server);

  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket server");
      break;
    case WStype_CONNECTED:
      Serial.println("Connected to WebSocket server");
      break;
    case WStype_TEXT:
      Serial.println("Received text: ");
      Serial.println((char*)payload);
      break;
  }
}
