#include "gps_module.h"

TinyGPSPlus gps;
HardwareSerial GPS_Serial(2); // UART2 (Pins 16 RX, 17 TX)

void initGPS() {
  GPS_Serial.begin(9600);
}

void updateGPS() {
  // Feed the GPS object with serial data as it arrives
  while (GPS_Serial.available() > 0) {
    gps.encode(GPS_Serial.read());
  }
}

bool hasValidLocation() {
  return (gps.location.isUpdated() && gps.location.isValid());
}

String getGPSTelemetry(const char* vehicle_id) {
  // Construct lightweight JSON payload manually to save memory
  String payload = "{";
  payload += "\"id\":\"" + String(vehicle_id) + "\",";
  payload += "\"lat\":" + String(gps.location.lat(), 6) + ",";
  payload += "\"lng\":" + String(gps.location.lng(), 6) + ",";
  payload += "\"spd\":" + String(gps.speed.kmph(), 2);
  payload += "}";
  return payload;
}