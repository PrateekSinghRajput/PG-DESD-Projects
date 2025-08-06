#include "display.h"
#include <Adafruit_GFX.h>
#include "globals.h"

#define ST7735_WHITE 0xFFFF
#define ST77XX_BLUE 0x001F
#define ST77XX_BLACK 0x0000
#define ST77XX_YELLOW 0xFFE0
#define ST77XX_RED 0xF800
#define ST77XX_GREEN 0x07E0
#define ST77XX_CYAN 0x07FF

void drawHeader() {
  tft.fillRect(0, 0, 160, 20, ST77XX_BLUE);
  tft.setTextColor(ST7735_WHITE, ST77XX_BLUE);
  tft.setTextSize(2);
  tft.setCursor(10, 2);
  tft.print("CAN Rx");
}

void drawNode1(float temp, bool waterEmpty) {
  tft.fillRect(0, 22, 160, 38, ST77XX_BLACK);
  tft.drawRect(0, 22, 160, 38, ST7735_WHITE);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(6, 28);
  tft.print("Node1");
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(70, 28);
  tft.print("Temp: ");
  tft.print(temp, 1);
  tft.print("C");
  tft.setCursor(70, 40);
  tft.print("Oil: ");
  tft.setTextColor(waterEmpty ? ST77XX_RED : ST77XX_GREEN, ST77XX_BLACK);
  tft.print(waterEmpty ? "Empty" : "Full ");
}

void drawNode2(bool doorOpen, float loadWeight, float latitude, float longitude) {
  tft.fillRect(0, 62, 160, 88, ST77XX_BLACK);
  tft.drawRect(0, 62, 160, 88, ST7735_WHITE);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(6, 68);
  tft.print("Node2");
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(70, 68);
  tft.print("Door: ");
  tft.setTextColor(doorOpen ? ST77XX_RED : ST77XX_GREEN, ST77XX_BLACK);
  tft.print(doorOpen ? "CLOSED" : "OPEN");

  tft.setCursor(6, 90);
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.print("Load: ");
  tft.print(loadWeight, 2);

  tft.setCursor(6, 110);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.print("GPS:");

  tft.setCursor(40, 110);
  tft.print(latitude, 6);

  tft.setCursor(40, 122);
  tft.print(longitude, 6);
}

void drawCANStatus(bool canOK) {
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(115, 6);
  tft.print(canOK ? "CAN OK" : "NO CAN");
}
