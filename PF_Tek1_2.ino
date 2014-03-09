/*
 * PF_Tek1.2
 *
 * 9.3.2014
 *
 * DESCRIPTION
 *
 * Systems keeps uptime stored in eeprom, and writes it there every defined
 * interval, to avoid loss of uptime data.
 *
 * This uptime information is then used to keep air change somewhat constant
 * despite system downtime.
 *
 * EEPROM (in this order)
 *  Uptime (uint32_t)
 *  Air status (uint8_t)
 *  Air time (uint32_t)
 *
 * LIMITATIONS
 *
 *  System uptime will overflow every 3.268 year
 *
 * CAUTIONS
 *
 *  EEPROM has write-life of 100,000 to 1,000,000 cycles; do not store uptime too often
 *
 */

#include <OneWire.h>
#include <DS18B20.h>
#include <EEPROMex.h>
#include <EEPROMVar.h>

#define TO_HUMAN_UPTIME(X)  (unsigned long)X/1000
#define TO_MACHINE_UPTIME(X)  (unsigned long)X*1000

#define SECONDS_IN_DAY  (unsigned long)(60 * 60 * 24);
#define SECONDS_IN_HOUR  (unsigned long)(60 * 60);

#define DEBUG
#undef DISABLE_WRITE
#define SERIAL_BAUD 115200

#define AIR_INTERVAL 3600 // every hour
#define AIR_DURATION 120 // for 2 minutes
#define UPTIME_STORE_INTERVAL 60 // maximum 60 seconds uptime loss

#define PIN_TEMPERATURE 7
#define PIN_HUMIDITY 0

DS18B20 Temperature(PIN_TEMPERATURE);

uint32_t uptime = 0;
uint32_t uptimeStoreTime = 0;
uint32_t localUptime = 0;
uint32_t previousUptime = 0;

int addressUptime;
int addressAirStatus;
int addressAirTime;

uint32_t lastAir = 0;
uint8_t airStatus = 0;
uint32_t airStart = 0;
uint32_t timeSinceAir = 0;

static byte temperatureSensors[1][8];
boolean conversionRunning = false;
unsigned long conversionStart = 0;

struct Temperature {
  
  byte *Address;
  float Temperature;
  unsigned long updateTime;
  
};

struct Environment {
  
  float Humidity;
  float HumidityRaw;
  struct Temperature Temperatures[1];
  
} Environment;

void setup(void) {
  initEepromAddressing();
  initUptime();
  initAir();
  initSerial();
  initTemperature();
}

void initEepromAddressing(void) {
  addressUptime = EEPROM.getAddress(sizeof(long));
  addressAirStatus = EEPROM.getAddress(sizeof(byte));
  addressAirTime = EEPROM.getAddress(sizeof(long));
}

void initUptime(void) {
  uptime = EEPROM.readLong(addressUptime);
  uptimeStoreTime = uptime;
}

void initAir(void) {
  // Initialize airStart, otherwise bug in start if air was running before power down
  airStart = EEPROM.readLong(addressAirTime);
}

void initSerial(void) {
  Serial.begin(SERIAL_BAUD);
}

void initTemperature(void) {
  Temperature.init(1);
  Temperature.getAddresses(temperatureSensors);
  
  // Copy sensor address data into struct elements
  for(byte i = 0;i < sizeof(Environment.Temperatures) / sizeof(struct Temperature);i++) {
    Environment.Temperatures[i].Address = temperatureSensors[i];
    Environment.Temperatures[i].Temperature = 0.0;
    Environment.Temperatures[i].updateTime = 0;
  }
  
}

void loop(void) {
  #ifdef DEBUG
    Serial.println("loop");
  #endif
  
  envTemperature();
  envHumidity();
  
  updateLocalUptime();
  
  airStatus = EEPROM.readByte(addressAirStatus);
  lastAir = EEPROM.readLong(addressAirTime);
  
  calculateTimeSinceAir();
  
  // Do we have air?
  if (isAir()) {
    // Yes!
    continueAir();
  }
  
  // Do we still need air?
  if (needAir()) {
    // Yes!
    if (!isAir()) {
      // But we didn't have it YET
      startAir();
    }
  }
  else {
    // Don't need no air (no more)
    if (isAir()) {
      stopAir();
    }
  }

  if (periodicUptimeSave()) {
    storeUptime();
  }
  
  // Indicate life
  pinMode(13, OUTPUT); digitalWrite(13, !digitalRead(13));
  
  serialPrint();
  
  delay(1000);
  
}

void updateLocalUptime(void) {
  uptime += (millis() - previousUptime);
  previousUptime = millis();
}

