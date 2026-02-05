#pragma once
#include <Arduino.h>

// Base class for sensors that can be managed by core_v1
class Sensor {
public:
    virtual ~Sensor() {}
    virtual void setup() = 0;
    virtual void loop() = 0;
    virtual bool handleMqttMessage(const String& topic, const String& payload) { return false; }
};
