/* Control horizontal blinds using a 28BYJ-48
 * stepper motor. Using my custom library for
 * stepper control. Send position as a binary
 * encoded integer via the websocket.
 */

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include "Simple28BYJ48.h"
#include "credentials.h"

// Globals
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
Simple28BYJ48 stepper(12, 14, 27, 26);
WebSocketsServer webSocket = WebSocketsServer(80);

int bin_to_int_le(uint8_t* bin)
{
    // Convert binary data to int (little-endian representation)
    return (bin[0] << 24) | (bin[1] << 16) | (bin[2] << 8) | (bin[3]);
}

void websocket_event(uint8_t num, WStype_t type, uint8_t* payload, size_t length)
{
    switch (type) {
    case WStype_BIN: {
        stepper.set_target_pos(bin_to_int_le(payload));
        // webSocket.sendBIN(num, payload, length); //optional response
    } break;
    // Other types ignored at the moment!
    case WStype_TEXT:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    case WStype_DISCONNECTED:
    case WStype_CONNECTED:
    default:
        break;
    }
}

void setup()
{
    Serial.begin(9600);
    pinMode(stepper.in1, OUTPUT);
    pinMode(stepper.in2, OUTPUT);
    pinMode(stepper.in3, OUTPUT);
    pinMode(stepper.in4, OUTPUT);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        }
        else { // U_SPIFFS
            type = "filesystem";
        }
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        }
        else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        }
        else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        }
        else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        }
        else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });
    ArduinoOTA.begin();

    // Start WebSocket server and assign a callback function
    webSocket.begin();
    webSocket.onEvent(websocket_event);
}

void loop()
{
    ArduinoOTA.handle();
    webSocket.loop();
    stepper.keep_target_pos();
}
