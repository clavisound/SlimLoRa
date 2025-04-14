/*
 * Copyright (c) 2021-2025 Michales Michaloudes
 * Copyright (c) 2018-2021 Hendrik Hagendorn
 * Copyright (c) 2015-2016 Ideetron B.V. - AES routines
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http:// www.gnu.org/licenses/>.
 */
#include <avr/pgmspace.h>

#if ARDUINO_EEPROM == 0
	#include <avr/eeprom.h>
#endif

#include <Arduino.h>
#include <SPI.h>

#include "SlimLoRa.h"

#if LORAWAN_OTAA_ENABLED
extern const uint8_t DevEUI[8];
extern const uint8_t JoinEUI[8];
#if LORAWAN_V1_1_ENABLED
extern const uint8_t NwkKey[16];
#endif // LORAWAN_V1_1_ENABLED
extern const uint8_t AppKey[16];
#else
extern const uint8_t NwkSKey[16];
extern const uint8_t AppSKey[16];
extern const uint8_t DevAddr[4];
#endif // LORAWAN_OTAA_ENABLED

#ifdef CATCH_DIVIDER
uint8_t clockShift;
#endif

#if DEBUG_RXSYMBOLS >= 1
	int32_t microsStart;
#endif

static SPISettings RFM_spisettings = SPISettings(4000000, MSBFIRST, SPI_MODE0);

#if ARDUINO_EEPROM == 0
/**
 * AVR style EEPROM variables
 * https://www.nongnu.org/avr-libc/user-manual/group__avr__eeprom.html
 */
uint16_t eeprom_lw_tx_frame_counter	EEMEM = 1;
uint16_t eeprom_lw_rx_frame_counter	EEMEM = 0;
uint8_t eeprom_lw_rx1_data_rate_offset	EEMEM = 0;
uint8_t eeprom_lw_rx2_data_rate		EEMEM = 0;
uint8_t eeprom_lw_rx1_delay		EEMEM = 0;
uint8_t eeprom_lw_NbTrans		EEMEM = 0;
uint16_t eeprom_lw_ChMask		EEMEM = 0;
uint8_t eeprom_lw_down_packet[64];
uint8_t eeprom_lw_down_port;
#endif

#ifdef EU863
// Frequency band for europe
// TODO remove PROGMEM code to enable CFlist and New Channel Req
// First 3 channels are enabled by default
// LoRaWAN spec. p. 24 of regional parameters 1.0.3
const uint8_t PROGMEM SlimLoRa::kFrequencyTable[9][3] = {
	{ 0xD9, 0x06, 0x8B }, // Channel 0 868.100 MHz / 61.035 Hz = 14222987 = 0xD9068B
	{ 0xD9, 0x13, 0x58 }, // Channel 1 868.300 MHz / 61.035 Hz = 14226264 = 0xD91358
	{ 0xD9, 0x20, 0x24 }, // Channel 2 868.500 MHz / 61.035 Hz = 14229540 = 0xD92024
	{ 0xD8, 0xC6, 0x8B }, // Channel 3 867.100 MHz / 61.035 Hz = 14206603 = 0xD8C68B
	{ 0xD8, 0xD3, 0x58 }, // Channel 4 867.300 MHz / 61.035 Hz = 14209880 = 0xD8D358
	{ 0xD8, 0xE0, 0x24 }, // Channel 5 867.500 MHz / 61.035 Hz = 14213156 = 0xD8E024
	{ 0xD8, 0xEC, 0xF1 }, // Channel 6 867.700 MHz / 61.035 Hz = 14216433 = 0xD8ECF1
	{ 0xD8, 0xF9, 0xBE }, // Channel 7 867.900 MHz / 61.035 Hz = 14219710 = 0xD8F9BE
	{ 0xD9, 0x61, 0xBE }  // Downlink  869.525 MHz / 61.035 Hz = 14246334 = 0xD961BE
};
#endif

#ifdef AU915 // According to Regional Parameters of LoRaWAN 1.0.3 spec page: 37 line 850 there is 64 channels starting from 915.200 MHz to 927.800 MHz with 200MHz steps.
const uint8_t PROGMEM SlimLoRa::kFrequencyTable[9][3] = {
    {0xE5, 0x33, 0x5A}, // Channel 0 916.800 MHz / 61.035 Hz = 15020890 = 0xE5335A
    {0xE5, 0x40, 0x26}, // Channel 1 917.000 MHz / 61.035 Hz = 15024166 = 0xE54026
    {0xE5, 0x4C, 0xF3}, // Channel 2 917.200 MHz / 61.035 Hz = 15027443 = 0xE54CF3
    {0xE5, 0x59, 0xC0}, // Channel 3 917.400 MHz / 61.035 Hz = 15030720 = 0xE559C0
    {0xE5, 0x66, 0x8D}, // Channel 4 917.600 MHz / 61.035 Hz = 15033997 = 0xE5668D
    {0xE5, 0x73, 0x5A}, // Channel 5 917.800 MHz / 61.035 Hz = 15037274 = 0xE5735A
    {0xE5, 0x80, 0x27}, // Channel 6 918.000 MHz / 61.035 Hz = 15040551 = 0xE58027
    {0xE5, 0x8C, 0xF3}, // Channel 7 918.200 MHz / 61.035 Hz = 15043827 = 0xE58CF3
    {0xE5, 0x8C, 0xF3}  // Downlink ??? MHz / 61.035 Hz = 15043827 = 0xE58CF3 // TODO 8 channels LoRa BW 500kHz, DR8 to DR13 starting at 923.300 MHz to 927.500 MHz, steps: 600KHz.
};
#endif


// BEELAN https://github.com/ElectronicCats/Beelan-LoRaWAN/blob/82da458bf8f98e8bf0e4c4eb5b7dd1b66787d2ba/src/arduino-rfm/RFM95.cpp#L38

#ifdef US902 // page 21 line 435: 64 chanels starting 902.300 MHz to 914.900 MHz steps: 200KHz. DR0 (SF10) to DR3 (SF7) only!
const uint8_t PROGMEM SlimLoRa::kFrequencyTable[9][3] = {
    {0xE1, 0xF9, 0xC0}, // Channel 0 903.900 MHz / 61.035 Hz = 14809536 = 0xE1F9C0
    {0xE2, 0x06, 0x8C}, // Channel 1 904.100 MHz / 61.035 Hz = 14812812 = 0xE2068C
    {0xE2, 0x13, 0x59}, // Channel 2 904.300 MHz / 61.035 Hz = 14816089 = 0xE21359
    {0xE2, 0x20, 0x26}, // Channel 3 904.500 MHz / 61.035 Hz = 14819366 = 0xE22026
    {0xE2, 0x2C, 0xF3}, // Channel 4 904.700 MHz / 61.035 Hz = 14822643 = 0xE22CF3
    {0xE2, 0x39, 0xC0}, // Channel 5 904.900 MHz / 61.035 Hz = 14825920 = 0xE239C0
    {0xE2, 0x46, 0x8C}, // Channel 6 905.100 MHz / 61.035 Hz = 14829196 = 0xE2468C
    {0xE2, 0x53, 0x59}, // Channel 7 905.300 MHz / 61.035 Hz = 14832473 = 0xE25359
    {0xE5, 0x8C, 0xF3}  // Downlink RX2 923.300 MHz / 61.035 Hz = 14832473 = 0xE25359 page 25 line 556
};
// TODO if more than 8 channels: RX1 page 25 line 554 'Downlink channel is modulo 8 upstream'. In other words: up 0 /down 0, up 7 /down 7, up 8 /down 0, up 15 /down 7.
// in bash words: for i in `seq 0 63`; do echo -n "up: $i RX1: "; expr $i % 8; done
#endif

#ifdef AS920
const uint8_t PROGMEM SlimLoRa::kFrequencyTable[9][3] = {
    {0xE6, 0xCC, 0xF4}, // Channel 0 868.100 MHz / 61.035 Hz = 15125748 = 0xE6CCF4
    {0xE6, 0xD9, 0xC0}, // Channel 1 868.300 MHz / 61.035 Hz = 15129024 = 0xE6D9C0
    {0xE6, 0x8C, 0xF3}, // Channel 2 868.500 MHz / 61.035 Hz = 15109363 = 0xE68CF3
    {0xE6, 0x99, 0xC0}, // Channel 3 867.100 MHz / 61.035 Hz = 15112640 = 0xE699C0
    {0xE6, 0xA6, 0x8D}, // Channel 4 867.300 MHz / 61.035 Hz = 15115917 = 0xE6A68D
    {0xE6, 0xB3, 0x5A}, // Channel 5 867.500 MHz / 61.035 Hz = 15119194 = 0xE6B35A
    {0xE6, 0xC0, 0x27}, // Channel 6 867.700 MHz / 61.035 Hz = 15122471 = 0xE6C027
    {0xE6, 0x80, 0x27}, // Channel 7 867.900 MHz / 61.035 Hz = 15106087 = 0xE68027
    {0xE5, 0x8C, 0xF3}  // Downlink ??? MHz / 61.035 Hz = 15043827 = 0xE58CF3 // TODO
};
#endif

// TODO for other regions.
// Data rate
const uint8_t PROGMEM SlimLoRa::kDataRateTable[7][3] = {
	// bw	sf   agc
	{ 0x72, 0xC4, 0x0C }, // SF12BW125
	{ 0x72, 0xB4, 0x0C }, // SF11BW125
	{ 0x72, 0xA4, 0x04 }, // SF10BW125
	{ 0x72, 0x94, 0x04 }, // SF9BW125
	{ 0x72, 0x84, 0x04 }, // SF8BW125
	{ 0x72, 0x74, 0x04 }, // SF7BW125
	{ 0x82, 0x74, 0x04 }  // SF7BW250
};

// Half symbol times
// TODO for other regions
const uint32_t PROGMEM SlimLoRa::kDRMicrosPerHalfSymbol[7] = {
	(((uint32_t)128 << 7) * MICROS_PER_SECOND + 500000) / 1000000, // SF12BW125 (uint32_t) to protect from buffer overflow
	(((uint32_t)128 << 6) * MICROS_PER_SECOND + 500000) / 1000000, // SF11BW125
	(((uint32_t)128 << 5) * MICROS_PER_SECOND + 500000) / 1000000, // SF10BW125
	((128 << 4) * MICROS_PER_SECOND + 500000) / 1000000, // SF9BW125
	((128 << 3) * MICROS_PER_SECOND + 500000) / 1000000, // SF8BW125
	((128 << 2) * MICROS_PER_SECOND + 500000) / 1000000, // SF7BW125
	((128 << 1) * MICROS_PER_SECOND + 500000) / 1000000  // SF7BW250
};

// S table for AES encryption
const uint8_t PROGMEM SlimLoRa::kSTable[16][16] = {
	{0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76},
	{0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0},
	{0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15},
	{0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75},
	{0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84},
	{0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF},
	{0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8},
	{0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2},
	{0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73},
	{0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB},
	{0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79},
	{0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08},
	{0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A},
	{0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E},
	{0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF},
	{0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16}
};

// TODO add pins for IRQ.
SlimLoRa::SlimLoRa(uint8_t pin_nss) {
	pin_nss_ = pin_nss;
}

#if COUNT_TX_DURATION == 1
// return the Duration of transmission in ms.
uint16_t SlimLoRa::GetTXms(){
	return slimTotalTXms;
}

// After RX done, calculate the TX ms duration here to not break RX timing windows.
// Also store the lastTXms duration (current transmission)
void SlimLoRa::CalculateTXms(){
	slimLastTXms   = slimEndTXtimestamp - slimStartTXtimestamp;
	slimTotalTXms += slimLastTXms;
}

// erase the duration of TXms
void SlimLoRa::ZeroTXms(){
	slimTotalTXms = 0;
	slimLastTXms = 0;
}
#endif // COUNT_TX_DURATION == 1

void SlimLoRa::printHex(uint8_t *value, uint8_t len){ 
  	Serial.print(F("MSB: 0x"));
	for (int8_t i = 0; i < len; i++ ) {
		if (value[i] == 0x0 ) { Serial.print(F("00")); continue; }
		if (value[i] <= 0xF ) { Serial.print(F("0")); Serial.print(value[i], HEX);continue; }
  		Serial.print(value[i], HEX);
		}
  	Serial.print(F(", LSB: 0x"));
	for (int8_t i = len - 1; i >= 0; i-- ) {
		if (value[i] == 0x0 ) { Serial.print(F("00")); continue; }
		if (value[i] <= 0xF ) { Serial.print(F("0")); Serial.print(value[i], HEX); continue; }
  		Serial.print(value[i], HEX);
		}
}

#if DEBUG_SLIM >= 1
void SlimLoRa::printDownlink(){
	Serial.print(F("\nPort Down\t: "));	Serial.print(downPort);
	Serial.print(F("\nfopts.length\t: "));	Serial.print(f_options_length);
	Serial.print(F("\nPacket Length\t: "));	Serial.print(packet_length);
	Serial.print(F("\nPayload Length\t: "));Serial.print(payload_length);
	Serial.print(F("\nSNR 8bit\t: "));	Serial.print(last_packet_snrB);
	Serial.print(F("\nSNR 8bit / 4\t: "));	Serial.print(last_packet_snr_);
	Serial.flush();
}

#if LORAWAN_OTAA_ENABLED == 0
void printDevAddr(){
  	Serial.print(F("\nMSB: 0x"));
	for (int8_t i = 0; i < 4; i++ ) {
		if (DevAddr[i] == 0x0 ) { Serial.print(F("00")); continue; }
		if (DevAddr[i] <= 0xF ) { Serial.print(F("0")); Serial.print(DevAddr[i], HEX);continue; }
  		Serial.print(DevAddr[i], HEX);
		}
  	Serial.print(F("\nLSB: 0x"));
	for (int8_t i = 4 - 1; i >= 0; i-- ) {
		if (DevAddr[i] == 0x0 ) { Serial.print(F("00")); continue; }
		if (DevAddr[i] <= 0xF ) { Serial.print(F("0")); Serial.print(DevAddr[i], HEX); continue; }
  		Serial.print(DevAddr[i], HEX);
		}
  	Serial.println();
}
#endif // LORAWA_OTAA_ENABLED == 0

// Mark data in Serial log that must be kept secret.
void printNOWEB(){
	Serial.print(F("\nNOWEB "));
}
#endif

#if ARDUINO_EEPROM == 1

