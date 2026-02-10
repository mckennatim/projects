#pragma once
#include "Sensor.h"
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>
#include "Config.h"
#include "MqttManager.h"

// Provide access to the shared instance if needed globally, 
// or pass it in. We'll pass it in.

class CTSensor : public Sensor {
private:
    OneWire* _oneWire;
    DallasTemperature* _sensors;
    MqttManager* _mqtt;
    CT_Config _config;
    
    // Runtime state
    float _lastReportedValue = 1.1;