void setup() {
#ifdef CLOCK_DIVIDER
  clock_prescale_set(clock_div_8);

  // Serial is fine with FTDI, feather-32u4 is picky via USB
  // 16Mhz  = 11.05mA if your sketch makes many calculations this is more efficient.
  // 8Mhz   =  7.00mA
  // 4Mhz   =  4.50mA
  // 2MHz   =  3.00mA
  // 1MHz   =  2.30mA
  // 500KHz =  1.60mA
  /*
   *  .---------------------------------------------------------------------------------------.
   *  | 16MHz MCU | sketch baud | real baud | MCU clock | SLIMLORA_DRIFT (TrackermegabrickV1) |
   *  |-----------|-------------|-----------|-----------|-------------------------------------|
   *  | _div_2    | 115200      | 57600     | 8MHz      | 1                                   |
   *  | _div_2    |  38400      | 19200     | 8MHz      | 1                                   |
   *  | _div_4    |  38400      |  9600     | 4MHz      | 1                                   |
   *  | _div_8    |  38400      |  4800     | 2MHz      | 1 (50% success?)                    |
   *  | _div_16   | 115200      |  3600     | 1MHz      | 1                                   |
   *  | _div_16   |  38400      |  2400     | 1MHz      | 1                                   |
   *  | _div_32   | 115200      |  3600     | 500KHz    | 1                                   |
   *  | _div_32   |  38400      |  1200     | 500KHz    | This is so funny. Like Teletype.    |
   *  | _div_64   |  38400      |   600     | 250KHz    | This is hacker-style.               |
   *  | _div_128  |  38400      |   300     | 125KHz    | This is hacker-style #2             |
   *  \______________________________________________________________________________________/
   */
  // Feather: 3 is fine, others are not for serial comms.
  clockDivider = clock_prescale_get();  // 1 is _div_ 2, 2 is _div_4, 3 is _div_8, 4 is_div_16, 5 is _div_32
#endif

#ifdef LED_PRESENT
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

#if DEVICE < 3
  pinMode(VBATPIN, INPUT);
#endif

  delay(3000 >> clockDivider);

#if DEBUG_INO == 1
  Serial.begin(38400);
  while (! Serial);                 // don't start unless we have serial connection
  Serial.println(F("Starting"));
  Serial.print(F("\nRFM_CS_PIN: "));Serial.print(RFM_CS_PIN);
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
  lora.SetAdrEnabled(1); // 0 to disable. Network can still send ADR command to device. This is a preference, not an order.

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
      delay(3000 >> clockDivider);
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

      // Uncomment to ask for time, demodulation margin and gateway count
      // lora.TimeLinkCheck = 3; // 1 for only time, 2 for margin and gateway count. 3 for both.

      // with lora.epoch you have access to GPS epoch from LNS.
      // lora.fracSeconds holds the value of seconds in ms.
      // lora.margin holds the demodulation margin (0-20)
      // lora.GwCnt holds the number of Gateways received the message.

} // setup()
