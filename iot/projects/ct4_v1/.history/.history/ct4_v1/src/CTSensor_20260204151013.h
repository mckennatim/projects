#pragma once
#include "Sensor.h"
#include "Config.h"
#include <Adafruit_ADS1X15.h>
#include <Wire.h>

// Forward declaration - will be provided by whoever includes this
class MqttManager;

class CTSensor : public Sensor {
private:
    Adafruit_ADS1115 _ads;
    CT_Config* _configs;
    uint8_t _numSensors;
    unsigned long _lastReadTime;
    unsigned long _readInterval;
    float _zeroOffsetADC;
    MqttManager* _mqtt;
    
    void calibrateZeroOffset() {
        const int numSamples = 500;
        float sum = 0.0f;
        _ads.setGain(GAIN_ONE);
        delay(50);
        
        for (int i = 0; i < numSamples; i++) {
            sum += _ads.readADC_SingleEnded(0);
            delay(10);
        }
        
        _zeroOffsetADC = sum / numSamples;
        Serial.printf("Zero offset calibrated: %.2f ADC counts\n", _zeroOffsetADC);
        Serial.printf("This corresponds to: %.3f V bias\n", _zeroOffsetADC * 0.000125);
        _ads.setGain(GAIN_ONE);
    }
    
    float readCurrent(CT_Config& config) {
        _ads.setGain(config.gain);
        delay(100);
        _ads.readADC_SingleEnded(config.pin);  // Dummy read
        delay(10);
        
        const int samples = 2000;
        float sumSquares = 0.0;
        
        for (int i = 0; i < samples; i++) {
            int16_t reading = _ads.readADC_SingleEnded(config.pin);
            float acComponent = reading - _zeroOffsetADC;
            sumSquares += acComponent * acComponent;
        }
        
        float rms_counts = sqrt(sumSquares / samples);
        float rms_voltage = rms_counts * config.lsbVolts;
        
        // Convert voltage to current using calibration
        // I = (V - b) / m, where m is turnsRatio * rBurden
        float m = config.turnsRatio * config.rBurden;
        float rms_current = (rms_voltage - 0.0) / m;
        
        // Apply calibration factor
        rms_current *= config.calibration;
        
        return rms_current;
    }

public:
    CTSensor(CT_Config* configs, uint8_t numSensors, MqttManager* mqtt, unsigned long readInterval = 30000) 
        : _configs(configs), _numSensors(numSensors), _mqtt(mqtt), 
          _readInterval(readInterval), _lastReadTime(0), _zeroOffsetADC(0) {}
    
    void setup() override {
        Serial.println("=== CTSensor Setup ===");
        
        // Initialize I2C
        Wire.begin(I2C_SDA, I2C_SCL);
        Wire.setClock(400000);
        
        // Initialize ADS1115
        if (!_ads.begin(0x48, &Wire)) {
            Serial.println("ERROR: Failed to initialize ADS1115!");
            Serial.printf("Using Pins SDA: %d, SCL: %d\n", I2C_SDA, I2C_SCL);
            return;
        }
        
        _ads.setGain(GAIN_ONE);
        Serial.println("ADS1115 initialized successfully");
        
        // Calibrate zero offset
        Serial.println("Calibrating zero offset... ensure CT is not measuring any current");
        delay(3000);
        calibrateZeroOffset();
        
        Serial.printf("Will read %d sensors every %lu ms\n", _numSensors, _readInterval);
    }
    
    void loop() override {
        unsigned long now = millis();
        
        // Take readings at specified interval
        if (now - _lastReadTime >= _readInterval) {
            _lastReadTime = now;
            
            Serial.println("\n=== Taking CT Readings ===");
            
            for (uint8_t i = 0; i < _numSensors; i++) {
                float current = readCurrent(_configs[i]);
                
                // Only report if above threshold
                if (current >= _configs[i].minPower) {
                    Serial.printf("%s (CH%d): %.2f A\n", 
                        _configs[i].name, _configs[i].pin, current);
                    
                    // Publish to MQTT
                    if (_mqtt) {
                        String topic = String("sensors/ct/") + _configs[i].name;
                        String payload = String(current, 2);
                        _mqtt->publish(topic, payload);
                    }
                } else {
                    Serial.printf("%s (CH%d): %.2f A (below threshold)\n", 
                        _configs[i].name, _configs[i].pin, current);
                }
            }
            
            Serial.println("=== Readings Complete ===\n");
        }
    }
    
    bool handleMqttMessage(const String& topic, const String& payload) override {
        // Handle commands like "sensors/ct/read" to force a reading
        if (topic == "sensors/ct/read") {
            Serial.println("Forcing CT read via MQTT command");
            _lastReadTime = 0;  // Force next loop to read
            return true;
        }
        
        // Handle interval change: "sensors/ct/interval"
        if (topic == "sensors/ct/interval") {
            unsigned long newInterval = payload.toInt() * 1000;  // Convert seconds to ms
            if (newInterval >= 1000) {
                _readInterval = newInterval;
                Serial.printf("Read interval changed to %lu ms\n", _readInterval);
                return true;
            }
        }
        
        return false;
    }
};
