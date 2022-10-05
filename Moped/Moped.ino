#include <SoftwareSerial.h>
#include <EEPROM.h>
SoftwareSerial ble_device(4, 3); // RX | TX
#define relayPin A1

int rpm;
int limit_rpm;
int kill_delay;
unsigned long timeold;
bool isLimitOn = true;
bool isDebug = false;
String str_ii = "";

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(relayPin, OUTPUT);
  rpm = 0;
  timeold = 0;
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(0, magnet_detect, RISING); //Initialize the intterrupt pin (Arduino digital pin 2)
  limit_rpm = readIntFromEEPROM(1);
  kill_delay = readIntFromEEPROM(3);
  ble_device.begin(9600);
  digitalWrite(LED_BUILTIN, LOW);
}

void magnet_detect(){
  rpm = 60000 / (millis() - timeold);
  timeold = millis();
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
  ble_device.println("Delay "+String(kill_delay));
  ble_device.println("Limiter "+String(isLimitOn));
}

void loop()
{
  if ((rpm > limit_rpm)) {
    if (isLimitOn) {
      digitalWrite(relayPin, LOW);
      if (isDebug){
        digitalWrite(LED_BUILTIN, HIGH);
        ble_device.println("RPM "+String(rpm));
      }
      delay(kill_delay);
      rpm = 0;
    }
  } else {
    digitalWrite(relayPin, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (ble_device.available()) {
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
      if (str_ii.startsWith("DELAY_")) {
        kill_delay = str_ii.substring(6, str_ii.length()).toInt();
        writeIntIntoEEPROM(3, kill_delay);
      }
      printLimitsStatus();
      str_ii = "";
    }
  }
}