#ifndef SLIM_LORA_H
#define SLIM_LORA_H

#if defined (__AVR__)
#include <avr/power.h>
#endif

// START OF USER DEFINED OPTIONS

// Region. Valid Value: EU863. NOT USED needs further work: US902, AS920, AU915
// https://github.com/TheThingsNetwork/lorawan-frequency-plans/
#define EU863

// NbTrans (re-transmissions). Normally 1 max is 15
// Olivier Seller for static devices proposes 4.
// See TTN conference Amsterdam 2024 at 7 minute presentation.
#ifndef NBTRANS
#define NBTRANS	1
#endif

// TTN or Helium. Don't change those values
#define NET_TTN		1
#define NET_HELIUM	2

// change this according to your network
#ifndef NETWORK
#define NETWORK NET_TTN	// Two options: NET_HELIUM = helium, NET_TTN = TheThingsNetwork
				// NET_TTN: RX2 SF9
				// NET_HLM: RX2 SF12
#endif

// TODO: https://github.com/Xinyuan-LilyGO/tbeam-helium-mapper/blob/00cec9c130d4452839dcf933905f5624d9711e41/main/main.cpp#L195
// Helium requires a FCount reset sometime before hitting 0xFFFF
// 50,000 makes it obvious it was intentional
// I think helium needs re-join. TODO: EVAL with chirpstack
#ifndef MAX_FCOUNT
// #define MAX_FCOUNT 50000
#endif

// Make sure this value is the same with TTN console.
#define NET_TTN_RX_DELAY	5

// By Default HELIUM uses 1 sec of delay, but join is always at 5 seconds
#define NET_HELIUM_RX_DELAY	5

// Select Arduino style EEPROM handling.
#ifndef ARDUINO_EEPROM
#define ARDUINO_EEPROM	1	// 1: Uses static EEPROM storage. It helps debugging and to restore session after power cycle / or reset.
				// 2: for external I2C EEPROM. You also need SparkFun External EEPROM library
#endif

// Save 332 bytes of RAM. Only for AVR's / ATmega's
#ifndef SLIMLORA_USE_PROGMEM
#define SLIMLORA_USE_PROGMEM
#endif

#if !defined(__AVR__)
#undef SLIMLORA_USE_PROGMEM
#endif

// Debug SlimLoRa library via Serial.print()
#ifndef DEBUG_SLIM
#define DEBUG_SLIM	0	// is basic debugging, 2 more debugging, 0 to disable.
#endif

// Identify RX / join window and store LNS DeviceTime and LinkCheck
// This adds 96 bytes of program flash and 1 byte of RAM. 112 bytes for SAMD.
//#define SLIM_DEBUG_VARS

// accurate epoch to second. If we received the epoch in RX2 window add one second to epoch.
//#define EPOCH_RX2_WINDOW_OFFSET
#if defined EPOCH_RX2_WINDOW_OFFSET && !defined SLIM_DEBUG_VARS
#error You enabled EPOCH_RX2_WINDOW_OFFSET without SLIM_DEBUG_VARS
#endif

// Enable LoRaWAN Over-The-Air Activation
#ifndef LORAWAN_OTAA_ENABLED
#define LORAWAN_OTAA_ENABLED    1
#endif

// Store the session data to EEPROM
#ifndef LORAWAN_KEEP_SESSION
#define LORAWAN_KEEP_SESSION    1 // needs 254 program flash for ATmega, 356 for SAMD.
				  // Don't disabled it. Unless you know what you are doing.
#endif

// Store counters every X times to protect EEPROM from constant writings
#ifndef EEPROM_WRITE_TX_COUNT
#define EEPROM_WRITE_TX_COUNT	200	// SlimLoRa default: 10 	clv: 200
#endif

#ifndef EEPROM_WRITE_RX_COUNT
#define EEPROM_WRITE_RX_COUNT	10	// SlimLoRa default: 3		clv: 10
#endif

