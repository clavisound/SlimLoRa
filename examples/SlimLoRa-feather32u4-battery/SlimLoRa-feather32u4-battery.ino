/*
 * This sketch joins and sends the battery level every 30 minutes.
 * 
 * Tested with Helium SF10 in Europe.
 * 
 * Problems with TTN:
 * TTN sends RX2 with SF12, although I selected SF9 for RX2
 * Join fails after 10 efforts. But maybe my gateway
 * (nfuse sx1308 via usb) is the culprit.
 * 
 * You need 
 * To be in Europe! **Only 868 region**. Sorry. Please feel free
 * to modify the SlimLoRa library and send PR's to my github
 * https://github.com/clavisound/SlimLoRa
 * 
 * I pretty sure TinyLoRa code will be helpfull to write the US frequencies
 * 
 * Hardware: feather32u4
 * For other pin configuration you have to modify the SlimLoRa library.
 * Send PR's to https://github.com/clavisound/SlimLoRa
 * 
 * Software:
 * Arduino 1.8.19 (binary on Linux),
 * SleepyDog library from Adafruit,
 * In config tab add your keys from your Network provider.
 * 
 * In SlimLoRa library edit SlimLoRa.h to KEEP_SESSION and select OTAA.
 * 
 */

#include <stdint.h>

#include "SlimLoRa.h"
#include <Adafruit_SleepyDog.h>

#define DEBUG_INO

#define VBATPIN   A9

uint8_t joinEfforts = 10; // how many times we will try to join.

uint32_t joinStart, joinEnd, RXend, vbat;
uint8_t dataRate, txPower = 16, payload[1], payload_length, vbatC;
uint8_t fport = 1;

SlimLoRa lora = SlimLoRa(8);    // OK for feather 32u4 (CS featherpin. Aka: nss_pin for SlimLoRa). TODO: support other pin configurations.

void setup() {
    pinMode(LED_BUILTIN, OUTPUT); // Initialize pin LED_BUILTIN as an output
  
    delay(9000);
    #ifdef DEBUG_INO
      while (! Serial);                 // don't start unless we have serial connection
      Serial.println(F("Starting"));
    #endif

    lora.Begin();
    lora.SetDataRate(SF9BW125);
    lora.SetPower(txPower);
    lora.SetAdrEnabled(1); // 0 to disable

    // For DEBUG only
    lora.ForceTxFrameCounter(9);

    #ifdef LORAWAN_KEEP_SESSION // lora.GetHasJoined needs LORAWAN_KEEP_SESSION
      while (!lora.GetHasJoined() && joinEfforts >= 1) {
     #else
      while (!lora.HasJoined() && joinEfforts >= 1) {
    #endif // LORAWAN_KEEP_SESSION
        // Visible inform that we try to join.
        digitalWrite(LED_BUILTIN, HIGH);
        
        #ifdef DEBUG_INO
          Serial.print(F("\nJoining. Efforts remaining: "));Serial.println(joinEfforts);
        #endif
        
        joinEfforts--;
        joinStart = micros();
        lora.Join();
        joinEnd   = micros();

        // join effort is done. Close the lights.
        digitalWrite(LED_BUILTIN, LOW);
        
        // We have efforts to re-try  
        if (!lora.HasJoined() && joinEfforts > 0) {
          #ifdef DEBUG_INO
            Serial.print(F("\nJoinStart vs RXend micros (first number seconds): "));Serial.print(joinEnd - joinStart);
            Serial.println(F("\nRetry join in 4 minutes"));
        #else
            blinkLed(240, 10, 1); // approx 3 minutes times, duration (ms), every seconds
        #endif
    }
}

#ifdef DEBUG_INO
#ifdef LORAWAN_KEEP_SESSION
   if ( lora.GetHasJoined() ) {
     Serial.println(F("\nNo need to join. Session restore from EEPROM."));
    return;
#else
   if ( lora.HasJoined() ) {
#endif // LORAWAN_KEEP_SESSION
   Serial.println(F("\nJust joined. Session started."));
   }
#endif // DEBUG_INO
}

void loop() {
  #ifdef LORAWAN_KEEP_SESSION // lora.GetHasJoined needs LORAWAN_KEEP_SESSION
    if (!lora.GetHasJoined() && joinEfforts < 1) {
  #else
    // Not joined. Abort everything. Just blink every 9 seconds for ever.
    if (!lora.HasJoined() && joinEfforts < 1) {
  #endif // LORAWAN_KEEP_SESSION
  blinkLed(220, 25, 9); // ~33 minutes: times, duration (ms), every seconds
}

// send uplink
#ifdef LORAWAN_KEEP_SESSION
  if ( lora.GetHasJoined() ) {
#else
  if ( lora.HasJoined() ) {
#endif // LORAWAN_KEEP_SESSION

  #ifdef DEBUG_INO
    Serial.println(F("\nSending uplink."));
  #endif
    checkBatt();

    payload_length = sizeof(payload);
    lora.SendData(fport, payload, payload_length);

    // blink every 3 seconds for ~15 minutes. We joined.
    blinkLed(300, 50, 3);
        
    // testing power
    lora.SetPower(txPower);
    txPower = txPower - 1;
    if (txPower <= 1 ) { txPower = 16; }
  }
} 
