#include "thingspeak.h"
#include <WiFi.h>
#include <ThingSpeak.h>
#include "globals.h"

// ThingSpeak channel and key - replace with your own
const unsigned long THINGSPEAK_CHANNEL_ID = 3026763;
const char* thingspeakApiKey = "I3V58JNL7DM6EG53";

extern WiFiClient client;

void updateThingSpeak(float temp, bool oilEmpty, bool doorOpen, float lat, float lon, float load,
                      const char* ssid, const char* password) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.begin(ssid, password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < 10000)) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Failed to reconnect WiFi for ThingSpeak update.");
      return;
    }
  }

  ThingSpeak.setField(1, temp);
  ThingSpeak.setField(2, oilEmpty ? 0 : 1);
  ThingSpeak.setField(3, doorOpen ? 1 : 0);
  ThingSpeak.setField(4, lat);
  ThingSpeak.setField(5, lon);
  ThingSpeak.setField(6, load);

  int response = ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, thingspeakApiKey);
  if (response == 200) {
    Serial.println("ThingSpeak update successful");
  } else {
    Serial.print("ThingSpeak update failed, code: ");
    Serial.println(response);
  }
}