// downlink size payload. You can gain some flash / RAM memory here
#ifndef SLIMLORA_DOWNLINK_PAYLOAD_SIZE
#define SLIMLORA_DOWNLINK_PAYLOAD_SIZE	12	// more than 51 bytes are impossible in SF12.
						// SF12 in Europe is 36 bytes (15 byte are MAC commands worst case scenario
						// I suggest 12 bytes. MAX is around 250 bytes for SF7
#endif

// LoRaWAN ADR
// https://lora-developers.semtech.com/documentation/tech-papers-and-guides/implementing-adaptive-data-rate-adr/implementing-adaptive-data-rate/
#ifndef LORAWAN_ADR_ACK_LIMIT
#define LORAWAN_ADR_ACK_LIMIT   164	// Request downlink after XX uplinks to verify we have connection.	Minimum sane value: 64
#endif

#ifndef LORAWAN_ADR_ACK_DELAY
#define LORAWAN_ADR_ACK_DELAY   32	// Wait XX times to consider connection lost.				Minimum sane value: 32
#endif

// if you you want to save 6 bytes of RAM and you don't need to provision the Duty Cycle
// because you transmitting only on high Data Rates (DR). You save 76 byte of flash memory if you comment this.
#ifndef COUNT_TX_DURATION
#define COUNT_TX_DURATION	1
#endif

// You gain 12 bytes of program flash if you comment this.
// Use it only WITHOUT ADR and only for experiments
// since SlimLoRa can't receive SF7BW250 downlinks.
// TTN does not want this. Helium is not supported.
//#define EU_DR6 // applicable for EU RU AS CN

// Comment this only if you don't changed the clock of your AVR MCU.
// with clock_prescale_set(clock_div_XX);
#define CATCH_DIVIDER

// Disable CATCH_DIVIDER for non-AVR MCU's
#if !defined (__AVR__)
#undef CATCH_DIVIDER
#endif

// Reduce LNA gain if DR is fast. Probably we are close to a gateway.
// I have seen minimal benefit, or it's bad implemented by the code.
//#define REDUCE_LNA

/* Drift adjustment.
  Default:	2 works with feather-32u4 TTN and helium at 5 seconds RX delay.
  Tested with Helium at SF7 feather-32u4
*/

/*  MegaBrick - no lower than 1 (room temperature)
 1: it works for 5s join SF7 and RX2 SF12 at 2s receive
 0: it works with Helium SF12 @ 2secs - MegaBrick
 0: DOES NOT joins at 5s SF7 - MegaBrick
 With clock_div_4 DRIFT 1 fails to receive downlinks.
*/
#ifndef SLIMLORA_DRIFT
#define SLIMLORA_DRIFT		2
#endif

// Uncomment this to enable MAC requests for TimeReq and LinkCheck (margin, gateway count)
// This needs 397 of Program Flash and 9 bytes of RAM
#define MAC_REQUESTS

// default is 64. That means 51 bytes of LoRaWAN payload for SF10, SF11, SF12.
// If you send or receive MAC commands along with big payloads
// expect buffer overflows! Maximum for frame options is 15 bytes
// So you can expect 51 - 15 - 9 LoRaWAN headers = 27 bytes for
// worst case scenario.
// Without MAC commands is 51 - 9 = 45 bytes for SF10-SF12
// Without MAC commands is 64 - 9 = 55 bytes for SF7-9
#ifndef SLIMLORA_UPLINK_PACKET_SIZE
#define SLIMLORA_UPLINK_PACKET_SIZE	64 // Safest value around 255 bytes
#endif

#define DYNAMIC_ADR_ACK_LIMIT	// If you want change dynamically ACK_LIMIT via variable: adr_ack_limit
#define ATOMIC_ENABLE		// COST: 64 bytes Uncomment if you trust your code and there is not interruption when transmitting.

#ifndef ENABLE_CONF_UPLINKS
#define ENABLE_CONF_UPLINKS	0 // At the expense of 26 bytes of Program Flash and one byte of SRAM
#endif

// END OF USER DEFINED OPTIONS

// SlimLoRa needs EEPROM or fails. TODO: && with KEEP_SESSION
#if !defined (__AVR__)
#define ARDUINO_EEPROM 2
#endif

