#include "MqttManager.h"
#include "Config.h" 

MqttManager::MqttManager(PubSubClient& client, const char* devId, const char* user, const char* pwd)
    : _client(client), _user(user), _password(pwd), _lastReconnectAttempt(0), _appCallback(nullptr) {
    strncpy(_deviceId, devId, sizeof(_deviceId) - 1);
    _deviceId[sizeof(_deviceId) - 1] = '\0';
}

void MqttManager::begin(const char* server, uint16_t port) {
    _client.setServer(server, port);
}

void MqttManager::setCallback(MqttAppCallback cb) {
    _appCallback = cb;
}

void MqttManager::_subscribeToTopics() {
    Serial.println("Subscribing to topics:");
    
    #ifdef NUMTOPICS
    // Subscribe to specific topics defined in Config.h
    for (int i = 0; i < NUMTOPICS; i++) {
        char topic[50];
        strcpy(topic, _deviceId);
        strcat(topic, "/");
        strcat(topic, subTopics[i]);
        
        bool success = _client.subscribe(topic);
        Serial.printf("  %s: %s\n", topic, success ? "OK" : "FAILED");
    }
    #else
    // Fallback to wildcard if no topics defined
    char subPattern[30];
    strcpy(subPattern, _deviceId);
    strcat(subPattern, "/#");
    _client.subscribe(subPattern);
    Serial.printf("  %s: OK\n", subPattern);
    #endif
}

void MqttManager::_onConnect() {
    _subscribeToTopics();
    Serial.println("MQTT Connected");
    
    // Iteration 1: Send the Time Sync Message
    #ifdef MSG_TIME_TOPIC
    Serial.println("Sending Time Request...");
    publish(MSG_TIME_TOPIC, MSG_TIME_PAYLOAD);
    #endif
}

void MqttManager::loop() {
    if (!_client.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > _reconnectInterval) {
            _lastReconnectAttempt = now;
            // Attempt to connect
            Serial.print("MqttManager: Attempting connection...");
            
            if (_client.connect(_deviceId, _user, _password)) {
                _onConnect();
            } else {
                Serial.print("failed, rc=");
                Serial.println(_client.state());
            }
        }
    } else {
        _client.loop();
    }
}

void MqttManager::onMessage(char* topic, uint8_t* payload, unsigned int length) {
    // Create null-terminated payload buffer
    #ifndef MQTT_MAX_PACKET_SIZE
    #define MQTT_MAX_PACKET_SIZE 512
    #endif
    
    char payloadBuffer[MQTT_MAX_PACKET_SIZE + 1];
    size_t copyLen = (length < MQTT_MAX_PACKET_SIZE) ? length : MQTT_MAX_PACKET_SIZE;
    memcpy(payloadBuffer, payload, copyLen);
    payloadBuffer[copyLen] = '\0';
    
    // Check if topic starts with device ID
    size_t devIdLen = strlen(_deviceId);
    if (strncmp(topic, _deviceId, devIdLen) == 0 && topic[devIdLen] == '/') {
        // Extract relative topic (part after "DEVICEID/")
        char* relativeTopic = topic + devIdLen + 1;
        
        if (_appCallback) {
            _appCallback(relativeTopic, payloadBuffer);
        }
    }
    // else: ignore messages not for this device
}

void MqttManager::publish(const char* subtopic, const char* message) {
    if (_client.connected()) {
        char fullTopic[100];
        strcpy(fullTopic, _deviceId);
        strcat(fullTopic, "/");
        strcat(fullTopic, subtopic);
        _client.publish(fullTopic, message, true); // Retain=true per legacy example
    }
}
