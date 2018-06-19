#include <DallasTemperature.h>

#include <Arduino.h>
#include <OneWire.h>

/*
    Board Type NodeMCU 1.0 (ESP-12E Module)
*/
#define ONE_WIRE_BUS 5

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS1820(&oneWire);
char temperatureCString[7];
char temperatureFString[7];

void getTemperature() {
  float tempC;
  float tempF;
  do {
    DS1820.requestTemperatures(); 
    tempC = DS1820.getTempCByIndex(0);
    dtostrf(tempC, 2, 2, temperatureCString);
    tempF = DS1820.getTempFByIndex(0);
    dtostrf(tempF, 3, 2, temperatureFString);
    delay(100);
  } while (tempC == 85.0 || tempC == (-127.0));
}

void setup()
{
    Serial.begin(115200);
    DS1820.begin();

    Serial.println(F("READY:"));
}

void loop()
{
    Serial.println("BLINK");
    getTemperature();
    Serial.print("Degrees F ");
    Serial.println(temperatureFString);
    delay(1000);
}