#if (ARDUINO_EEPROM == 1 || ARDUINO_EEPROM == 0) && !defined (__AVR__)
#error You defined internal ARDUINO_EEPROM but you dont have an AVR / ATmega
#endif

#if defined SLIMLORA_USE_PROGMEM && !defined (__AVR__)
#error You defined SLIMLORA_USE_PROGMEM but you dont have an AVR / ATmega
#endif

#if ARDUINO_EEPROM == 0
	#include <avr/eeprom.h>
#endif

#if ARDUINO_EEPROM == 1
	#include <EEPROM.h>
#endif

#if ARDUINO_EEPROM == 2

	#include <Wire.h>
	#include "SparkFun_External_EEPROM.h"

	extern ExternalEEPROM EEPROM;
        #define put putChanged // Arduino put method uses update.
			       // SparkFun EXTERNAL EEPROM does not have update
			       // function but putChanged.
        #define update putChanged // Arduino put method uses update.

	#ifndef SLIMLORA_EEPROM_MEMORY_TYPE
	#define SLIMLORA_EEPROM_MEMORY_TYPE	2	// TODO: this is valid only for 2kbit EEPROM (512 bytes)
	#endif

	#ifndef EXTERNAL_EEPROM_ADDRESS
	#define EXTERNAL_EEPROM_ADDRESS      0x50	// 24AA02E64 EERPROM of MightyBrick EEPROM listes from HEX: 0x50 to 0x57
	#endif

	#ifndef EXTERNAL_EEPROM_EUI_ADDRESS
	#define EXTERNAL_EEPROM_EUI_ADDRESS  0xF8	// M24AA02E6 EEPROM
	#endif

#endif

#ifndef DEBUG_RXSYMBOLS
#define DEBUG_RXSYMBOLS 0	// Masked 1 = duration, 2 breaks timing with debug prints
#endif

// Arduino library of eeprom is simpler / with less functionality than avr/eeprom.h
// It needs extra work. We need to define the address of each data.
// It's better for future firmware updates. Data remains in same place in contrast of avr/eeprom.h
#if ARDUINO_EEPROM >= 1
	#ifndef EEPROM_OFFSET
	#define EEPROM_OFFSET		  0	// Change this from 0 to EEPROM size - EEPROM_END if you feel 
						// that you gonna burn the EEPROM to use another area
						// of EEPROM
	#endif

								// EEPROM reliability for AVR's. Around 1.000.000 writes.
	#define EEPROM_DEVADDR		  0 + EEPROM_OFFSET	// 4 bytes array
	#define EEPROM_TX_COUNTER	  4 + EEPROM_OFFSET	// 4 bytes but in practice 2 bytes: future proof 4 bytes
	#define EEPROM_RX_COUNTER	  8 + EEPROM_OFFSET	// 4 bytes but in practice 2 bytes: future proof 4 bytes
	#define EEPROM_RX1DR_OFFSET	 12 + EEPROM_OFFSET	// 1 byte but I need 3 bits (decimal 7 [0-3]
	#define EEPROM_JOINED		 12 + EEPROM_OFFSET	// SAME ADDRESS WITH EEPROM_RX1DR_OFFSET: 1 bit (byte) [7]
	#define EEPROM_RX2_DR		 13 + EEPROM_OFFSET	// 1 byte but I need 4 bits (decimal 15) [0-3]
	#define EEPROM_RX_DELAY		 13 + EEPROM_OFFSET	// SAME ADDRESS WITH EEPROM_RX2_DR. Nibble. I need 4 bits (decimal 15) [4-7]
	#define EEPROM_DEVNONCE		 14 + EEPROM_OFFSET	// 2 bytes
	#define EEPROM_JOINNONCE	 16 + EEPROM_OFFSET	// 4 bytes
	#define EEPROM_APPSKEY		 20 + EEPROM_OFFSET	// 16 bytes array
	#define EEPROM_FNWKSIKEY	 36 + EEPROM_OFFSET	// 16 bytes array
	#define EEPROM_SNWKSIKEY	 52 + EEPROM_OFFSET	// 16 bytes array
	#define EEPROM_NW_ENC_KEY	 68 + EEPROM_OFFSET	// 16 bytes array
	#define EEPROM_NBTRANS		 84 + EEPROM_OFFSET	// 4 bits [0-15]. 4 BITS to spare
	#define EEPROM_CHMASK		 85 + EEPROM_OFFSET	// 2 bytes
	#define EEPROM_NETID		 87 + EEPROM_OFFSET	// 4 bytes
	#define EEPROM_END		 90 + EEPROM_OFFSET	// last byte of SlimLoRa on EEPROM
