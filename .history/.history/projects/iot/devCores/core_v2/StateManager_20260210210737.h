#pragma once
#include <Arduino.h>

#define MAX_SENSORS 8        // CT4 = 4 sensors max, leave room for expansion
#define MAX_NAME_LEN 16      // "ASHP-fl1" fits comfortably

struct SensorState {
    uint8_t sa;                      // Sensor/actuator ID
    float value;                     // Current value
    char name[MAX_NAME_LEN];         // Fixed buffer, no heap
    unsigned long timestamp;         // Last update (millis)
    bool valid;                      // Has been initialized?
    
    SensorState() : sa(0), value(0.0), timestamp(0), valid(false) {
        name[0] = '\0';
    }
};

class StateManager {
private:
    SensorState _states[MAX_SENSORS];  // Fixed array, predictable memory
    uint8_t _count;                     // How many active states
    
    // Find index of sensor by sa ID
    int8_t findIndex(uint8_t sa) {
        for (uint8_t i = 0; i < _count; i++) {
            if (_states[i].sa == sa) return i;
        }
        return -1;
    }
    
public:
    StateManager() : _count(0) {}
    
    void updateState(uint8_t sa, float value, const char* name) {
        int8_t idx = findIndex(sa);
        
        if (idx < 0) {
            // New sensor
            if (_count >= MAX_SENSORS) {
                Serial.println("ERR: Max sensors reached");
                return;
            }
            idx = _count++;
        }
        
        _states[idx].sa = sa;
        _states[idx].value = value;
        strncpy(_states[idx].name, name, MAX_NAME_LEN - 1);
        _states[idx].name[MAX_NAME_LEN - 1] = '\0';
        _states[idx].timestamp = millis();
        _states[idx].valid = true;
    }
    
    bool getState(uint8_t sa, SensorState& out) {
        int8_t idx = findIndex(sa);
        if (idx >= 0 && _states[idx].valid) {
            out = _states[idx];
            return true;
        }
        return false;
    }
    
    // Build JSON response in provided buffer
    // Returns bytes written (not including null terminator)
    int buildStateJson(uint8_t sa, char* buffer, size_t bufSize) {
        SensorState state;
        if (!getState(sa, state)) {
            return snprintf(buffer, bufSize, 
                "{\"sa\":%d,\"valid\":false}", sa);
        }
        
        return snprintf(buffer, bufSize,
            "{\"sa\":%d,\"val\":%.2f,\"name\":\"%s\",\"ts\":%lu}",
            state.sa, state.value, state.name, state.timestamp);
    }
    
    // Build response for multiple sensors
    int buildAllStatesJson(const uint8_t* saList, uint8_t saCount, 
                          char* buffer, size_t bufSize) {
        if (bufSize < 20) return 0;
        
        char* ptr = buffer;
        size_t remaining = bufSize;
        int written;
        
        written = snprintf(ptr, remaining, "{\"states\":[");
        ptr += written;
        remaining -= written;
        
        for (uint8_t i = 0; i < saCount; i++) {
            if (i > 0) {
                if (remaining < 2) break;
                *ptr++ = ',';
                remaining--;
            }
            
            written = buildStateJson(saList[i], ptr, remaining);
            ptr += written;
            remaining -= remaining;
            
            if (remaining < 10) break;  // Need room for closing
        }
        
        written = snprintf(ptr, remaining, "]}");
        ptr += written;
        
        return ptr - buffer;
    }
};
