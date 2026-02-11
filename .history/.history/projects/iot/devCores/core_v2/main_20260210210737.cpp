#include <Arduino.h>
#include <vector>
#include "Config.h"
#include "MqttManager.h"
#include "connWIFI.h" // Reuse existing WiFi wrapper
#include "Sensor.h"
#include "ProjectFactory.h" // <-- Defines configureSensors() and project globals
#include "StateManager.h"

// --- Global Infrastructure ---
WiFiClient espClient;
PubSubClient client(espClient);
MqttManager mqtt(client, DEV_ID, MQTT_USER, MQTT_PASS);

std::vector<Sensor*> sensors;
StateManager stateManager;

// --- Buffer Sizes for Request Handling ---
#define REQ_BUFFER_SIZE 256
#define RESP_BUFFER_SIZE 512

// --- State Request Handler ---

// Simple JSON parser for {"sa":[0,1,2,3],"req":"srstate"}
bool parseStateRequest(const char* payload, uint8_t* saList, uint8_t& saCount) {
    saCount = 0;
    
    // Find "req":"srstate"
    const char* reqPtr = strstr(payload, "\"req\"");
    if (!reqPtr) return false;
    
    const char* srstate = strstr(reqPtr, "srstate");
    if (!srstate) return false;
    
    // Find "sa":[ array
    const char* saPtr = strstr(payload, "\"sa\"");
    if (!saPtr) return false;
    
    const char* arrayStart = strchr(saPtr, '[');
    if (!arrayStart) return false;
    
    // Parse array of numbers
    const char* ptr = arrayStart + 1;
    while (*ptr && *ptr != ']' && saCount < MAX_SENSORS) {
        while (*ptr == ' ' || *ptr == ',') ptr++; // Skip whitespace/commas
        
        if (*ptr >= '0' && *ptr <= '9') {
            saList[saCount++] = atoi(ptr);
            while (*ptr >= '0' && *ptr <= '9') ptr++;  // Skip number
        } else {
            ptr++;
        }
    }
    
    return saCount > 0;
}

void handleStateRequest(const char* payload, size_t length) {
    uint8_t saList[MAX_SENSORS];
    uint8_t saCount;
    
    if (!parseStateRequest(payload, saList, saCount)) {
        Serial.println("ERR: Failed to parse state request");
        return;
    }
    
    Serial.printf("State request for %d sensors\n", saCount);
    
    // Build response
    char response[RESP_BUFFER_SIZE];
    int len = stateManager.buildAllStatesJson(saList, saCount, 
                                              response, sizeof(response));
    
    if (len > 0) {
        mqtt.publish("state", response);
        Serial.printf(">> State: %s\n", response);
    }
}

// --- Callbacks ---

// 1. The Shim for PubSubClient
void globalMqttCallback(char* topic, byte* payload, unsigned int length) {
    mqtt.onMessage(topic, payload, length);
}

// 2. The App Logic / Router
void appMqttCallback(const char* topic, const char* payload) {
    Serial.printf("Pkt: -> %s %s\n", topic, payload);

    // Iteration 1: Catch the Time Response
    if (strcmp(topic, MSG_TIME_TOPIC) == 0) {
         Serial.println(">> Time Sync Received!");
         Serial.println(payload);
         return;
    }

    // Handle state requests
    if (strcmp(topic, "req") == 0) {
        handleStateRequest(payload, strlen(payload));
        return;
    }

    // Pass to sensors
    for (auto& sensor : sensors) {
        if (sensor->handleMqttMessage(topic, payload)) return;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== Iteration 3: Generic Main ===");

    // 1. Connect WiFi
    if (!setupWIFI()) {
        Serial.println("WiFi Failed");
        // ESP.restart(); // Optional
    }

    // 2. Connect MQTT
    mqtt.begin(MQTT_SERVER, MQTT_PORT);
    mqtt.setCallback(appMqttCallback);
    client.setCallback(globalMqttCallback);

    // 3. Setup Sensors (Project Factory)
    configureSensors(sensors, &mqtt, &stateManager); 
    
    // Only call setup() on the created objects
    for (auto& s : sensors) s->setup();

    Serial.println("Setup Complete, entering loop...");
    Serial.printf("Total Sensors: %d\n", sensors.size());
}

// Round-Robin Scheduler State
unsigned long lastSensorRunTime = 0;
size_t currentSensorIndex = 0;
bool isSensorActive = false;
// Interval between completion of one sensor and start of next
const unsigned long SENSOR_INTERVAL = 3000; 

void loop() {
    // 1. Always run MQTT (KeepAlive & Messages)
    mqtt.loop();

    // 2. Scheduled Sensor Execution
    if (isSensorActive) {
        // Run the active state machine
        CTSensor* sensor = (CTSensor*)sensors[currentSensorIndex]; // Safe cast for this project
        sensor->loop();
        
        if (sensor->isIdle()) {
            Serial.printf("[Scheduler] Sensor %d finished\n", currentSensorIndex);
            isSensorActive = false;
            lastSensorRunTime = millis();
            
            // Move index for next time
            currentSensorIndex++;
            if (currentSensorIndex >= sensors.size()) {
                currentSensorIndex = 0;
            }
        }
    } else {
        // Wait for interval
        if (millis() - lastSensorRunTime > SENSOR_INTERVAL) {
             if (!sensors.empty()) {
                Serial.printf("[Scheduler] Starting Sensor %d\n", currentSensorIndex);
                CTSensor* sensor = (CTSensor*)sensors[currentSensorIndex];
                sensor->startReading();
                isSensorActive = true;
             }
        }
    }
}
