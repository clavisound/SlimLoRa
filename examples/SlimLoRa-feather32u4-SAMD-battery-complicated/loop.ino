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
        Serial.print(F("\nSending uplink."));
#endif

        checkBatt();

#if PHONEY == 0
        payload_length = sizeof(payload);
        lora.SendData(fport, payload, payload_length);
        //switchDR(); //NOWEB Testing, ignore this.

#else // PHONEY
        Serial.print(F("\nFake uplink."));
#endif

        // if we received downlink on port 1 change the minutes interval.
        if ( lora.downlinkSize > 0 ) {
          if ( lora.downPort == 1 ) {

            // Notify via LED that we received a valid downlink
#ifdef LED_PRESENT
            digitalWrite(LED_BUILTIN, HIGH);
#endif

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

#ifdef LED_PRESENT
            delay(3000 >> clockDivider);
            digitalWrite(LED_BUILTIN, LOW);
#endif

          } else {

#if DEBUG_INO == 1
            Serial.print(F("\nUnsupported Port\t: "));Serial.print(lora.downPort);
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

        // blink every 8 seconds for ~15 minutes. We joined.
        blinkLed(7 * minutes, 50, 8); // SAMD max is 16, AVR / ATmega is 8

      } // (Get)HasJoined()
} // loop()