/**
 * Function to write array to eeprom
 *
 * @param eepromAddr eeprom address.
 * @param arrayData Array to store
 * @param size size of array
 *
 */
void SlimLoRa::setArrayEEPROM(uint16_t eepromAddr, uint8_t *arrayData, uint8_t size) {
   for ( uint8_t i = 0; i < size; i++ ) {
    EEPROM.update(eepromAddr + i, arrayData[i]);
#if (DEBUG_SLIM & 0x08) == 0x08
    if ( i == 0 ) {
   Serial.print(F("WRITE EEPROM Address: 0x"));Serial.print(eepromAddr + i, HEX);Serial.print(F("->0x"));Serial.print(arrayData[i], HEX);
    } else {
   Serial.print(F(", "));Serial.print(eepromAddr + i, HEX);Serial.print(F("->0x"));Serial.print(arrayData[i], HEX);
    }
#endif
   }
#if (DEBUG_SLIM & 0x08) == 0x08
   Serial.print(F("WRITE: "));printHex(arrayData, size);
#endif
}

/**
 * Function to read array to eeprom
 *
 * @param eepromAddr eeprom address.
 * @param arrayData Array to read
 * @param size size of array
 *
 */
void SlimLoRa::getArrayEEPROM(uint16_t eepromAddr, uint8_t *arrayData, uint8_t size) {
   for ( uint8_t i = 0; i < size; i++ ) {
    arrayData[i] = EEPROM.read(eepromAddr + i);
#if DEBUG_SLIM >= 3
    if ( i == 0 ) {
   Serial.print(F("\nREAD EEPROM Address->Value: 0x"));Serial.print(eepromAddr + i, HEX);Serial.print(F("->0x"));Serial.print(arrayData[i], HEX);
    } else {
   Serial.print(F(", "));Serial.print(eepromAddr + i, HEX);Serial.print(F("->0x"));Serial.print(arrayData[i], HEX);
    }
#endif
   }
#if DEBUG_SLIM >= 3
   Serial.print(F("\nread- "));printHex(arrayData, size);
#endif
}
#endif // ARDUINO_EEPROM == 1

#if DEBUG_SLIM >= 1
void SlimLoRa::printMAC(){
#if LORAWAN_OTAA_ENABLED
	uint8_t dev_addr[4], app_s_key[16], nwk_s_key[16];
	Serial.print(F("\n\nEEPROM Addr: "));Serial.print(EEPROM_OFFSET);
	Serial.print(F("\nMAC Join: "));Serial.print(GetHasJoined());
	Serial.print(F("\ndevNonce DEC\t\t: "));;Serial.print(GetDevNonce() >> 8);Serial.print(GetDevNonce());
	Serial.print(F("\njoinDevNonce DEC\t: "));Serial.print(GetJoinNonce() >> 24);Serial.print(GetJoinNonce() >> 16);Serial.print(GetJoinNonce() >> 8);Serial.println(GetJoinNonce());
	GetDevAddr(dev_addr);Serial.print(F("\nDevAddr\t: "));printHex(dev_addr, 4);
	GetNwkSEncKey(nwk_s_key);
	//GetFNwkSIntKey(); // not used
	GetAppSKey(app_s_key);
#else
	Serial.print(F("\nABP DevAddr: "));printDevAddr();
#endif // LORAWAN_OTAA_ENABLED
	Serial.print(F("\nTx#\t\t: "));Serial.print(GetTxFrameCounter());Serial.print(F("\tRAM: "));Serial.println(tx_frame_counter_);
	Serial.print(F("Rx#\t\t: "));Serial.print(GetRxFrameCounter());Serial.print(F("\tRAM: "));Serial.println(rx_frame_counter_);
	Serial.print(F("Asked ack_\t: "));Serial.print(ack_);
	Serial.print(F("\nNbTrans_counter\t: "));Serial.print(NbTrans_counter);
	Serial.print(F("\nNbTrans\t\t: "));Serial.print(NbTrans);
	Serial.print(F("\nChMask\t\t: "));Serial.println(ChMask);
	Serial.print(F("\nRX1 delay\t: "));Serial.print(GetRx1Delay());Serial.print(F(", System Setting: "));Serial.print(LORAWAN_JOIN_ACCEPT_DELAY1_MICROS / 1000000);Serial.print(F("s, RX2: "));Serial.print(LORAWAN_JOIN_ACCEPT_DELAY2_MICROS / 1000000);Serial.println("s, ");
	Serial.print(F("Rx1 DR\t\t: "));Serial.println(data_rate_);
	Serial.print(F("Rx1 DR offset\t: "));Serial.println(GetRx1DataRateOffset());
	Serial.print(F("Rx2 DR RAM\t: "));Serial.println(rx2_data_rate_);
	Serial.print(F("Rx2 DR EEPROM\t: "));Serial.println(GetRx2DataRate());
	Serial.print(F("ADR_ACK_cnt\t: "));Serial.print(adr_ack_limit_counter_);
      	Serial.print(F("\nSlimLoRa drift\t: "));Serial.print(SLIMLORA_DRIFT);
      	Serial.print(F("\ndelay RX1\t: "));Serial.print(CalculateRxDelay(data_rate_, GetRx1Delay() * MICROS_PER_SECOND));
       	Serial.print(F("\trx_symbols_: "));Serial.print(rx_symbols_);
       	Serial.print(F("\trx_symbols_ MSB: "));Serial.print(RfmRead(RFM_REG_MODEM_CONFIG_2) & 0b11);
      	Serial.print(F("\ndelay RX2\t: "));Serial.print(CalculateRxDelay(rx2_data_rate_, GetRx1Delay() * MICROS_PER_SECOND + MICROS_PER_SECOND));
       	Serial.print(F("\trx_symbols_: "));Serial.print(rx_symbols_);
       	Serial.print(F("\trx_symbols_ MSB: "));Serial.print(RfmRead(RFM_REG_MODEM_CONFIG_2) & 0b11);
      	Serial.print(F("\ndelay Join RX1\t: "));Serial.print(CalculateRxDelay(data_rate_, LORAWAN_JOIN_ACCEPT_DELAY1_MICROS));
       	Serial.print(F("\trx_symbols_: "));Serial.print(rx_symbols_);
       	Serial.print(F("\trx_symbols_ MSB: "));Serial.print(RfmRead(RFM_REG_MODEM_CONFIG_2) & 0b11);
      	Serial.print(F("\ndelay Join RX2\t: "));Serial.print(CalculateRxDelay(rx2_data_rate_, LORAWAN_JOIN_ACCEPT_DELAY2_MICROS));
       	Serial.print(F("\trx_symbols_: "));Serial.print(rx_symbols_);
       	Serial.print(F("\trx_symbols_ MSB: "));Serial.println(RfmRead(RFM_REG_MODEM_CONFIG_2) & 0b11);
      	//Serial.print(F("\nLNA REG : "));Serial.println(RfmRead(RFM_REG_LNA));
      	//Serial.print(F("\nCONFIG_3: "));Serial.println(RfmRead(RFM_REG_MODEM_CONFIG_3));
}
#endif // DEBUG_SLIM

void SlimLoRa::Begin() {
	uint8_t detect_optimize;

	SPI.begin();

#ifdef CATCH_DIVIDER
	// for printMAC correct values.
	clockShift = clock_prescale_get();
#endif

	pinMode(pin_nss_, OUTPUT);

	// Sleep
	RfmWrite(RFM_REG_OP_MODE, 0x00);

	// LoRa mode
	RfmWrite(RFM_REG_OP_MODE, 0x80);

	// +16 dBm output power. Upper limit for EU868
	SetPower(16);

	// Preamble length: 8 symbols
	// 0x0008 + 4 = 12
	RfmWrite(RFM_REG_PREAMBLE_MSB, 0x00);
	RfmWrite(RFM_REG_PREAMBLE_LSB, 0x08);

	// LoRa sync word
	RfmWrite(RFM_REG_SYNC_WORD, 0x34);

	// EVAL
//	SetCurrentLimit(60);

	// Errata Note - 2.3 Receiver Spurious Reception
	detect_optimize = RfmRead(RFM_REG_DETECT_OPTIMIZE);
	RfmWrite(RFM_REG_DETECT_OPTIMIZE, (detect_optimize & 0x78) | 0x03);
	RfmWrite(RFM_REG_IF_FREQ_1, 0x00);
	RfmWrite(RFM_REG_IF_FREQ_2, 0x40);

	// FIFO pointers
	RfmWrite(RFM_REG_FIFO_TX_BASE_ADDR, 0x80);
	RfmWrite(RFM_REG_FIFO_RX_BASE_ADDR, 0x00);

	// Init MAC state
#if LORAWAN_KEEP_SESSION && LORAWAN_OTAA_ENABLED
	has_joined_	   = GetHasJoined();
	GetChMask();
	GetNbTrans();
#endif

	NbTrans_counter = NbTrans;

#if LORAWAN_KEEP_SESSION
	tx_frame_counter_ = GetTxFrameCounter();
	rx_frame_counter_ = GetRxFrameCounter();
	rx2_data_rate_	  = GetRx2DataRate();
	rx1_delay_micros_ = GetRx1Delay() * MICROS_PER_SECOND;
#endif

#if DEBUG_SLIM >= 1
	Serial.println(F("\nInit of RFM done."));
	printMAC();
#endif
}

// This function is only usefull to gain some mA during setup on application.
// It offers a useless jump in library. Don't used if you need some bytes of flash memory
void SlimLoRa::sleep() {
	RfmWrite(RFM_REG_OP_MODE, 0x00);
}

/* Copied from RadioLib project
      \brief Sets current limit for over current protection at transmitter amplifier. Allowed values range from 45 to 120 mA in 5 mA steps and 120 to 240 mA in 10 mA steps.
      \param currentLimit Current limit to be set (in mA).
*/

void SlimLoRa::setCurrentLimit(uint8_t currentLimit) {
  // check allowed range
  if(!(((currentLimit >= 45) && (currentLimit <= 240)) || (currentLimit == 0))) {
    currentLimit = 60; // default value. Taken from RadioLib
  }

  // Switch RFM to standby
  RfmWrite(RFM_REG_OP_MODE, 0x81);

  // set OCP limit
  uint8_t raw;
  if(currentLimit == 0) {
    // limit set to 0, disable OCP
  	RfmWrite(RFM_REG_OCP_TRIM, RFM_OCP_TRIM_OFF);
  } else if(currentLimit <= 120) {
    raw = (currentLimit - 45) / 5;
  	RfmWrite(RFM_REG_OCP_TRIM, RFM_OCP_TRIM_ON | raw);
  } else if(currentLimit <= 240) {
    raw = (currentLimit + 30) / 10;
  	RfmWrite(RFM_REG_OCP_TRIM, RFM_OCP_TRIM_ON | raw);
  }
}

void SlimLoRa::wait_until(unsigned long microsstamp) {
	long delta;
	
	// this probably breaks timing for RX2 window
#if DEBUG_SLIM >= 2
	Serial.print(F("\nwait_until: "));Serial.print(microsstamp);
	Serial.print(F("\nmicros now: "));Serial.println(micros());
       	Serial.print(F("\nrx_symbols_\t: "));Serial.print(rx_symbols_);
       	Serial.print(F("\n... MSB\t\t: "));Serial.println(RfmRead(RFM_REG_MODEM_CONFIG_2) & 0b11);
#endif

#if DEBUG_RXSYMBOLS >= 1
	microsStart = micros();
#endif

	while (1) {
		// overflow fail safe logic used. Described here: https://arduino.stackexchange.com/a/12588/59046
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			delta = microsstamp - micros(); // overflow after 70 minutes
		}
		if (delta <= 0) {
			break;
		}
	}
}

void SlimLoRa::SetDataRate(uint8_t dr) {
	data_rate_ = dr;
}

uint8_t SlimLoRa::GetDataRate() {
	return data_rate_;
}

/**************************************************************************/
/*! 
	@brief Sets the TX power
	@param power How much TX power in dBm
*/
/**************************************************************************/
// Valid values in dBm are: 80, +1 to +17 and +20.
//
// 18-19dBm are undefined in doc but maybe possible. Here are ignored.
// Chip works with three modes. This function offer granularity of 1dBm
// but the chip is capable of more.
//
// -4.2 to 0 is in reality -84 to -80dBm

void SlimLoRa::SetPower(uint8_t power) {

  // values to be packed in one byte
  bool PaBoost;		// TODO: merge those to one byte
  int8_t OutputPower;	// 0-15
  int8_t MaxPower;	// 0-7

  // this value goes to the register (packed bytes)
  uint8_t DataPower;

  // 1st possibility -80
  if ( power == 80 ) { // force -80dBm (lower power)
  	RfmWrite(RFM_REG_PA_CONFIG, 0);

	#ifdef EU863
		tx_power = 7; // fake -14dBm report.
	#endif

  	return;
  // 2nd possibility: range 1 to 17dBm 
  } else if ( power >= 0 && power < 2 ) { // assume 1 db is given.
	PaBoost = 1;
	MaxPower = 7;
	OutputPower = 1;
	// Store LoRaWAN style tx_power
	tx_power = 7;
  } else if ( power >= 2 && power <=17 ) {
	PaBoost = 1;
	MaxPower = 7;
	// formula to find the OutputPower.
	OutputPower = power - 2;

	// Store LoRaWAN style tx_power to be on-par with the protocol.
	if ( power > 14 ) { 
		tx_power = 0;
	} else {
		// EU868, AS923, KR920, RU864 14dB attenuation.
		// TODO: #ifdef (EU863 | AS923 ...)
		#ifdef EU863
			tx_power = ((LORAWAN_EU868_TX_POWER_MAX * 2) - power) / 2;
		#endif

		// IN865 20dB attenuation
		#ifdef IN865
			tx_power = ((LORAWAN_IN865_TX_POWER_MAX * 2) - power) / 2;
		#endif
		
		#ifdef US920
		// US902 28dB attenuation
			tx_power = ((LORAWAN_US902_TX_POWER_MAX * 2) - power) / 2;
		#endif
	}
  }

  // 3rd possibility. 20dBm. Special case
  // Max Antenna VSWR 3:1, Duty Cycle <1% or destroyed(?) chip
  if ( power == 20 ) {
	PaBoost = 1;
	OutputPower = 15;
	MaxPower = 7;
	RfmWrite(RFM_REG_PA_DAC, 0x87); // only for +20dBm probably with 0x86,0x85 = 19,18dBm
  } else {
	// Setting for non +20dBm power
	RfmWrite(RFM_REG_PA_DAC, 0x84);
  }

  // Pack the above data to one byte and send it to HOPE RFM9x
  DataPower = (PaBoost << 7) + (MaxPower << 4) + OutputPower;
  	
  //PA pin. Default value is 0x4F (DEC 79, 3dBm) from HOPE, 0xFF (DEC 255 / 17dBm) from adafruit.
  RfmWrite(RFM_REG_PA_CONFIG,DataPower);

#if DEBUG_SLIM >= 1
  Serial.print(F("\nPower (dBm): "));Serial.println(power);
#endif // DEBUG_SLIM
}

