/*
 * PF_Tek1.3
 *
 * 9.3.2014
 *
 * DESCRIPTION
 *
 * Completely new design. Not super happy about this one, but it has some better
 * design decisions.
 *
 * Does not work with binutils shipped with Arduino IDE. Refer to:
 * https://github.com/arduino/Arduino/issues/1071
 *
 * EEPROM (in this order)
 *  Uptime (uint32_t)
 *  Air status (uint8_t)
 *  Air start (uint32_t)
 *  Air stop (uint32_t)
 *
 * LIMITATIONS
 *
 *  System uptime will overflow every 3.268 year
 *
 * CAUTIONS
 *
 *  EEPROM has write-life of 100,000 to 1,000,000 cycles; do not store uptime too often
 *
 * TODO
 *
 *  Passed time needs to be counter even when powered off
 *  Passed time shall not be counter if airOn when powered off, to ensure necessary air exchange despite of
 *
 */

#include <EEPROMex.h>
#include <EEPROMVar.h>

#define TO_HUMAN_TIME(X)  (unsigned long)X/1000
#define TO_MACHINE_TIME(X)  (unsigned long)X*1000

#define SECONDS_IN_DAY  (unsigned long)(60 * 60 * 24)
#define SECONDS_IN_HOUR  (unsigned long)(60 * 60)

#define SERIAL

#define DELAY

#ifndef SERIAL
  #undef DEBUG0
  #undef DEBUG1
  #undef STATUS
  #undef WRITE
#else
  #undef DEBUG0
  #define DEBUG1
  #define STATUS
  #define WRITE
#endif

#undef DISABLE_WRITE

#define SERIAL_BAUD 115200

#define AIR_INTERVAL 3600 // every hour
#define AIR_DURATION 120 // for 2 minutes
#define UPTIME_STORE_INTERVAL 60 // maximum 60 seconds uptime loss

uint32_t uptime = 0;
uint32_t uptimeStoreTime = 0;
uint32_t previousUptime = 0;

int addressUptime;
int addressAirStatus;
int addressAirStart;
int addressAirStop;

uint8_t airStatus = 0;
uint32_t airStart = 0;
uint32_t airStop = 0;

void setup(void) {
  delay(50);
  
  #ifdef SERIAL
    initSerial();
  #endif
  
  initInit();
  initEepromAddressing();
  initUptime();
}

void initSerial(void) {
  Serial.begin(SERIAL_BAUD);
}

void initInit(void) {
  delay(2000);
  
  #ifdef DEBUG0
    Serial.println("Begin Init");
  #endif
}

void initEepromAddressing(void) {
  #ifdef DEBUG0
    Serial.println("Init eeprom addressing");
  #endif
  
  addressUptime = EEPROM.getAddress(sizeof(long));
  addressAirStatus = EEPROM.getAddress(sizeof(byte));
  addressAirStart = EEPROM.getAddress(sizeof(long));
  addressAirStop = EEPROM.getAddress(sizeof(long));
}

void initUptime(void) {
  #ifdef DEBUG0
    Serial.println("Init uptime");
  #endif
  
  uptime = EEPROM.readLong(addressUptime);
  uptimeStoreTime = uptime;
}

void loop(void) {

  countTime();
  
  airStatus = EEPROM.readByte(addressAirStatus);
  airStart = EEPROM.readLong(addressAirStart);
  airStop = EEPROM.readLong(addressAirStop);
  
  #ifdef DEBUG1
    Serial.print("airStatus: "); Serial.println(airStatus);
    Serial.print("airStart: "); Serial.println(airStart);
    Serial.print("airStop: "); Serial.println(airStop);
    Serial.print("uptime: "); Serial.println(uptime);
    Serial.print("difference scheduledAir: "); Serial.println(uptime-airStop);
    Serial.print("difference needAir: "); Serial.println(uptime-airStart);
  #endif
  
  if (!airOn()) {
    
    if (scheduledAir()) {
      
      #ifdef DEBUG0
        Serial.println("scheduledAir");
      #endif
      
      setAirOn();
    }
  }

  if (airOn()) {
    
    if (needAir()) {
      
      #ifdef DEBUG0
        Serial.println("needAir");
      #endif
      
      deviceSetOn();
    }
    else {
      
      #ifdef DEBUG0
        Serial.println("no needAir");
      #endif
      
      setAirOff();
      deviceSetOff(); 
    }
  }

  if (periodicUptimeSave()) {
    storeUptime();
  }

  #ifdef STATUS
    serialPrint();
  #endif
  
  #ifdef DELAY
    delay(1000);
  #endif
  
}

void countTime(void) {
  uptime += (millis() - previousUptime);
  previousUptime = millis();
}

boolean scheduledAir(void) {
  return ((uptime - airStop) > TO_MACHINE_TIME(AIR_INTERVAL));
}

boolean airOn(void) {
  return (airStatus == 1);
}

void deviceSetOn(void) {
  digitalWrite(6, HIGH);
}

void deviceSetOff(void) {
  digitalWrite(6, LOW);
}

boolean needAir(void) {
  return ((uptime - airStart) < TO_MACHINE_TIME(AIR_DURATION));
}

void setAirOn(void) {
  #ifndef DISABLE_WRITE
    EEPROM.updateByte(addressAirStatus, (uint8_t) 1);
    EEPROM.updateLong(addressAirStart, uptime);
    
    storeUptime();
    
    #ifdef WRITE
      Serial.println("write");
    #endif
    
  #endif
}

void setAirOff(void) {
  #ifndef DISABLE_WRITE
    EEPROM.updateByte(addressAirStatus, (uint8_t) 0);
    EEPROM.updateLong(addressAirStop, uptime);
    
    storeUptime();
    
    #ifdef WRITE
      Serial.println("write");
    #endif
    
  #endif
}

boolean periodicUptimeSave(void) {
  return (uptime > uptimeStoreTime + TO_MACHINE_TIME(UPTIME_STORE_INTERVAL));
}

void storeUptime(void) {
  #ifdef DEBUG0
    Serial.println("storeUptime");
  #endif
  
  uptimeStoreTime = uptime;
  
  #ifndef DISABLE_WRITE
    EEPROM.writeLong(addressUptime, uptime);
    
    #ifdef WRITE
      Serial.println("write");
    #endif
    
  #endif
}

void serialPrint(void) {

  unsigned long uptime_seconds = TO_HUMAN_TIME(uptime);
  
  unsigned long days = uptime_seconds / (SECONDS_IN_DAY);
  unsigned long hours = (uptime_seconds / SECONDS_IN_HOUR) % 24;
  unsigned long minutes = (uptime_seconds / 60) % 60;
  unsigned int seconds = uptime_seconds % 60;
  
  Serial.print(days); Serial.print(" days ");
  Serial.print(hours); Serial.print(" hours ");
  Serial.print(minutes); Serial.print(" minutes ");
  Serial.print(seconds); Serial.print(" seconds");
  
  Serial.println(); Serial.println();

}
