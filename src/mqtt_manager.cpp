#include "mqtt_manager.h"
#include <ArduinoJson.h>

MQTTManager::MQTTManager() : mqttClient(wifiClient) {
    _broker = nullptr;
    _port = 1883;
    _clientId = nullptr;
    _username = nullptr;
    _password = nullptr;
    _lwt_topic = nullptr;
    _lwt_message = nullptr;
    _lwt_qos = 0;
    _lwt_retain = false;
}

bool MQTTManager::begin(const char* broker, uint16_t port, const char* clientId) {
    _broker = broker;
    _port = port;
    _clientId = clientId;

    mqttClient.setServer(_broker, _port);

    Serial.println("MQTT Manager initialized");
    Serial.printf("Broker: %s:%d\n", _broker, _port);
    Serial.printf("Client ID: %s\n", _clientId);

    return true;
}

bool MQTTManager::connect(const char* username, const char* password) {
    _username = username;
    _password = password;

    if (!WiFi.isConnected()) {
        Serial.println("WiFi not connected. Cannot connect to MQTT broker.");
        return false;
    }

    Serial.printf("Connecting to MQTT broker %s:%d...\n", _broker, _port);

    bool connected = false;

    // Connect with or without credentials and Last Will
    if (_lwt_topic != nullptr && _lwt_message != nullptr) {
        // Connect with Last Will and Testament
        if (_username != nullptr && _password != nullptr) {
            connected = mqttClient.connect(_clientId, _username, _password,
                                          _lwt_topic, _lwt_qos, _lwt_retain, _lwt_message);
        } else {
            connected = mqttClient.connect(_clientId, _lwt_topic, _lwt_qos, _lwt_retain, _lwt_message);
        }
    } else {
        // Connect without Last Will
        if (_username != nullptr && _password != nullptr) {
            connected = mqttClient.connect(_clientId, _username, _password);
        } else {
            connected = mqttClient.connect(_clientId);
        }
    }

    if (connected) {
        Serial.println("MQTT connected successfully!");
        return true;
    } else {
        Serial.print("MQTT connection failed, rc=");
        Serial.println(mqttClient.state());
        return false;
    }
}

bool MQTTManager::reconnect(const char* username, const char* password) {
    if (_username != nullptr) {
        username = _username;
    }
    if (_password != nullptr) {
        password = _password;
    }

    return connect(username, password);
}

bool MQTTManager::isConnected() {
    return mqttClient.connected();
}

void MQTTManager::loop() {
    if (!mqttClient.connected()) {
        // Optionally auto-reconnect here
        // reconnect(_username, _password);
    } else {
        mqttClient.loop();
    }
}

bool MQTTManager::sendSensorValue(const char* topic, const char* sensorName, float value) {
    JsonDocument doc;
    doc["sensor"] = sensorName;
    doc["value"] = value;
    doc["timestamp"] = millis();

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    return sendJSON(topic, jsonBuffer);
}

bool MQTTManager::sendSensorValue(const char* topic, const char* sensorName, int value) {
    JsonDocument doc;
    doc["sensor"] = sensorName;
    doc["value"] = value;
    doc["timestamp"] = millis();

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    return sendJSON(topic, jsonBuffer);
}

bool MQTTManager::sendSensorValue(const char* topic, const char* sensorName, bool value) {
    JsonDocument doc;
    doc["sensor"] = sensorName;
    doc["value"] = value;
    doc["timestamp"] = millis();

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    return sendJSON(topic, jsonBuffer);
}

bool MQTTManager::sendJSON(const char* topic, const char* jsonMessage) {
    if (!mqttClient.connected()) {
        Serial.println("MQTT not connected. Cannot publish message.");
        return false;
    }

    bool published = mqttClient.publish(topic, jsonMessage);

    if (published) {
        Serial.printf("Published to %s: %s\n", topic, jsonMessage);
    } else {
        Serial.printf("Failed to publish to %s\n", topic);
    }

    return published;
}

void MQTTManager::setCallback(void (*callback)(char*, uint8_t*, unsigned int)) {
    mqttClient.setCallback(callback);
}

bool MQTTManager::subscribe(const char* topic) {
    if (!mqttClient.connected()) {
        Serial.println("MQTT not connected. Cannot subscribe.");
        return false;
    }

    bool subscribed = mqttClient.subscribe(topic);

    if (subscribed) {
        Serial.printf("Subscribed to topic: %s\n", topic);
    } else {
        Serial.printf("Failed to subscribe to topic: %s\n", topic);
    }

    return subscribed;
}

void MQTTManager::setLastWill(const char* topic, const char* message, uint8_t qos, bool retain) {
    _lwt_topic = topic;
    _lwt_message = message;
    _lwt_qos = qos;
    _lwt_retain = retain;

    Serial.printf("Last Will set: topic=%s, message=%s\n", topic, message);
}