/**
 * Function for receiving a packet using the RFM
 *
 * @param packet Pointer to RX packet array.
 * @param packet_max_length Maximum number of bytes to read from RX packet. project
 * @param channel The frequency table channel index.
 * @param dri The data rate table index.
 * @param rx_microsstamp Listen until rx_microsstamp elapsed.
 * @return The packet length or an error code.
 */
int8_t SlimLoRa::RfmReceivePacket(uint8_t *packet, uint8_t packet_max_length, uint8_t channel, uint8_t dri, uint32_t rx_microsstamp) {
	uint8_t modem_config_3, irq_flags, packet_length, read_length;

#if DEBUG_SLIM >= 2
	if ( channel == 8 ) Serial.println(F("\n\nRX2\n"));
#endif

	// Wait for start time
	wait_until(rx_microsstamp - LORAWAN_RX_SETUP_MICROS);

	// Switch RFM to standby
	RfmWrite(RFM_REG_OP_MODE, 0x81);

	// Invert IQ
	RfmWrite(RFM_REG_INVERT_IQ, 0x66);
	RfmWrite(RFM_REG_INVERT_IQ_2, 0x19);

	// Set SPI pointer to start of Rx part in FiFo
	RfmWrite(RFM_REG_FIFO_ADDR_PTR, 0x00);

	// Channel
	RfmWrite(RFM_REG_FR_MSB, pgm_read_byte(&(kFrequencyTable[channel][0])));
	RfmWrite(RFM_REG_FR_MID, pgm_read_byte(&(kFrequencyTable[channel][1])));
	RfmWrite(RFM_REG_FR_LSB, pgm_read_byte(&(kFrequencyTable[channel][2])));

	// Bandwidth / Coding Rate / Implicit Header Mode
	RfmWrite(RFM_REG_MODEM_CONFIG_1, pgm_read_byte(&(kDataRateTable[dri][0])));

	// Spreading Factor / Tx Continuous Mode / Crc
	RfmWrite(RFM_REG_MODEM_CONFIG_2, pgm_read_byte(&(kDataRateTable[dri][1])));

	// Automatic Gain Control / Low Data Rate Optimize
	modem_config_3 = pgm_read_byte(&(kDataRateTable[dri][2]));
	if (dri == SF12BW125 || dri == SF11BW125) {
		modem_config_3 |= 0x08;
	}

#ifdef REDUCE_LNA
	// disable AgcAutoOn
	modem_config_3 &= 0b11111011; // clear [2] bit to disable AgcAutoOn p. 109
	RfmWrite(RFM_REG_MODEM_CONFIG_3, modem_config_3);

	// 0b1 is maximum, b110 (6) is minimum. So 6 = SF7, 5 = SF8, 4 = SF9, 3 = SF10, 2 = SF11, 1 = SF12
	modem_config_3 = data_rate_ + 1;
	if ( modem_config_3 > 6 ) modem_config_3 = 6; // handle overflow
	modem_config_3 <<= 5;		// Shift to 7-5 bits
	RfmWrite(RFM_REG_LNA, modem_config_3);
#endif

	// [DISABLED by CLV] Automatic Gain Control / Low Data Rate Optimize
	RfmWrite(RFM_REG_MODEM_CONFIG_3, modem_config_3);

	// Rx timeout
	modem_config_3 = RfmRead(RFM_REG_MODEM_CONFIG_2) & 0b11111100; // EVAL: added 10+ instructions (delay)
	RfmWrite(RFM_REG_MODEM_CONFIG_2, modem_config_3 | rx_symbols_ >> 8); // MSB [0-1]
	RfmWrite(RFM_REG_SYMB_TIMEOUT_LSB, rx_symbols_); // LSB 8-bits

	// Clear interrupts
	RfmWrite(RFM_REG_IRQ_FLAGS, 0xFF);

	// Wait for rx time
	wait_until(rx_microsstamp);

	// Switch RFM to Rx
	RfmWrite(RFM_REG_OP_MODE, 0x86);

	// Wait for RxDone or RxTimeout
	// TODO: switch to IRQ code to save some MCU cycles
	
	// Probably it breaks timing.
	#if DEBUG_SLIM >= 2
	Serial.print(F("\npre IRQ\t\t: "));Serial.print(micros());
	#endif

	do {
		irq_flags = RfmRead(RFM_REG_IRQ_FLAGS);
	} while (!(irq_flags & 0xC0));

	// Probably it breaks timing.
	#if DEBUG_SLIM >= 2
	Serial.print(F("\nafter timeout\t: "));Serial.println(micros());
	#endif

	packet_length = RfmRead(RFM_REG_RX_NB_BYTES);
	RfmWrite(RFM_REG_FIFO_ADDR_PTR, RfmRead(RFM_REG_FIFO_RX_CURRENT_ADDR));

	if (packet_max_length < packet_length) {
		read_length = packet_max_length;
	} else {
		read_length = packet_length;
	}
	for (uint8_t i = 0; i < read_length; i++) {
		packet[i] = RfmRead(RFM_REG_FIFO);
	}

	// SNR
#if DEBUG_SLIM >= 1 // temp DEBUG to check RadioLib vs novag method
	last_packet_snr_ = (int8_t) RfmRead(RFM_REG_PKT_SNR_VALUE);
	last_packet_snrB = last_packet_snr_;
	last_packet_snr_ /= 4;
#else
	last_packet_snr_ = (int8_t) RfmRead(RFM_REG_PKT_SNR_VALUE) / 4;
#endif

#if DEBUG_RXSYMBOLS >= 1 && DEBUG_SLIM > 0
	Serial.print(F("\n\n >>>RX duration (ms): "));Serial.println( (micros() - microsStart) / 1000 );
#endif

	// Clear interrupts
	RfmWrite(RFM_REG_IRQ_FLAGS, 0xFF);

	// Switch RFM to sleep
	RfmWrite(RFM_REG_OP_MODE, 0x00);

	switch (irq_flags & 0xC0) {
		case RFM_STATUS_RX_TIMEOUT:
			return RFM_ERROR_RX_TIMEOUT;
		case RFM_STATUS_RX_DONE_CRC_ERROR:
			return RFM_ERROR_CRC;
		case RFM_STATUS_RX_DONE:
			return packet_length;
	}

	return RFM_ERROR_UNKNOWN;
}

/**
 * Senda a packet using the RFM.
 *
 * @param packet Pointer to TX packet array.
 * @param packet_length Length of the TX packet.
 * @param channel The frequency table channel index.
 * @param dri The data rate table index.
 */
void SlimLoRa::RfmSendPacket(uint8_t *packet, uint8_t packet_length, uint8_t channel, uint8_t dri) {
	// TODO if dri is FSK re-init modem.

#ifdef CATCH_DIVIDER
	clockShift = clock_prescale_get();
	#if DEBUG_SLIM == 1
	Serial.print(F("\nclockShift: "));Serial.println(clockShift);
	#endif
#endif

	uint8_t modem_config_3;

	// Switch RFM to standby
	RfmWrite(RFM_REG_OP_MODE, 0x81);

	// Don't invert IQ
	RfmWrite(RFM_REG_INVERT_IQ, 0x27);
	RfmWrite(RFM_REG_INVERT_IQ_2, 0x1D);

	// Channel
	#if defined(EU_DR6)
	// SF7BW250 is only for channel 868.3 - second channel.
	if ( dri == SF7BW250 ) {
		channel_ = 1; // this is needed for downlink channel.
		
		// Only with that works. Compiler BUG?
		RfmWrite(RFM_REG_FR_MSB, pgm_read_byte(&(kFrequencyTable[1][0])));
		RfmWrite(RFM_REG_FR_MID, pgm_read_byte(&(kFrequencyTable[1][1])));
		RfmWrite(RFM_REG_FR_LSB, pgm_read_byte(&(kFrequencyTable[1][2])));
	} else {
	#endif
	RfmWrite(RFM_REG_FR_MSB, pgm_read_byte(&(kFrequencyTable[channel][0])));
	RfmWrite(RFM_REG_FR_MID, pgm_read_byte(&(kFrequencyTable[channel][1])));
	RfmWrite(RFM_REG_FR_LSB, pgm_read_byte(&(kFrequencyTable[channel][2])));
	#if defined(EU_DR6)
	}
	#endif

	// Bandwidth / Coding Rate / Implicit Header Mode
	RfmWrite(RFM_REG_MODEM_CONFIG_1, pgm_read_byte(&(kDataRateTable[dri][0])));

	// Spreading Factor / Tx Continuous Mode / Crc
	RfmWrite(RFM_REG_MODEM_CONFIG_2, pgm_read_byte(&(kDataRateTable[dri][1])));

	// Automatic Gain Control / Low Data Rate Optimize
	modem_config_3 = pgm_read_byte(&(kDataRateTable[dri][2]));
	if (dri == SF12BW125 || dri == SF11BW125) {
		modem_config_3 |= 0x08;
	}
	RfmWrite(RFM_REG_MODEM_CONFIG_3, modem_config_3);

	// Set payload length to the right length
	RfmWrite(RFM_REG_PAYLOAD_LENGTH, packet_length);

	// Set SPI pointer to start of Tx part in FiFo
	RfmWrite(RFM_REG_FIFO_ADDR_PTR, 0x80);

	// Write Payload to FiFo
	for (uint8_t i = 0; i < packet_length; i++) {
		RfmWrite(RFM_REG_FIFO, *packet);
		packet++;
	}
	
#if COUNT_TX_DURATION == 1
	// Timestamp before TX. Don't to anything else to protect timing.
	slimStartTXtimestamp = millis();
#endif

	// Switch RFM to Tx
	RfmWrite(RFM_REG_OP_MODE, 0x83);

	// Wait for TxDone in the RegIrqFlags register.
	//
	// START OF TinyLoRa
	// EVAL it's better with _irq to TxDone?
	//
	// Switch _irq to TxDone
	// RfmWrite(0x40, 0x40);
	//
	// white _irq to pull high
	// while (digitalRead(_irq) == LOW) { }
	// END OF TinyLoRa
	while ((RfmRead(RFM_REG_IRQ_FLAGS) & RFM_STATUS_TX_DONE) != RFM_STATUS_TX_DONE);

	// https://arduino.stackexchange.com/questions/77494/which-arduinos-support-atomic-block
	// disable interrupts.
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		tx_done_micros_ = micros();
	}

#if COUNT_TX_DURATION == 1
	// EVAL: maybe this is breaking timing.
	// Works with SF7 JOIN 		TTN GW 		in same room 	delay of 5s
	// Works with SF8 JOIN 		Helium GW	in outdoors 	delay of 5s
	// Works with SF7 downlinks 	Helium GW	in same room 	delay 2s - RX2 window
	slimEndTXtimestamp = millis();
#endif

	// Clear interrupt
	RfmWrite(RFM_REG_IRQ_FLAGS, 0xFF);

	// Switch RFM to sleep
	RfmWrite(RFM_REG_OP_MODE, 0x00);

	// BUG: this is not needed when Join. But Join calls this RfmSendPacket function
	// NbTrans controls the fCnt
	if ( NbTrans_counter > 0 ) {
		NbTrans_counter--;
	}

	// Increase tx_frame_counter if NbTrans_counter is 0 and reset NbTrans_counter
	// p. 19 l. 520
	// TODO fCnt don't increase if we have CONFIRMED_DATA_UP without ACK
	// semi TODO fCnt don't increase if we havee UNCONFIRMED_DATA_UP and NbTrans to do.
	if ( NbTrans_counter == 0 ) {
		tx_frame_counter_++;
		adr_ack_limit_counter_++;
		if ( adr_ack_limit_counter_ >= LORAWAN_ADR_ACK_LIMIT ) {
			adr_ack_delay_counter_++;
		}
		NbTrans_counter = NbTrans;
	}

#if LORAWAN_KEEP_SESSION
	// Saves memory cycles. We jump at worst case scenario EEPROM_WRITE_TX_COUNT fCnt.
	if (tx_frame_counter_ % EEPROM_WRITE_TX_COUNT == 0) {
		SetTxFrameCounter();
	}
#endif
}

/**
 * Writes a value to a register of the RFM.
 *
 * @param address Address of the register to be written.
 * @param data Data to be written.
 */
void SlimLoRa::RfmWrite(uint8_t address, uint8_t data) {
	SPI.beginTransaction(RFM_spisettings);

	// Set NSS pin Low to start communication
	digitalWrite(pin_nss_, LOW);

	// Send addres with MSB 1 to write
	SPI.transfer(address | 0x80);
	// Send Data
	SPI.transfer(data);

	// Set NSS pin High to end communication
	digitalWrite(pin_nss_, HIGH);

	SPI.endTransaction();
}

/**
 * Reads a value from a register of the RFM.
 *
 * @param address Address of the register to be read.
 * @return The value of the register.
 */
uint8_t SlimLoRa::RfmRead(uint8_t address) {
	uint8_t data;

	SPI.beginTransaction(RFM_spisettings);

	// Set NSS pin low to start SPI communication
	digitalWrite(pin_nss_, LOW);

	// Send Address
	SPI.transfer(address);
	// Receive
	data = SPI.transfer(0x00);

	// Set NSS high to end communication
	digitalWrite(pin_nss_, HIGH);

	SPI.endTransaction();

	// Return received data
	return data;
}

/**
 * Calculates the clock drift adjustment. Default: (+-5%).
 */