#endif

#define MICROS_PER_SECOND               1000000
#define OVERFLOW_MILLIS 		86400000	// overflow timer (millis) every 49 days and 17 hours.

// RFM registers
#define RFM_REG_FIFO                    0x00
#define RFM_REG_OP_MODE                 0x01
#define RFM_REG_FR_MSB                  0x06
#define RFM_REG_FR_MID                  0x07
#define RFM_REG_FR_LSB                  0x08
#define RFM_REG_PA_CONFIG               0x09
#define RFM_REG_PA_DAC                  0x4D ///<PA Higher Power Settings
#define RFM_REG_OCP_TRIM		0x0B
#define RFM_REG_LNA			0x0C
#define RFM_REG_FIFO_ADDR_PTR           0x0D
#define RFM_REG_FIFO_TX_BASE_ADDR       0x0E
#define RFM_REG_FIFO_RX_BASE_ADDR       0x0F
#define RFM_REG_FIFO_RX_CURRENT_ADDR    0x10
#define RFM_REG_IRQ_FLAGS_MASK          0x11
#define RFM_REG_IRQ_FLAGS               0x12
#define RFM_REG_RX_NB_BYTES             0x13
#define RFM_REG_PKT_SNR_VALUE           0x19
#define RFM_REG_PKT_RSSI_VALUE          0x1A
#define RFM_REG_MODEM_CONFIG_1          0x1D
#define RFM_REG_MODEM_CONFIG_2          0x1E
#define RFM_REG_SYMB_TIMEOUT_LSB        0x1F
#define RFM_REG_PREAMBLE_MSB            0x20
#define RFM_REG_PREAMBLE_LSB            0x21
#define RFM_REG_PAYLOAD_LENGTH          0x22
#define RFM_REG_MODEM_CONFIG_3          0x26
// Only in SX1276 datasheet
#define RFM_REG_IF_FREQ_2               0x2F
#define RFM_REG_IF_FREQ_1               0x30
#define RFM_REG_DETECT_OPTIMIZE         0x31
#define RFM_REG_INVERT_IQ               0x33
#define RFM_REG_SYNC_WORD               0x39
#define RFM_REG_INVERT_IQ_2             0x3B

// Temperature register and low battery accessible via FSK mode
#define RFM_REG_FSK_IMAGE_CAL		0x3B
#define RFM_REG_FSK_TEMP		0x3C
#define RFM_REG_FSK_LOW_BAT		0x3D

// RFM status
#define RFM_STATUS_TX_DONE              0x08
#define RFM_STATUS_RX_DONE              0x40
#define RFM_STATUS_RX_DONE_CRC_ERROR    0x60
#define RFM_STATUS_RX_TIMEOUT           0x80

// RFM vars
#define RFM_OCP_TRIM_OFF		0x00
#define RFM_OCP_TRIM_ON			0x20

#define RFM_ERROR_RX_TIMEOUT    -1
#define RFM_ERROR_CRC           -2
#define RFM_ERROR_UNKNOWN       -3

#define RFM_FREQ_MULTIPLIER	61035

// LoRaWAN
#define LORAWAN_MTYPE_JOIN_REQUEST          0x00
#define LORAWAN_MTYPE_JOIN_ACCEPT           0x20
#define LORAWAN_MTYPE_UNCONFIRMED_DATA_UP   0x40
#define LORAWAN_MTYPE_UNCONFIRMED_DATA_DOWN 0x60
#define LORAWAN_MTYPE_CONFIRMED_DATA_UP     0x80
#define LORAWAN_MTYPE_CONFIRMED_DATA_DOWN   0xA0
#define LORAWAN_MTYPE_RFU                   0xC0
#define LORAWAN_MTYPE_PROPRIETARY           0xE0

