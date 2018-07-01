#include <DallasTemperature.h>
#include <Homie.h>
#include <Arduino.h>
#include <OneWire.h>

/*
    Board Type NodeMCU 1.0 (ESP-12E Module)
*/
#define ONE_WIRE_BUS 5
const uint8_t MAX_TEMPERATURE_SENSORS = 2;
const int TEMPERATURE_INTERVAL_SECONDS = 2; // how often to read temperatures.
const int TEMPERATURE_REPORT_INTERVAL_SECONDS = 30; // How often to report temperatures
const int STRING_BUFFER_SIZE = 20;
const int DEFAULT_HIGH_TEMP_ALARM_DEG_F = 84 * 100;
const int DEFAULT_HIGH_TEMP_ALARM_HOLD_SEC = 10;
typedef uint8_t DeviceAddress[8];
char buffer[STRING_BUFFER_SIZE];
typedef struct
{
    DeviceAddress Address;
    int LastTempInHundredths;
    unsigned long AlarmTicks;
    int HighTempAlarmDegF;
    int HighTempAlarmDelaySecs;
    int IsHighAlarmPending : 1;
    int IsHighAlarm         : 1;
} TempInfo_t;

TempInfo_t tempInfo[MAX_TEMPERATURE_SENSORS] = { 
    {0, 0.0, 0, 0, 0, 0 },
    {0, 0.0, 0, 0, 0, 0 }
 };

uint8_t temperatureProbeCount = 0;
unsigned long lastTemperatureRead = 0;
unsigned long lastTemperatureReported = 0;
uint8_t reportingTempTolerance = 100; // Hundredths of a degree F difference between readings causes a report
uint8_t reportingTempIntervalInSeconds = 60;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS1820(&oneWire);

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
    temperatureNode.setProperty("unit").send("f");
    for(uint8_t x = 0; x < temperatureProbeCount; x++)
    {
        addressToString(tempInfo[x].Address, buffer);
        temperatureNode.advertise(buffer);
    }
}

void HighTemperatureDetected(uint8_t sensorIndex)
{
    tempInfo[sensorIndex].IsHighAlarm = true;
    // Set this message to sound again
    tempInfo[sensorIndex].AlarmTicks = millis() + (tempInfo[sensorIndex].HighTempAlarmDelaySecs * 1000);    
    Homie.getLogger() << "High Temp Alarm index " << sensorIndex << " " << (tempInfo[sensorIndex].LastTempInHundredths / 100.0) << endl;
}

void HighTemperatureAlarmCancel(uint8_t sensorIndex)
{
    tempInfo[sensorIndex].IsHighAlarm = false;
    Homie.getLogger() << "High Temp Alarm Cancelled "  << sensorIndex << " " << (tempInfo[sensorIndex].LastTempInHundredths / 100.0) << endl;
}

void loopHandler(){
    static bool isWaiting = false;
    //Homie.getLogger() << "loopHandler() "  << endl;
    unsigned long now = millis();
    if (now - lastTemperatureRead >= TEMPERATURE_INTERVAL_SECONDS * 1000UL || lastTemperatureRead == 0) {
        requestTemperatures();
        lastTemperatureRead = now;
        isWaiting = true;
    }
    if (isWaiting & DS1820.isConversionComplete())
    {
        isWaiting = false;
        readTemperatures();
        if (now - lastTemperatureReported >= TEMPERATURE_REPORT_INTERVAL_SECONDS * 1000UL || TEMPERATURE_REPORT_INTERVAL_SECONDS == 0)
        {
            reportAllTemperatures();
            lastTemperatureReported = now;
        }
        for(byte i = 0; i < temperatureProbeCount; i++)
        {
            if (!tempInfo[i].IsHighAlarmPending)
            {   // High Temperature alarm not pending
                if (tempInfo[i].IsHighAlarm)
                {   // Cancel any active High Temp alarms
                    HighTemperatureAlarmCancel(i);
                }
                // No high temp alarm pending...check if there should be.
                if (tempInfo[i].LastTempInHundredths >= tempInfo[i].HighTempAlarmDegF )
                {
                    Homie.getLogger() << "High Temp Alarm Pending "  << endl;
                    // Set alarm to send in X seconds if temperature still over threshold.
                    tempInfo[i].AlarmTicks = now + (tempInfo[i].HighTempAlarmDelaySecs * 1000);
                    tempInfo[i].IsHighAlarmPending = true;
                }
            }else{
                // High Temp alarm pending.
                if (tempInfo[i].LastTempInHundredths < tempInfo[i].HighTempAlarmDegF )
                {   // Temp has fallen, cancel the pending high temp alarm.
                    Homie.getLogger() << "High Temp Alarm Pending Cancelled"  << endl;
                    tempInfo[i].IsHighAlarmPending = false;
                }else{
                    // High temp has not fallen, high temp alarm still pending.
                    if (now >= tempInfo[i].AlarmTicks)
                    {
                        HighTemperatureDetected(i);
                    }
                }
            }
        }
    }
}

void requestTemperatures()
{
    DS1820.requestTemperatures(); 
}

void readTemperatures()
{
    float tempF;
    for(uint8_t x = 0; x < temperatureProbeCount; x++)
    {
        tempF = DS1820.getTempF(tempInfo[x].Address);
        tempInfo[x].LastTempInHundredths = (int)(tempF * 100);
    }
}

void reportAllTemperatures()
{
    for(uint8_t x = 0; x < temperatureProbeCount; x++)
    {
        addressToString(tempInfo[x].Address, buffer);
        temperatureNode.setProperty(buffer).send(String(tempInfo[x].LastTempInHundredths / 100.0));
    }
}

void setupTemperatureProbes()
{
    DeviceAddress addr;

    temperatureProbeCount = DS1820.getDS18Count();
    temperatureProbeCount = min(temperatureProbeCount, MAX_TEMPERATURE_SENSORS);
    for (uint8_t x = 0; x < temperatureProbeCount; x++)
    {
        if (DS1820.getAddress(addr, x)){
            memcpy(tempInfo[x].Address, addr, 8);
            tempInfo[x].LastTempInHundredths = 0;
            tempInfo[x].HighTempAlarmDegF = DEFAULT_HIGH_TEMP_ALARM_DEG_F;
            tempInfo[x].HighTempAlarmDelaySecs = DEFAULT_HIGH_TEMP_ALARM_HOLD_SEC;
            tempInfo[x].IsHighAlarmPending = false;
        }
    }
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
    addressToString(deviceAddress, buffer);
    Serial << buffer; 
}

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
}