uint32_t SlimLoRa::CalculateDriftAdjustment(uint32_t delay, uint16_t micros_per_half_symbol) {
	// Clock drift
	uint32_t drift = delay * SLIMLORA_DRIFT / 100;
	delay -= drift;

	// 8bits symbols are overflowing with value of 493, DRIFT 5 at 5 seconds RX12 SF12
#if DEBUG_RXSYMBOLS >= 2
	rx_symbols_ = LORAWAN_RX_MIN_SYMBOLS + drift / micros_per_half_symbol;
	rx_symbolsB = LORAWAN_RX_MIN_SYMBOLS + drift / micros_per_half_symbol;
#else
	/* old novag code it creates excessive timeouts (8 seconds in SF12, 1 second in SF7)
	if ((uint32_t)(LORAWAN_RX_MAX_SYMBOLS - rx_symbols_) * micros_per_half_symbol < drift) {
		rx_symbols_ = LORAWAN_RX_MAX_SYMBOLS;
	} else {
		rx_symbols_ = LORAWAN_RX_MIN_SYMBOLS + drift / micros_per_half_symbol;
	}
	*/
	rx_symbols_ = LORAWAN_RX_MIN_SYMBOLS + drift / micros_per_half_symbol;
#endif

#if DEBUG_SLIM >= 2
	Serial.print(F("\ndelay / drift / micros_per_half_symbol:\t"));Serial.print(delay);Serial.print("\t");Serial.print(drift);Serial.print("\t");Serial.print(micros_per_half_symbol);
	Serial.print(F("\n\(LORAWAN_RX_MAX_SYMBOLS - rx_symbols_) * micros_per_half_symbol: "));Serial.print((uint32_t)(LORAWAN_RX_MAX_SYMBOLS - rx_symbols_) * (uint32_t)micros_per_half_symbol);
	Serial.print(F("\nrx_symbols_: "));Serial.println(rx_symbols_);
#if DEBUG_RXSYMBOLS > 1
	Serial.print(F("\n\(LORAWAN_RX_MAX_SYMBOLS - rx_symbolsB) * micros_per_half_symbol: "));Serial.print((uint32_t)(LORAWAN_RX_MAX_SYMBOLS - rx_symbolsB) * micros_per_half_symbol);
	Serial.print(F("\nrx_symbolsB: "));Serial.println(rx_symbolsB);
#endif
#endif

	return delay;
}

/**
 * Calculates the centered RX window offest.
 */
int32_t SlimLoRa::CalculateRxWindowOffset(int16_t micros_per_half_symbol) {
	const uint16_t micros_per_symbol = 2 * micros_per_half_symbol;

	// EVAL uint16_t
	uint8_t rx_symbols = ((2 * LORAWAN_RX_MIN_SYMBOLS - 8) * micros_per_symbol + 2 * LORAWAN_RX_ERROR_MICROS + micros_per_symbol - 1) / micros_per_symbol;
	if (rx_symbols < LORAWAN_RX_MIN_SYMBOLS) {
		rx_symbols = LORAWAN_RX_MIN_SYMBOLS;
	}
	rx_symbols_ = rx_symbols;

	return (8 - rx_symbols) * micros_per_half_symbol - LORAWAN_RX_MARGIN_MICROS;
}

/**
 * Calculates the RX delay for a given data rate. delay = seconds in μS
 *
 * @return The RX delay in micros.
 */
uint32_t SlimLoRa::CalculateRxDelay(uint8_t data_rate, uint32_t delay) {
	uint32_t micros_per_half_symbol;
	int32_t offset;

	micros_per_half_symbol = pgm_read_word(&(kDRMicrosPerHalfSymbol[data_rate]));
	offset = CalculateRxWindowOffset(micros_per_half_symbol);

#ifdef CATCH_DIVIDER
	return (CalculateDriftAdjustment(delay + offset, micros_per_half_symbol)) >> clockShift;
//	return CalculateDriftAdjustment(delay + offset, micros_per_half_symbol);
#else
	return CalculateDriftAdjustment(delay + offset, micros_per_half_symbol);
#endif
}

/**
 * Enables/disables the ADR mechanism.
 */
void SlimLoRa::SetAdrEnabled(bool enabled) {
	adr_enabled_ = enabled;
}

/**
 * Validates the calculated 4-byte MIC against the received 4-byte MIC.
 *
 * @param cmic Calculated 4-byte MIC.
 * @param rmic Received 4-byte MIC.
 */
bool SlimLoRa::CheckMic(uint8_t *cmic, uint8_t *rmic) {
	return cmic[0] == rmic[0] && cmic[1] == rmic[1]
			&& cmic[2] == rmic[2] && cmic[3] == rmic[3];
}

#if LORAWAN_OTAA_ENABLED
/**
 * Check if the device joined a LoRaWAN network.
 */
bool SlimLoRa::HasJoined() {
	return has_joined_;
}

/**
 * Constructs a LoRaWAN JoinRequest packet and sends it.
 *
 * @return 0 or an error code.
 */
// TODO: back-off according to 1.0.3:
// a. 36 secs for 1st hour, 36 secs for next 11 hours, 8.7 secs after that.
// b. random delay between re-tries.
// I think it's to aggressive, probably I will implement if for every minutes.
int8_t SlimLoRa::Join() {
	uint8_t packet[1 + LORAWAN_JOIN_REQUEST_SIZE + 4];
	uint8_t packet_length;

	uint16_t dev_nonce;
	uint8_t mic[4];

	// Set RX2 DR default
	rx2_data_rate_	  = RX_SECOND_WINDOW;

	packet[0] = LORAWAN_MTYPE_JOIN_REQUEST;

	packet[1] = JoinEUI[7];
	packet[2] = JoinEUI[6];
	packet[3] = JoinEUI[5];
	packet[4] = JoinEUI[4];
	packet[5] = JoinEUI[3];
	packet[6] = JoinEUI[2];
	packet[7] = JoinEUI[1];
	packet[8] = JoinEUI[0];

	packet[9] = DevEUI[7];
	packet[10] = DevEUI[6];
	packet[11] = DevEUI[5];
	packet[12] = DevEUI[4];
	packet[13] = DevEUI[3];
	packet[14] = DevEUI[2];
	packet[15] = DevEUI[1];
	packet[16] = DevEUI[0];

	dev_nonce = GetDevNonce();
	SetDevNonce(++dev_nonce);

	packet[17] = dev_nonce & 0xFF;
	packet[18] = dev_nonce++ >> 8;

	packet_length = 1 + LORAWAN_JOIN_REQUEST_SIZE;

#if LORAWAN_V1_1_ENABLED
	CalculateMic(NwkKey, packet, NULL, mic, packet_length);
#else
	CalculateMic(AppKey, packet, NULL, mic, packet_length);
#endif // LORAWAN_V1_1_ENABLED
	for (uint8_t i = 0; i < 4; i++) {
		packet[i + packet_length] = mic[i];
	}
	packet_length += 4;

	/*
	channel_ = pseudo_byte_ & 0b11; 		// Mask with first 4 channels [0-3].
	if ( channel_ == 0b11 ) { channel_ = 0b10; }	// But we can join only on 3 channels: 868.100 868.300 and 868.500
	*/
	
	defaultChannel();

#if DEBUG_SLIM >= 1
	Serial.print(F("\nMAC before join: "));printMAC();
#endif
	// Reset some counters if joining
	/*
	ChMask 	= 0xFFFF; SetChMask();
	NbTrans	= NBTRANS;SetNbTrans();
	// TODO RX delay e.t.c.
	*/

	// This is hack. NbTrans_counter is decreased in RfmSendPacket.
	NbTrans_counter++;

	RfmSendPacket(packet, packet_length, channel_, data_rate_);

	if (!ProcessJoinAccept(1)) {
		return 0;
	}

	return ProcessJoinAccept(2);
}

// Select one of the first 3 channels.
// TODO for other regions
void SlimLoRa::defaultChannel(){
	channel_ = pseudo_byte_ & 0b11; 		// Mask with first 4 channels [0-3].
	if ( channel_ == 0b11 ) { channel_ = 0b10; }	// If channel is [3] aka: fourth subtract one to select third channel.
}

/**
 * Processes LoRaWAN 1.0 JoinAccept message.
 *
 * @param packet Received JoinAccept packet bytes.
 * @param packet_length Length of the received packet.
 * @return True if MIC validation succeeded, else false.
 */
bool SlimLoRa::ProcessJoinAccept1_0(uint8_t *packet, uint8_t packet_length) {
	uint8_t buffer[16], mic[4];
	uint8_t packet_length_no_mic = packet_length - 4;
	uint16_t dev_nonce;

	CalculateMic(AppKey, packet, NULL, mic, packet_length_no_mic);

	if (!CheckMic(mic, packet + packet_length_no_mic)) {
		return false;
	}

	dev_nonce = GetDevNonce();

	// Derive AppSKey, FNwkSIntKey, SNwkSIntKey, NwkSEncKey
	for (uint8_t i = 1; i <= 2; i++) {
		memset(buffer, 0, 16);

		buffer[0] = i;

		// JoinNonce
		buffer[1] = packet[1];
		buffer[2] = packet[2];
		buffer[3] = packet[3];

		// NetID
		buffer[4] = packet[4];
		buffer[5] = packet[5];
		buffer[6] = packet[6];

		// DevNonce
		buffer[7] = dev_nonce & 0xFF;
		buffer[8] = dev_nonce >> 8;

		AesEncrypt(AppKey, buffer);

		if (i == 1) {
			SetFNwkSIntKey(buffer);
			SetSNwkSIntKey(buffer);
			SetNwkSEncKey(buffer);
		} else {
			SetAppSKey(buffer);
		}
	}

	return true;
}

#if LORAWAN_V1_1_ENABLED
/**
 * Processes a LoRaWAN 1.1 JoiNAccept message.
 *
 * @param packet Received JoinAccept packet bytes.
 * @param packet_length Length of the received packet.
 * @return True if MIC validation succeeded, else false.
 */
bool SlimLoRa::ProcessJoinAccept1_1(uint8_t *packet, uint8_t packet_length) {
	uint8_t buffer[40] = { 0 }, mic[4];
	uint8_t packet_length_no_mic = packet_length - 4;
	uint16_t dev_nonce;

	// JoinReqType | JoinEUI | DevNonce | MHDR | JoinNonce | NetID | DevAddr | DLSettings | RxDelay | CFList
	buffer[0] = 0xFF; // TODO: JoinReqType

	// JoinEUI
	buffer[1] = JoinEUI[7];
	buffer[2] = JoinEUI[6];
	buffer[3] = JoinEUI[5];
	buffer[4] = JoinEUI[4];
	buffer[5] = JoinEUI[3];
	buffer[6] = JoinEUI[2];
	buffer[7] = JoinEUI[1];
	buffer[8] = JoinEUI[0];

	// DevNonce
	buffer[9] = dev_nonce & 0xFF;
	buffer[10] = dev_nonce >> 8;

	// MHDR
	buffer[11] = packet[0];

	// JoinNonce
	buffer[12] = packet[1];
	buffer[13] = packet[2];
	buffer[14] = packet[3];

	// NetID
	buffer[15] = packet[4];
	buffer[16] = packet[5];
	buffer[17] = packet[6];

	// DevAddr
	buffer[18] = packet[7];
	buffer[19] = packet[8];
	buffer[20] = packet[9];
	buffer[21] = packet[10];

	// DLSettings
	buffer[22] = packet[11];

	// RxDelay
	buffer[23] = packet[12];

	if (packet_length > 17) {
		// CFList
		for (uint8_t i = 0; i < 16; i++) {
			buffer[24 + i] = packet[13 + i];
		}

		//CalculateMic(JSIntKey, buffer, NULL, mic, 40);
	} else {
		//CalculateMic(JSIntKey, buffer, NULL, mic, 24);
	}

	if (!CheckMic(mic, packet + packet_length_no_mic)) {
		return false;
	}

	dev_nonce = GetDevNonce();

	// Derive AppSKey, FNwkSIntKey, SNwkSIntKey and NwkSEncKey
	for (uint8_t i = 1; i <= 4; i++) {
		memset(buffer, 0, 16);

		buffer[0] = i;

		// JoinNonce
		buffer[1] = packet[1];
		buffer[2] = packet[2];
		buffer[3] = packet[3];

		// JoinEUI
		buffer[4] = JoinEUI[7];
		buffer[5] = JoinEUI[6];
		buffer[6] = JoinEUI[5];
		buffer[7] = JoinEUI[4];
		buffer[8] = JoinEUI[3];
		buffer[9] = JoinEUI[2];
		buffer[10] = JoinEUI[1];
		buffer[11] = JoinEUI[0];

		// DevNonce
		buffer[12] = dev_nonce & 0xFF;
		buffer[13] = dev_nonce >> 8;

		if (i == 2) {
			AesEncrypt(AppKey, buffer);
		} else {
			AesEncrypt(NwkKey, buffer);
		}

		switch (i) {
			case 1:
				SetFNwkSIntKey(buffer);
				break;
			case 2:
				SetAppSKey(buffer);
				break;
			case 3:
				SetSNwkSIntKey(buffer);
				break;
			case 4:
				SetNwkSEncKey(buffer);
				break;
		}
	}
	return true;
}
#endif // LORAWAN_V1_1_ENABLED

/**
 * Listens for and processes a LoRaWAN JoinAccept message.
 *
 * @param window Index of the receive window [1,2].
 * @return 0 if successful, else error code.
 */
