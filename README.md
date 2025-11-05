![CI](https://github.com/clavisound/SlimLoRa/actions/workflows/main.yml/badge.svg)

# SlimLoRa - Probably the easiest and smallest footprint LoRaWAN library for Arduino library and EU868.

This library is probably the most easy to use LoRaWAN library. The target is LoRaWAN-1.0.3 specification. It supports OTAA / Join, most important MAC commands - like DR, power, NBtrans, downlinks for user application and session is stored to EEPROM. Applications downlinks are static selectable via `#define` in `SlimLoRa.h`. Default is 11 bytes. If you want a complete LoRaWAN library try [Radiolib](https://github.com/jgromes/RadioLib/) (needs ~52kBytes of program flash, ~64kBytes for SAMD), or LMIC (around 36kBytes of program flash).

SlimLoRa needs around 12558 Bytes* (13kBytes). SlimLoRa gives LoRaWAN life to old μCUs like the ATmega 328 with 32kBytes of flash. For SAMD with an external I2C EEPROM, it needs ~32kB. The price of 32-bits?

\* in fact, I think the overhead is around 9kBytes.

[SlimLoRa MAC response in MAC command via Helium original console.](https://krg.etraq.eu/minisites/lora/mac-command-response_crop.png)

The majority of the work was done by Hendrik Hagendorn and Ideetron B.V. Thanks to both of them. I ported the library to Arduino, expanded the most important MAC commands, enabled application downlinks, enabled ACKs for confirmed downlinks, and corrected the channel for SF7BW250.

# Working

- [x] OTAA join with Feather 32u4 EU868 region. Europe only.
  - [x] TTN joins - Feather32u4
    - [x] Join SF9 on TTN and power 0dBm in a different room. Success on 1st window.
    - [x] Join SF8 on TTN and power 0dBm in the same room. Success on 1st window.
    - [x] Join SF7 on TTN and power 0dBm in a different room. Success on 1st window.
      - [x] Megabrick joins
        - [ ] FAIL with SF7 and power 13dBm in a different room and an antenna.
        - [x] Join with SF9 and power 13dBm in a different room and an antenna.
  - [x] Helium Joins - Feather32u4
    - [x] old console
      - [x] Join SF10 with Helium. Success every time on 1st window but not on the first attempt.
    - [x] chirpstack Console
      - [x] Join SF9 on Helium chirpstack outdoors. Success on 1st window.
      - [x] Join SF8 on Helium chirpstack outdoors. Success on 1st window in the second or third attempt.
      - [x] Join SF7 on Helium and power 0dBm in a different room. Success on 1st window.
        - [x] Megabrick joins
          - [x] Join with SF7 and GW power 16dBm in a different room and an antenna.
          - [x] Join with SF8 and GW power 16dBm in a different room and an antenna.
- [x] Downlinks
  - [x] Helium on 2nd window (SF12) always works on Chirpstack.
- [x] SetPower
- [x] Deep Sleep
- [x] Restore session from EEPROM (Arduino style)
- [x] Downlink for application.
- [x] ACK to confirmed Downlink.
- [x] NbTrans - edit SlimLoRa.h to configure.
- [x] Most important MAC commands - more to follow
- [x] Session saved to EEPROM. You only need to join once in the lifetime of the device.

# MAC commands supported.

- [x] ADR for Data Rate and TxPower. After joining with SF9 in a different room from the gateway (RSSI -85), TTN sends an SF7 (or SF8) ADR command and SlimLoRa conforms.
- [x] NbTrans
- [x] Channel Mask
- [x] RxTimingSetup
- [x] DevStatusAns. Battery status is working, I think the margin is fine.
- [x] LinkCheckReq. (Margin and Gateways count).
- [x] DeviceTimeReq. Fractional second on purpose is truncated to cs (centiseconds). 1 = 10ms. I can't understand how precise the timing from the LNS is. I have to verify this with GPS. The TTN RX window is 5 seconds, and the Helium RX window is 1 second. So the epoch is off by 5 and 1 seconds, respectively. If you want to correct the epoch, uncomment both `#define SLIM_DEBUG_VARS` and `#define EPOCH_RX2_WINDOW_OFFSET` at the cost of 1768 bytes of program flash and 64 fewer bytes of RAM.

# BUGS

- [ ] SF7BW250 is working with TTN, but SlimLoRa does not receive the downlink. This DR6 is currently not supported by the Helium packet broker.

# Untested

- [ ] ABP
- [ ] AVR style EEPROM

## EEPROM handling to consider
I recommend `ARDUINO_EEPROM == 1` in `SlimLoRa.h` otherwise when you use `avr/eeprom.h` style and you compile with different options, or if you change part of your sketch relative to EEPROM (EEMEM) the **addresses of the data change!** This is a "[bug](https://arduino.stackexchange.com/a/93879/59046)" on avr/eeprom.h. With the AVR style, if you change your sketch, the behavior of SlimLoRa is unpredictable. You need to re-join. With the Arduino EEPROM style, the behavior is predictable, and you don't need to re-join.

Solutions with AVR style.

- Solution #1: Erase ALL the EEPROM after uploading the new firmware.
- Solution #2: Hint: You can track the EEPROM addresses with: `avr-objdump -D` on the .eemem section.
- Solution #3: Don't enable keep session.
- Solution #4: use Arduino style EEPROM in `SlimLoRa.h`

## SAMD EEPROM

SAMD series don't have EEPROM like ATmegas, so if you have an external I2C EEPROM you can define in `SlimLoRa.h`: `#define ARDUINO_EEPROM 2` to enable the external EEPROM. In that case, you need the [SparkFun External EEPROM](https://github.com/sparkfun/SparkFun_External_EEPROM_Arduino_Library/) library. Make sure that other values like `#define SLIMLORA_EEPROM_MEMORY_TYPE` and others are properly defined. Check the example SlimLoRa-battery-feather32u4-SAMD-complicated.

You also need to start I2C and several other EEPROM settings like `Wire.begin();
  Wire.setClock(400000);
  EEPROM.setMemoryType(SLIMLORA_EEPROM_MEMORY_TYPE);`

# Semi-Working

- [x] `ADR_ACK_LIMIT`, must follow the directive on p. 17 based on `ADR_ACK_DELAY`
- [x] Duty Cycle. Added GetTXms function to return the TOTAL duration of ALL transmissions. At SF7 1byte reports 45-48ms vs 46ms [theoretical](https://avbentem.github.io/airtime-calculator/ttn/eu868/1) at SF8 reports 84ms vs 82ms (theoretical). SF7 5 bytes reports 52ms vs 51.5ms (theoretical). The application HAS to read the value of GetTXms() after every transmission to check if the Duty Cycle is respected. I decided to not respect Duty Cycle on SlimLoRa, since if the device is going to Deep Sleep and wakes up via an accelerometer on AVR MCUs it freezes the timer0. I think the solution is the RTC or to read a time from GPS.
- [x] Power to the people. Several values made public. Be careful not to write to them, or you may lose access to the network. Instead of using getters, I selected to make some variables public.
- [x] MAC Commands.

# TODOs (PRs welcome) - In order of importance.

- [ ] MAC command `NEW_CHANNEL_REQ` is not properly implemented. SlimLoRa responds with "fine" to help the LNS stop sending downlinks, but in reality, it does not modify the channels and DRs.
- [ ] Join back-off
- [ ] Extern variable for Duty Cycle if the application can provide time.
- [ ] Confirmed Uplink
- [ ] Make DevNonce random.
- [ ] More regions. Currently, only EU868 is working.
- [ ] Add pin mappings infrastructure for other boards.
- [ ] Add compile options for battery status (unable to measure, connected to external power)
- [ ] Random delay for TX.
- [ ] Duty Cycle per channel.
- [ ] Respect Dwell MAC command (only for US902?)
- [ ] CFlist only for US and AS?

# Clock Divider support

If you downclock your AVR MCU with `clock_prescale_set(clock_div_4)` SlimLoRa can adapt the RX timing window to receive the downlinks. Even with `clock_prescale_set(clock_div_32)` at 500KHz, a MegaBrick joins at SF7. Uncomment `#define CATCH_DIVIDER` to enable this function.

# Undoable on AVR and Deep Sleep

- [x] Respect Duty Cycle
Since the AVR's timer0 freezes during Deep Sleep, SlimLoRa is unable to keep track of time.
- [x] Respect Join Back-off (not faster than 36 seconds). Must be handled by the application. In the case of Deep Sleep, SlimLoRa can't keep track of the time.

# Maybe good ideas

- [ ] 32-bit Frame Counter.

# How to use it (mini-tutorial)

Download the library and extract it to the Arduino/libraries folder or install it via the library manager of the Arduino IDE. For manual installation: Rename SlimLoRa-master or SlimLoRa-VERSION to SlimLoRa. Read the details in the examples. If something goes wrong, erase ALL the EEPROM and retry the join.

SlimLoRa changes the payload data. Don't use payload data for program logic.

You can monitor the duty cycle with the function `GetTXms()` after every transmission. SlimLoRa will return the duration in ms of the LAST transmission. You have to add this to a variable in your program to keep track of the Duty Cycle every day. If you access SlimLoRa via the `lora` object, for example `SlimLoRa lora = SlimLoRa(8);`, you can also read (don't write) the values `lora.slimTotalTXms` and `lora.slimLastTXms`. After one day you have to zero the `lora.slimTotalTXms` with the function `lora.ZeroTXms()` or `lora.slimTotalTXms = 0;`. I think I will remove the functions. They require 12 more bytes of program flash if used.

If you send a MAC request for Time or CheckLink you can read the variables from `lora.epoch`, `lora.fracSecond`, `lora.margin`, and `lora.GwCnt`. `lora.epoch` is greater than `0` if Time from the LNS is received. Also, `lora.GwCnt` is greater than `0` if LinkCheck is received.

I have also made some values public, like the frame counter. Now the application can do cool stuff like transmitting verbose data every X frames. So every 20 - 30 uplinks, you can send more data. Yes, this can be done in the app logic, but one less variable is used this way.

# Advanced usage

You can have access to epoch time, if before an uplink you issue `lora.TimeCheckLink = 1;` you will have access to `lora.epoch` and `lora.fracSecond` after the LNS responds with the epoch time. The 'complicated' example also has this information.

With `lora.TimeCheckLink = 2;` before an uplink, you can have access to the `lora.margin` (0-254) and `lora.GwCnt` variables.

With `lora.TimeCheckLink = 3;` before an uplink, you can have access to all of the above: `lora.epoch`, `lora.fracSecond`, `lora.margin` (0-254) and `lora.GwCnt` variables.

# Payload size

The maximum uplink payload size in ideal situations (for SF10, SF11, SF12) is 51 bytes but *don't* count on that. You can change that with `SLIM_LORAWAN_PACKET_SIZE` in `SlimLoRa.h`. For SF7, SF8, SF9 the protocol (p. 28 RP) says: 155 and 222 bytes of payload. In reality, you can be sure that you can send `51 - 15 = 36` bytes.

For downlinks, the limit is 12 bytes via `DOWNLINK_PAYLOAD_SIZE`.

You can change those values in the loss of RAM.

## Drift - Only room temperature tested. Cold or Heat untested

If your project relies on a small battery, try to lower `SLIMLORA_DRIFT` from `2` to `1` or even to `0`. There is a danger of not receiving downlinks. Verify the behavior with cold and heat. Make sure your device/network can handle that. With MegaBrick and `SLIMLORA_DRIFT 1` I managed to join at with SF7 and receive downlinks in room temperature. Your device/network may not be capable of that. `3` and `2` seem to be ok with feather-32u4 for RX2 SF12 at 2s. Join is fine with `2`, `3`, `4`. With drift `1` join fails with feather-32u4, so the best for feather-32u4 is `2` or `3`. MegaBrick joins with `1` but fails with `0`. So, the best for MegaBrick is `1` or `2`.

SAMD21E18 @48Mhz works with RX2 (SF12) in 2seconds and DRIFT 0. Best for SAMD21E18: UNTESTED probably `1` or `2`.

Table with wait (lost energy) in ms for every SF and Drift.

| Drift |    SF7  |   SF8   |   SF9   |  SF10   |  SF11   |(RX1)    SF12  |
|-------|---------|---------|---------|---------|---------|---------------|
|   5   |   100ms |   112ms |   124ms |   149ms |   296ms |(656ms) 1112ms |
|   4   |    87ms |    92ms |   104ms |   124ms |   263ms |(558ms)  918ms |
|   3   |    66ms |    73ms |    84ms |   108ms |   214ms |(460ms)  756ms |
|   2   |    47ms |    53ms |    63ms |    84ms |   182ms |(362ms)  558ms |
|   1   |    27ms |    32ms |    43ms |    67ms |   133ms |(263ms)  362ms |
|   0   |     8ms |    14ms |    26ms |    51ms |   100ms |(198ms)  198ms |

# Tips for your project

If you need to handle array data with EEPROM, some helper functions `getArrayEEPROM`, `setArrayEEPROM` and `printHex` are public to use them in your program.

# Customization

`#define MAC_REQUESTS` enables MAC requests for `DeviceTime` and `LinkCheck`. Comment it to gain 397 bytes of program flash and 9 bytes of RAM.
`#define SLIM_DEBUG_VARS` enables a `lora.LoRaWANreceived` variable to check if you received a MAC command, a downlink, a Join Accept message in the 1st or 2nd window, or a downlink in the 1st or 2nd window.

# PROGMEM

It seems that when using PROGMEM for memory operations, the AVR's current consumption increases, and the speed is lower. If you have 336 bytes of SRAM to spare, you can comment `#define SLIMLORA_USE_PROGMEM` to gain speed and lower current consumption.

---

**Note:**

# Original message from Hendrik Hagendorn [some parts have been deleted]. #

This library evolved from a fun project, [LoRa-ATtiny-Node](https://github.com/novag/LoRa-ATtiny-Node). It was never meant to run on more powerful UCs than an ATtiny85. This fork of the original library tries to support the Arduino ecosystem.

SlimLoRa originated from a code base by Ideentron B.V.. Therefore its structure is similar to the TinyLoRa library.
SlimLoRa is very different from LMIC. This does not mean it's better, it's different. E.g. SlimLoRa has no eventing system.

---

This repository contains the SlimLoRa LoRaWAN library. It uses AES encryption routines originally written by Ideentron B.V.. SlimLoRa is compatible with all radio chips based on the Semtech SX1276 (e.g. HopeRF RFM95).

SlimLoRa implements the ABP and OTAA activation schemes. It has support for downlink messages and the Adaptive Data Rate (ADR) mechanism.

Although the basic things of the LoRaWAN 1.0.3 Class A specification are implemented, this library does not cover the entire specification and thus *is not fully* LoRaWAN 1.0 compliant. Please verify its behavior before using it on public networks.  
It also **does not** enforce a duty cycle. This must be ensured by the user.

**Contents:**

- [Features](#features)
- [Requirements and Limitations](#requirements-and-limitations)
- [Configuration](#configuration)
- [Supported hardware](#supported-hardware)
- [Release History](#release-history)
- [Contributions](#contributions)
- [Trademark Acknowledgements](#trademark-acknowledgements)
- [License](#license)

## Features

This library provides a LoRaWAN 1.0 Class A implementation for the EU-868 band.

The library has some support for LoRaWAN 1.1 which is not usable yet.

- Sending uplink packets, *not* taking into account duty cycles.
- Message encryption and message integrity checking.
- Over-the-Air Activation (OTAA).
- Activation by Personalization (ABP).
- Adaptive Data Rate (ADR).
- Receiving downlink packets in the RX1 and RX2 windows.
- MAC command processing for LinkAdr, RxParamSetup, DevStatus and RxTimingSetup requests.

## Requirements and Limitations

### No event handling
This library does not implement any kind of event handling. A call to the Join and SendData methods will block for several seconds until the end of the second receive window.

Be sure to put connected sensors into sleep mode before sending data to save energy!

## Contributions

This library started from parts written by Ideetron B.V.. Thanks to Ideetron B.V.!

- novag ([LoRa-ATtiny-Node](https://github.com/novag/LoRa-ATtiny-Node))

- Ideentron B.V. - [RFM95W_Nexus](https://github.com/Ideetron/RFM95W_Nexus)

## Trademark Acknowledgements

LoRa is a registered trademark of Semtech Corporation. LoRaWAN is a registered trademark of the LoRa Alliance.

All other trademarks are the properties of their respective owners.

## License

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see [<http://www.gnu.org/licenses/>](http://www.gnu.org/licenses/).
