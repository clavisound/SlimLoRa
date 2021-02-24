#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// LoRaWAN ADR
#define LORAWAN_ADR_ACK_LIMIT   48
#define LORAWAN_ADR_ACK_DELAY   16

// Enable LoRaWAN Over-The-Air Activation
#define LORAWAN_OTAA_ENABLED    1
#define LORAWAN_KEEP_SESSION    0

#if LORAWAN_OTAA_ENABLED
const uint8_t DevEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t /*AppEUI*/ JoinEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t AppKey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#else
const uint8_t NwkSKey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t AppSKey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t DevAddr[4] = { 0x00, 0x00, 0x00, 0x00 };
#endif // LORAWAN_OTAA_ENABLED

#endif
