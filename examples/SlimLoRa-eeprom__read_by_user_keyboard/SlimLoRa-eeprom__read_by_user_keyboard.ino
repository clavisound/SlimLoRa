/* Manage SlimLoRa EEPROM interactively
 * Started SEP24 by clavisound
 * In SlimLoRa.h DEBUG_SLIM write 2 instead of 0 to display also sensitive data.
 */

// #define DEBUG_SLIM 1 // This is not working. Fault of Arduino IDE

/* How to enable DEBUG_SLIM
 * 
 * >>> FAILED with Arduino IDE 1.8.19
 * for IDE add this compiler.cpp.extra_flags=-DDEBUG_SLIM=1 build_opt.txt
 * 
 * >>> Works
 * arduino-cli compile ./ -b adafruit:avr:adafruit32u4 --build-property "compiler.cpp.extra_flags=\"-DDEBUG_SLIM=1\""
 * 
 * PlatformIO .ini
 * 
 * Add:
 * build_flags = -DDEBUG_SLIM=1
 * 
 */

#if ARDUINO_EEPROM == 1
  #include <EEPROM.h>
#endif
#include "SlimLoRa.h"

#define DEVICE 'feather'
#define TOTALBYTES EEPROM_END // EEPROM_END grabbed from SlimLoRa.h

#define ASCII_ZERO  48

uint8_t  inByte;
uint8_t  menuPrinted;
uint16_t temp;

uint16_t originalOffset = EEPROM_OFFSET;
uint16_t targetOffset   = EEPROM_OFFSET + EEPROM_END;

//uint8_t tempOffset     = 615; // valid for 1K EEPROM. Not used.
uint8_t key[16], keystrokes[8];
uint8_t bufferOriginal[TOTALBYTES], bufferTarget[TOTALBYTES];

uint32_t eeprom_size;

#if DEVICE == 'megaBrick'
  SlimLoRa lora = SlimLoRa(4);
#else
  SlimLoRa lora = SlimLoRa(8);
#endif

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (! Serial); // don't start unless we have serial connection

  lora.Begin();
  lora.sleep();

  eeprom_size = EEPROM.length();
}

void loop()
{

  if ( menuPrinted == 0 ) {
    printMenu();
    menuPrinted = 1;
  }
  
  while (Serial.available() > 0) {
    inByte = Serial.read();
    if (inByte == 'd') { setRXdelay(); menuPrinted = 0; break; }
    if (inByte == 'e') { eepromOffset(); menuPrinted = 0; break; }
    if (inByte == 'i') { increaseFCnt(); menuPrinted = 0; break; }
    //if(inByte == 'a') { lora.SetAppSKey(key); menuPrinted = 0; break; }
    //if(inByte == 'e') { menuPrinted = 0; break; } // devEUI is in firmware.
    if (inByte == 'k') { lora.SetHasJoined(false); menuPrinted = 0; break; }
    if (inByte == 'j') { lora.SetHasJoined(true); menuPrinted = 0; break; }
    // if (inByte > 0 || inByte < 10 ) { lora.SetRx1Delay(inByte); menuPrinted = 0; break; }
    if (inByte == 'o') { rx1droffset(); menuPrinted = 0; break; }
    if (inByte == 'R') { rx2dr(); menuPrinted = 0; break; }
    if (inByte == 'm') { lora.printMAC(); menuPrinted = 0; break; }
    if (inByte == 's') { swapMACstatus(); menuPrinted = 0; break; }
    if (inByte == 'Z') { eraseOriginal(); menuPrinted = 0; break; }
    if (inByte == 'F') { fullErase(); menuPrinted = 0; break; }
  }
}