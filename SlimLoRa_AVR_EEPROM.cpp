#include "SlimLoRaTimers.h"

#if LORAWAN_OTAA_ENABLED && ARDUINO_EEPROM == 0
#if LORAWAN_KEEP_SESSION
uint8_t eeprom_lw_has_joined		EEMEM = 0;
#endif // LORAWAN_KEEP_SESSION
uint8_t eeprom_lw_dev_addr[4]		EEMEM;
uint16_t eeprom_lw_dev_nonce		EEMEM = 0;
uint32_t eeprom_lw_join_nonce		EEMEM = 0;
uint8_t eeprom_lw_app_s_key[16]		EEMEM;
uint8_t eeprom_lw_f_nwk_s_int_key[16]	EEMEM;
uint8_t eeprom_lw_s_nwk_s_int_key[16]	EEMEM;
uint8_t eeprom_lw_nwk_s_enc_key[16]	EEMEM;
uint32_t eeprom_lw_tx_frame_counter		EEMEM;
uint32_t eeprom_lw_rx_frame_counter		EEMEM;

#if ARDUINO_EEPROM == 0
// TxFrameCounter
uint32_t SlimLoRa::GetTxFrameCounter() {
	uint32_t value = eeprom_read_dword(&eeprom_lw_tx_frame_counter);

	if (value == 0xFFFFFFFF) {
		return 0;
	}

	return value;
}

void SlimLoRa::SetTxFrameCounter() {
	eeprom_write_dword(&eeprom_lw_tx_frame_counter, tx_frame_counter_);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE Tx#: "));Serial.print(tx_frame_counter_);
#endif
}

// RxFrameCounter
uint32_t SlimLoRa::GetRxFrameCounter() {
	uint32_t value = eeprom_read_dword(&eeprom_lw_rx_frame_counter);
	if (value == 0xFFFFFFFF) { return 0; }
	return value;
}

void SlimLoRa::SetRxFrameCounter() {
	eeprom_write_dword(&eeprom_lw_rx_frame_counter, rx_frame_counter_);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE Rx#: "));Serial.print(rx_frame_counter_);
#endif
}

// Rx1DataRateOffset
uint8_t SlimLoRa::GetRx1DataRateOffset() {
	uint8_t value = eeprom_read_byte(&eeprom_lw_rx1_data_rate_offset);

	if (value == 0xFF) {
		return 0;
	}

	return value;
}

void SlimLoRa::SetRx1DataRateOffset(uint8_t value) {
	eeprom_write_byte(&eeprom_lw_rx1_data_rate_offset, value);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE Rx1_offset: "));Serial.print(value);
#endif
}

// Rx2DataRate
uint8_t SlimLoRa::GetRx2DataRate() {
	uint8_t value = eeprom_read_byte(&eeprom_lw_rx2_data_rate);

	if (value == 0xFF) {
#if LORAWAN_OTAA_ENABLED
		// return SF12BW125;
		return RX_SECOND_WINDOW;
#else
		return RX_SECOND_WINDOW;
#endif // LORAWAN_OTAA_ENABLED
	}
	return value;
}

void SlimLoRa::SetRx2DataRate(uint8_t value) {
	eeprom_write_byte(&eeprom_lw_rx2_data_rate, value);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE Rx2_DR: "));Serial.print(value);
#endif
}

// Rx1Delay
uint8_t SlimLoRa::GetRx1Delay() {
	uint8_t value = eeprom_read_byte(&eeprom_lw_rx1_delay);

	switch (value) {
		case 0x00:
		case 0xFF:
			return 1;
	}

	return value;
}

void SlimLoRa::SetRx1Delay(uint8_t value) {
	eeprom_write_byte(&eeprom_lw_rx1_delay, value);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE Rx1_delay: "));Serial.print(value);
#endif
}

// ChMask
void SlimLoRa::RestoreChMask() {
	ChMask = eeprom_read_word(&eeprom_lw_ChMask);
}

void SlimLoRa::SetChMask() {
	eeprom_write_word(&eeprom_lw_ChMask, ChMask);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWrite ChMask: "));Serial.print(ChMask);
#endif
}

// NbTrans
void SlimLoRa::RestoreNbTrans() {
	NbTrans = eeprom_read_byte(&eeprom_lw_NbTrans) & 0x0F; // Grab only 0-3 bits
}

void SlimLoRa::SetNbTrans() {
	eeprom_write_byte(&eeprom_lw_NbTrans, NbTrans);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWrite NbTrans: "));Serial.print(NbTrans);
#endif
}

