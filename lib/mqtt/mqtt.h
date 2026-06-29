#ifndef MQTT_H
#define MQTT_H
#include <Arduino.h>

void initMQTT(const char* ssid, const char* password, const char* mqtt_server);
void maintainNetwork();
void publishTelemetry(const char* topic, String payload);

#endif