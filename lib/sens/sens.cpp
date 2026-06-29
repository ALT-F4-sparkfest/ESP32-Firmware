#include "sens.h"

const byte SEN_PIN = 32; // Vibration Module Sensor
unsigned long lastSensTime = 0;
const unsigned long SENS_TIMEOUT = 60000; // 1 Minute if it is idle then no more telemetry
bool jeepStatus = false; // tells if the jeep is running or not based on the vibration sensor

void initSens() {
  pinMode(SEN_PIN, INPUT);
}

void updateSensStatus() {
    if (digitalRead(SEN_PIN) == HIGH) {
        lastSensTime = millis();
    }
    jeepStatus = (millis() - lastSensTime) < SENS_TIMEOUT;
}

bool getJeepStatus() {
    return jeepStatus;
}