#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is connected to pin D2 (GPIO4 on ESP8266)
#define ONE_WIRE_BUS D2

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

// Arrays to hold device addresses
DeviceAddress sensor1, sensor2;

void setup() {
  // Start serial communication
  Serial.begin(115200);
  Serial.println("\n\nDS18B20 Two Sensor Temperature Monitor");
  Serial.println("=====================================");
  
  // Start up the library
  sensors.begin();
  
  // Locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  
  // Check if we have at least 2 sensors
  if (sensors.getDeviceCount() < 2) {
    Serial.println("ERROR: Less than 2 sensors found!");
    Serial.println("Please check your wiring and restart.");
    while(1); // Halt
  }
  
  // Assign addresses to each sensor
  if (!sensors.getAddress(sensor1, 0)) {
    Serial.println("Unable to find address for Sensor 1");
  }
  if (!sensors.getAddress(sensor2, 1)) {
    Serial.println("Unable to find address for Sensor 2");
  }
  
  // Print sensor addresses
  Serial.print("Sensor 1 Address: ");
  for (uint8_t i = 0; i < 8; i++) {
    if (sensor1[i] < 16) Serial.print("0");
    Serial.print(sensor1[i], HEX);
  }
  Serial.println();
  
  Serial.print("Sensor 2 Address: ");
  for (uint8_t i = 0; i < 8; i++) {
    if (sensor2[i] < 16) Serial.print("0");
    Serial.print(sensor2[i], HEX);
  }
  Serial.println();
  
  // Set resolution to 12 bit (max precision)
  sensors.setResolution(sensor1, 12);
  sensors.setResolution(sensor2, 12);
  
  Serial.println("\nReady to read temperatures!");
  Serial.println("=====================================\n");
}

void loop() {
  // Request temperatures from all sensors on the bus
  sensors.requestTemperatures();
  
  // Read temperature from sensor 1
  float temp1 = sensors.getTempF(sensor1);
  
  // Read temperature from sensor 2
  float temp2 = sensors.getTempF(sensor2);
  
  // Print results to serial monitor
  Serial.print("Sensor 1: ");
  Serial.print(temp1);
  Serial.print("°F  |  Sensor 2: ");
  Serial.print(temp2);
  Serial.print("°F  |  Average: ");
  Serial.print((temp1 + temp2) / 2.0);
  Serial.println("°F");
  
  // Wait 2 seconds between readings
  delay(2000);
}
