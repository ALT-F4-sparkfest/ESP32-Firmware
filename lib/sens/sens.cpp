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
        lastSensTime = millis(); // update the last time the vibration sensor was triggered
    }
    jeepStatus = (millis() - lastSensTime) < SENS_TIMEOUT; // tells the elapsed time since the last vibration was detected, if it is less than the timeout then the jeep is running
}

bool getJeepStatus() {
    return jeepStatus; // returns the current status of the jeep, true if it is running, false if it is not
}