#define LORAWAN_FCTRL_ADR                   0x80
#define LORAWAN_FCTRL_ADR_ACK_REQ           0x40
#define LORAWAN_FCTRL_ACK                   0x20

#define LORAWAN_DIRECTION_UP                0
#define LORAWAN_DIRECTION_DOWN              1

#define LORAWAN_UPLINK_CHANNEL_COUNT        8 // Valid Value only 8.
					      // In future downlink channel must move to another index - not 8 or in another variable.

// LoRaWAN epoch
#define	LORAWAN_EPOCH_DRIFT			18 		// Leap Seconds since 1980
#define LORAWAN_DEVICE_TIME_FRACTIONAL_STEPS	0.00390625	// p. 32 aka: 0.5^8

// LoRaWAN frame options
#define LORAWAN_FOPT_LINK_CHECK_REQ         0x02
#define LORAWAN_FOPT_LINK_CHECK_ANS         0x02
#define LORAWAN_FOPT_LINK_ADR_REQ           0x03
#define LORAWAN_FOPT_LINK_ADR_ANS           0x03
#define LORAWAN_FOPT_DUTY_CYCLE_REQ         0x04
#define LORAWAN_FOPT_DUTY_CYCLE_ANS         0x04
#define LORAWAN_FOPT_RX_PARAM_SETUP_REQ     0x05
#define LORAWAN_FOPT_RX_PARAM_SETUP_ANS     0x05
#define LORAWAN_FOPT_DEV_STATUS_REQ         0x06
#define LORAWAN_FOPT_DEV_STATUS_ANS         0x06
#define LORAWAN_FOPT_NEW_CHANNEL_REQ        0x07
#define LORAWAN_FOPT_NEW_CHANNEL_ANS        0x07
#define LORAWAN_FOPT_RX_TIMING_SETUP_REQ    0x08
#define LORAWAN_FOPT_RX_TIMING_SETUP_ANS    0x08
#define LORAWAN_FOPT_TX_PARAM_SETUP_REQ     0x09
#define LORAWAN_FOPT_TX_PARAM_SETUP_ANS     0x09
#define LORAWAN_FOPT_DL_CHANNEL_REQ         0x0A
#define LORAWAN_FOPT_DL_CHANNEL_ANS         0x0A
#define LORAWAN_FOPT_DEVICE_TIME_REQ        0x0D
#define LORAWAN_FOPT_DEVICE_TIME_ANS        0x0D

// LoRaWAN frame option payload size
#define LORAWAN_FOPT_LINK_CHECK_ANS_SIZE        2
#define LORAWAN_FOPT_LINK_ADR_REQ_SIZE          4
#define LORAWAN_FOPT_DUTY_CYCLE_REQ_SIZE        1
#define LORAWAN_FOPT_RX_PARAM_SETUP_REQ_SIZE    4
#define LORAWAN_FOPT_DEV_STATUS_REQ_SIZE        0
#define LORAWAN_FOPT_NEW_CHANNEL_REQ_SIZE       5
#define LORAWAN_FOPT_RX_TIMING_SETUP_REQ_SIZE   1
#define LORAWAN_FOPT_TX_PARAM_SETUP_REQ_SIZE    1
#define LORAWAN_FOPT_DL_CHANNEL_REQ_SIZE        4
#define LORAWAN_FOPT_DEVICE_TIME_ANS_SIZE       5
#define LORAWAN_PORT_SIZE			1
#define LORAWAN_MIC_SIZE			4
#define LORAWAN_MAC_AND_FRAME_HEADER		8 // MAC Header is 1 byte. Frame header is 7..22 bytes
#define LORAWAN_START_OF_FRM_PAYLOAD		9

// LoRaWAN Join packet sizes
#define LORAWAN_JOIN_REQUEST_SIZE           18
#define LORAWAN_JOIN_ACCEPT_MAX_SIZE        28

