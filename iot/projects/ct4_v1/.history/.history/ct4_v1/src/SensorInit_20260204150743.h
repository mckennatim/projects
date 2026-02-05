#pragma once
#include <vector>
#include "Sensor.h"
#include "MqttManager.h"

void configureSensors(std::vector<Sensor*>& sensors, MqttManager* mqtt);
