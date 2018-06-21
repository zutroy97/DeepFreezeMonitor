#include <DallasTemperature.h>
#include <Homie.h>

#include <Arduino.h>
#include <OneWire.h>

/*
    Board Type NodeMCU 1.0 (ESP-12E Module)
*/
#define ONE_WIRE_BUS 5
const int TEMPERATURE_INTERVAL = 5;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS1820(&oneWire);
char temperatureCString[7];
char temperatureFString[7];

unsigned long lastTemperatureSent = 0;

HomieNode temperatureNode("temperature", "temperature");

void setupHandler(){
    temperatureNode.setProperty("unit").send("c");
}

void loopHandler(){
    float tempC;
    if (millis() - lastTemperatureSent >= TEMPERATURE_INTERVAL * 1000UL || lastTemperatureSent == 0) {
        do {
            DS1820.requestTemperatures(); 
            tempC = DS1820.getTempCByIndex(0);
            //dtostrf(tempF, 3, 2, temperatureFString);
            delay(100);
        } while (tempC == 85.0 || tempC == (-127.0));
        Homie.getLogger() << "Temperature: " << tempC << " °C" << endl;
        temperatureNode.setProperty("degrees").send(String(tempC));
        lastTemperatureSent = millis();
    }
}
/*
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
*/
void setup()
{
    Serial.begin(115200);
    Serial << "Startup" << endl;
    DS1820.begin();

    Homie_setFirmware("bare-minimum", "0.0.1");
    Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);
    //Serial.println(F("READY:"));
    temperatureNode.advertise("unit");
    temperatureNode.advertise("degrees");

    Homie.setup();
    Serial << "Startup" << endl;
}

void loop()
{
    Homie.loop();
    /*
    Serial.println("BLINK");
    getTemperature();
    Serial.print("Degrees F ");
    Serial.println(temperatureFString);
    delay(1000);
    */
}