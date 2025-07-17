 /*  
 * This sketch joins and sends the battery level every 5 minutes.
 * It accepts downlinks to change this interval.
 * 
 * Send a downlink to port 1 with a value range of 0-255
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
 * SleepyDog library from Adafruit,
 * Arduino Low Power (for mightybrick)
 * In 01_config.h tab add your keys from your Network provider.
 * 
 * You may need to edit SlimLoRa.h for various options.
 * 
 */

#define FEATHER32U4 1
#define MIGHTYBRICK 2

#define DEVICE FEATHER32U4

#include <stdint.h>
#include "SlimLoRa.h"
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

// program behaviour
#define DEBUG_INO   1     // DEBUG via Serial.print
                          // You also need DEBUG_SLIM 1 in SlimLoRa.h
#define PHONEY      0     // don't transmit. for DEBUGing
#define POWER	14     // Transmittion power

uint8_t joinEfforts = 5; // how many times we will try to join.

uint32_t joinStart, joinEnd, RXend, newfCnt;
uint16_t vbat;
uint8_t dataRate, txPower = POWER, payload[1], payload_length, vbatC;
uint8_t fport = 1;
uint8_t minutes = 5;

SlimLoRa lora = SlimLoRa(RFM_CS_PIN);    // OK for feather 32u4 (CS featherpin. Aka: nss_pin for SlimLoRa). TODO: support other pin configurations.