// LoRaWAN delays in seconds
#define RX_SECOND_WINDOW SF12BW125

// Usefull for ABP devices only. OTAA devices grab RX2 window DR in after join.
#if NETWORK == NET_TTN
	#define RX_SECOND_WINDOW SF9BW125
	#define LORAWAN_JOIN_ACCEPT_DELAY1_MICROS   NET_TTN_RX_DELAY       * MICROS_PER_SECOND
	#define LORAWAN_JOIN_ACCEPT_DELAY2_MICROS   (NET_TTN_RX_DELAY + 1) * MICROS_PER_SECOND
#endif

#if NETWORK == NET_HELIUM
	#define RX_SECOND_WINDOW SF12BW125
	#define LORAWAN_JOIN_ACCEPT_DELAY1_MICROS   NET_HELIUM_RX_DELAY       * MICROS_PER_SECOND
	#define LORAWAN_JOIN_ACCEPT_DELAY2_MICROS   (NET_HELIUM_RX_DELAY + 1) * MICROS_PER_SECOND
#endif

#define LORAWAN_RX_ERROR_MICROS             10000   // 10 ms
#define LORAWAN_RX_MARGIN_MICROS            2000    // 2000 us
#define LORAWAN_RX_SETUP_MICROS             2000    // 2000 us
#define LORAWAN_RX_MIN_SYMBOLS              6
#define LORAWAN_RX_MAX_SYMBOLS              1023    // MSB are located in register RFM_REG_MODEM_CONFIG_2 [0-1]
						    // original was 255

#define LORAWAN_FOPTS_MAX_SIZE              10
#define LORAWAN_STICKY_FOPTS_MAX_SIZE       6

// EU868 region settings
#define LORAWAN_EU868_TX_POWER_MAX          7
#define LORAWAN_EU868_RX1_DR_OFFSET_MAX     5

// IN865 region settings
#define LORAWAN_IN865_TX_POWER_MAX          10
//#define LORAWAN_IN865_RX1_DR_OFFSET_MAX     5

// US902 region settings
#define LORAWAN_US902_TX_POWER_MAX          14
//#define LORAWAN_US902_RX1_DR_OFFSET_MAX     5

// LoRaWAN Error
#define LORAWAN_ERROR_NO_PACKET_RECEIVED    -1
#define LORAWAN_ERROR_SIZE_EXCEEDED         -2
#define LORAWAN_ERROR_UNEXPECTED_MTYPE      -3
#define LORAWAN_ERROR_INVALID_FRAME_COUNTER -4
#define LORAWAN_ERROR_UNEXPECTED_DEV_ADDR   -5
#define LORAWAN_ERROR_INVALID_MIC           -6
#define LORAWAN_ERROR_INVALID_JOIN_NONCE    -7

// LoRaWAN spreading factors
// TODO for other regions. Example: DR0 for US902 is SF10BW125 and DR8 is SF12BW500
// check https://www.thethingsnetwork.org/docs/lorawan/regional-parameters/
//#define FSK	    7 // TODO only 868.8 Mhz
#define SF7BW250    6 // only 868.3 Mhz
#define SF7BW125    5
#define SF8BW125    4
#define SF9BW125    3
#define SF10BW125   2
#define SF11BW125   1
#define SF12BW125   0

// SlimLoRa debug values for LoRaWANreceived
#define SLIMLORA_JOINED_WINDOW1		0x01
#define SLIMLORA_JOINED_WINDOW2		0x02
#define SLIMLORA_MAC_PROCESSING		0x04
#define SLIMLORA_DOWNLINK_RX1		0x08
#define SLIMLORA_DOWNLINK_RX2		0x10
#define SLIMLORA_LINK_CHECK_ANS		0x20
#define SLIMLORA_LNS_TIME_ANS		0x40

typedef struct {
    uint8_t length;
    uint8_t fopts[LORAWAN_FOPTS_MAX_SIZE];
} fopts_t;

typedef struct {
    uint8_t length;
    uint8_t fopts[LORAWAN_STICKY_FOPTS_MAX_SIZE];
} sticky_fopts_t;

