#include <DallasTemperature.h>
#include <Homie.h>

#include <Arduino.h>
#include <OneWire.h>

/*
    Board Type NodeMCU 1.0 (ESP-12E Module)
*/
#define ONE_WIRE_BUS 5
const uint8_t MAX_TEMPERATURE_SENSORS = 2;
const int TEMPERATURE_INTERVAL = 5;
typedef uint8_t DeviceAddress[8];

typedef struct
{
    DeviceAddress Address;
    float LastTemp;
    char Name[17];
} TempInfo_t;

TempInfo_t tempInfo[MAX_TEMPERATURE_SENSORS] = { 
    {0, 0.0, '\0' },
    {0, 0.0, '\0' }
 };

uint8_t temperatureProbeCount = 0;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS1820(&oneWire);

unsigned long lastTemperatureSent = 0;


HomieNode temperatureNode("temperature", "temperature");

void addressToString(DeviceAddress addr, char* result)
{
    byte b;
    for (uint8_t x=0, y=0; y < 8; ++y, ++x)
    {
        b = (byte)(addr[y] >> 4);
        result[x] = (b > 9 ? b + 0x37: b + 0x30);
        b = (byte)(addr[y] & 0xF);
        result[++x] = (char) (b > 9 ? b + 0x37 : b + 0x30);
    }
    result[16] = '\0';
}

void setupHandler(){
    //char addr[17];
    
    temperatureNode.setProperty("unit").send("f");
    for(uint8_t x = 0; x < temperatureProbeCount; x++)
    {
        //addressToString(tempInfo[x].Address, addr);
        temperatureNode.advertise(tempInfo[x].Name);
    }
    //temperatureNode.advertise("degrees");
}

void loopHandler(){
    float tempF;
    static bool isWaiting;
    char addr[17];
    //Homie.getLogger() << "loopHandler() "  << endl;
    if (millis() - lastTemperatureSent >= TEMPERATURE_INTERVAL * 1000UL || lastTemperatureSent == 0) {
        isWaiting = true;
        DS1820.requestTemperatures(); 
        Homie.getLogger() << "DS1820.requestTemperatures(): "  << endl;
        lastTemperatureSent = millis();
    }
    if (isWaiting & DS1820.isConversionComplete())
    {
        isWaiting = false;
        for(uint8_t x = 0; x < temperatureProbeCount; x++)
        {
            tempF = DS1820.getTempF(tempInfo[x].Address);
            addressToString(tempInfo[x].Address, addr);
            temperatureNode.setProperty(addr).send(String(tempF));
            tempInfo[x].LastTemp = tempF;
        }
    }
}

void setupTemperatureProbes()
{
    DeviceAddress addr;
    char name[17];
    temperatureProbeCount = DS1820.getDS18Count();
    temperatureProbeCount = min(temperatureProbeCount, MAX_TEMPERATURE_SENSORS);
    for (uint8_t x = 0; x < temperatureProbeCount; x++)
    {
        if (DS1820.getAddress(addr, x)){
            memcpy(tempInfo[x].Address, addr, 8);
            tempInfo[x].LastTemp = 0.0;
            addressToString(tempInfo[x].Address, name);
            memcpy(tempInfo[x].Name, name, 17);
        }
    }
    //DS1820.requestTemperatures(); 
}


// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{

  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
/*
void loopHandlerTest(){
    float tempF;
    static bool isWaiting;
    //Homie.getLogger() << "loopHandler() "  << endl;
    if (millis() - lastTemperatureSent >= TEMPERATURE_INTERVAL * 1000UL || lastTemperatureSent == 0) {
        isWaiting = true;
        DS1820.requestTemperatures(); 
        Homie.getLogger() << "DS1820.requestTemperatures(): "  << endl;
        lastTemperatureSent = millis();
    }
    if (isWaiting & DS1820.isConversionComplete())
    {
        isWaiting = false;
        for(uint8_t x = 0; x < temperatureProbeCount; x++)
        {
            tempF = DS1820.getTempF(tempInfo[x].Address);
            Serial << x << ") ";
            printAddress(tempInfo[x].Address);
            Serial << " " << tempF << " F" << endl;
            tempInfo[x].LastTemp = tempF;
        }
    }
}
*/
void setup()
{
    Serial.begin(115200);
    Serial << "Startup" << endl;
    DS1820.begin();
    setupTemperatureProbes();

    Homie_setFirmware("bare-minimum", "0.0.1");
    Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);
    temperatureNode.advertise("unit");
    Homie.setup();
  
    Serial << "Startup" << endl;
}

void loop()
{
    Homie.loop();
    //loopHandlerTest();
    /*
    char addr[17];
    addressToString(tempInfo[0].Address, addr);
    Serial.println(addr);
*/
    
}