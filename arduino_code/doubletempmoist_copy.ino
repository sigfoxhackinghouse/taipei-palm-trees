// Arduino SigFox for MKRFox1200 - Version: 1.0.0
//#include <SigFox.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);


#include <DHT.h>
//Constants
#define DHTPIN 7     // what pin we're connected to
#define DHTTYPE DHT11   // DHT11
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor for normal 16mhz Arduino
float hum;  //Stores humidity value
float temp; //Stores temperature
int successCount;
//char zz[] = {0,0,0,0};
//char ff[]={255,255,255,255};
char data[8] = {255, 255, 255, 255, 255, 255, 255, 0};
float treeTemp;

//https://www.hackster.io/unabiz/building-iot-applications-with-arduino-sigfox-and-ubidots-7e910c

#include "SIGFOX.h"                         //  Include the unabiz-arduino library.
static const String device = "";       //  Set this to your device name if you're using UnaBiz Emulator.
static const bool useEmulator = false;      //  Set to true if using UnaBiz Emulator.
static const bool echo = true;              //  Set to true if the Sigfox library should display the executed commands.
static const Country country = COUNTRY_SG;  //  Set this to your country to configure the Sigfox transmission frequencies.
static UnaShieldV2S transceiver(country, useEmulator, device, echo);

int lastSense=0;

// magic number which should be at first byte of memory,
// if not memory will be reset:
static const int MEMORY_MAGIC_NUMBER = 47;

static const int TEMPERATURE_MEMORY_LOCATION = 10;
static const int TEMPERATURE_MEMORY_SIZE = 12;

void setup()
{
  Serial.begin(9600);
  
  
  pinMode(LED_BUILTIN, INPUT);
  //Initialize the DHT sensor
  //dht.begin();
  //delay(1000);//Wait before accessing Sensor
  //successCount = 0;
  
  //disable ADC
  // ADCSRA=0;
  
}

void loop () 
{
  Serial.print("Loop ");
  Serial.print(millis());
  Serial.print("\n");

  if(millis() - lastSense > 1000) {
    lastSense = millis();
      
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
  
    memReset();
    
    recordSensorData();
    uploadData();
    
    printMem(10, 25);
    digitalWrite(LED_BUILTIN, LOW);
    pinMode(LED_BUILTIN, INPUT);
  }
  
  
  delay(8000);
  // replace delay withh goSleep():
  //goSleep()
}

void goSleep() {
  // disable ADC
  ADCSRA = 0;  

  // clear various "reset" flags
  MCUSR = 0;     
  // allow changes, disable reset
  WDTCSR = bit (WDCE) | bit (WDE);
  // set interrupt mode and an interval 
  WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);    // set WDIE, and 8 seconds delay
  wdt_reset();  // pat the dog
  
  set_sleep_mode (SLEEP_MODE_STANDBY);  
  noInterrupts ();           // timed sequence follows
  sleep_enable();
 
  // turn off brown-out enable in software
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS); 
  interrupts ();             // guarantees next instruction executed
  sleep_cpu ();  
  
  // cancel sleep as a precaution
  sleep_disable();
}

int recordSensorData() {
  Serial.print("Requesting temperatures...\n");
  sensors.begin(); 
  sensors.requestTemperatures(); // request command to get temperature readings
  
  // useful temperature range 10-60°C is mapped to int range 0-250 (with 0.2°C accuracy)
  // in order to save data size
  int treeTemp = (int)(sensors.getTempCByIndex(0) * 5) - 50;
  //int hum = (int)(dht.readHumidity());
  //int temp = (int)(dht.readTemperature() * 5) - 50;
  
  Serial.print("tree ");
  Serial.print(((double)(treeTemp + 50)) / 5.0);
  //Serial.print(" hum ");
  //Serial.print(hum);
  //Serial.print(" temp ");
  //Serial.print(temp);
  Serial.print("\n");

  memWriteTemperature(treeTemp);
}


void uploadData() {
  if(EEPROM.read(TEMPERATURE_MEMORY_LOCATION) == 0) {
    if (!transceiver.begin()) {
      stop(F("Unable to init Sigfox module, may be missing"));
    }
    
    // send 12 temperature datapoints:
    String data = "";
    for(int i=1; i<=TEMPERATURE_MEMORY_SIZE;i++) {
      int temp = EEPROM.read(TEMPERATURE_MEMORY_LOCATION + i);
      
      data.concat(String(temp / 16, 'HEX'));
      data.concat(String(temp % 16, 'HEX'));
    }
    
    Serial.print("Uploading... ");
    Serial.print(data);
    Serial.print("\n");
  
    transceiver.sendString(data);
  }
}

ISR(TIMER1_OVF_vect)        // interrupt service routine 
{
  wdt_disable();
}

void printMem(int start, int stop) {
  do {
    Serial.print(String(EEPROM.read(start), HEX));
    Serial.print(" ");
  } while(start++ < stop);
  Serial.print("\n");
}

void memReset() {
  // memory reset:
  if(EEPROM.read(0) != MEMORY_MAGIC_NUMBER) {
    Serial.println("resetting memory");
     
    EEPROM.write(0, MEMORY_MAGIC_NUMBER);
    for(int i=1; i<255; i++) {
        EEPROM.write(i, 0);
    }
  }
}

void memWriteTemperature(int treeTemp) {
  int index = EEPROM.read(TEMPERATURE_MEMORY_LOCATION) + 1;
  EEPROM.write(TEMPERATURE_MEMORY_LOCATION + index, treeTemp);
  EEPROM.write(TEMPERATURE_MEMORY_LOCATION, index % TEMPERATURE_MEMORY_SIZE);
}