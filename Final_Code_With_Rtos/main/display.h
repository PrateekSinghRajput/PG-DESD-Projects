#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 tft;

void drawHeader();
void drawNode1(float temp, bool waterEmpty);
void drawNode2(bool doorOpen, float loadWeight, float latitude, float longitude);
void drawCANStatus(bool canOK);

#endif
