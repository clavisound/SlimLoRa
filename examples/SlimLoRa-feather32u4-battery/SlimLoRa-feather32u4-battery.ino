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
 * In SlimLoRa library edit SlimLoRa.h to KEEP_SESSION or to
 * select OTAA.
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
      Serial.println("Starting");
    #endif

    lora.Begin();
    lora.SetDataRate(SF10BW125);
    lora.SetPower(txPower);
    lora.SetAdrEnabled(0); // 0 to disable
}

void loop() {
  #ifdef LORAWAN_OTAA_ENABLED
    #ifdef LORAWAN_KEEP_SESSION // lora.GetHasJoined needs LORAWAN_KEEP_SESSION
      while (!lora.HasJoined() && !lora.GetHasJoined() && joinEfforts >= 1) {
     #else
      while (!lora.HasJoined() && joinEfforts >= 1) {
    #endif
        // Visible inform that we try to join.
        digitalWrite(LED_BUILTIN, HIGH);
        
        #ifdef DEBUG_INO
          Serial.print("\nJoin... Efforts remaining: ");Serial.println(joinEfforts);
        #endif
        
        joinEfforts--;
        joinStart = micros();
        lora.Join();
        joinEnd   = micros();

        // join effort is done. Close the lights.
        digitalWrite(LED_BUILTIN, LOW);
        
        #ifdef DEBUG_INO
        // We have efforts to re-try  
        if (!lora.HasJoined() && joinEfforts > 0) {
            Serial.print("JoinStart vs RXend micros (first number seconds): ");Serial.println(joinEnd - joinStart);
            Serial.println("Retry join in 4 minutes");
              blinkLed(220, 25, 1); // approx 4 minutes times, duration, every seconds
        }
        #else
            blinkLed(220, 25, 1); // approx 4 minutes times, duration, every seconds
        #endif
    }

// Not joined. Abort everythinh. Just blink every 9 seconds for ever.
if (!lora.HasJoined() && joinEfforts < 1) {
            blinkLed(220, 25, 9); // ~33 minutes: times, duration, every seconds
}

// send uplink
  if (lora.HasJoined()) {

      #ifdef DEBUG_INO
        Serial.println("Joined, sending uplink.");
      #endif
        checkBatt();

      #ifdef DEBUG_INO
        Serial.print("bat: ");Serial.println(vbat);
      #endif

        payload_length = sizeof(payload);
        lora.SendData(fport, payload, payload_length);

        // blink every 3 seconds for ~30 minutes. We joined.
        blinkLed(600, 50, 3);
        
        // testing power
        lora.SetPower(txPower);
        txPower = txPower - 1;
        if (txPower <= 1 ) { txPower = 16; }
  }
#endif // LORAWAN_OTAA_ENABLED
} 
