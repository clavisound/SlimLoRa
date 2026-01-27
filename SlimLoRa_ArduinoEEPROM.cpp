#if ARDUINO_EEPROM >= 1
// TxFrameCounter
uint32_t SlimLoRa::GetTxFrameCounter() {
	uint32_t value;
	EEPROM.get(EEPROM_TX_COUNTER, value );
	if (value == 0xFFFFFFFF) { // This check might need adjustment if 0xFFFFFFFF is a valid frame counter
		return 0;
	}
	return value;
}

void SlimLoRa::SetTxFrameCounter() {
	// BUG: Why it does not work?
	//EEPROM.update(EEPROM_TX_COUNTER, tx_frame_counter_);
#if ARDUINO_EEPROM == 2
	EEPROM.putChanged(EEPROM_TX_COUNTER, tx_frame_counter_);
#else
	EEPROM.put(EEPROM_TX_COUNTER, tx_frame_counter_);
#endif
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE Tx#: "));Serial.print(tx_frame_counter_);
#endif
}

// RxFrameCounter
uint32_t SlimLoRa::GetRxFrameCounter() {
	uint32_t value;
	EEPROM.get(EEPROM_RX_COUNTER, value);
	if (value == 0xFFFFFFFF) {
		return 0;
	}
	return value;
}

void SlimLoRa::SetRxFrameCounter(){
#if ARDUINO_EEPROM == 2
	EEPROM.putChanged(EEPROM_RX_COUNTER, rx_frame_counter_);
#else
	EEPROM.put(EEPROM_RX_COUNTER, rx_frame_counter_);
#endif
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE Rx#: "));Serial.print(rx_frame_counter_);
#endif
}

// Rx1DataRateOffset
uint8_t SlimLoRa::GetRx1DataRateOffset() {
	uint8_t value;
	value = EEPROM.read(EEPROM_RX1DR_OFFSET) & 0x7F;	// Get 7 bytes [0-6] strip last bit. Shared byte with EEPROM_JOINED
	if (value > 0x3F ) {					// since we stripped last bit full value 0xFF is 0x3F: bits: 00111111
		return 0;
	}
	return value;
}

void SlimLoRa::SetRx1DataRateOffset(uint8_t value) {
	uint8_t tmp_joined;
	tmp_joined = EEPROM.read(EEPROM_JOINED) << 7;	// Get only [7] bit
	value |= tmp_joined;				// shared byte with EEPROM_joined. Merge them.
	EEPROM.write(EEPROM_RX1DR_OFFSET, value);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE Rx1_offset: "));Serial.print(value &= 0x0F);
	Serial.print(F("\nWRITE Rx1_offset RAW: "));Serial.print(value);
#endif
}

// Rx2DataRate
uint8_t SlimLoRa::GetRx2DataRate() {
	uint8_t value;
	value = EEPROM.read(EEPROM_RX2_DR) & 0x0F;	// Get only [0-3] 4 bits. Shared byte with EEPROM_RX_DELAY
	if (value == 0x0F) {	// probably erased EEPROM.
#if LORAWAN_OTAA_ENABLED
		return SF12BW125;	// default LORAWAN 1.0.3
#if NETWORK == NET_TTN		// TTN
		return SF9BW125;
#endif
#if NETWORK == NET_HLM		// Helium
		return SF12BW125;
#endif
#else	// ABP settings
		return SF12BW125;	// default LORAWAN 1.0.3
#if NETWORK == NET_TTN		// TTN
		return SF9BW125;
#endif
#if NETWORK == NET_HLM		// Helium
		return SF12BW125;
#endif
#endif // LORAWAN_OTAA_ENABLED
	}
	return value;
}

void SlimLoRa::SetRx2DataRate(uint8_t value) {
	uint8_t tmp_rx_delay;
	tmp_rx_delay = EEPROM.read(EEPROM_RX_DELAY) & 0xF0;	// get only [7-4] bits.
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE Rx2_DR: "));Serial.print(value);
#endif
	value |= tmp_rx_delay;			// shared byte with EEPROM_RX_DELAY
	EEPROM.update(EEPROM_RX2_DR, value);
}

// Rx1Delay
uint8_t SlimLoRa::GetRx1Delay() {
	uint8_t value;
	value = EEPROM.read(EEPROM_RX_DELAY) >> 4;	// shared byte with EEPROM_RX2_DATARATE
	if ( value == 0 || value >= 0xF ) {		// probably erased EEPROM
#if NETWORK == NET_TTN
		value = NET_TTN_RX_DELAY;		// default for TTN
#endif

#if NETWORK == NET_HELIUM
		value = NET_HELIUM_RX_DELAY;		// default for Helium
#endif
	}
	return value;
}