void setup() {
    pinMode(LED_BUILTIN, OUTPUT); // Initialize pin LED_BUILTIN as an output
    pinMode(VBATPIN, INPUT);
  
    delay(3000);
    #if DEBUG_INO == 1
      while (! Serial);                 // don't start unless we have serial connection
      Serial.println(F("Starting"));
    #endif

  #if DEVICE == MIGHTYBRICK
  analogReference(AR_DEFAULT); 
  // EEPROM MightyBrick. SlimLoRa needs this.
  Wire.begin();
  Wire.setClock(400000);
  EEPROM.setMemoryType(2);

  //EEPROM.get(248, DevEUI); 		// SparkFun Style
  lora.getArrayEEPROM(248, DevEUI, 8);  // SlimLoRa Style

  #if DEBUG_INO == 1
  Serial.print(F("\nDevEUI from EEPROM: "));
  lora.printHex(DevEUI, 8);
  #endif
  
  #endif
  
    lora.Begin();
    lora.SetDataRate(SF7BW125);
    lora.SetPower(txPower);
    lora.SetAdrEnabled(1); // 0 to disable. Network can still send ADR command to device. This is preference, not an order.

    // increase frame counter after reset
    lora.tx_frame_counter_ += EEPROM_WRITE_TX_COUNT;
    lora.SetTxFrameCounter();

    // Show data stored in EEPROM
    #if DEBUG_INO == 1
      Serial.println(F("\nAfter 9 seconds the program will start."));
    #endif // DEBUG_INO

    // Delay for 9 seconds with blink
     blinkLed(6, 500, 1); // times, duration (ms), seconds

     #if DEBUG_INO == 1
     checkBatt();
     #endif

    // Join if we have efforts.
    #if LORAWAN_KEEP_SESSION == 1 && LORAWAN_OTAA_ENABLED == 1 // lora.GetHasJoined needs LORAWAN_KEEP_SESSION
      while (!lora.GetHasJoined() && joinEfforts >= 1) {
    #endif
    
    #if LORAWAN_KEEP_SESSION == 0 && LORAWAN_OTAA_ENABLED == 1 // lora.HasJoined without LORAWAN_KEEP_SESSION
      while (!lora.HasJoined() && joinEfforts >= 1) {
    #endif // LORAWAN_KEEP_SESSION
        // Steady LED indicates that we try to join.
        digitalWrite(LED_BUILTIN, HIGH);
        
        #if DEBUG_INO == 1
          Serial.print(F("\nJoining. Efforts remaining: "));Serial.println(joinEfforts - 1);
        #endif
        
        joinEfforts--;
        joinStart = micros();
        
        #if PHONEY == 1
          Serial.print(F("\nPhoney trasmit."));
          delay(3000);
        #endif
        
        #if PHONEY == 0
          lora.Join();
        #endif
        
        joinEnd   = micros();

        // join effort is done. Close the lights.
        digitalWrite(LED_BUILTIN, LOW);
        
        // Check if we joined. If not delay.
        #if LORAWAN_KEEP_SESSION == 1 && LORAWAN_OTAA_ENABLED == 1 // lora.GetHasJoined needs LORAWAN_KEEP_SESSION
          if (!lora.GetHasJoined() && joinEfforts >= 1) {
        #endif
    
        #if LORAWAN_KEEP_SESSION == 0 && LORAWAN_OTAA_ENABLED == 1 // lora.HasJoined without LORAWAN_KEEP_SESSION
           if (!lora.HasJoined() && joinEfforts >= 1) {
        #endif // LORAWAN_KEEP_SESSION
          #if DEBUG_INO == 1
            Serial.print(F("\nJoinStart vs RXend micros (first number seconds): "));Serial.print(joinEnd - joinStart);
            Serial.println(F("\nRetry join in 6 minutes"));
            Serial.print("DR: ");Serial.println(SF7BW125);
            blinkLed(320, 10, 1); // approx 6 minutes times, duration (ms), every seconds
          #else
           blinkLed(320, 10, 1);
          #endif
    }
  }

// Check if we joined to exit setup.
#if DEBUG_INO == 1
#if LORAWAN_KEEP_SESSION == 1 && LORAWAN_OTAA_ENABLED == 1
   if ( lora.GetHasJoined() ) {
     Serial.println(F("\nNo need to join. Just Joined or session restore from EEPROM."));
    return;
   }
#endif // LORAWAN_OTAA_ENABLED
#if LORAWAN_KEEP_SESSION == 0 && LORAWAN_OTAA_ENABLED == 1
   if ( lora.HasJoined() ) {
   Serial.println(F("\nJust joined. Session started."));
   Serial.print(F("\nJoinStart vs RXend micros (first number seconds): "));Serial.print(joinEnd - joinStart);
   }
#endif // LORAWAN_KEEP_SESSION
#endif // DEBUG_INO

// Uncomment to ask for time and demodulation margin and gateway count
// lora.TimeLinkCheck = 3; // 1 for only time, 2 for margin and gateway count. 3 for both.

// with lora.epoch you have access to GPS epoch from LNS.
// lora.fracSeconds holds the value of seconds in ms.
// lora.margin holds the demodulation margin (0-20)
// lora.GwCnt holds the number of Gateways received the message.

} // setup()

void loop() {
  // SETUP will exit after the failed effors. Check again if we joined.
  #if LORAWAN_KEEP_SESSION == 1 && LORAWAN_OTAA_ENABLED == 1 // lora.GetHasJoined needs LORAWAN_KEEP_SESSION
    if (!lora.GetHasJoined() && joinEfforts < 1) {
  #else
    // Not joined. Abort everything. Just blink every 9 seconds for ever.
    if (!lora.HasJoined() && joinEfforts < 1) {
  #endif // LORAWAN_KEEP_SESSION
  blinkLed(220, 25, 9); // ~33 minutes: times, duration (ms), every seconds
  }

// send uplink
#if LORAWAN_KEEP_SESSION == 1 && LORAWAN_OTAA_ENABLED == 1
  if ( lora.GetHasJoined() ) {
#endif
#if LORAWAN_KEEP_SESSION == 0 && LORAWAN_OTAA_ENABLED == 1
  if ( lora.HasJoined() ) {
#endif // LORAWAN_KEEP_SESSION

  #if DEBUG_INO == 1
    Serial.println(F("\nSending uplink."));
  #endif
    checkBatt();

    #if PHONEY == 0
    payload_length = sizeof(payload);
    lora.SendData(fport, payload, payload_length);
    switchDR(); //NOWEB Testing, ignore this.
    #else // PHONEY
    Serial.print(F("\nFake uplink."));
    #endif

    // if we received downlink on port 1 change the minutes interval.
    if ( lora.downlinkSize > 0 ) {
      if ( lora.downPort == 1 ) {

        #if DEBUG_INO == 1
        Serial.print(F("\nUsed data from Port\t: "));Serial.print(lora.downPort);
        Serial.print(F("\ndownlinkSize\t\t: "));Serial.print(lora.downlinkSize);
        #endif

        // update the interval of reporting
        minutes = lora.downlinkData[0];
        // in case payload of downlink is too small.
        if ( minutes < 2 ) {
          minutes = 2;
        } // minutes
      
      #if DEBUG_INO == 1
      Serial.print(F("\nNew minutes\t\t: "));Serial.print(minutes);
      #endif

      } else {

      #if DEBUG_INO == 1
      Serial.print(F("\nUndefined Port\t: "));Serial.print(lora.downPort);
      Serial.print(F("\ndownlinkSize\t: "));Serial.print(lora.downlinkSize);
      #endif
      
      } // downPort
    } else { 
      
      #if DEBUG_INO == 1
      Serial.print(F("\nNo downlink data."));
      #endif
      
      // downlinkSize
    }

  #if DEBUG_INO == 1
    Serial.println(F("\nUplink done."));
    Serial.print(F("\nSleeping for minutes: "));Serial.print(minutes);
  #endif

   // blink every 3 seconds for ~15 minutes. We joined.
   blinkLed(20 * minutes, 50, 3);
  
 } // (Get)HasJoined()
} // loop()

void switchDR(){
  if (lora.data_rate_ == 6 ) {
  lora.SetDataRate(SF7BW125);
  } else {
  lora.SetDataRate(SF7BW250);
  }
}
