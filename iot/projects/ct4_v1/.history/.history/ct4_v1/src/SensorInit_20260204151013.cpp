#include "SensorInit.h"
#include <MqttManager.h>  // Full definition needed here
#include "CTSensor.h"
#include "Config.h"

void configureSensors(std::vector<Sensor*>& sensors, MqttManager* mqtt) {
    Serial.println("=== Configuring Sensors ===");
    
    // Create CT sensor with 30-second read interval
    CTSensor* ctSensor = new CTSensor(ct_sensors, 4, mqtt, 30000);
    sensors.push_back(ctSensor);
    
    Serial.printf("Added %d sensor(s) to system\n", sensors.size());
}