void SlimLoRa::SetRx1Delay(uint8_t value) {
	uint8_t temp_rx2_dr;
	temp_rx2_dr = EEPROM.read(EEPROM_RX2_DR) & 0x0F;			// Get only the [0-3] bits
	value = (value << 4) | temp_rx2_dr;					// shared byte with EEPROM_RX2_DATARATE
	EEPROM.update(EEPROM_RX_DELAY, value);
#if DEBUG_SLIM >= 1
	// EVAL something wrong here? RAW value is 7 (binary 111) vs 0101 0011 (83 or 0x53)
	Serial.print(F("\nWRITE Rx1_delay: "));Serial.print(value >> 4);
	Serial.print(F("\nWRITE Rx1_delay RAW: "));Serial.print(value);
#endif
}

// ChMask
void SlimLoRa::GetChMask() {
	EEPROM.get(EEPROM_CHMASK, ChMask);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nGetChMask: "));Serial.print(ChMask);
#endif
}

void SlimLoRa::SetChMask() {
	EEPROM.put(EEPROM_CHMASK, ChMask); // BUG: EEPROM.update does not work?
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE ChMask\t: "));Serial.print(ChMask);
	GetChMask();
#endif
}

// NbTrans
void SlimLoRa::GetNbTrans() {
	if ( EEPROM.read(EEPROM_NBTRANS) == 0xFF ) {
		NbTrans = NBTRANS;	// EEPROM erased, default NbTrans
	} else {
		NbTrans = EEPROM.read(EEPROM_NBTRANS) & 0x0F;		// EEPROM is not erased. Get the LSB bits
	}
#if DEBUG_SLIM >= 1
	Serial.print(F("\nGet NbTrans: "));Serial.print(NbTrans);
#endif
}

void SlimLoRa::SetNbTrans() {
#if DEBUG_SLIM >= 1
	GetChMask();
#endif
	uint8_t temp_none;
	temp_none = (EEPROM.read(EEPROM_NBTRANS) & 0xF0) | NbTrans;		// Get the MSB bits
	//EEPROM.update(EEPROM_NBTRANS, temp_none);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE temp_none\t: "));Serial.print(temp_none >> 4);
	Serial.print(F("\nWRITE NbTrans\t: "));Serial.print(NbTrans);
	Serial.print(F("\nWRITE RAW\t: "));Serial.print( temp_none | NbTrans );
	GetChMask();
#endif
}
#endif // ARDUINO_EEPROM >= 1
       // ARDUINO style EEPROM. I had problems with avr/eeprom.h with debugging.
       // When added Serial.print commands to either sketch or library the avr/eeprom.h
       // for unknown to me reason changes the locations of EEMEM variables.
       // Sooooooo... Static addressing. :-(
       //
       // In fact it seems that it's bug on linker
       // https://arduino.stackexchange.com/questions/93873/how-eemem-maps-the-variables-avr-eeprom-h
#if LORAWAN_OTAA_ENABLED && ARDUINO_EEPROM >= 1
#if LORAWAN_KEEP_SESSION
uint8_t eeprom_lw_has_joined 	= 0;
#endif // LORAWAN_KEEP_SESSION
uint8_t eeprom_lw_dev_addr[4];
uint16_t eeprom_lw_dev_nonce	= 1;
uint32_t eeprom_lw_join_nonce	= 0;
uint8_t eeprom_lw_app_s_key[16];
uint8_t eeprom_lw_f_nwk_s_int_key[16];
uint8_t eeprom_lw_s_nwk_s_int_key[16];
uint8_t eeprom_lw_nwk_s_enc_key[16];

#if LORAWAN_KEEP_SESSION
bool SlimLoRa::GetHasJoined() {
	uint8_t value = EEPROM.read(EEPROM_JOINED);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nEEPROM join value (shared with DR1_OFFSET): "));Serial.println(value);Serial.print(F("addr : 0x"));Serial.print(EEPROM_JOINED, HEX);
#endif
	if ( value == 0xFF ) { 		// Erased EEPROM
		return 0;
	}
	value = value >> 7;		// Same address with DR1_OFFSET. Take the last bit.
#if DEBUG_SLIM >= 1
	Serial.print(F("\nEEPROM join value: "));Serial.println(value);
#endif
	return value;
}

