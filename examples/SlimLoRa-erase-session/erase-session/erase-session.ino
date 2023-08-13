/*  
 * This sketch tests eeprom of SlimLoRA. I use it to debug the arduino style eeprom store.
 */

#include <stdint.h>

#include "SlimLoRa.h"
#include <Adafruit_SleepyDog.h>
  
SlimLoRa lora = SlimLoRa(8);    // OK for feather 32u4 (CS featherpin. Aka: nss_pin for SlimLoRa). TODO: support other pin configurations.

  uint8_t addr[4]  = { 0x26, 0xFF, 0xFF, 0xFF }; // 0x26 is hommage to TTN :)
  uint8_t temp[16];
  uint32_t tempCounters;
  
void setup() {
    pinMode(LED_BUILTIN, OUTPUT); // Initialize pin LED_BUILTIN as an output
  
    delay(3000);
    while (! Serial);                 // don't start unless we have serial connection
    Serial.println(F("Starting"));

// TEST EEPROM data of SlimLoRa
lora.SetHasJoined(0);
Serial.print("HasJoined: ");Serial.println(lora.GetHasJoined());
lora.SetTxFrameCounter(0);
Serial.print("TxFrameCounter: ");Serial.println(lora.GetTxFrameCounter());
lora.SetRxFrameCounter(0);
Serial.print("RxFrameCounter: ");Serial.println(lora.GetRxFrameCounter());
lora.SetRx1DataRateOffset(0);
Serial.print("Rx1DataRateOffset: ");Serial.println(lora.GetRx1DataRateOffset());
lora.SetRx2DataRate(3); // default for TTN
Serial.print("Rx2DataRate: ");Serial.println(lora.GetRx2DataRate());
lora.SetRx1Delay(5);  // default for TTN
Serial.print("Rx1Delay: ");Serial.println(lora.GetRx1Delay());
lora.SetDevNonce(0);
Serial.print("DevNonce: ");Serial.println(lora.GetDevNonce());
lora.SetJoinNonce(0);
Serial.print("JoinNonce: ");Serial.println(lora.GetJoinNonce());
// Arrays
lora.SetDevAddr(addr);
lora.GetDevAddr(temp);
Serial.print("DevAddr: ");printHexB(temp, 4);
// every two bytes the temp array is filled with value 4
fillTemp(4);lora.SetAppSKey(temp);
// fill temp array with zeroes. After GetFunction we should not see only zeroes.
fillTemp(0);lora.GetAppSKey(temp);
Serial.print("AppSKey: ");printHexB(temp, 16);
// value 8 every 2 bytes
fillTemp(8);lora.SetFNwkSIntKey(temp);
fillTemp(0);lora.GetFNwkSIntKey(temp);
Serial.print("FNwkSIntKey: ");printHexB(temp, 16);
// value 16 every 2 bytes
fillTemp(16);lora.SetSNwkSIntKey(temp);
fillTemp(0);lora.GetSNwkSIntKey(temp);
Serial.print("SNwkSIntKey: ");printHexB(temp, 16);
// value 32 every 2 bytes
fillTemp(32);lora.SetNwkSEncKey(temp);
fillTemp(0);lora.GetNwkSEncKey(temp);
Serial.print("NwkSEncKey: ");printHexB(temp, 16);
}

void loop() {
  delay(3000);
  Watchdog.sleep(8000);
}

void printHexB(uint8_t *value, uint8_t len){ 
        Serial.print(F("\nMSB: 0x"));
      for (int8_t i = 0; i < len; i++ ) {
        if (value[i] == 0x0 ) { Serial.print(F("00")); continue; }
        if (value[i] <= 0xF ) { Serial.print(F("0")); Serial.print(value[i], HEX);continue; }
      Serial.print(value[i], HEX);
      }
      Serial.print(F("\nLSB: 0x"));
      for (int8_t i = len - 1; i >= 0; i-- ) {
        if (value[i] == 0x0 ) { Serial.print(F("00")); continue; }
        if (value[i] <= 0xF ) { Serial.print(F("0")); Serial.print(value[i], HEX); continue; }
      Serial.print(value[i], HEX);
      }
        Serial.println();
}

void fillTemp(uint8_t value){
for( uint8_t i = 0; i < 16; i++ ) {
  if ( i % 2 == 0 ) {
  temp[i] = 0;
  }
  else {
    temp[i] = value;
   }
 } // for
} //fillTemp