int8_t SlimLoRa::ProcessJoinAccept(uint8_t window) {
	int8_t result;
	uint32_t rx_delay;

	uint8_t packet[1 + LORAWAN_JOIN_ACCEPT_MAX_SIZE + 4];
	int8_t packet_length;

	bool mic_valid = false;
	uint8_t dev_addr[4];
	uint32_t join_nonce;

	if (window == 1) {
		rx_delay = CalculateRxDelay(data_rate_, LORAWAN_JOIN_ACCEPT_DELAY1_MICROS);
		packet_length = RfmReceivePacket(packet, sizeof(packet), channel_, data_rate_, tx_done_micros_ + rx_delay);
	} else {
		rx_delay = CalculateRxDelay(rx2_data_rate_, LORAWAN_JOIN_ACCEPT_DELAY2_MICROS);
		#ifdef US920 // TODO DR8 = SF12BW500
		packet_length = RfmReceivePacket(packet, sizeof(packet), 8, rx2_data_rate_, tx_done_micros_ + rx_delay);
		#endif

		#ifdef EU863
		packet_length = RfmReceivePacket(packet, sizeof(packet), 8, rx2_data_rate_, tx_done_micros_ + rx_delay);
		#endif
	}

	if (packet_length <= 0) {
		result = LORAWAN_ERROR_NO_PACKET_RECEIVED;
		goto end;
	}

	if (packet_length > 1 + LORAWAN_JOIN_ACCEPT_MAX_SIZE + 4) {
		result = LORAWAN_ERROR_SIZE_EXCEEDED;
		goto end;
	}

	if (packet[0] != LORAWAN_MTYPE_JOIN_ACCEPT) {
		result = LORAWAN_ERROR_UNEXPECTED_MTYPE;
		goto end;
	}

#if LORAWAN_V1_1_ENABLED
	AesEncrypt(NwkKey, packet + 1);
#else
	AesEncrypt(AppKey, packet + 1);
#endif // LORAWAN_V1_1_ENABLED
	if (packet_length > 17) {
#if LORAWAN_V1_1_ENABLED
		AesEncrypt(NwkKey, packet + 17);
#else
		AesEncrypt(AppKey, packet + 17);
#endif // LORAWAN_V1_1_ENABLED
	}

	// Check JoinNonce validity
	join_nonce = packet[1] | packet[2] << 8 | (uint32_t) packet[3] << 16;
	if (GetJoinNonce() >= join_nonce) {
		result = LORAWAN_ERROR_INVALID_JOIN_NONCE;
		goto end;
	}

	// Check OptNeg flag
	if (packet[11] & 0x80) {
		// LoRaWAN1.1+
#if LORAWAN_V1_1_ENABLED
		mic_valid = ProcessJoinAccept1_1(packet, packet_length);
#endif // LORAWAN_V1_1_ENABLED
	} else {
		// LoRaWAN1.0
		mic_valid = ProcessJoinAccept1_0(packet, packet_length);
	}

	if (!mic_valid) {
		has_joined_ = false;
		result = LORAWAN_ERROR_INVALID_MIC;
		goto end;
	}

	SetJoinNonce(join_nonce);

	dev_addr[0] = packet[10];
	dev_addr[1] = packet[9];
	dev_addr[2] = packet[8];
	dev_addr[3] = packet[7];
	SetDevAddr(dev_addr);

	// Get DLSettings (p. 35)
	rx1_data_rate_offset_ = (packet[11] & 0x70) >> 4;
	SetRx1DataRateOffset(rx1_data_rate_offset_);

	rx2_data_rate_ = packet[11] & 0xF;
	SetRx2DataRate(rx2_data_rate_);

	SetRx1Delay(packet[12] & 0xF);
	rx1_delay_micros_ = GetRx1Delay() * MICROS_PER_SECOND;

	// TODO optional CFlist
	// if packet[13] exists. aka: check packet_length
	// regional parameters p. 27

	// reset counters
	tx_frame_counter_ = 1;
	SetTxFrameCounter();

	rx_frame_counter_ = 0;
	SetRxFrameCounter();

	// reset counters. EVAL: I use this twice. Maybe better a function?
	adr_ack_limit_counter_ = 0;
	adr_ack_delay_counter_ = 0;
	NbTrans_counter = NbTrans;

	has_joined_ = true;
#if LORAWAN_KEEP_SESSION
	SetHasJoined(true);
#endif // LORAWAN_KEEP_SESSION

	result = 0;

end:
	#ifdef SLIM_DEBUG_VARS
	if ( result == 0 ) {
		if ( window == 1 ) {
		LoRaWANreceived |= 0x01; // Join Accept Window 1
		} else  {
		LoRaWANreceived |= 0x02; // Join Accept Window 2
		}
	}
	#endif
#if DEBUG_SLIM >= 1
	if ( result == 0 ) {
	Serial.print(F("\nPacket Length: "));Serial.println(packet_length);
	Serial.print(F("\nJoined on window: "));Serial.println(window);
	printMAC();
	}
#endif

#if COUNT_TX_DURATION == 1
	CalculateTXms();
#endif
	return result;
}
#endif // LORAWAN_OTAA_ENABLED

/**
 * Processes frame options of downlink packets.
 *
 * @param options Pointer to the start of the frame options section.
 * @param f_options_length Length of the frame options section.
 */