void SlimLoRa::SetHasJoined(bool value) {
	uint8_t temp;
	EEPROM.get(EEPROM_JOINED, temp);		// Same address with DR1_OFFSET. Keep the first bits.
	if ( value == true ) {
		temp = ( ( value << 7 ) | temp );
	} else {
		temp &= 0x7F;
	}
	EEPROM.write(EEPROM_JOINED, temp);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE EEPROM: joined: ")); Serial.print(value);Serial.print(F(", RAW value: "));Serial.print(temp, BIN);
#endif
}
#endif // LORAWAN_KEEP_SESSION

// DevAddr
void SlimLoRa::GetDevAddr(uint8_t *dev_addr) {
	getArrayEEPROM(EEPROM_DEVADDR, dev_addr, 4);
}

void SlimLoRa::SetDevAddr(uint8_t *dev_addr) {
	setArrayEEPROM(EEPROM_DEVADDR, dev_addr, 4);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE DevAddr: "));printHex(dev_addr, 4);
#endif
}

// DevNonce
uint16_t SlimLoRa::GetDevNonce() {
	uint16_t value;
	EEPROM.get(EEPROM_DEVNONCE, value);

	if (value == 0xFFFF) {
		return 0;
	}
	return value;
}

void SlimLoRa::SetDevNonce(uint16_t dev_nonce) {
	EEPROM.put(EEPROM_DEVNONCE, dev_nonce);
#if DEBUG_SLIM >= 1
	uint8_t temp[2];
	temp[0] = dev_nonce >> 8;
	temp[1] = dev_nonce;
	Serial.print(F("\nWRITE DevNonce: "));printHex(temp, 2);
#endif
}

// JoinNonce
uint32_t SlimLoRa::GetJoinNonce() {
	uint32_t value;
	EEPROM.get(EEPROM_JOINNONCE, value);
	if (value == 0xFFFFFFFF) {
		return 0;
	}
	return value;
}

void SlimLoRa::SetJoinNonce(uint32_t join_nonce) {
	EEPROM.put(EEPROM_JOINNONCE, join_nonce);
#if DEBUG_SLIM >= 1
	uint8_t temp[4];
	temp[0] = join_nonce >> 24;
	temp[1] = join_nonce >> 16;
	temp[2] = join_nonce >> 8;
	temp[3] = join_nonce;
	Serial.print(F("\nWRITE JoinNonce: "));printHex(temp, 4);
#endif
}

// AppSKey
void SlimLoRa::GetAppSKey(uint8_t *key) {
	getArrayEEPROM(EEPROM_APPSKEY, key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("Read appSkey\t: "));printHex(key, 16);
#endif
}

void SlimLoRa::SetAppSKey(uint8_t *key) {
	setArrayEEPROM(EEPROM_APPSKEY, key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("WRITE appSkey\t: "));printHex(key, 16);
#endif
}

// FNwkSIntKey
void SlimLoRa::GetFNwkSIntKey(uint8_t *key) {
	getArrayEEPROM(EEPROM_FNWKSIKEY, key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("FNwkSInt\t: "));printHex(key, 16);
#endif
}

void SlimLoRa::SetFNwkSIntKey(uint8_t *key) {
	setArrayEEPROM(EEPROM_FNWKSIKEY, key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("WRITE FNwkSInt\t: "));printHex(key, 16);
#endif
}

// SNwkSIntKey
void SlimLoRa::GetSNwkSIntKey(uint8_t *key) {
	getArrayEEPROM(EEPROM_SNWKSIKEY, key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("SNwkSInt\t: "));printHex(key, 16);
#endif
}

void SlimLoRa::SetSNwkSIntKey(uint8_t *key) {
	setArrayEEPROM(EEPROM_SNWKSIKEY, key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("WRITE SNwkSInt\t: "));printHex(key, 16);
#endif
}

// NwkSEncKey
void SlimLoRa::GetNwkSEncKey(uint8_t *key) {
	getArrayEEPROM(EEPROM_NW_ENC_KEY, key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("NwkSEncKey\t: "));printHex(key, 16);
#endif
}

void SlimLoRa::SetNwkSEncKey(uint8_t *key) {
	setArrayEEPROM(EEPROM_NW_ENC_KEY, key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("WRITE NwkSEnc\t: "));printHex(key, 16);
#endif
}
#endif // LORAWAN_OTAA_ENABLED && ARDUINO_EEPROM >= 1
#endif // ARDUINO_EEPROM >= 1
