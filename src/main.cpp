#include <Arduino.h>
#include <WiFi.h>
#include <secrets.h>

#include <SPIFFS.h>
#include <fs_manager.h>
#include <mqtt_manager.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#define MAX_THING_NAME_LENGTH 16
char thing_name[MAX_THING_NAME_LENGTH];
char mqtt_topic[64];  // Buffer for topic + thing_name

FsManager fsManager;
MQTTManager mqttManager;

// GPIO pin used
const int sensorPin = 13;
int previousValue = -1;  // Track previous sensor value (-1 = uninitialized)
unsigned long lastHeartbeat = 0;  // Track last heartbeat time
const unsigned long heartbeatInterval = 5000;  // 5 seconds

void setupOTA() {
    // Port defaults to 3232
    // ArduinoOTA.setPort(3232);

    // Hostname defaults to esp3232-[MAC]
    ArduinoOTA.setHostname(thing_name);

    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

    ArduinoOTA.begin();
}

void ConnectToWifi()
{
  if (WiFi.status() == WL_CONNECTED)
    return;

  while (true)
  {
    unsigned int connectStartTime = millis();
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, WIFI_PASSWORD);
    Serial.printf("Attempting to connect to SSID: ");
    Serial.printf(SSID);
    while (millis() - connectStartTime < 10000)
    {
      Serial.print(".");
      delay(1000);
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.println("connected to wifi");
        return;
      }
    }
    Serial.println(" could not connect for 10 seconds. retry");
  }
}

void setup() {
    Serial.begin(115200);
    Serial.println("LD1020 Motion Sensor");

    // mount filesystem and read thing name
    fsManager.setup();
    bool hasThingName = fsManager.ReadThingName(thing_name, 16);
    while (!hasThingName)
    {
        String str = "no name";
        strcpy(thing_name, str.c_str());
        Serial.println("Thing name not configured - upload 'thing_info' file to continue");
        delay(5000);
    }
    Serial.print("Thing name: "); Serial.println(thing_name);

    // Build MQTT topic with thing name
    snprintf(mqtt_topic, sizeof(mqtt_topic), "%s/%s", MQTT_TOPIC_SENSORS, thing_name);
    Serial.print("MQTT topic: "); Serial.println(mqtt_topic);

    // init sensor IO as input
    pinMode(sensorPin, INPUT_PULLDOWN);

    // init wifi and send startup message
    ConnectToWifi();
    mqttManager.begin(MQTT_BROKER_IP, MQTT_BROKER_PORT, thing_name);

    // Set Last Will and Testament - send value 0 if device dies
    JsonDocument lwtDoc;
    lwtDoc["sensor"] = thing_name;
    lwtDoc["value"] = 0;
    lwtDoc["alive"] = false;
    lwtDoc["timestamp"] = 0;
    char lwtMessage[256];
    serializeJson(lwtDoc, lwtMessage);
    mqttManager.setLastWill(mqtt_topic, lwtMessage, 0, true);

    if (mqttManager.connect())
    {
        Serial.println("connected to MQTT broker");
        JsonDocument startupDoc;
        startupDoc["message"] = "device started";
        startupDoc["thing_name"] = thing_name;
        startupDoc["alive"] = true;
        char startupMsg[256];
        serializeJson(startupDoc, startupMsg);
        mqttManager.sendJSON(mqtt_topic, startupMsg);
    }
    else
    {
        Serial.println("could not connect to MQTT broker");
    }

    // setup Over The Air updates feature
    setupOTA();
}

void loop() {
    ConnectToWifi();

    // maintain MQTT connection
    mqttManager.loop();

    // sample GPIO and print its value
    int value = digitalRead(sensorPin);
    Serial.print("Time: ");
    Serial.print(millis());
    Serial.print(" Sensor value: ");
    Serial.println(value);

    // Send MQTT message if value changed OR if 5 seconds elapsed
    bool valueChanged = (value != previousValue);
    bool heartbeatDue = (millis() - lastHeartbeat >= heartbeatInterval);

    // if (valueChanged || heartbeatDue) { // restore for heartbeat sending
    if (valueChanged) {
        if (valueChanged) {
            Serial.println("Sensor value changed! Sending MQTT message...");
        } else {
            Serial.println("Heartbeat due. Sending MQTT message...");
        }

        if (mqttManager.isConnected()) {
            // Send message with alive=true
            JsonDocument doc;
            doc["sensor"] = thing_name;
            doc["value"] = value;
            doc["alive"] = true;
            doc["timestamp"] = millis();
            char jsonMsg[256];
            serializeJson(doc, jsonMsg);
            mqttManager.sendJSON(mqtt_topic, jsonMsg);
            lastHeartbeat = millis();
        } else {
            Serial.println("MQTT not connected, attempting reconnect...");
            if (mqttManager.reconnect()) {
                JsonDocument doc;
                doc["sensor"] = thing_name;
                doc["value"] = value;
                doc["alive"] = true;
                doc["timestamp"] = millis();
                char jsonMsg[256];
                serializeJson(doc, jsonMsg);
                mqttManager.sendJSON(mqtt_topic, jsonMsg);
                lastHeartbeat = millis();
            }
        }
        previousValue = value;
    }
    ArduinoOTA.handle();

    delay(500); // slow down sampling and output for load management
}
