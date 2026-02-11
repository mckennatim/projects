#pragma once
#include <Arduino.h>
#include <PubSubClient.h>
#include <functional>

// Cross-platform WiFi include
#ifdef ESP32
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

using MqttAppCallback = std::function<void(const char* topic, const char* payload)>;

class MqttManager {
private:
    PubSubClient& _client;
    char _deviceId[20];
    const char* _user;
    const char* _password;
    
    unsigned long _lastReconnectAttempt;
    const unsigned long _reconnectInterval = 5000;
    MqttAppCallback _appCallback;

    void _subscribeToTopics();
    void _onConnect();

public:
    MqttManager(PubSubClient& client, const char* devId, const char* user, const char* pwd);
    void begin(const char* server, uint16_t port);
    void setCallback(MqttAppCallback cb);
    void onMessage(char* topic, uint8_t* payload, unsigned int length);
    void loop();
    void publish(const char* subtopic, const char* message);
};