class SlimLoRa {
  public:
    SlimLoRa(uint8_t pin_nss); // TODO: TinyLoRa rfm_dio0 (7), rfm_nss (8), rfm_rst (4), bat pin? EEPROM?
    void Begin(void);
    void sleep(void);
    bool HasJoined(void);
    int8_t Join();
    void SendData(uint8_t fport, uint8_t *payload, uint8_t payload_length);
    void SetAdrEnabled(bool enabled);
    void SetDataRate(uint8_t dr);
    uint8_t GetDataRate();
    void SetPower(uint8_t power);
    bool GetHasJoined();
    void GetDevAddr(uint8_t *dev_addr);

    // ADR variables
    bool 	adr_enabled_;
    uint8_t	adr_ack_limit_counter_;
    uint8_t	adr_ack_delay_counter_;

    uint8_t data_rate_;
    uint16_t tx_frame_counter_;
    uint16_t rx_frame_counter_;
    uint8_t NbTrans = NBTRANS;	// changed by the LNS or by DEFINE
    uint8_t NbTrans_counter;
    uint8_t pseudo_byte_;
    uint8_t tx_power;

    uint16_t GetTxFrameCounter();
    void SetTxFrameCounter();
    void SetRxFrameCounter();

    // MAC Request variables
#ifdef MAC_REQUESTS
    uint8_t TimeLinkCheck;
    uint32_t epoch;
    uint8_t fracSecond;
    uint8_t margin, GwCnt; // For LinkCheckAns
#endif

#if COUNT_TX_DURATION == 1
    uint16_t slimLastTXms, slimTotalTXms;
#endif
#if COUNT_TX_DURATION == 1
    uint16_t GetTXms();
    void    ZeroTXms();
#endif // COUNT_TX_DURATION

#if ARDUINO_EEPROM >= 1
    void getArrayEEPROM(uint16_t eepromAdr, uint8_t *arrayData, uint8_t size);
    void setArrayEEPROM(uint16_t eepromAdr, uint8_t *arrayData, uint8_t size);
    void printHex(uint8_t *value, uint8_t len);
#endif

	uint8_t downlinkData[SLIMLORA_DOWNLINK_PAYLOAD_SIZE];
	uint8_t downlinkSize;
	uint8_t downPort;

#if DEBUG_SLIM >= 1
	void printMAC();
	void printDownlink();
	uint8_t packet[SLIMLORA_UPLINK_PACKET_SIZE];
	int8_t packet_length;
	uint8_t f_options_length, payload_length;
#endif

#ifdef SLIM_DEBUG_VARS
	uint8_t LoRaWANreceived;
#endif

#if DEBUG_RXSYMBOLS >= 2
	uint16_t rx_symbolsB;
#endif

#ifdef DYNAMIC_ADR_ACK_LIMIT
	uint8_t adr_ack_limit = LORAWAN_ADR_ACK_LIMIT;
#endif

#if DEBUG_SLIM == 0 // if not debuging, those are private. If debugging everything is public
  private:
#else
  public:
#endif
    uint8_t pin_nss_;	// TODO TinyLoRa irg_, rst_ bat_; bat=battery level pin, EEPROM start
    uint8_t channel_;
    uint8_t rx1_data_rate_offset_;
    uint8_t rx2_data_rate_ = RX_SECOND_WINDOW;
    uint32_t rx1_delay_micros_;
    bool has_joined_	= false;
    bool ack_		= false;

#if ENABLE_CONF_UPLINKS == 1
    bool upToAck_	= false;
#endif

    fopts_t pending_fopts_ = {0};
    fopts_t sticky_fopts_ = {0};
    uint16_t rx_symbols_ = LORAWAN_RX_MIN_SYMBOLS;
    unsigned long tx_done_micros_;
    int8_t last_packet_snr_;

#if DEBUG_SLIM >= 1
    int8_t last_packet_snrB;
#endif

    uint16_t ChMask;

