#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "timer1.h"

SoftwareSerial ble_device(4, 3); // RX | TX
#define relayPin A1

volatile unsigned long revolutionsCounter = 0;
volatile unsigned long revolutionsResult = 0;

int limit_rpm;
bool isLimitOn = true;
bool isDebug = false;
int duration = 0;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    pinMode(relayPin, OUTPUT);
    pinMode(2, INPUT_PULLUP);
    attachInterrupt(0, magnet_detect, RISING); //Initialize the intterrupt pin (Arduino digital pin 2)
    limit_rpm = readIntFromEEPROM(1);
    duration = readIntFromEEPROM(3);
    ble_device.begin(9600);
    digitalWrite(LED_BUILTIN, LOW);

    unsetCoutner();
}

void magnet_detect(){
    ++revolutionsCounter;
}

void writeIntIntoEEPROM(int address, int number)
{ 
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void printLimitsStatus()
{
  ble_device.println("Debug "+String(isDebug));  
  ble_device.println("RPM "+String(limit_rpm));  
  ble_device.println("Duration "+String(duration));
  ble_device.println("Limiter "+String(isLimitOn));
}

void loop()
{
    if ((revolutionsResult > limit_rpm * duration / 1000)) {
        if (isLimitOn) {
            digitalWrite(relayPin, LOW);
            if (isDebug){
                digitalWrite(LED_BUILTIN, HIGH);
                ble_device.println("RPM "+String(revolutionsResult));
            }
        }
    } else {
        digitalWrite(relayPin, HIGH);
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (ble_device.available()) {
        static String str_ii = "";

        digitalWrite(LED_BUILTIN, HIGH);
        char in_char = ble_device.read();
        if (int(in_char) != -1 and int(in_char) != 42) {
            if (in_char != '\n' and in_char != '\r') {
                str_ii += in_char;
            }
        }
        if (in_char == '\n') {
            if (str_ii == "TOGGLE_LIMIT") {
                isLimitOn = !isLimitOn;
            }
            if (str_ii == "TOGGLE_DEBUG") {
                isDebug = !isDebug;
            }
            if (str_ii.startsWith("RPM_")) {
                limit_rpm = str_ii.substring(4, str_ii.length()).toInt();
                writeIntIntoEEPROM(1, limit_rpm);
            }
            if (str_ii.startsWith("DURATION_")) {
                duration = str_ii.substring(9, str_ii.length()).toInt();
                writeIntIntoEEPROM(3, duration);
            }
            str_ii = "";
            printLimitsStatus();
        }
    }
}

void unsetCoutner()
{
    revolutionsCounter = 0;
    timer1::instance().init(static_cast<double>(duration) / 1000);
    timer1::instance().start();
}

ISR(TIMER1_COMPA_vect)
{
    cli();
    revolutionsResult = revolutionsCounter;
    unsetCoutner();
    sei();
}

