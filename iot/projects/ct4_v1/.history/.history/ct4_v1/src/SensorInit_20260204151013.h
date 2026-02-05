#pragma once
#include <vector>
#include "Sensor.h"

// Forward declaration to avoid include dependency
class MqttManager;

void configureSensors(std::vector<Sensor*>& sensors, MqttManager* mqtt);
