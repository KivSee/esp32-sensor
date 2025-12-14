#include <Arduino.h>
#include <WiFi.h>
#include <secrets.h>

// #include <ESPmDNS.h>
// #include <WiFiUdp.h>
// #include <ArduinoOTA.h>
#include <SPIFFS.h>
#include <fs_manager.h>
#include <mqtt_manager.h>
#include <ArduinoOTA.h>

#define MAX_THING_NAME_LENGTH 16
char thing_name[MAX_THING_NAME_LENGTH];

FsManager fsManager;
MQTTManager mqttManager;

// GPIO pin used
const int sensorPin = 13;
int previousValue = -1;  // Track previous sensor value (-1 = uninitialized)

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

    // init sensor IO as input
    pinMode(sensorPin, INPUT_PULLDOWN);

    // init wifi and send startup message
    ConnectToWifi();
    mqttManager.begin(MQTT_BROKER_IP, MQTT_BROKER_PORT, thing_name);
    if (mqttManager.connect())
    {
        Serial.println("connected to MQTT broker");
        String startupMsg = String("{\"message\":\"device started\",\"thing_name\":\"") + String(thing_name) + String("\"}");
        mqttManager.sendJSON(MQTT_TOPIC_SENSORS, startupMsg.c_str());
    }
    else
    {
        Serial.println("could not connect to MQTT broker");
    }

    // setup Over The Air updates feature
    setupOTA();
}

void loop() {
    // maintain MQTT connection
    mqttManager.loop();

    // sample GPIO and print its value
    int value = digitalRead(sensorPin);
    Serial.print("Time: ");
    Serial.print(millis());
    Serial.print(" Sensor value: ");
    Serial.println(value);

    // check if value changed and send MQTT message
    if (value != previousValue) {
        Serial.println("Sensor value changed! Sending MQTT message...");
        if (mqttManager.isConnected()) {
        mqttManager.sendSensorValue(MQTT_TOPIC_SENSORS, thing_name, value);
        } else {
        Serial.println("MQTT not connected, attempting reconnect...");
        if (mqttManager.reconnect()) {
            mqttManager.sendSensorValue(MQTT_TOPIC_SENSORS, thing_name, value);
        }
        }
        previousValue = value;
    }
    ArduinoOTA.handle();

    delay(500); // slow down sampling and output for load management
}
