#include <Arduino.h>
#include "sens.h"
#include "gps.h"
#include "mqtt.h"

const char* ssid = ""; // WIFI Name
const char* password = ""; // WIFI Password
const char* mqtt_server = ""; // MQTT broker address
const char* mqtt_topic = ""; //  MQTT topic
const char* vehicle_id = ""; //MQTT client ID

unsigned long lastPublishTime = 0; // last time the data was published
const unsigned long PUBLISH_INTERVAL = 10000; // 10 seconds interval for publishing data

void setup() {
  Serial.begin(115200);
  initSens(); // Initialize the vibration sensor
}

void loop() {

}