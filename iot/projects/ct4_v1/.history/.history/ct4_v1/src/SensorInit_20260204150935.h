#pragma once
#include <vector>
#include "Sensor.h"
#include <MqttManager.h>  // From core_v1

void configureSensors(std::vector<Sensor*>& sensors, MqttManager* mqtt);
