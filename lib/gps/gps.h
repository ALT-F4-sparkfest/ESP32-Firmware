#ifndef  GPS_H
#define  GPS_H
#include <Arduino.h>
#include <TinyGPS++.h>

void initGPS();
void updateGPS();
bool hasValidLocation();
String getGPSTelemetry(const char* vehicle_id);

#endif