void SlimLoRa::ProcessFrameOptions(uint8_t *options, uint8_t f_options_length) {
	uint8_t status, new_rx1_dr_offset, new_rx2_dr; // new_rx2_dr is also used as a temp value
	uint8_t freqMSB, freqMID, freqLSB;
	uint8_t freqNewMSB, freqNewMID, freqNewLSB;

#ifdef SLIM_DEBUG_VARS
	LoRaWANreceived |= 0x04; // MAC command
#endif

	for (uint8_t i = 0; i < f_options_length; i++) {

	#if DEBUG_SLIM >= 1
	Serial.print(F("\nProcessing MAC\t: "));Serial.print(options[i]);
	Serial.print(F("\tf_options_length: "));Serial.print(f_options_length);
	Serial.print(F("\nlast_packet_snr_ 8bit / 4: "));Serial.print(last_packet_snr_);
	#endif

		switch (options[i]) {
			case LORAWAN_FOPT_LINK_CHECK_ANS:
				#ifdef MAC_REQUESTS
				// p. 23
				margin = options[i + 1]; // 0-20, 255 is reserved
				GwCnt  = options[i + 2];

				#if DEBUG_SLIM >= 1
				Serial.print(F("\nmargin/GwCnt: "));Serial.print(margin);Serial.print(F("/"));Serial.println(GwCnt);
				#endif
				
				#ifdef SLIM_DEBUG_VARS	
				LoRaWANreceived |= 0x20; // LinkCheck Received
				#endif
				#endif

				i += LORAWAN_FOPT_LINK_CHECK_ANS_SIZE;
				break;
			case LORAWAN_FOPT_LINK_ADR_REQ:
				// ChMask p. 24 and NbTrans. RP p. 18

				// check ChMaskCntrl
				// read Redundancy byte, grab only ChMaskCntl
				new_rx2_dr = ( options[i + 4] >> 4 ) & 0b111;

				#if DEBUG_SLIM >= 1
				Serial.print(F("\nChMaskCntl\t: "));Serial.print(new_rx2_dr);
				#endif

				// enable all channels
				if ( new_rx2_dr == 6 ) { 
					// TODO for other regions.
					#ifdef EU863
					ChMask = 0xFF;
					#endif

					status = 0x1; // enable ChMask ACK
				}


				// Apply ChMask
				// TODO US: Apply ChMask according to ChMaskCntl. p. 33 Regional parameters
				if ( new_rx2_dr == 0 ) { 
					ChMask = (uint16_t) options[i + 2] | options[i + 3] << 8;

					// Store to EEPROM
					SetChMask();

					#if DEBUG_SLIM >= 1
					Serial.print(F("\nChMask BIN\t: "));Serial.println(ChMask, BIN);
					#endif

					status = 0x1; // enable ChMask ACK
				}

				// report error
				if ( new_rx2_dr != 0 && new_rx2_dr != 6 ) {
					status = 0x0; // disable ChMask ACK
				}

				// Grab NbTrans from LNS
				NbTrans = options[i + 4] & 0xF;
				if ( NbTrans == 0 ) { NbTrans = NBTRANS; }	// Default NbTrans
				NbTrans_counter = NbTrans;			// Reset counter
				SetNbTrans();

				new_rx2_dr = options[i + 1] >> 4; 
				tx_power   = options[i + 1] & 0xF;

				#if defined(EU_DR6)
				if (new_rx2_dr == 0xF || (new_rx2_dr >= SF12BW125 && new_rx2_dr <= SF7BW250)) { // Reversed table index
				#else
				if (new_rx2_dr == 0xF || (new_rx2_dr >= SF12BW125 && new_rx2_dr <= SF7BW125)) { // Reversed table index
				#endif
					status |= 0x2;	// DR ACK
				}
				// tx_power = 15 or less than 7
				// TODO: other regions.
				if (tx_power == 0xF || tx_power <= LORAWAN_EU868_TX_POWER_MAX) {
					status |= 0x4; // Power ACK
				}
				if (status == 0x7 || status == 0x6) {
					if (new_rx2_dr != 0xF) {
						data_rate_ = new_rx2_dr;
				
						#if DEBUG_SLIM >= 1
						Serial.print(F("\n\nReceived DR\t:"));Serial.print(data_rate_);
						#endif
					}
					if (tx_power != 0xF) {
						// 0xF0 = PA_BOOST 1, MAX_POWER 7 
						// p. 104
						// TODO: other regions
						#ifdef EU863
						RfmWrite(RFM_REG_PA_CONFIG, 0xF0 | ((LORAWAN_EU868_TX_POWER_MAX * 2) - tx_power * 2));
						#endif
				
						#if DEBUG_SLIM >= 1
						Serial.print(F("\nReceived Power\t:"));Serial.print(tx_power);
						#endif
					}
				}

				pending_fopts_.fopts[pending_fopts_.length++] = LORAWAN_FOPT_LINK_ADR_ANS;
				pending_fopts_.fopts[pending_fopts_.length++] = status;

				#if DEBUG_SLIM >= 1
				Serial.print(F("\nReceived NbTrans:"));Serial.println(NbTrans);
				#endif

				i += LORAWAN_FOPT_LINK_ADR_REQ_SIZE;
				break;
			case LORAWAN_FOPT_DUTY_CYCLE_REQ:
				// TODO p. 26 1/2^value[0-15]. 0 is no duty cycle.
				i += LORAWAN_FOPT_DUTY_CYCLE_REQ_SIZE;
				break;
			case LORAWAN_FOPT_RX_PARAM_SETUP_REQ:
				// TODO p. 27
				// Handle erroneous values from network.
				status = 0x1;

				new_rx1_dr_offset 	= (options[i + 1] & 0x70) >> 4;
				new_rx2_dr 		= options[i + 1] & 0xF;

				if (new_rx1_dr_offset <= LORAWAN_EU868_RX1_DR_OFFSET_MAX) {
					status |= 0x4;
				}
				#if defined(EU_DR6)
				if (new_rx2_dr >= SF12BW125 && new_rx2_dr <= SF7BW250) { // Reversed table index
				#else
				if (new_rx2_dr >= SF12BW125 && new_rx2_dr <= SF7BW125) { // Reversed table index
				#endif
					status |= 0x2;
				}

				if (status == 0x7) {
					rx1_data_rate_offset_ = new_rx1_dr_offset;
					SetRx1DataRateOffset(rx1_data_rate_offset_);

					rx2_data_rate_ = new_rx2_dr;
					SetRx2DataRate(rx2_data_rate_);
				}

				#if DEBUG_SLIM >= 1
				Serial.print(F("\nRX1_DR_OFFSET: "));Serial.println(rx1_data_rate_offset_);
				Serial.print(F("RX2_DR: "));Serial.println(rx2_data_rate_);
				#endif

				sticky_fopts_.fopts[sticky_fopts_.length++] = LORAWAN_FOPT_RX_PARAM_SETUP_ANS; // until a downlink received
				sticky_fopts_.fopts[sticky_fopts_.length++] = status;

				i += LORAWAN_FOPT_RX_PARAM_SETUP_REQ_SIZE;
				break;
			case LORAWAN_FOPT_DEV_STATUS_REQ:
				pending_fopts_.fopts[pending_fopts_.length++] = LORAWAN_FOPT_DEV_STATUS_ANS;
				// TODO: make it a function. 
				// If we have value pin procced, 
				// if we have value pin 255 don't proceed.

				#if defined(ARDUINO_AVR_FEATHER32U4)
				#define VBATPIN A9	   // pin to measure battery voltage
				uint8_t vbat;
				vbat = analogRead(VBATPIN) - 450; // convert to 8bit
				/*
				vbat *= 2	// we divided by 2, so multiply back
				vbat *= 3.3;	// Multiply by 3.3V, our reference voltage
				vbat /= 1024;	// convert to voltage
				all in one: vbat *=0.0064453125
				*/
				// Connected to USB feather reports 4.29Volts with above conversion
				// vbat 699 seems to be the Maximum raw value.
				// raw values (without minus 450 calculation):
				// 465.5 = 3.0Volts, 480=3.1 volts, 496=3.2 volts 699=4.5Volts
				// Capacity measured with voltage is not linear! https://learn.adafruit.com/assets/979
				// 3.95V is 80%, 3.8V = 60%, 3.75V = 40%, 3.7V = 20%
				if ( vbat < 30 ) {
			   		vbat = 1;				// empty battery
		   		}
				else {
					vbat = map(vbat, 30, 200, 1, 254);	// LoRaWAN wants value from 1-254.
										// 255 reserved for not measured.
										// 0 reserved for external source
				}
				if ( vbat > 254 ) { vbat = 0; }			// probably charging, aka: external source. 238 = 688 - 450
					pending_fopts_.fopts[pending_fopts_.length++] = vbat;
				#else
					pending_fopts_.fopts[pending_fopts_.length++] = 0xFF; // Unable to measure the battery level.
				#endif //ARDUINO_AVR_FEATHER32U4
			
				// This is for debugging to understand the values, will be removed.	
				#if DEBUG_SLIM >= 1
				Serial.print(F("\nlast_packet_snr_ 8bit / 4: "));Serial.print(last_packet_snr_);
				#if defined(ARDUINO_AVR_FEATHER32U4)
				Serial.print(F("\nSTATUS ANS -- vbat, SNR shifts: "));Serial.print(vbat);Serial.print(F(", "));Serial.print((last_packet_snr_ & 0x80) >> 2 | last_packet_snr_ & 0x3F, BIN);
				#endif

				// convert to 6 bit. Code from RadioLib
				// valid values: -32 to 31
				if ( last_packet_snrB < 128 ) {
					last_packet_snrB /= 4;
				} else {
					last_packet_snrB = (last_packet_snrB - 256 ) / 4;
				}

				Serial.print(F("\nSNR radiolib: "));Serial.println(last_packet_snrB);
				#endif
				
				// convert 8bit signed to 6bit signed -32 to 31
				pending_fopts_.fopts[pending_fopts_.length++] = (last_packet_snr_ & 0x80) >> 2 | last_packet_snr_ & 0x3F;

				i += LORAWAN_FOPT_DEV_STATUS_REQ_SIZE;
				break;
			case LORAWAN_FOPT_NEW_CHANNEL_REQ:
				// TODO p. 28
				// Not supported yet. Have to implement 
				// a: FSK, 
				// b: remove the channels from PROGMEM
				// c: and create a function to create channels.
				
				/* Hacky solution
				 *
				 * a. Check if the FREQ is already listed in PROGMEM.
				 * if YES status |= 1
				 *
				 * b. Check if min DR is 0 and max DR 5
				 * if YES status |= 2
				 *
				 */

				// TODO: How Can I know the number of channels to 
				// be created / modified?
				// Maybe from f_options_length?

				// ChIndex
				new_rx2_dr = options[i + 1]; // store Channel Index to temp variable
				if ( new_rx2_dr >= 0 && new_rx2_dr <= 7 ) {
					// TODO compare first 20 bits with kFrequencyTable if they are the same.
					freqMSB = pgm_read_byte(&(kFrequencyTable[new_rx2_dr][0]));
					freqMID = pgm_read_byte(&(kFrequencyTable[new_rx2_dr][1]));
					freqLSB = pgm_read_byte(&(kFrequencyTable[new_rx2_dr][2])) & 0xF0; // erase 4 LSB bits to keep 20bits resolution
					
					// Frequency
					// TODO if Freq is 0 then channel is disabled
					freqNewMSB = options[i + 2];
					freqNewMID = options[i + 3];
					freqNewLSB = options[i + 4];

					#if DEBUG_SLIM == 1
					Serial.print(F("\nfreqMSB   : "));Serial.print(freqMSB);
					Serial.print(F("\nfreqNewMSB: "));Serial.print(freqNewMSB);
					Serial.print(F("\nfreqMID   : "));Serial.print(freqMID);
					Serial.print(F("\nfreqNewMID: "));Serial.print(freqNewMID);
					Serial.print(F("\nfreqLSB   : "));Serial.print(freqLSB);
					Serial.print(F("\nfreqNewLSB: "));Serial.print(freqNewLSB);
					#endif

					status |= 1;
				}

				if ( options[i + 5] & 0x0F == 0 && options[i + 5] & 0xF0 == 5 ) {
					status |= 2; // DR ok
				} 

				// DEBUGING ERASE FOR PRODUCTION
				status = 0; // 1 = FREQ ok, 2 DR ok

				pending_fopts_.fopts[pending_fopts_.length++] = LORAWAN_FOPT_NEW_CHANNEL_ANS;
				pending_fopts_.fopts[pending_fopts_.length++] = status;

				i += LORAWAN_FOPT_NEW_CHANNEL_REQ_SIZE;
				break;
			case LORAWAN_FOPT_RX_TIMING_SETUP_REQ:
				// spec p. 30
				new_rx2_dr = (options[i + 1] & 0xF);
				// zero value = 1
				if ( new_rx2_dr == 0 ) { new_rx2_dr = 1; }

				// Save to EEPROM
				SetRx1Delay(new_rx2_dr);
				rx1_delay_micros_ = GetRx1Delay() * MICROS_PER_SECOND;

				sticky_fopts_.fopts[sticky_fopts_.length++] = LORAWAN_FOPT_RX_TIMING_SETUP_ANS; // sticky until downlink received.

				i += LORAWAN_FOPT_RX_TIMING_SETUP_REQ_SIZE;
				break;
			case LORAWAN_FOPT_TX_PARAM_SETUP_REQ:
				// TODO. p. 30, not for EU
				i += LORAWAN_FOPT_TX_PARAM_SETUP_REQ_SIZE;
				break;
			case LORAWAN_FOPT_DL_CHANNEL_REQ:
				// TODO p. 28
				// https://learn.semtech.com/mod/book/view.php?id=173&chapterid=144
				// Not needed for US and AUS regions.
				// clv
				// sticky_fopts_.fopts[sticky_fopts_.length++] = LORAWAN_FOPT_DL_CHANNEL_ANS;
				// report not accepted. SlimLoRa does not support new frequency for downlink.
				// TODO make kFrequencyTable dynamic so we can accept a new downlink frequency
				// pending_fopts_.fopts[pending_fopts_.length++] = 0;

				i += LORAWAN_FOPT_DL_CHANNEL_REQ_SIZE;
				break;
			case LORAWAN_FOPT_DEVICE_TIME_ANS:
				// p. 32
				#ifdef MAC_REQUESTS 
				// store 4 bytes of time epoch since 1980 and remove 18 leap seconds (thanks to RadioLib for the heads up)
				epoch = ( (uint32_t) options[i + 4] << 24 
						| (uint32_t) options[i + 3] << 16 
						| options[i + 2] << 8 
						| options[i + 1]) - LORAWAN_EPOCH_DRIFT;
				// Fractional second 0-100. This calculation is NOT precise
				fracSecond = ( options[i + 5] * LORAWAN_DEVICE_TIME_FRACTIONAL_STEPS ) * 100; // 100 = 1 second
				#if DEBUG_SLIM >= 1
				Serial.print(F("\nepoch from LNS: "));Serial.print(epoch);
				Serial.print(F("\tfrac: "));Serial.print(fracSecond);
				#endif
				#ifdef SLIM_DEBUG_VARS	
				LoRaWANreceived |= 0x40; // Time received
				#endif
				#endif
				i += LORAWAN_FOPT_DEVICE_TIME_ANS_SIZE;
				break;
			default:
				return;
		}
	}
}

/**
 * Listens for and processes LoRaWAN downlink packets.
 *
 * @param window Receive window index [1,2].
 * @return 0 if successful, else error code.
 */
int8_t SlimLoRa::ProcessDownlink(uint8_t window) {
	int8_t result;
	uint8_t rx1_offset_dr;
	uint8_t temp;
	uint32_t rx_delay;

#if DEBUG_SLIM == 0
	uint8_t packet[64];
	int8_t packet_length;

	uint8_t f_options_length, payload_length;
#endif

	uint16_t frame_counter;

	uint8_t mic[4];

#if LORAWAN_OTAA_ENABLED
	uint8_t dev_addr[4];
	GetDevAddr(dev_addr);
#endif // LORAWAN_OTAA_ENABLED

	if (window == 1) {
		rx1_offset_dr = data_rate_ + rx1_data_rate_offset_; // Reversed table index
		#if defined(EU_DR6)
		if (rx1_offset_dr > SF7BW250) {
			rx1_offset_dr = SF7BW250;
		#else
		if (rx1_offset_dr > SF7BW125) {
			rx1_offset_dr = SF7BW125;
		#endif
		}
		rx_delay = CalculateRxDelay(rx1_offset_dr, rx1_delay_micros_);
		packet_length = RfmReceivePacket(packet, sizeof(packet), channel_, rx1_offset_dr, tx_done_micros_ + rx_delay);
	} else {
		rx_delay = CalculateRxDelay(rx2_data_rate_, rx1_delay_micros_ + MICROS_PER_SECOND);
		packet_length = RfmReceivePacket(packet, sizeof(packet), 8, rx2_data_rate_, tx_done_micros_ + rx_delay);
	}

	if (packet_length <= 0) {
		result = LORAWAN_ERROR_NO_PACKET_RECEIVED;
		goto end;
	}

	if (packet_length > sizeof(packet)) {
		result = LORAWAN_ERROR_SIZE_EXCEEDED;
		goto end;
	}

	if (packet[0] != LORAWAN_MTYPE_UNCONFIRMED_DATA_DOWN && packet[0] != LORAWAN_MTYPE_CONFIRMED_DATA_DOWN) {
		result = LORAWAN_ERROR_UNEXPECTED_MTYPE;
		goto end;
	}

	// EVAL for overflow
	frame_counter = packet[7] << 8 | packet[6];
	if (frame_counter < rx_frame_counter_) {
		result = LORAWAN_ERROR_INVALID_FRAME_COUNTER;
		goto end;
	}

#if LORAWAN_OTAA_ENABLED
	if (!(packet[4] == dev_addr[0] && packet[3] == dev_addr[1]
			&& packet[2] == dev_addr[2] && packet[1] == dev_addr[3])) {
		result = LORAWAN_ERROR_UNEXPECTED_DEV_ADDR;
		goto end;
	}
#else
	// ABP DevAddr
	if (!(packet[4] == DevAddr[0] && packet[3] == DevAddr[1]
			&& packet[2] == DevAddr[2] && packet[1] == DevAddr[3])) {
		result = LORAWAN_ERROR_UNEXPECTED_DEV_ADDR;
		goto end;
	}
#endif // LORAWAN_OTAA_ENABLED

	// Check MIC
	CalculateMessageMic(packet, mic, packet_length - 4, frame_counter, LORAWAN_DIRECTION_DOWN);
	if (!CheckMic(mic, &packet[packet_length - 4])) {
#if DEBUG_SLIM >= 1
	Serial.print(F("MIC error: "));Serial.println(LORAWAN_ERROR_INVALID_MIC);
#endif
		return LORAWAN_ERROR_INVALID_MIC;
	}

	// Clear sticky fopts
	memset(sticky_fopts_.fopts, 0, sticky_fopts_.length);
	sticky_fopts_.length = 0;

	// Saves memory cycles
	rx_frame_counter_++; 
	if (rx_frame_counter_ % EEPROM_WRITE_RX_COUNT == 0) {
		SetRxFrameCounter();
	}

	// Mark ack_ as true so the next uplink we will have ACK enabled.
	if (packet[0] == LORAWAN_MTYPE_CONFIRMED_DATA_DOWN) {
		ack_ = true;
	}

	// We received downlink. Reset some values.
	// Reset ADR acknowledge counter
	adr_ack_limit_counter_ = 0; 
	adr_ack_delay_counter_ = 0; 
	// Reset NbTrans
	NbTrans_counter = NbTrans;

	// Grab frame options size in bytes.
	f_options_length = packet[5] & 0xF; // p. 17
	
	// If we don't have payload port must be null. In that case maybe we have MAC commands, ACK - not implemented -, or nothing.
	if ( packet_length == LORAWAN_MAC_AND_FRAME_HEADER + f_options_length + LORAWAN_MIC_SIZE ) {

		#if DEBUG_SLIM >= 1
		Serial.print(F("\n\nNull downPort"));
		#endif
	
		// If we have frame options check them.
		// This is pedantic, but maybe the LNS have send empty packet.
		if ( f_options_length > 0 ) {
			ProcessFrameOptions(&packet[LORAWAN_MAC_AND_FRAME_HEADER], f_options_length);
		}
		
		// null port. We don't have application data. We finished.
		goto end;
	}

	downPort = packet[LORAWAN_MAC_AND_FRAME_HEADER + f_options_length];

	#if DEBUG_SLIM >= 1
		printDownlink();
	#endif

	// Parse MAC commands from payload if packet on port 0
	if (downPort == 0) {
		#if DEBUG_SLIM >= 1
			Serial.print(F("\n\nMAC Options Mode"));
		#endif
		payload_length = packet_length - LORAWAN_MAC_AND_FRAME_HEADER - f_options_length - LORAWAN_MIC_SIZE;
		EncryptPayload(&packet[LORAWAN_MAC_AND_FRAME_HEADER + f_options_length + LORAWAN_PORT_SIZE], payload_length, frame_counter, LORAWAN_DIRECTION_DOWN);

	#if DEBUG_SLIM >= 1
//		printDownlink();
	#endif

		ProcessFrameOptions(&packet[LORAWAN_MAC_AND_FRAME_HEADER + f_options_length + LORAWAN_PORT_SIZE], payload_length);

	#if DEBUG_SLIM >= 1
		printDownlink();
	#endif

	}

	// application downlink and possibly MAC commands
	if ( downPort > 0 && downPort < 224 ) { // downPort 0 and 224 are special case

		// Process MAC commands
		if ( f_options_length > 0 ) {
		#if DEBUG_SLIM >= 1
			Serial.print(F("\n\nFrame Options Mode"));
			Serial.print(F("\nPacket RAW HEX\t: "));printHex(packet, packet_length);
			printDownlink();
		#endif
			ProcessFrameOptions(&packet[LORAWAN_MAC_AND_FRAME_HEADER], f_options_length);
		}

		#if DEBUG_SLIM >= 1
			Serial.print(F("\nPacket RAW HEX\t: "));printHex(packet, packet_length);
			printDownlink();
		#endif

		// stollen from MAC command
		payload_length = packet_length - LORAWAN_MAC_AND_FRAME_HEADER - f_options_length - LORAWAN_MIC_SIZE;
		#if DEBUG_SLIM >= 1
			Serial.print(F("\nPacket RAW HEX\t: "));printHex(packet, packet_length);
			printDownlink();
		#endif

		// Decrypt data after MAC, Frame Header, Frame Options and Port.
		EncryptPayload(&packet[LORAWAN_MAC_AND_FRAME_HEADER + f_options_length + LORAWAN_PORT_SIZE], payload_length, frame_counter, LORAWAN_DIRECTION_DOWN);

		// Store downlink payload to downlinkData. Skip MAC header and Device Address.
		temp = LORAWAN_START_OF_FRM_PAYLOAD + f_options_length;		// Data starts at 10th byte and ends 4 bytes before MIC.
										// If we have frame options we have to read bytes after the frame options.
		for ( ; downlinkSize < payload_length - 1; downlinkSize++) {	// BUG? payload_length is always plus 1. WHY?
			
			if ( downlinkSize > DOWNLINK_PAYLOAD_SIZE ) {		// Protection for buffer overflow.
										// If downlinkSize > DOWNLINK_PAYLOAD_SIZE invalidate and abort income data.

				#if DEBUG_SLIM >= 1
				Serial.print(F("\ndownlinkSize is larger than DOWNLINK_PAYLOAD_SIZE: "));Serial.print(DOWNLINK_PAYLOAD_SIZE);
				Serial.print(F("\ndownlinkSize: "));Serial.print(downlinkSize);
				#endif

				downlinkSize = 0;

				break;
			}
			downlinkData[downlinkSize] = packet[temp];
			temp++;
		}
			
		#if DEBUG_SLIM >= 1
		if ( downlinkSize > 0 ) {
			Serial.print(F("\nDownlinkData\t: "));printHex(downlinkData, downlinkSize);
			//printMAC();
		}
		#endif
	}

#if DEBUG_SLIM >= 1
	Serial.print(F("\nPacket decrypted HEX: "));printHex(packet, packet_length);
	printDownlink();
#endif
	// TEMPORARY
	// Store the received packet for debugging purposes.
#if ARDUINO_EEPROM  == 1
	// Important. Otherwise corrupted EEPROM
	/*if ( packet_length > 64 ) { packet_length = 64; }
	setArrayEEPROM(EEPROM_DOWNPACKET, packet, packet_length ); // We have place for 64
	//EEPROM.put(EEPROM_DOWNPORT, downPort);
	*/
#else
	eeprom_write_block(eeprom_lw_down_packet, packet, 64);
	eeprom_write_byte(eeprom_lw_down_port, downPort);
#endif

	result = 0;

end:
#ifdef SLIM_DEBUG_VARS
	if (window == 1) {
		LoRaWANreceived |= 0x08; // RX1 received data
	} else {
		LoRaWANreceived |= 0x10; // RX2 received data
	}
#endif

#if DEBUG_SLIM >= 1
		Serial.print(F("\n\nMAC status\t: "));Serial.print(result);
		Serial.print(F("\tRx counter: "));Serial.println(GetRxFrameCounter());
		Serial.print(F("\ntx_done_micros: "));Serial.print(tx_done_micros_);
		Serial.print(F("\trx_delay: "));Serial.print(rx_delay);
		Serial.print(F("\tADDed: "));Serial.println(tx_done_micros_ + rx_delay);
		printMAC();
		Serial.flush();
#if LORAWAN_OTAA_ENABLED
		Serial.print(F("\nDevAddr after RX windows: "));printHex(dev_addr, 4);
		Serial.flush();
#endif // LORAWAN_OTAA_ENABLED
#endif
	return result;
}

/**
 * Constructs a LoRaWAN packet and transmits it.
 *
 * @param port FPort of the frame.
 * @param payload Pointer to the array of data to be transmitted.
 * @param payload_length Length of the data to be transmitted.
 */
void SlimLoRa::Transmit(uint8_t fport, uint8_t *payload, uint8_t payload_length) {
	uint8_t packet[64];
	uint8_t packet_length = 0;

	uint8_t mic[4];

	// start fresh
	downlinkSize	= 0;
	downPort	= 0;
	ack_		= 0;

#ifdef SLIM_DEBUG_VARS
	LoRaWANreceived = 0;
#endif
	
#if LORAWAN_OTAA_ENABLED
	uint8_t dev_addr[4];
	GetDevAddr(dev_addr);
#endif // LORAWAN_OTAA_ENABLED

	// Encrypt the data
	EncryptPayload(payload, payload_length, tx_frame_counter_, LORAWAN_DIRECTION_UP);

	// ADR back-off
	// TODO p. 17, after first increase if DR, increase every LORAWAN_ADR_ACK_DELAY overflow
	if (adr_enabled_ && adr_ack_limit_counter_ >= LORAWAN_ADR_ACK_LIMIT && adr_ack_delay_counter_ == 0 ) {
		adr_ack_delay_counter_++; // this way, this if statement will be executed only once
		if ( data_rate_ > SF12BW125 ) {
			data_rate_--;
		}
	// EVAL: I think the order is important for the HOPE
	SetPower(16);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nADR backoff 1st stage, DR: "));Serial.print(data_rate_);
#endif
	}

	// Increase DR every overflow of LORAWAN_ADR_ACK_DELAY
	if (adr_enabled_ && adr_ack_delay_counter_ >= LORAWAN_ADR_ACK_DELAY ) {
		adr_ack_delay_counter_ = 0; // reset overflow
		if ( data_rate_ > SF12BW125 ) {
			data_rate_--;
		}
	// EVAL: I think the order is important for the HOPE
	SetPower(16);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nADR backoff 2nd stage, DR: "));Serial.print(data_rate_);
#endif
	}

	// Build the packet
	// TODO if ??
	//packet[packet_length++] = LORAWAN_MTYPE_CONFIRMED_DATA_UP;
	packet[packet_length++] = LORAWAN_MTYPE_UNCONFIRMED_DATA_UP;

#if LORAWAN_OTAA_ENABLED
	packet[packet_length++] = dev_addr[3];
	packet[packet_length++] = dev_addr[2];
	packet[packet_length++] = dev_addr[1];
	packet[packet_length++] = dev_addr[0];
#else
	packet[packet_length++] = DevAddr[3];
	packet[packet_length++] = DevAddr[2];
	packet[packet_length++] = DevAddr[1];
	packet[packet_length++] = DevAddr[0];
#endif // LORAWAN_OTAA_ENABLED

	// Frame control
	packet[packet_length] = 0;
	if (adr_enabled_) {
		packet[packet_length] |= LORAWAN_FCTRL_ADR;
	
		// Request ADR if adr_ack_limit_counter_ is over the limit of LORAWAN_ADR_ACK_LIMIT
		// p. 17
		if (adr_ack_limit_counter_ >= LORAWAN_ADR_ACK_LIMIT ) {
			packet[packet_length] |= LORAWAN_FCTRL_ADR_ACK_REQ;
		}
	}

	// if we received a confirmed downlink respond with ACK
	if ( ack_ == true ) {
		packet[packet_length] |= LORAWAN_FCTRL_ACK;
	}

#ifdef MAC_REQUESTS
	// first bit is for CheckTime
	if ( ( TimeLinkCheck & 0x01 ) == 0x01 ) {
		pending_fopts_.fopts[pending_fopts_.length++] = LORAWAN_FOPT_DEVICE_TIME_REQ;
	}

	// second bit is for CheckLink
	if ( ( TimeLinkCheck & 0x02 ) == 0x02 ) {
		pending_fopts_.fopts[pending_fopts_.length++] = LORAWAN_FOPT_LINK_CHECK_REQ;
	}

	// Reset TimeLinkCheck
	TimeLinkCheck = 0;
#endif

	packet[packet_length++] |= pending_fopts_.length + sticky_fopts_.length;

	// Uplink frame counter
	packet[packet_length++] = tx_frame_counter_ & 0xFF;
	packet[packet_length++] = tx_frame_counter_ >> 8;

	// Apply Frame options
	for (uint8_t i = 0; i < pending_fopts_.length; i++) {
		packet[packet_length++] = pending_fopts_.fopts[i];
	}

	// Apply Sticky Frame options
	for (uint8_t i = 0; i < sticky_fopts_.length; i++) {
		packet[packet_length++] = sticky_fopts_.fopts[i];
	}

	// Clear fopts
	memset(pending_fopts_.fopts, 0, pending_fopts_.length);
	pending_fopts_.length = 0;

	// Frame port
	packet[packet_length++] = fport;

	// Copy payload
	for (uint8_t i = 0; i < payload_length; i++) {
		packet[packet_length++] = payload[i];
	}

	// Calculate MIC
	CalculateMessageMic(packet, mic, packet_length, tx_frame_counter_, LORAWAN_DIRECTION_UP);
	for (uint8_t i = 0; i < 4; i++) {
		packet[packet_length++] = mic[i];
	}

	// TODO: other regions
	#ifdef EU863

	// experiment to apply ChMask in channel selection
	// TODO: conflict with channel [8] which is downlink. Downlink channel must move to another variable or index.
	channel_ = micros() >> 8 & ( LORAWAN_UPLINK_CHANNEL_COUNT - 1 ) ; // [7] for 8 channels [15] for 16 channels;
	#if DEBUG_SLIM >= 1
		Serial.print(F("\nchannel_\t:"));Serial.print(channel_);
	#endif

	#if DEBUG_SLIM >= 1
	uint8_t countEfforts;
	#endif

	// check if channel_ allowed with ChMask
	while ( ( ( ChMask >> channel_ ) & 0x01 ) == 0 ) {

		#if DEBUG_SLIM >= 1
		Serial.print(F("\nChannel disabled."));
		countEfforts++;
		#endif
		
		// next channel
		channel_++;

		// exit while loop if channel is available
		if ( ( ( ChMask >> channel_ ) & 0x01 ) == 1 ) {
			break;
		}

		// out-of-bounds, re-choose channel. Break only if we find a valid
		if ( channel_ >= LORAWAN_UPLINK_CHANNEL_COUNT ) {
			// TODO for 16 channels
			channel_ = micros() >> 8 & ( LORAWAN_UPLINK_CHANNEL_COUNT - 1 ) ; // [7] for 8 channels [15] for 16 channels;

			// We found enabled channel
			if ( ( ( ChMask >> channel_ ) & 0x01 ) == 1 ) {
				break;
			}

		}
	}

	#if DEBUG_SLIM >= 1
		Serial.print(F("\ncountEfforts\t:"));Serial.print(countEfforts);
		Serial.print(F("\nchannel_\t:"));Serial.println(channel_);
	#endif
	
	// TODO this is valid for 8 channels, not for 16. Need to move downlink to another variable or index.
	// Old code without ChMask check
	// channel_ = pseudo_byte_ & 0b111; 		// Channel 9 [8] is downlink. Channels [0-7] are for uplinks. Mask with 0x07 (0b111) to use the first 8 channels.
	
	#endif // EU868

	RfmSendPacket(packet, packet_length, channel_, data_rate_);
}

/**
 * Sends data and listens for downlink messages on RX1 and RX2.
 *
 * @param fport FPort of the frame.
 * @param payload Pointer to the array of data to be transmitted.
 * @param payload_length Length of data to be transmitted.
 */
void SlimLoRa::SendData(uint8_t fport, uint8_t *payload, uint8_t payload_length) {

	#ifdef US902
	// TODO payloads for US902 https://www.thethingsnetwork.org/docs/lorawan/regional-parameters/#us902-928-maximum-payload-size
	// SF10 11  bytes
	// SF 9 53  bytes
	// SF 8 128 bytes
	// SF 7 242 bytes
	if ( payload_length > 11 ) {

		#if DEBUG_SLIM >= 1
		// TODO check SF with dri
		Serial.println(F("Big Payload (11 bytes), data not send."));
		#endif

		return -1;
	}
	#endif // US902

#if DEBUG_SLIM >= 1
	Serial.println(F("\nSlimLoRa SendData"));printMAC();
#endif

	// TODO: protect buffer overflow.
	// EU: RP p. 28. DR0-2 (SF12-SF10), 51 bytes, DR3 (SF9): 115 bytes, DR4-5 (SF8, SF7): 222 bytes
	// We have to also subtract frame options

	Transmit(fport, payload, payload_length);

	if (ProcessDownlink(1)) {
		ProcessDownlink(2);
	}

#if COUNT_TX_DURATION == 1
	CalculateTXms();
#endif
}

/**
 * Encrypts/Decrypts the payload of a LoRaWAN data message.
 *
 * Decryption is performed the same way as encryption as the network server 
 * conveniently uses AES decryption for encrypting download payloads.
 *
 * @param payload Pointer to the data to encrypt or decrypt Address of the register to be written.
 * @param payload_length Number of bytes to process.
 * @param frame_counter Frame counter for upstream frames.
 * @param direction Direction of message.
 */
void SlimLoRa::EncryptPayload(uint8_t *payload, uint8_t payload_length, unsigned int frame_counter, uint8_t direction) {
	uint8_t block_count = 0;
	uint8_t incomplete_block_size = 0;

	uint8_t block_a[16];

#if LORAWAN_OTAA_ENABLED
	uint8_t dev_addr[4], app_s_key[16];
	GetDevAddr(dev_addr);
	GetAppSKey(app_s_key);
#endif // LORAWAN_OTAA_ENABLED

	// Calculate number of blocks
	block_count = payload_length / 16;
	incomplete_block_size = payload_length % 16;
	if (incomplete_block_size != 0) {
		block_count++;
	}

	for (uint8_t i = 1; i <= block_count; i++) {
		block_a[0] = 0x01;
		block_a[1] = 0x00;
		block_a[2] = 0x00;
		block_a[3] = 0x00;
		block_a[4] = 0x00;

		block_a[5] = direction;

#if LORAWAN_OTAA_ENABLED
		block_a[6] = dev_addr[3];
		block_a[7] = dev_addr[2];
		block_a[8] = dev_addr[1];
		block_a[9] = dev_addr[0];
#else
		block_a[6] = DevAddr[3];
		block_a[7] = DevAddr[2];
		block_a[8] = DevAddr[1];
		block_a[9] = DevAddr[0];
#endif // LORAWAN_OTAA_ENABLED

		block_a[10] = frame_counter & 0xFF;
		block_a[11] = frame_counter >> 8;

		block_a[12] = 0x00; // Frame counter upper bytes
		block_a[13] = 0x00;

		block_a[14] = 0x00;

		block_a[15] = i;

		// Calculate S
#if LORAWAN_OTAA_ENABLED
		AesEncrypt(app_s_key, block_a);
#else
		AesEncrypt(AppSKey, block_a);
#endif // LORAWAN_OTAA_ENABLED

		// Check for last block
		if (i != block_count) {
			for (uint8_t j = 0; j < 16; j++) {
				*payload ^= block_a[j];
				payload++;
			}
		} else {
			if (incomplete_block_size == 0) {
				incomplete_block_size = 16;
			}

			for (uint8_t j = 0; j < incomplete_block_size; j++) {
				*payload ^= block_a[j];
				payload++;
			}
		}
	}
}

/**
 * Calculates an AES MIC of given data using a key.
 *
 * @param key 16-byte long key.
 * @param data Pointer to the data to process.
 * @param initial_block Pointer to an initial 16-byte block.
 * @param final_mic 4-byte array for final MIC output.
 * @param data_length Number of bytes to process.
 */
void SlimLoRa::CalculateMic(const uint8_t *key, uint8_t *data, uint8_t *initial_block, uint8_t *final_mic, uint8_t data_length) {
	uint8_t key1[16] = {0};
	uint8_t key2[16] = {0};

	uint8_t old_data[16] = {0};
	uint8_t new_data[16] = {0};

	uint8_t block_count = 0;
	uint8_t incomplete_block_size = 0;
	uint8_t block_counter = 1;

	// Calculate number of blocks and blocksize of last block
	block_count = data_length / 16;
	incomplete_block_size = data_length % 16;

	if (incomplete_block_size != 0) {
		block_count++;
	}

	GenerateKeys(key, key1, key2);

	// Copy initial block to old_data if present
	if (initial_block != NULL) {
		for (uint8_t i = 0; i < 16; i++) {
			old_data[i] = *initial_block;
			initial_block++;
		}

		AesEncrypt(key, old_data);
	}

	// Calculate first block_count - 1 blocks
	while (block_counter < block_count) {
		for (uint8_t i = 0; i < 16; i++) {
			new_data[i] = *data;
			data++;
		}

		XorData(new_data, old_data);
		AesEncrypt(key, new_data);

		for (uint8_t i = 0; i < 16; i++) {
			old_data[i] = new_data[i];
		}

		block_counter++;
	}

	// Pad and calculate last block
	if (incomplete_block_size == 0) {
		for (uint8_t i = 0; i < 16; i++) {
			new_data[i] = *data;
			data++;
		}

		XorData(new_data, key1);
		XorData(new_data, old_data);
		AesEncrypt(key, new_data);
	} else {
		for (uint8_t i = 0; i < 16; i++) {
			if (i < incomplete_block_size) {
				new_data[i] = *data;
				data++;
			}

			if (i == incomplete_block_size) {
				new_data[i] = 0x80;
			}

			if (i > incomplete_block_size) {
				new_data[i] = 0x00;
			}
		}

		XorData(new_data, key2);
		XorData(new_data, old_data);
		AesEncrypt(key, new_data);
	}

	final_mic[0] = new_data[0];
	final_mic[1] = new_data[1];
	final_mic[2] = new_data[2];
	final_mic[3] = new_data[3];

	// Original
	//pseudo_byte_ = final_mic[3];
	// clv
	//pseudo_byte_ = micros(); pseudo_byte_ = pseudo_byte_ >> 5; // 0 - 7
	pseudo_byte_ = micros() >> 8 & ( LORAWAN_UPLINK_CHANNEL_COUNT - 1 ) ; // [7] for 8 channels [15] for 16 channels;
}

/**
 * Calculates an AES MIC of a LoRaWAN message.
 *
 * @param data Pointer to the data to process.
 * @param final_mic 4-byte array for final MIC output.
 * @param data_length Number of bytes to process.
 * @param frame_counter Frame counter for uplink frames.
 * @param direction Number of message.
 */
void SlimLoRa::CalculateMessageMic(uint8_t *data, uint8_t *final_mic, uint8_t data_length, unsigned int frame_counter, uint8_t direction) {
	uint8_t block_b[16];
#if LORAWAN_OTAA_ENABLED
	uint8_t dev_addr[4], nwk_s_key[16];

	GetDevAddr(dev_addr);
	GetNwkSEncKey(nwk_s_key);
#endif // LORAWAN_OTAA_ENABLED

	block_b[0] = 0x49;
	block_b[1] = 0x00;
	block_b[2] = 0x00;
	block_b[3] = 0x00;
	block_b[4] = 0x00;

	block_b[5] = direction;

#if LORAWAN_OTAA_ENABLED
	block_b[6] = dev_addr[3];
	block_b[7] = dev_addr[2];
	block_b[8] = dev_addr[1];
	block_b[9] = dev_addr[0];
#else
	block_b[6] = DevAddr[3];
	block_b[7] = DevAddr[2];
	block_b[8] = DevAddr[1];
	block_b[9] = DevAddr[0];
#endif // LORAWAN_OTAA_ENABLED

	block_b[10] = frame_counter & 0xFF;
	block_b[11] = frame_counter >> 8;

	block_b[12] = 0x00; // Frame counter upper bytes
	block_b[13] = 0x00;

	block_b[14] = 0x00;
	block_b[15] = data_length;

#if LORAWAN_OTAA_ENABLED
	CalculateMic(nwk_s_key, data, block_b, final_mic, data_length);
#else
	CalculateMic(NwkSKey, data, block_b, final_mic, data_length);
#endif // LORAWAN_OTAA_ENABLED
}

/**
 * Generate keys for MIC calculation.
 *
 * @param key .
 * @param key1 Pointer to key 1.
 * @param key2 Pointer to key 2.
 */
void SlimLoRa::GenerateKeys(const uint8_t *key, uint8_t *key1, uint8_t *key2) {
	uint8_t msb_key;

	// Encrypt the zeros in key1 with the NwkSkey
	AesEncrypt(key, key1);

	// Create key1
	// Check if MSB is 1
	if ((key1[0] & 0x80) == 0x80) {
		msb_key = 1;
	} else {
		msb_key = 0;
	}

	// Shift key1 one bit left
	ShiftLeftData(key1);

	// if MSB was 1
	if (msb_key == 1) {
		key1[15] = key1[15] ^ 0x87;
	}

	// Copy key1 to key2
	for (uint8_t i = 0; i < 16; i++) {
		key2[i] = key1[i];
	}

	// Check if MSB is 1
	if ((key2[0] & 0x80) == 0x80) {
		msb_key = 1;
	} else {
		msb_key = 0;
	}

	// Shift key2 one bit left
	ShiftLeftData(key2);

	// Check if MSB was 1
	if (msb_key == 1) {
		key2[15] = key2[15] ^ 0x87;
	}
}

void SlimLoRa::ShiftLeftData(uint8_t *data) {
	uint8_t overflow = 0;

	for (uint8_t i = 0; i < 16; i++) {
		// Check for overflow on next byte except for the last byte
		if (i < 15) {
			// Check if upper bit is one
			if ((data[i + 1] & 0x80) == 0x80) {
				overflow = 1;
			} else {
				overflow = 0;
			}
		} else {
			overflow = 0;
		}

		// Shift one left
		data[i] = (data[i] << 1) + overflow;
	}
}

void SlimLoRa::XorData(uint8_t *new_data, uint8_t *old_data) {
	for (uint8_t i = 0; i < 16; i++) {
		new_data[i] = new_data[i] ^ old_data[i];
	}
}

/**
 * AES encrypts data with 128 bit key.
 *
 * @param key 128 bit key.
 * @param data Plaintext to encrypt.
 */
void SlimLoRa::AesEncrypt(const uint8_t *key, uint8_t *data) {
	uint8_t round;
	uint8_t round_key[16];
	uint8_t state[4][4];

	// Copy input to state arry
	for (uint8_t column = 0; column < 4; column++) {
		for (uint8_t row = 0; row < 4; row++) {
			state[row][column] = data[row + (column << 2)];
		}
	}

	// Copy key to round key
	memcpy(&round_key[0], &key[0], 16);

	// Add round key
	AesAddRoundKey(round_key, state);

	// Preform 9 full rounds with mixed collums
	for (round = 1; round < 10; round++) {
		// Perform Byte substitution with S table
		for (uint8_t column = 0; column < 4; column++) {
			for (uint8_t row = 0; row < 4; row++) {
				state[row][column] = AesSubByte(state[row][column]);
			}
		}

		// Perform row Shift
		AesShiftRows(state);

		// Mix Collums
		AesMixCollums(state);

		// Calculate new round key
		AesCalculateRoundKey(round, round_key);

		// Add the round key to the round_key
		AesAddRoundKey(round_key, state);
	}

	// Perform Byte substitution with S table whitout mix collums
	for (uint8_t column = 0; column < 4; column++) {
		for (uint8_t row = 0; row < 4; row++) {
			state[row][column] = AesSubByte(state[row][column]);
		}
	}

	// Shift rows
	AesShiftRows(state);

	// Calculate new round key
	AesCalculateRoundKey(round, round_key);

	// Add round key
	AesAddRoundKey(round_key, state);

	// Copy the state into the data array
	for (uint8_t column = 0; column < 4; column++) {
		for (uint8_t row = 0; row < 4; row++) {
			data[row + (column << 2)] = state[row][column];
		}
	}
}

void SlimLoRa::AesAddRoundKey(uint8_t *round_key, uint8_t (*state)[4]) {
	for (uint8_t column = 0; column < 4; column++) {
		for (uint8_t row = 0; row < 4; row++) {
			state[row][column] ^= round_key[row + (column << 2)];
		}
	}
}

uint8_t SlimLoRa::AesSubByte(uint8_t byte) {
	// uint8_t S_Row, S_Collum;
	// uint8_t S_Byte;

	// S_Row	= ((byte >> 4) & 0x0F);
	// S_Collum = ((byte >> 0) & 0x0F);
	// S_Byte   = kSTable[S_Row][S_Collum];

	// return kSTable[((byte >> 4) & 0x0F)][((byte >> 0) & 0x0F)]; // original
	return pgm_read_byte(&(kSTable[((byte >> 4) & 0x0F)][((byte >> 0) & 0x0F)]));
}

void SlimLoRa::AesShiftRows(uint8_t (*state)[4]) {
	uint8_t buffer;

	// Store firt byte in buffer
	buffer	  = state[1][0];
	// Shift all bytes
	state[1][0] = state[1][1];
	state[1][1] = state[1][2];
	state[1][2] = state[1][3];
	state[1][3] = buffer;

	buffer	  = state[2][0];
	state[2][0] = state[2][2];
	state[2][2] = buffer;
	buffer	  = state[2][1];
	state[2][1] = state[2][3];
	state[2][3] = buffer;

	buffer	  = state[3][3];
	state[3][3] = state[3][2];
	state[3][2] = state[3][1];
	state[3][1] = state[3][0];
	state[3][0] = buffer;
}

void SlimLoRa::AesMixCollums(uint8_t (*state)[4]) {
	uint8_t a[4], b[4];

	for (uint8_t column = 0; column < 4; column++) {
		for (uint8_t row = 0; row < 4; row++) {
			a[row] =  state[row][column];
			b[row] = (state[row][column] << 1);

			if ((state[row][column] & 0x80) == 0x80) {
				b[row] ^= 0x1B;
			}
		}

		state[0][column] = b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3];
		state[1][column] = a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3];
		state[2][column] = a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3];
		state[3][column] = a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3];
	}
}

