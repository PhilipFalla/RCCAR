#include <WiFi.h>
#include <WebSocketsClient.h>

const char* ssid = "Philip";
const char* password = "simedas100pesos";
const char* websocket_server = "2fbb-2800-98-1122-1c36-b5ed-bd78-c9be-903e.ngrok-free.app";

WebSocketsClient webSocket;

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Configure WebSocket connection (WSS)
    webSocket.beginSSL(websocket_server, 443, "/");  // Use `beginSSL` for WSS
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    webSocket.loop();

    // Example: Send a test message every 2 seconds
    static unsigned long lastSendTime = 0;
    if (millis() - lastSendTime > 2000) {
        String message = "{\"type\":\"esp32\",\"value\":123}";
        webSocket.sendTXT(message);
        Serial.println("Sent: " + message);
        lastSendTime = millis();
    }
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
            Serial.printf("Received: %s\n", payload);
            break;
        case WStype_ERROR:
            Serial.println("WebSocket error");
            break;
    }
}