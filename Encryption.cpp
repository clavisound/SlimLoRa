
#include <Arduino.h>
#include <string.h>
#include "SlimLoRa.h"

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
void SlimLoRa::EncryptPayload(uint8_t *payload, uint8_t payload_length, uint32_t frame_counter, uint8_t direction) {
	uint8_t block_count = 0;
	uint8_t incomplete_block_size = 0;

	uint8_t block_a[16];

#if LORAWAN_OTAA_ENABLED
	uint8_t dev_addr[4], app_s_key[16]; //, nwk_s_key[16];
	GetDevAddr(dev_addr);
	// special case. We have to use NwkSKey for downlink in port 0.
	if ( downPort == 0 && direction == LORAWAN_DIRECTION_DOWN ) {
		GetNwkSEncKey(app_s_key);
	} else {
		GetAppSKey(app_s_key);
	}
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
		block_a[12] = frame_counter >> 16; // Frame counter upper bytes
		block_a[13] = frame_counter >> 24;

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
void SlimLoRa::CalculateMessageMic(uint8_t *data, uint8_t *final_mic, uint8_t data_length, uint32_t frame_counter, uint8_t direction) {
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
	block_b[11] = (frame_counter >> 8) & 0xFF;
	block_b[12] = (frame_counter >> 16) & 0xFF; // Frame counter upper bytes
	block_b[13] = (frame_counter >> 24) & 0xFF;

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
#if defined (__AVR__) && defined SLIMLORA_USE_PROGMEM
	return pgm_read_byte(&(kSTable[((byte >> 4) & 0x0F)][((byte >> 0) & 0x0F)]));
#else
	return kSTable[((byte >> 4) & 0x0F)][((byte >> 0) & 0x0F)];
#endif
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