void SlimLoRa::AesCalculateRoundKey(uint8_t round, uint8_t *round_key) {
	uint8_t tmp[4];

	// Calculate rcon
	uint8_t rcon = 0x01;
	while (round != 1) {
		uint8_t b = rcon & 0x80;
		rcon = rcon << 1;

		if (b == 0x80) {
			rcon ^= 0x1b;
		}
		round--;
	}

	// Calculate first tmp
	// Copy laste byte from previous key and subsitute the byte, but shift the array contents around by 1.
	tmp[0] = AesSubByte(round_key[12 + 1]);
	tmp[1] = AesSubByte(round_key[12 + 2]);
	tmp[2] = AesSubByte(round_key[12 + 3]);
	tmp[3] = AesSubByte(round_key[12 + 0]);

	// XOR with rcon
	tmp[0] ^= rcon;

	// Calculate new key
	for (uint8_t i = 0; i < 4; i++) {
		for (uint8_t j = 0; j < 4; j++) {
			round_key[j + (i << 2)] ^= tmp[j];
			tmp[j] = round_key[j + (i << 2)];
		}
	}
}


#if ARDUINO_EEPROM == 0
// TxFrameCounter
uint16_t SlimLoRa::GetTxFrameCounter() {
	uint16_t value = eeprom_read_word(&eeprom_lw_tx_frame_counter);

	if (value == 0xFFFF) {
		return 0;
	}

	return value;
}

