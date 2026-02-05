#include <Arduino.h>
#include "CONFIG.h"

// Include the core (provides setup() and loop())
#include <mqtt_core_v1/core_main.cpp>

// Optional: Project-specific code
void onProjectSetup() {
  // Any greenhouse-specific initialization
  Serial.println("Greenhouse monitor initialized");
}

void onProjectLoop() {
  // Any custom logic not handled by gadgets
  // This runs every loop iteration
}