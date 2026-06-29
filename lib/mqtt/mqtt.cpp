#include <WiFi.h>
#include <PubSubClient.h>
#include "mqtt.h"

WiFiClient espClient;
PubSubClient client(espClient);

void initMQTT(const char* ssid, const char* password, const char* mqtt_server) { 
    delay(10); // Allow time for the ESP32 to initialized
    Serial.print("Connecting to WiFi");
    Serial.print(ssid);
    WiFi.begin(ssid, password); // Connect to the specified WiFi network

    while (WiFi.status() != WL_CONNECTED) { // Wait until the ESP32 is connected to the WiFi network
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");

    client.setServer(mqtt_server, 1883); // Set the MQTT server and port
}

void maintainNetwork() {
  while (!client.connected()) { // Check if the MQTT client is connected to the broker
    Serial.print("Attempting MQTT connection...");
    String clientId = "SmartRoute-";
    clientId += String(random(0xffff), HEX); // Generate a random client ID for the MQTT connection
    
    if (client.connect(clientId.c_str())) { //  Attempt to connect to the MQTT broker
      Serial.println("connected to MQTT");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state()); // Print the MQTT connection state for debugging
      Serial.println(" try again in 5 seconds");
      delay(5000); 
    }
  }
  client.loop(); 
}

void publishTelemetry(const char* topic, String payload) { 
  client.publish(topic, payload.c_str()); // Publish the telemetry data to the specified MQTT topic
  Serial.print("Published: ");
  Serial.println(payload);
}
