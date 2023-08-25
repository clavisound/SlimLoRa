 /*  
 * This sketch joins and sends the battery level every 15 minutes.
 * It stores the MAC state in EEPROM. If the battery dies, the
 * device does not re-joins.
 * 
 * You need 
 * To be in Europe! **Only EU868 region**. Sorry. Please feel free
 * to modify the SlimLoRa library and send PR's to my github
 * https://github.com/clavisound/SlimLoRa
 * I am pretty sure TinyLoRa code will be helpful to write the US frequencies.
 * 
 * Hardware: feather32u4
 * For other pin configuration you have to modify the SlimLoRa library.
 * Send PR's to https://github.com/clavisound/SlimLoRa
 * 
 * Software:
 * Arduino 1.8.19 (x86_64 binary on Linux),
 * SleepyDog library from Adafruit,
 * In 01_config tab add your keys from your Network provider.
 * 
 * You may need to edit SlimLoRa.h for various options.
 * 
 * It needs in SlimLoRa.h:
 * #define EU863
 * #define LORAWAN_OTAA_ENABLED
 * #define LORAWAN_KEEP_SESSION
 * #define ARDUINO_EEPROM 1
 * 
 */

#include <stdint.h>

#include "SlimLoRa.h"
#include <Adafruit_SleepyDog.h>
  
#define DEBUG_INO 0       // DEBUG via Serial.print. If you enable this, it's a battery killer. Disable DEBUG_INO and you will have deep sleep.
#define PHONEY    0       // don't transmit. for DEBUGing

#define VBATPIN   A9

uint8_t joinEfforts = 5; // how many times we will try to join.

uint32_t joinStart, joinEnd, RXend, vbat;

// Modify txPower in dBm if the gateway is not in you neighbourhood.
// Maximum for EU863 is 16. Don't use more than 16 unless you have a VSWR 3:1 antenna and less than 1% duty cycle.
// 16 dBm is more than enough.
uint8_t txPower = 0;

uint8_t payload[1], payload_length, vbatC;
uint8_t fport = 1;

SlimLoRa lora = SlimLoRa(8);    // OK for feather 32u4 (CS featherpin. Aka: nss_pin for SlimLoRa). TODO: support other pin configurations.

void setup() {
    pinMode(LED_BUILTIN, OUTPUT); // Initialize pin LED_BUILTIN as an output
  
    delay(3000);
    #if DEBUG_INO == 1
      Serial.begin(9600);
      while (! Serial);                 // don't start unless we have serial connection
      Serial.println(F("Starting"));
    #endif

    lora.Begin();
    lora.SetDataRate(SF7BW125); // choose the Data Rate. SF11 and SF12 are not welcome.
    lora.SetPower(txPower);
    lora.SetAdrEnabled(1);      // 0 to disable. Network can still send ADR command to device. This is preference, not an order.

    // Show data stored in EEPROM
    #if DEBUG_INO == 1
      printMAC_EEPROM();
      Serial.println(F("Disconnect / power off the device and study the log. You have 30 seconds time.\nAfter that the program will continue."));
    #endif // DEBUG_INO

    // Just a delay for 30 seconds
     blinkLed(20, 500, 1); // times, duration (ms), seconds

    while (!lora.GetHasJoined() && joinEfforts >= 1) {
        // Steady LED indicates that we try to join.
        digitalWrite(LED_BUILTIN, HIGH);
        
        #if DEBUG_INO == 1
          Serial.print(F("\nJoining. Efforts remaining: "));Serial.println(joinEfforts - 1);
        #endif
        
        joinEfforts--;

        // joinStart and joinEnd to count the time to Join. (for DEBUG)
        joinStart = micros();
        lora.Join();
        joinEnd   = micros();

        // join effort is done. Close the lights.
        digitalWrite(LED_BUILTIN, LOW);
        
        // Check if we joined. If not delay.
          if (!lora.GetHasJoined() && joinEfforts >= 1) {
          #if DEBUG_INO == 1
            Serial.print(F("\nJoinStart vs RXend micros (first number seconds): "));Serial.print(joinEnd - joinStart);
            Serial.println(F("\nRetry join in 6 minutes"));
            printMAC_EEPROM();
            Serial.print("DR: ");Serial.println(SF7BW125);
            blinkLed(320, 10, 1); // approx 6 minutes times, duration (ms), every seconds
          #else
           blinkLed(320, 10, 1);
          #endif
    }
  }

// Check if we joined to inform the user.
#if DEBUG_INO == 1
   if ( lora.GetHasJoined() ) {
     Serial.println(F("\nNo need to join. Just Joined or session restore from EEPROM."));
    return;
   }
#endif // DEBUG_INO
} // setup()

void loop() {
  // SETUP will exit after the failed effors. Check again if we joined.
    if (!lora.GetHasJoined() && joinEfforts < 1) {
      blinkLed(220, 25, 9); // ~33 minutes: times, duration (ms), every seconds
} // if !lora.GetHasJoined

// send uplink
  if ( lora.GetHasJoined() ) {
  #if DEBUG_INO == 1
    Serial.println(F("\nSending uplink."));
    printMAC_EEPROM();
  #endif

    // read battery voltage, convert to capacity value 0 (empty) 3 (full)
    checkBatt();

    payload_length = sizeof(payload);
    lora.SendData(fport, payload, payload_length);

  #if DEBUG_INO == 1
    Serial.println(F("\nUplink done."));
    printMAC_EEPROM();
  #endif

   // blink every 3 seconds for ~15 minutes. We joined.
   blinkLed(300, 50, 3);
  
 } // if GetHasJoined()
} // loop()
