#if DEBUG_INO == 1
  uint8_t temp[16];
  uint32_t tempCounters;
#endif

void checkBatt(){
    vbat = analogRead(VBATPIN) - 450; // fit to 8bit. Will add 450 on decoder.
    /*
    vbat *= 2;    // we divided by 2, so multiply back
    vbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    vbat /= 1024; // convert to voltage
    all in one: vbat *=0.0064453125
    */
    
    // EVAL vbat: Connected to USB 4.29Volts, vbat operating: 688MAX
    // vbat charging: 3.9, 4.1, 4.5 (699)

    // map to 0, 40, 60, 80% baterry charging levels, EVAL: check for lowest level and highest level for fauny.
    // IMPORTANT: 465.5 = 3.0Volt. < DON'T GO THERE! Those are safe: 480 = 3.1Volts, 496 = 3.2 volts
    // Capacity measured with voltage is not linear! https://learn.adafruit.com/assets/979
    // 3.95V is 80%, 3.8V = 60%, 3.75 = 40%, 3.7Volt = 20%
    if ( vbat < 29 ) {
      vbatC = 0;
    }
    else {
    vbatC = map(vbat, 30, 200, 0, 3);
    }
      
    if ( vbatC > 3 ){ vbatC = 3; } // sometimes vbat is > 635 (aka: 185 after 8bit conversion) and we have overflow.
    
    #if DEBUG_INO == 1
      Serial.print(F("VBat (8bit): ")); Serial.print(vbat);Serial.print(F(", VBatB (volt): ")); Serial.print((vbat + 450) * 0.0064453125);
      Serial.print(F(", VBatC (range): ")); Serial.println(vbatC);
    #endif
    payload[0] = vbatC; // loraData[0] = highByte(vbat);loraData[1] = lowByte(vbat);
}

// function to wait and blink
void blinkLed(uint16_t times, uint16_t duration, uint8_t pause) { // x, ms, seconds
  if ( times == 0 ) times = 1; // make sure we have one loop
  for ( times > 0; times--; ) {
    digitalWrite(LED_BUILTIN, HIGH);
    #if DEBUG_INO == 1
      delay(duration);
      digitalWrite(LED_BUILTIN, LOW);
      if ( times % 80 == 0 ) {
        Serial.println();
      } else {
        Serial.print(F("."));
      }
      delay(pause * 1000);
    #else
      Watchdog.sleep(duration);               // Sleep for up to 8 seconds (8000ms, 4000, 2000, 1000, 500, 250, 120, 60, 30, 15ms)
      digitalWrite(LED_BUILTIN, LOW);
      Watchdog.sleep(pause * 1000);           // Sleep for up to 8 seconds (8000ms, 4000, 2000, 1000, 500, 250, 120, 60, 30, 15ms)
    #endif
  }
}

#if DEBUG_INO == 1
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

#endif // DEBUG_INO
