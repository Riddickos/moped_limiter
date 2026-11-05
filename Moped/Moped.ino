// Aprilia RS4 50 limiter.
// Hall sensor on the gearbox -> pin 2 (INT0). When RPM goes over the limit,
// the solid state relay cuts ignition (kill switch) for KILL time, then releases.
// Settings come over BLE (MLT-BT05) and are stored in EEPROM.

#include <SoftwareSerial.h>
#include <EEPROM.h>

// ---- Pins ----
const uint8_t SENSOR_PIN = 2;   // must be an interrupt pin
const uint8_t RELAY_PIN  = A1;  // HIGH = engine runs, LOW = ignition cut
const uint8_t BLE_RX_PIN = 4;
const uint8_t BLE_TX_PIN = 3;

// ---- EEPROM layout (kept compatible with older versions) ----
const int ADDR_LIMIT_RPM  = 1;
const int ADDR_KILL_DELAY = 3;

// ---- Sanity limits ----
const int DEFAULT_LIMIT_RPM  = 6000;
const int DEFAULT_KILL_DELAY = 20;     // ms
const int MIN_LIMIT_RPM      = 100;
const int MAX_LIMIT_RPM      = 20000;
const int MAX_KILL_DELAY     = 2000;   // ms

// Pulses closer than this are treated as noise (30000 rpm).
const unsigned long MIN_PULSE_PERIOD_US = 2000;
// No pulse for this long means the shaft is (almost) stopped.
const unsigned long STALL_TIMEOUT_US = 500000;

SoftwareSerial ble(BLE_RX_PIN, BLE_TX_PIN);

// Shared with the interrupt
volatile unsigned long lastPulseUs = 0;
volatile unsigned long pulsePeriodUs = 0;

// Settings
int limitRpm;
int killDelayMs;
bool isLimitOn = true;
bool isDebug = false;

// Cut state
bool isCutting = false;
unsigned long cutStartMs = 0;

// Incoming command buffer
char cmd[24];
uint8_t cmdLen = 0;

// ---------------------------------------------------------------------------

void onSensorPulse()
{
  unsigned long now = micros();
  unsigned long period = now - lastPulseUs;
  if (period < MIN_PULSE_PERIOD_US) return;  // ignore noise / bounce
  pulsePeriodUs = period;
  lastPulseUs = now;
}

unsigned int readRpm()
{
  noInterrupts();
  unsigned long period = pulsePeriodUs;
  unsigned long last = lastPulseUs;
  interrupts();

  if (period == 0 || micros() - last > STALL_TIMEOUT_US) return 0;
  return 60000000UL / period;
}

// Stored big-endian, as in previous versions.
void writeIntToEEPROM(int address, int value)
{
  EEPROM.update(address, value >> 8);
  EEPROM.update(address + 1, value & 0xFF);
}

int readIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) | EEPROM.read(address + 1);
}

void loadSettings()
{
  limitRpm = readIntFromEEPROM(ADDR_LIMIT_RPM);
  killDelayMs = readIntFromEEPROM(ADDR_KILL_DELAY);

  // Blank EEPROM reads as -1, which would cut the engine permanently.
  if (limitRpm < MIN_LIMIT_RPM || limitRpm > MAX_LIMIT_RPM) limitRpm = DEFAULT_LIMIT_RPM;
  if (killDelayMs < 0 || killDelayMs > MAX_KILL_DELAY) killDelayMs = DEFAULT_KILL_DELAY;
}

// ---------------------------------------------------------------------------

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // let the engine run

  pinMode(SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), onSensorPulse, RISING);

  loadSettings();
  ble.begin(9600);
}

void loop()
{
  updateLimiter();
  readCommands();
}

// ---- Limiter ----

void updateLimiter()
{
  if (isCutting) {
    if (millis() - cutStartMs < (unsigned long)killDelayMs) return;
    releaseIgnition();
  }

  unsigned int rpm = readRpm();
  if (isLimitOn && rpm > (unsigned int)limitRpm) {
    cutIgnition(rpm);
  }
}

void cutIgnition(unsigned int rpm)
{
  digitalWrite(RELAY_PIN, LOW);
  isCutting = true;
  cutStartMs = millis();

  if (isDebug) {
    digitalWrite(LED_BUILTIN, HIGH);
    ble.print(F("RPM "));
    ble.println(rpm);
  }
}

void releaseIgnition()
{
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_BUILTIN, LOW);
  isCutting = false;
}

// ---- BLE commands ----

void readCommands()
{
  while (ble.available()) {
    char c = ble.read();

    if (c == '\n') {
      cmd[cmdLen] = '\0';
      handleCommand(cmd);
      cmdLen = 0;
    } else if (c != '\r' && c != '*' && cmdLen < sizeof(cmd) - 1) {
      cmd[cmdLen++] = c;
    }
  }
}

void handleCommand(const char *command)
{
  if (strcmp(command, "TOGGLE_LIMIT") == 0) {
    isLimitOn = !isLimitOn;
  } else if (strcmp(command, "TOGGLE_DEBUG") == 0) {
    isDebug = !isDebug;
  } else if (strncmp(command, "RPM_", 4) == 0) {
    int value = atoi(command + 4);
    if (value >= MIN_LIMIT_RPM && value <= MAX_LIMIT_RPM) {
      limitRpm = value;
      writeIntToEEPROM(ADDR_LIMIT_RPM, limitRpm);
    } else {
      ble.println(F("Bad RPM value"));
    }
  } else if (strncmp(command, "DELAY_", 6) == 0) {
    int value = atoi(command + 6);
    if (value >= 0 && value <= MAX_KILL_DELAY) {
      killDelayMs = value;
      writeIntToEEPROM(ADDR_KILL_DELAY, killDelayMs);
    } else {
      ble.println(F("Bad DELAY value"));
    }
  }

  printStatus();
}

void printStatus()
{
  ble.print(F("Debug "));   ble.println(isDebug);
  ble.print(F("RPM "));     ble.println(limitRpm);
  ble.print(F("Delay "));   ble.println(killDelayMs);
  ble.print(F("Limiter ")); ble.println(isLimitOn);
}