#endif // ARDUINO_EEPROM == 0
#if LORAWAN_KEEP_SESSION
bool SlimLoRa::GetHasJoined() {
	uint8_t value = eeprom_read_byte(&eeprom_lw_has_joined);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nEEPROM join value: "));Serial.println(value);
#endif
	if ( value != 0x01 ) { return 0; }
	return value;
}

void SlimLoRa::SetHasJoined(bool value) {
	eeprom_write_byte(&eeprom_lw_has_joined, value);
#if (DEBUG_SLIM & 0x08) == 0x08
	Serial.print(F("\nWRITE EEPROM: joined"));
	uint16_t temp = &eeprom_lw_has_joined;
#endif
}
#endif // LORAWAN_KEEP_SESSION

// DevAddr
void SlimLoRa::GetDevAddr(uint8_t *dev_addr) {
	eeprom_read_block(dev_addr, eeprom_lw_dev_addr, 4);
}

void SlimLoRa::SetDevAddr(uint8_t *dev_addr) {
	eeprom_write_block(dev_addr, eeprom_lw_dev_addr, 4);
#if (DEBUG_SLIM & 0x08) == 0x08
	Serial.print(F("\nWRITE DevAddr: "));printHex(dev_addr, 4);
#endif
}

// DevNonce
uint16_t SlimLoRa::GetDevNonce() {
	uint16_t value = eeprom_read_word(&eeprom_lw_dev_nonce);

	if (value == 0xFFFF) {
	}

	return value;
}

void SlimLoRa::SetDevNonce(uint16_t dev_nonce) {
	eeprom_write_word(&eeprom_lw_dev_nonce, dev_nonce);
#if DEBUG_SLIM >= 1
	uint8_t temp[2];
	temp[0] = dev_nonce >> 8;
	temp[1] = dev_nonce;
	Serial.print(F("\nWRITE DevNonce: "));printHex(temp, 2);
#endif
}

// JoinNonce
uint32_t SlimLoRa::GetJoinNonce() {
	uint32_t value = eeprom_read_dword(&eeprom_lw_join_nonce);

	if (value == 0xFFFFFFFF) {
		return 0;
	}

	return value;
}

void SlimLoRa::SetJoinNonce(uint32_t join_nonce) {
	eeprom_write_dword(&eeprom_lw_join_nonce, join_nonce);
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
	eeprom_read_block(key, eeprom_lw_app_s_key, 16);
}

void SlimLoRa::SetAppSKey(uint8_t *key) {
	eeprom_write_block(key, eeprom_lw_app_s_key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("WRITE app_skey\t: "));printHex(key, 16);
#endif
}

// FNwkSIntKey
void SlimLoRa::GetFNwkSIntKey(uint8_t *key) {
	eeprom_read_block(key, eeprom_lw_f_nwk_s_int_key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("FNwkSInt\t: "));printHex(key, 16);
#endif
}

void SlimLoRa::SetFNwkSIntKey(uint8_t *key) {
	eeprom_write_block(key, eeprom_lw_f_nwk_s_int_key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("WRITE FNwkSInt\t: "));printHex(key, 16);
#endif
}

// SNwkSIntKey
void SlimLoRa::GetSNwkSIntKey(uint8_t *key) {
	eeprom_read_block(key, eeprom_lw_s_nwk_s_int_key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("SNwkSInt\t: "));printHex(key, 16);
#endif
}

void SlimLoRa::SetSNwkSIntKey(uint8_t *key) {
	eeprom_write_block(key, eeprom_lw_s_nwk_s_int_key, 16);
#if DEBUG_SLIM >= 1
	printNOWEB();Serial.print(F("WRITE SNwkSInt\t: "));printHex(key, 16);
#endif
}

// NwkSEncKey
void SlimLoRa::GetNwkSEncKey(uint8_t *key) {
	eeprom_read_block(key, eeprom_lw_nwk_s_enc_key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("NwkSEncKey\t: "));printHex(key, 16);
#endif
}

void SlimLoRa::SetNwkSEncKey(uint8_t *key) {
	eeprom_write_block(key, eeprom_lw_nwk_s_enc_key, 16);
#if (DEBUG_SLIM & 0x08) == 0x08
	printNOWEB();Serial.print(F("WRITE NwkSEnc\t: "));printHex(key, 16);
#endif
}
#endif // LORAWAN_OTAA_ENABLED && ARDUINO_EEPROM == 0