    static const uint8_t kFrequencyTable[9][3];
    static const uint8_t kDataRateTable[7][3];
    static const uint32_t kDRMicrosPerHalfSymbol[7];
    static const uint8_t kSTable[16][16];
    int8_t RfmReceivePacket(uint8_t *packet, uint8_t packet_max_length, uint8_t channel, uint8_t dri, uint32_t rx_tickstamp);
    void RfmSendPacket(uint8_t *packet, uint8_t packet_length, uint8_t channel, uint8_t dri);
    void RfmWrite(uint8_t address, uint8_t data);
    uint8_t RfmRead(uint8_t address);
    uint32_t CalculateDriftAdjustment(uint32_t delay, uint16_t micros_per_half_symbol);
    int32_t CalculateRxWindowOffset(int16_t micros_per_half_symbol);
    uint32_t CalculateRxDelay(uint8_t data_rate, uint32_t delay);
    bool CheckMic(uint8_t *cmic, uint8_t *rmic);
    bool ProcessJoinAccept1_0(uint8_t *rfm_data, uint8_t rfm_data_length);
    bool ProcessJoinAccept1_1(uint8_t *rfm_data, uint8_t rfm_data_length);
    int8_t ProcessJoinAccept(uint8_t window);
    void ProcessFrameOptions(uint8_t *options, uint8_t f_options_length);
    int8_t ProcessDownlink(uint8_t window);
    void Transmit(uint8_t fport, uint8_t *payload, uint8_t payload_length);
    void defaultChannel();
    void wait_until(unsigned long microsstamp);
#if COUNT_TX_DURATION == 1
    // Variables to calculate TX time in ms
    uint32_t slimStartTXtimestamp, slimEndTXtimestamp;
    void CalculateTXms();
#endif
    void SetCurrentLimit(uint8_t currentLimit);
    // Encryption
    void EncryptPayload(uint8_t *payload, uint8_t payload_length, unsigned int frame_counter, uint8_t direction);
    void CalculateMic(const uint8_t *key, uint8_t *data, uint8_t *initial_block, uint8_t *final_mic, uint8_t data_length);
    void CalculateMessageMic(uint8_t *data, uint8_t *final_mic, uint8_t data_length, unsigned int frame_counter, uint8_t direction);
    void GenerateKeys(const uint8_t *key, uint8_t *key1, uint8_t *key2);
    void ShiftLeftData(uint8_t *data);
    void XorData(uint8_t *new_data, uint8_t *old_data);
    void AesEncrypt(const uint8_t *key, uint8_t *data);
    void AesAddRoundKey(uint8_t *round_key, uint8_t (*state)[4]);
    uint8_t AesSubByte(uint8_t byte);
    void AesShiftRows(uint8_t (*state)[4]);
    void AesMixCollums(uint8_t (*state)[4]);
    void AesCalculateRoundKey(uint8_t round, uint8_t *round_key);

    // EEPROM
    uint16_t GetRxFrameCounter();
    void SetRxFrameCounter(uint16_t count);
    uint8_t GetRx1DataRateOffset();
    void SetRx1DataRateOffset(uint8_t value);
    uint8_t GetRx2DataRate();
    void SetRx2DataRate(uint8_t value);
    uint8_t GetRx1Delay();
    void SetRx1Delay(uint8_t value);
    void GetChMask();
    void SetChMask();
    void GetNbTrans();
    void SetNbTrans();

#if LORAWAN_KEEP_SESSION
    void SetHasJoined(bool value);
#endif // LORAWAN_KEEP_SESSION
    void SetDevAddr(uint8_t *dev_addr);
    uint16_t GetDevNonce();
    void SetDevNonce(uint16_t dev_nonce);
    uint32_t GetJoinNonce();
    void SetJoinNonce(uint32_t join_nonce);
    void GetAppSKey(uint8_t *key);
    void SetAppSKey(uint8_t *key);
    void GetFNwkSIntKey(uint8_t *key);
    void SetFNwkSIntKey(uint8_t *key);
    void GetSNwkSIntKey(uint8_t *key);
    void SetSNwkSIntKey(uint8_t *key);
    void GetNwkSEncKey(uint8_t *key);
    void SetNwkSEncKey(uint8_t *key);
};

#endif
