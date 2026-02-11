#pragma once
#include <Arduino.h>

class Sensor {
public:
    virtual ~Sensor() {}
    virtual void setup() = 0;
    virtual void loop() = 0;
    virtual bool handleMqttMessage(const char* subtopic, const char* payload) {
        return false; 
    }
};
