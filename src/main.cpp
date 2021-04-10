#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <ArduinoJson.h>

/* should contain
const char *ssid = "your network ssid";
const char *password = "your network password";
const char *hostname = "hostname of this device";
const char *mqttHostname = "mqtt hostname or address";
 */
#include "secret.h"

#define REED_PIN 2
#define SEND_INTERVAL 10 * 1000

#define ML_WATER_PER_TICK 7
#define SENSOR_AREA_IN_M2 (M_PI * 0.0555 * 0.0555)
#define ML_WATER_PER_M2_PER_TICK (ML_WATER_PER_TICK / SENSOR_AREA_IN_M2)


WiFiClient net;
MQTTClient client;

unsigned int lastSend = 0;

bool lastSensorStatus = false;
unsigned short sensorTriggerCount = 0;


void messageReceived(String &topic, String &payload) {

    // Note: Do not use the client in the callback to publish, subscribe or
    // unsubscribe as it may cause deadlocks when other things arrive while
    // sending and receiving acknowledgments. Instead, change a global variable,
    // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void connectWiFi() {

    delay(10);

    WiFi.mode(WIFI_STA);
    WiFi.hostname(hostname);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    randomSeed(micros());

}

void connectMqtt() {
    delay(10);

    client.begin(mqttHostname, net);
    client.onMessage(messageReceived);

    while (!client.connect("outsideRainSensor")) {
        delay(1000);
    }
}

void setup() {
    pinMode(REED_PIN, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    connectWiFi();
    connectMqtt();

    WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
}

void loop() {
    bool sensorClosed = digitalRead(REED_PIN);

    if(sensorClosed != lastSensorStatus) {
        lastSensorStatus = sensorClosed;
        if(sensorClosed) {
            sensorTriggerCount++;
        }
    }

    if (millis() - lastSend > SEND_INTERVAL) {
        lastSend = millis();

        StaticJsonDocument<200> json;
        json["rainTicks"] = sensorTriggerCount;
        json["rainMlPerM2"] = sensorTriggerCount * ML_WATER_PER_M2_PER_TICK;

        char jsonStr[200];
        serializeJson(json, jsonStr);

        client.publish("outside/rain", jsonStr);

        sensorTriggerCount = 0;
    }
}