void SlimLoRa::SetTxFrameCounter() {
	eeprom_write_word(&eeprom_lw_tx_frame_counter, tx_frame_counter_);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE Tx#: "));Serial.print(tx_frame_counter_);
#endif
}

// RxFrameCounter
uint16_t SlimLoRa::GetRxFrameCounter() {
	uint16_t value = eeprom_read_word(&eeprom_lw_rx_frame_counter);
	if (value == 0xFFFF) { return 0; }
	return value;
}

void SlimLoRa::SetRxFrameCounter() {
	eeprom_write_word(&eeprom_lw_rx_frame_counter, rx_frame_counter_);
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
	NbTrans = eeprom_read_byte(&eeprom_lw_NbTrans) & 0xF; // Grab only 0-3 bits
}

void SlimLoRa::SetNbTrans() {
	eeprom_write_byte(&eeprom_lw_NbTrans, NbTrans);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWrite NbTrans: "));Serial.print(NbTrans);
#endif
}


#endif // ARDUINO_EEPROM == 0

#if ARDUINO_EEPROM == 1
// TxFrameCounter
uint16_t SlimLoRa::GetTxFrameCounter() {
	uint16_t value;
	EEPROM.get(EEPROM_TX_COUNTER, value);
	if (value == 0xFFFF) {
		return 0;
	}
	return value;
}

void SlimLoRa::SetTxFrameCounter() {
	// BUG: Why it does not work?
	//EEPROM.update(EEPROM_TX_COUNTER, tx_frame_counter_);
	EEPROM.put(EEPROM_TX_COUNTER, tx_frame_counter_);
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE Tx#: "));Serial.print(tx_frame_counter_);
#endif
}

// RxFrameCounter
uint16_t SlimLoRa::GetRxFrameCounter() {
	uint16_t value;
       	EEPROM.get(EEPROM_RX_COUNTER, value);
	if (value == 0xFFFF) { return 0; }
	return value;
}

void SlimLoRa::SetRxFrameCounter() {
	EEPROM.put(EEPROM_RX_COUNTER, rx_frame_counter_);
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
#endif
}

// NbTrans
void SlimLoRa::GetNbTrans() {
	if ( EEPROM.read(EEPROM_NBTRANS) == 0xFF ) {
		NbTrans = NBTRANS;	// EEPROM erased, default NbTrans
	} else {
		NbTrans = EEPROM.read(EEPROM_NBTRANS) & 0xF;		// EEPROM is not erased. Get the LSB bits
	}
#if DEBUG_SLIM >= 1
	Serial.print(F("\nGet NbTrans: "));Serial.print(NbTrans);
#endif
}

void SlimLoRa::SetNbTrans() {
	uint8_t temp_none;
	temp_none = EEPROM.read(EEPROM_NBTRANS) & 0xF0;		// Get the MSB bits
	EEPROM.update(EEPROM_NBTRANS, ( temp_none << 4 ) | NbTrans );
#if DEBUG_SLIM >= 1
	Serial.print(F("\nWRITE temp_none\t: "));Serial.print(temp_none >> 4);
	Serial.print(F("\nWRITE NbTrans\t: "));Serial.print(NbTrans);
	Serial.print(F("\nWRITE RAW\t: "));Serial.print( ( temp_none << 4 ) | NbTrans );
#endif
}
#endif // ARDUINO_EEPROM == 1

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
	printNOWEB();Serial.print(F("WRITE NwkSEnc: "));printHex(key, 16);
#endif
}
#endif // LORAWAN_OTAA_ENABLED && ARDUINO_EEPROM == 0

// ARDUINO style EEPROM. I had problems with avr/eeprom.h with debugging.
// When added Serial.print commands to either sketch or library the avr/eeprom.h
// for unknown to me reason changes the locations of EEMEM variables.
// Sooooooo... Static addressing. :-(
//
// In fact it seems that it's bug on linker
// https://arduino.stackexchange.com/questions/93873/how-eemem-maps-the-variables-avr-eeprom-h
#if LORAWAN_OTAA_ENABLED && ARDUINO_EEPROM == 1
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
		temp = ( ( value << 7 ) & temp );
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
#endif // LORAWAN_OTAA_ENABLED && ARDUINO_EEPROM == 1