void calculateTimeSinceAir(void) {
  timeSinceAir = (uptime - lastAir);
}

boolean isAir(void) {
  return (airStatus == 1);
}

boolean needAir(void) {
  if (isAir()) {
    return ((uptime - airStart) < TO_MACHINE_UPTIME(AIR_DURATION));
  }
  else {
    return (timeSinceAir > TO_MACHINE_UPTIME(AIR_INTERVAL));
  }
}

void continueAir(void) {
}

void startAir(void) {
  #ifdef DEBUG
    Serial.println("startAir");
  #endif
  
  airStart = uptime;
  #ifndef DISABLE_WRITE
    EEPROM.updateByte(addressAirStatus, (uint8_t)1);
  #endif
  
  digitalWrite(6, HIGH);
}

void stopAir(void) {
  #ifdef DEBUG
    Serial.println("stopAir");
  #endif
  
  #ifndef DISABLE_WRITE
    EEPROM.updateLong(addressAirTime, uptime);
    EEPROM.updateByte(addressAirStatus, (uint8_t)0);
  #endif
  
  digitalWrite(6, LOW);
}

boolean periodicUptimeSave(void) {
  return (uptime > uptimeStoreTime + TO_MACHINE_UPTIME(UPTIME_STORE_INTERVAL));
}

void storeUptime(void) {
  #ifdef DEBUG
    Serial.println("storeUptime");
  #endif
  
  uptimeStoreTime = uptime;
  #ifndef DISABLE_WRITE
    EEPROM.writeLong(addressUptime, uptime);
  #endif
}

void envTemperature() {
  
  // Temperature requires 800ms delay time for each sensor
  // Because we don't want to use delay() to halt main loop(), we use software
  // to track if that 800ms has passed. Then we read the temperature.
  //
  // This needs to be done for each sensor, at separate times.
  // We loop through each sensor, always getting the one that hasn't been updated for the longest time
  // Then we either start conversion (and tracking of 800ms), or, if the conversion has finished (800ms passed)
  // we read the temperature, and move to next device.
  //
  // I don't know, if OneWire / DS18B20 supports requesting ALL devices to do the temperature
  // conversion at once, and then reading them all; until I know better, this will have to do.
  
  //for(byte i = 0;i < sizeof(Environment.Temperatures) / sizeof(struct Temperature);i++) {
    struct Temperature *TemperatureDevice = getTemperatureNextDevice();

    if (conversionRunning) {

      if ((millis() - conversionStart) > 800) {
        (*TemperatureDevice).Temperature = Temperature.getTemp((*TemperatureDevice).Address);
        (*TemperatureDevice).updateTime = millis();
        
        conversionRunning = false;
      }
      else {
        // Let the good times roll.. a bit more
      }
    }
    else {
      Temperature.startConversion((*TemperatureDevice).Address, false);
      
      conversionRunning = true;
      conversionStart = millis();
    }
    
  //}
  
}

struct Temperature *getTemperatureNextDevice(void) {
  unsigned long leastTime = 0;
  int leastId = -1;
  
  for(byte i = 0;i < sizeof(Environment.Temperatures) / sizeof(struct Temperature);i++) {
    if (leastId == -1) {
      leastId = i;
      leastTime = Environment.Temperatures[i].updateTime;
    }
    else {
      if (Environment.Temperatures[i].updateTime <= leastTime) {
        leastId = i;
        leastTime = Environment.Temperatures[i].updateTime;
      }
    }
  }
  
  return &Environment.Temperatures[leastId];
}

void envHumidity() {
  float supplyVolt = 5.0;

  int HIH4030_Value = analogRead(PIN_HUMIDITY);
  float voltage = HIH4030_Value/1023. * supplyVolt;

  Environment.HumidityRaw = 161.0 * voltage / supplyVolt - 25.8;
  Environment.Humidity = Environment.HumidityRaw / (1.0546 - 0.0026 * Environment.Temperatures[0].Temperature); // Temperature adjustment 
}

void serialPrint(void) {

  unsigned long uptime_seconds = TO_HUMAN_UPTIME(uptime);
  
  int days = uptime_seconds / SECONDS_IN_DAY;
  int hours = uptime_seconds / SECONDS_IN_HOUR;
  int minutes = uptime_seconds / 60;
  int seconds = uptime_seconds % 60;
  
  Serial.print(days); Serial.print(" days ");
  Serial.print(hours); Serial.print(" hours ");
  Serial.print(minutes); Serial.print(" minutes ");
  Serial.print(seconds); Serial.print(" seconds");
  
  Serial.println(); Serial.println();

}
