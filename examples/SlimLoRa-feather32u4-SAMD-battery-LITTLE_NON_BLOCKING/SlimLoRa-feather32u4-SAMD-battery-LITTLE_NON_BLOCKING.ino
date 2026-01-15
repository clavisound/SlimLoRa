 /*  
 * This sketch joins and sends the battery level every 5 minutes.
 * It accepts downlinks to change this interval.
 *
 * It's LITTLE non blocking. SlimLoRa does not fully support non
 * blocking operation. But you can have 500ms of free operation
 * for you MCU between TX <-> RX1 and RX1 <-> RX2
 * 
 * You can send a downlink to port 1 with a value range of 0-255
 * 
 * You need 
 * To be in Europe! **Only EU868 region**. Sorry. Please feel free
 * to modify the SlimLoRa library and send PR's to my github
 * https://github.com/clavisound/SlimLoRa
 * I am pretty sure TinyLoRa code will be helpful to write the US frequencies.
 * 
 * Hardware: feather32u4 or MightyBrick (not available to tindie.com yet)
 * For other pin configuration you have to modify the SlimLoRa library.
 * Send PR's to https://github.com/clavisound/SlimLoRa
 * 
 * Software:
 * Arduino 1.8.19 (x86_64 binary on Linux),
 * 
 * AVR / ATMega
 * SleepyDog library from Adafruit
 * 
 * SAMD
 * SleepyDog library from Adafruit
 * RTC library
 * Arduino Low Power
 * SparkFun External EEPROM
 * In 01_config.h tab add your keys from your Network provider.
 * 
 * You may need to edit SlimLoRa.h for various options.
 * 
 * Feather32u4 deep sleep: 200μA. (If I remember correctly. Offender is the LDO)
 * MegaBrick   deep sleep:  23μA.
 * MightBrick  deep sleep:  27μA.
 * 
 */

#define FEATHER32U4 1
#define MIGHTYBRICK 2
#define TRACKERMEGABRICK 3

#define DEVICE TRACKERMEGABRICK

#include <stdint.h>
#include "SlimLoRa.h"
#if NON_BLOCKING
#include "SlimLoRaTimers.h"
#endif
#include "01_config.h"

#if DEVICE == FEATHER32U4
#include <Adafruit_SleepyDog.h>
#define RFM_CS_PIN  8 // pin to enable LoRa module
#define VBATPIN    A9 // pin to measure battery voltage.
#endif

#if DEVICE == MIGHTYBRICK
#include <Adafruit_SleepyDog.h>
#include "ArduinoLowPower.h"
#define RFM_CS_PIN            11
#define VBATPIN               A3
#define EEPROM_ADDRESS      0x50 // I2C address
#define EEPROM_EUI_ADDRESS  0xF8
#endif

#if DEVICE == TRACKERMEGABRICK
#include <Adafruit_SleepyDog.h>
#define RFM_CS_PIN            4
#define LED_BUILTIN         A3
#endif

// program behaviour
#ifndef DEBUG_INO
#define DEBUG_INO   1     // DEBUG via Serial.print
#endif

#define PHONEY      0     // don't transmit. for DEBUGing
#define POWER       0//NOWEB     // Transmittion power
#define LED_PRESENT

// downclock your MCU (only ATmega supported, you can also downclock SAMD with other method)
#define CLOCK_DIVIDER

uint8_t joinEfforts = 5; // how many times we will try to join.

uint32_t joinStart, joinEnd, RXend, newfCnt;
uint16_t vbat;
uint8_t dataRate, txPower = POWER, payload[1], payload_length, vbatC;
uint8_t fport = 1;
uint8_t minutes = 5;
uint8_t clockDivider;

SlimLoRa lora = SlimLoRa(RFM_CS_PIN);    // OK for feather 32u4 (CS featherpin. Aka: nss_pin for SlimLoRa). TODO: support other pin configurations.
