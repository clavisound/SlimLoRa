void printMenu(){
  lora.printMAC();
  Serial.println(F("\nd[number]: setRXdelay (1 to 9 seconds)"));
  Serial.println(F("i[number]: set FCnt, k: disable joined, j: enable joined"));
  Serial.print(F("Z: EraZe original: "));Serial.println(originalOffset);
  Serial.println(F("eS<num>: set EEPROM offsets. eS<num> for Original, eT<num> for Target."));
  
  // those are in firmware! Patching to hex works? Examine with objdump.
  // Serial.println(F("a: appSkey, e: devEUI, j: joinEUI, n: nwkKey"));

  //TODO
  //Serial.println(F("o[number]: rx1 data rate offset, R: rx2 data rate"));
  //Serial.println(F("E [capital]: EEPROM address to COPY data."));
  Serial.println(F("m: read all MAC values."));
  Serial.println(F("s: swap MAC status."));
  Serial.println(F("F: erase *ALL* EEPROM."));
  //Serial.print(F("\tEEPROM size: "));Serial.print(eeprom_size);
  Serial.flush();
}

void increaseFCnt(uint16_t newFCntValue){
    delay(100);
    lora.tx_frame_counter_ = newFCntValue;
    delay(100);
    lora.SetTxFrameCounter();
    Serial.print(F("New FCnt: "));Serial.println(lora.tx_frame_counter_);
}

void appSkey(){
  
}

void rx1droffset(){
  Serial.println(F("Rx1 DR offset [usually 0]"));
  Serial.println(F("Type from 0 to 7. Default for Helium and TTN is 0."));
  Serial.print(F("Current value: "));Serial.println(lora.GetRx1DataRateOffset());
  while(inByte >= 0 + ASCII_ZERO && inByte < 8 + ASCII_ZERO ) {
    lora.SetRx1DataRateOffset(inByte - ASCII_ZERO);
    break;
    }
}

void rx2dr(){
  Serial.println(F("Rx2 DR. 0: SF12 [helium], 3: SF9 [ttn], 5: SF7"));
  Serial.print(F("Current value: "));Serial.println(lora.GetRx2DataRate());
  while(inByte >= 0 + ASCII_ZERO && inByte < 6 + ASCII_ZERO ) {
    lora.SetRx2DataRate(inByte - ASCII_ZERO);
  break;
  }
}

// using buffer for less EEPROM writes.
void swapMACstatus(){

  Serial.println(F("Swap MacStatus: "));
  
  // read original
  for ( temp = 0; temp <= TOTALBYTES; temp++ ) {
    bufferOriginal[temp] = EEPROM.read(originalOffset + temp);
  }

  // read target
  for ( temp = 0; temp <= TOTALBYTES; temp++ ) {
    bufferTarget[temp] = EEPROM.read(targetOffset + temp);
    Serial.print(F("byte: "));Serial.print(temp);
    Serial.print(F("->: "));Serial.println(bufferTarget[temp]);
  }

  // write target
  for ( temp = 0; temp <= TOTALBYTES; temp++ ) {
    EEPROM.update(targetOffset   + temp, bufferOriginal[temp]);
    Serial.print(F("target byte: "));Serial.print(temp);
    Serial.print(F("->: "));Serial.println(bufferOriginal[temp]);
  }

  // write original
  for ( temp = 0; temp <= TOTALBYTES; temp++ ) {
    EEPROM.update(originalOffset + temp, bufferTarget[temp]);
    Serial.print(F("original byte: "));Serial.print(temp);
    Serial.print(F("->: "));Serial.println(bufferTarget[temp]);
  }
}

void setRXdelay(){
  Serial.print(F("\n\nEnter a number 1 to 9. LoRaWAN spec says 1 to 15"));
    while (Serial.available() == 0 ) {
      // blocking call
    }
      inByte = Serial.read();
       if ( ( inByte >= 1 + ASCII_ZERO ) && ( inByte <= 9 + ASCII_ZERO ) ) {
        Serial.print(F("\tdone."));
        delay(300);
        Serial.flush();
        lora.SetRx1Delay(inByte - ASCII_ZERO);
        
        return;
    } else {
        Serial.print(F("\nWrong byte HEX: "));Serial.print(inByte, HEX);
  }
}

void eraseOriginal(){

  Serial.println(F("Erasing original MAC: "));
  
  // erase original
  for ( temp = 0; temp <= TOTALBYTES; temp++ ) {
    EEPROM.update(originalOffset + temp, 0xFF);
    Serial.println(originalOffset + temp);
    Serial.flush();
  }
}

void fullErase(){
  
  Serial.println(F("Erasing all EEPROM: "));
  
  // erase original
  for ( temp = 0; temp <= eeprom_size; temp++ ) {
    EEPROM.update(temp, 0xFF);
    Serial.println(temp);
    Serial.flush();
  }
}

uint16_t readNumberFromSerial() {
  uint16_t number = 0;
  char charIn;
  while (Serial.available() == 0) {
    // wait for input
  }
  delay(100); // Give time for more characters to arrive
  while (Serial.available() > 0) {
    charIn = Serial.read();
    if (charIn >= '0' && charIn <= '9') {
      number = number * 10 + (charIn - '0');
    } else {
      // Non-digit character (like newline or space) terminates input
      break;
    }
  }
  return number;
}
