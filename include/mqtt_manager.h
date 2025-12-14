#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#ifndef MQTT_BROKER_PORT
#define MQTT_BROKER_PORT 1883
#endif //MQTT_BROKER_PORT

#define MQTT_TOPIC_SENSORS "sensors"

class MQTTManager {
public:
    MQTTManager();

    // Initialize MQTT connection
    bool begin(const char* broker, uint16_t port, const char* clientId);

    // Connect to MQTT broker with optional credentials
    bool connect(const char* username = nullptr, const char* password = nullptr);

    // Reconnect if connection is lost
    bool reconnect(const char* username = nullptr, const char* password = nullptr);

    // Check if connected
    bool isConnected();

    // Maintain MQTT connection (call in loop)
    void loop();

    // Send sensor value in JSON format
    bool sendSensorValue(const char* topic, const char* sensorName, float value);
    bool sendSensorValue(const char* topic, const char* sensorName, int value);
    bool sendSensorValue(const char* topic, const char* sensorName, bool value);

    // Send custom JSON message
    bool sendJSON(const char* topic, const char* jsonMessage);

    // Set callback for incoming messages
    void setCallback(void (*callback)(char*, uint8_t*, unsigned int));

    // Subscribe to a topic
    bool subscribe(const char* topic);

private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    const char* _broker;
    uint16_t _port;
    const char* _clientId;
    const char* _username;
    const char* _password;
};

#endif // MQTT_MANAGER_H
