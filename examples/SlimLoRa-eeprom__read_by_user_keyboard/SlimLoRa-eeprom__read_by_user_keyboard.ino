 /* Manage SlimLoRa EEPROM interactively
 * SEP24 by clavisound
 * In SlimLoRa.h DEBUG_SLIM write 2 instead of 0 to display also sensitive data.
 */

#include <EEPROM.h>
#include "SlimLoRa.h"
#define DEVICE 'feather'
#define TOTALBYTES EEPROM_END // EEPROM_END grabbed from SlimLoRa.h

#define ASCII_ZERO  48

uint8_t  inByte;
uint16_t temp;

// 0, 152, 304, 456, 608, 760
uint16_t originalOffset = 152;
uint16_t targetOffset   = 410;

//uint8_t tempOffset     = 615; // valid for 1K EEPROM. Not used.
uint8_t key[16], keystrokes[8];
uint8_t bufferOriginal[TOTALBYTES], bufferTarget[TOTALBYTES];

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
  printMenu();
}

void loop()
{ 
  while (Serial.available() > 0) {
    inByte = Serial.read();
    if (inByte == 'd') { setRXdelay(); break; }
    if (inByte == 'e') { eepromOffset(); break; }
    if (inByte == 'i') { increaseFCnt(); break; }
    //if(inByte == 'a') { lora.SetAppSKey(key); break; }
    //if(inByte == 'e') { break; } // devEUI is in firmware.
    if (inByte == 'k') { lora.SetHasJoined(false); break; }
    if (inByte == 'j') { lora.SetHasJoined(true); break; }
    // if (inByte > 0 || inByte < 10 ) { lora.SetRx1Delay(inByte); break; }
    if (inByte == 'r') { rx1droffset(); break; }
    if (inByte == 'R') { rx2dr(); break; }
    if (inByte == 'm') { lora.printMAC(); break; }
    if (inByte == 's') { swapMACstatus(); break; }
    if (inByte == 'Z') { eraseOriginal(); break; }
  }
}
