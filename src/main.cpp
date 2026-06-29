#include <Arduino.h>
#include <WiFiManager.h>
#include "sens.h"
#include "gps.h"
#include "mqtt.h"

// Vehicle ID now becomes a variable we can configure
char vehicle_id[20] = "jeep_default_01";
unsigned long lastPublishTime = 0;
const unsigned long PUBLISH_INTERVAL = 3000;

void setup() {
  Serial.begin(115200);

  // WiFiManager will create a portal if it can't connect
  WiFiManager wm;
  
  // Create custom input field for Vehicle ID
  WiFiManagerParameter custom_vid("vid", "Vehicle ID", vehicle_id, 20);
  wm.addParameter(&custom_vid);

  // Try to connect to saved Wi-Fi, or open portal "SmartRoute_Setup"
  if (!wm.autoConnect("SmartRoute_Setup")) {
    Serial.println("Failed to connect, resetting...");
    ESP.restart();
  }

  // Save the ID input by the user
  strcpy(vehicle_id, custom_vid.getValue());
  
  initSens();
  initGPS();
  // initNetwork is now handled by WiFiManager
}

void loop() {
  maintainNetwork();
  updateSensStatus();
  updateGPS();

  if (getJeepStatus() && hasValidLocation()) {
    if (millis() - lastPublishTime >= PUBLISH_INTERVAL) {
      String payload = getGPSTelemetry(vehicle_id);
      publishTelemetry("smartroute/vehicles/live", payload);
      lastPublishTime = millis();
    }
  }
}