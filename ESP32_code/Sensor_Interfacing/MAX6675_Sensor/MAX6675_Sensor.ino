#include <WiFi.h>                
#include <Wire.h>
#include <LiquidCrystal_I2C.h>   
#include "max6675.h"

int thermoDO = 12;
int thermoCS = 15;
int thermoCLK = 14;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
LiquidCrystal_I2C lcd(0x27, 16, 2);

long temp_C, temp_F;

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("MAX6675 Temp");
  lcd.setCursor(0,1);
  lcd.print("Initializing...");
  delay(2000);  


}

void loop() {

  temp_C = thermocouple.readCelsius();
  temp_F = thermocouple.readFahrenheit();

  Serial.print("°C = "); 
  Serial.println(temp_C);
  Serial.print("°F = ");
  Serial.println(temp_F);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp_C);
  lcd.print((char)223);  
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temp_F);
  lcd.print((char)223);
  lcd.print("F");

  delay(1000);  
}
