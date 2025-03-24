// This is a DEBUGING program. You will see silly stuff.
// Modify SlimLoRa.h for more options. OTAA vs ABP, DEBUG_SLIM (library), set RX1-2_DELAY times. Store SESSION to EEPROM (I thinks it needs correction)

#include <stdint.h>

#include "SlimLoRa.h"
#include <Adafruit_SleepyDog.h>

// 1 to DEBUG via serial, 0 to disable DEBUG.
// You can alse enable DEBUG_SLIM 1 in SlimLoRa.h
#define DEBUGINO   1

uint8_t  payload[1], payload_length;
uint8_t  fport = 1;
uint32_t wait  = 30; // mins
uint32_t waitAfterJoin  = 30; // seconds
uint32_t uptime;     // ms
uint32_t wtimes;

// DEBUG
uint32_t joinTime, RX2End, temp;

// fake battery measurement
uint8_t battery = 0;

SlimLoRa lora = SlimLoRa(8);           // NSS (CS) pin. ok with Feather 32u4 LoRa.

void setup() {

#if DEBUGINO == 1 || DEBUGFTDI == 1
    Serial.begin(9600);
    while (! Serial);                 // don't start unless we have serial connection
    Serial.println("Starting");
#endif

    delay(1000);
    lora.Begin();
    lora.SetDataRate(SF7BW125);
    //lora.SetPower(80);          // 80 is -80dBm. It has range of some centimeters from the gateway.
                                  // Useful to test without interrupting the GW or the TTN
    //lora.ForceTxFrameCounter(0);// useful only for testing.

#if LORAWAN_OTAA_ENABLED
  #if LORAWAN_KEEP_SESSION
    while (!lora.GetHasJoined()) {
  #else
    while (!lora.HasJoined()) {
  #endif // LORAWAN_KEEPSESSION
    #if DEBUGINO == 1 || DEBUGFTDI == 1
      Serial.println(F("\nJoining..."));
    #endif
        joinTime = millis() / 1000;
        lora.Join();
        
    #if DEBUGINO == 1 || DEBUGFTDI == 1
     RX2End = millis() / 1000;
     Serial.println(F("\nJoin effort finished."));
     Serial.print(F("\nRx2End after: "));Serial.print(RX2End - joinTime);
    #endif
    if ( lora.HasJoined() ) {
      Serial.println(F("\nJoined Sending packet in half minute."));
      delay(waitAfterJoin * 1000);
      break;
    }
      delay(wait * 60 * 1000);
    }
#endif // LORAWAN_OTAA_ENABLED
}

void loop() {
  payload_length = sizeof(payload);
  payload[0] = battery;

  lora.SendData(fport, payload, payload_length);

#if DEBUGINO == 1 || DEBUGFTDI == 1
  Serial.println(F("\nLoRaWAN packet send."));
  Serial.print(F("\nWill re-send again after (minutes): "));Serial.println(wait);
#endif

  delay(wait * 60 * 1000);
  battery++;
}
