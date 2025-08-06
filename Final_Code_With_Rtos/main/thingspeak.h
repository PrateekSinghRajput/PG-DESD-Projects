#ifndef THINGSPEAK_H
#define THINGSPEAK_H

void updateThingSpeak(float temp, bool oilEmpty, bool doorOpen, float lat, float lon, float load,
                      const char* ssid, const char* password);

#endif
