#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// ESP32 IP

const char* IP = "172.20.10.3";

// WiFi credentials
const char* ssid = "Philip";
const char* password = "simedas100pesos";

// Set up the web server on port 80
AsyncWebServer server(80);

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    // Handle incoming JSON data via HTTP POST
    server.on("/controller", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "JSON Received");
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // Convert received data into a string
        String jsonStr;
        for (size_t i = 0; i < len; i++) {
            jsonStr += (char)data[i];
        }
        Serial.println("Received JSON:");
        Serial.println(jsonStr);
    });

    // Start server
    server.begin();
}

void loop() {
}