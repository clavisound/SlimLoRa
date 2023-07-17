#define HLM // TTN or HLM (helium) MSB values

// JS DECODER for Helium or TTN
/*

function Decoder(bytes, port) {
var decoded = {};
decoded.battery = bytes[0];
return decoded;
}

 */

#if LORAWAN_OTAA_ENABLED
  #ifdef HLM
  uint8_t DevEUI[8]             = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // You need to add those values.
  uint8_t /*AppEUI*/ JoinEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // You need to add those values.
  uint8_t AppKey[16]            = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // You need to add those values.
  // For LoRaWAN-1.1
  // uint8_t NwkKey[16]            = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // You need to add those values.
  #endif

  #ifdef TTN
  uint8_t DevEUI[8]             = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // You need to add those values.
  uint8_t /*AppEUI*/ JoinEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // You need to add those values.
  uint8_t AppKey[16]            = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // You need to add those values.
  // For LoRaWAN-1.1
  // uint8_t NwkKey[16]            = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // You need to add those values.
  #endif // ifdef HLM
#else // ABP
  uint8_t NwkSKey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // You need to add those values.
  uint8_t AppSKey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // You need to add those values.
  uint8_t DevAddr[4]  = { 0x00, 0x00, 0x00, 0x00 };
#endif // LORAWAN_OTAA_ENABLED
