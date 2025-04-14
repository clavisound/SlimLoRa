![CI](https://github.com/clavisound/SlimLoRa/actions/workflows/main.yml/badge.svg)

# SlimLoRa - Propably the easiest and smallest footprint LoRaWAN library for Arduino library and EU868.

This library is probably the most easy to use LoRaWAN library. The target is LoRaWAN-1.0.3 specification. It supports OTAA / Join, most important MAC commands - like DR, power, NBtrans, downlinks for user application and session is stored to EEPROM. Applications downlinks are static selectable via `#define` in `SlimLoRa.h`. Default is 11 bytes. If you want a complete LoRaWAN library try [Radiolib](https://github.com/jgromes/RadioLib/) (needs around 52kBytes of program flash), or LMIC (around 36kBytes of program flash).

SlimLoRa needs around 12558 Bytes* (13kBytes). SlimLoRa gives LoRaWAN life to old μCU's like ATmega 328 with 32kBytes of flash.

* in fact, I think the overhead is around 9kBytes.

[SlimLoRa MAC response in MAC command via Helium original console.](https://krg.etraq.eu/minisites/lora/mac-command-response_crop.png)

The majority of the work was done by Hendrik Hagendorn and Ideetron B.V. Thanks to both of them. I ported the library to Arduino, I expanded the most important MAC commands, enabled application downlinks, ACK for confirmed downlinks and corrected the channel for SF7BW250.

# Working

- [x] OTAA join with Feather 32u4 EU868 region. Europe only.
  - [x] TTN joins - Feather32u4
    - [x] Join SF9 on TTN and power 0dBm in different room. Success on 1st window.
    - [x] Join SF8 on TTN and power 0dBm in same room. Success on 1st window.
    - [x] Join SF7 on TTN and power 0dBm in different room. Success on 1st window.
      - [x] Megabrick joins
        - [ ] FAIL with SF7 and power 13dBm in different room and an antenna.
        - [x] Join with SF9 and power 13dBm in different room and an antenna.
  - [x] Heliun Joins - Feather32u4
    - [x] old console
      - [x] Join SF10 with Helium. Success everytime on 1st window but not in first attempt.
    - [x] chirpstack Console
      - [x] Join SF9 on Helium chripstack outdors. Success on 1st window.
      - [x] Join SF8 on Helium chripstack outdors. Success on 1st window in second or third attempt.
      - [x] Join SF7 on Helium and power 0dBm in different room. Success on 1st window.
        - [x] Megabrick joins
          - [x] Join with SF7 and GW power 16dBm in different room and an antenna.
          - [x] Join with SF8 and GW power 16dBm in different room and an antenna.
- [x] Downlinks
  - [x] Helium on 2nd window (SF12) always works on Chirpstack.
- [x] SetPower
- [x] Deep Sleep
- [x] Restore session from EEPROM (arduino style)
- [x] Downlink for application.
- [x] ACK to confirmed Downlink.
- [x] NbTrans - edit SlimLoRa.h to config.
- [x] Most important MAC commands - more to follow
- [x] Session saved to EEPROM. You only need to join once on the lifetime of the device.

# MAC commands supported.

- [x] ADR for Data Rate and TxPower. After join with SF9 in different room from gateway (RSSI -85), TTN sends SF7 (or SF8) ADR command and SlimLoRa conforms.
- [x] NbTrans
- [x] Channel Mask
- [x] RxTimingSetup
- [x] DevStatusAns. Battery status is working, I think margin is fine.
- [x] LinkCheckReq. (Margin and Gateways count)
- [x] DeviceTimeReq. Fractional second on purpose is truncated to ms resolution. I can't undestand how much precise is the timing from LNS. Have to verify with GPS.

# BUGS

- [ ] SF7BW250 is working with TTN but SlimLoRa does not receives the downlink. This DR6 is currently not supported by Helium [chirpstack-4.7].

# Untested

- [ ] ABP
- [ ] AVR style EERPOM

## EEPROM handling to consider
I recommend `ARDUINO_EEPROM == 1` in `SlimLoRa.h` otherwise when you use `avr/eeprom.h` style and you compile with different options, or if you change part of your sketch relative to EEPROM (EEMEM) the **address of the data are changing places!** This is a "[bug](https://arduino.stackexchange.com/a/93879/59046)" on avr/eeprom.h. With avr style if you change your sketch, maybe you need to re-join. With arduino eeprom style you don't need to re-join.

Solutions with avr style.

- Solution #1: Erase ALL the EEPROM after uploading the new firmware.
- Solution #2: Hint: You can track the EEPROM addresses with: `avr-objdump -D` on .eemem section.
- Solution #3: Don't enable keep session.
- Solution #4: use arduino style eeprom in `SlimLoRa.h`

# Semi-Working

- [x] `ADR_ACK_LIMIT`, must follow directive on p. 17 based on `ADR_ACK_DELAY`
- [x] Duty Cycle. Added GetTXms function to return the TOTAL duration of ALL transmissions. At SF7 1byte reports 45-48ms vs 46ms [theoretical](https://avbentem.github.io/airtime-calculator/ttn/eu868/1) at SF8 reports 84ms vs 82ms (theoretical). SF7 5 bytes reports 52ms vs 51.5ms (theoretical). Application HAVE to read the value of GetTXms() after every transmission to check if the the Duty Cycle is respected. I decided to not respect Duty Cycle on SlimLoRa, since if the device is going to Deep Sleep and wakes up via a accelerometer on AVR MCU's freezes the timer0. I think the solution is the RTC or to read a time from GPS.
- [x] Power to the people. Several values made public. Take care to not write them or you may loose access to the network. Instead of using getters I selected to make public some variables.
- [x] MAC Commands. 

# TODO's (PR's welcome) - In order of importance.

- [ ] Join back-off
- [ ] Extern variable for Duty Cycle if the application can provide time.
- [ ] Confirmed Uplink
- [ ] Make DevNonce random.
- [ ] More regions. Currently only EU868 is working.
- [ ] Add pin mappings infrastucture for other boards.
- [ ] Add compile options for battery status (unable to measure, connected to external power)
- [ ] Random delay for TX.
- [ ] Duty Cycle per channel.
- [ ] Respect Dwell MAC command (only for US902?)
- [ ] CFlist only for US and AS?

# Clock Divider support

If you downclock your AVR MCU with `clock_prescale_set(clock_div_4)` SlimLoRa can adapt the RX timing window to receive the downlinks. Even with `clock_prescale_set(clock_div_32)` at 500Khz a MegaBrick joins at SF7. Uncomment the `#define CATCH_DIVIDER` to enable this function.

# Undoable on AVR and Deep Sleep

- [x] Respect Duty Cycle
Since AVR on Deep Sleep freezes the timer0. SlimLoRa is unable to know about time.
- [x] Respect Join Back-off (not faster than 36 seconds). Must handle by application. In case of Deep Sleep SlimLoRa can't keep track the time.

# Maybe good ideas

- [ ] 32-bit Frame Counter.

# How to use it (mini-tutorial)

Download the library and extract to Arduino/libraries folder or install via the library manager of Arduino IDE. For manual install: Rename SlimLoRa-master or SlimLoRa-VERSION to SlimLoRa. Read the details on examples. If something goes bad erase ALL the EEPROM and re-try join.

SlimLoRa changes the data of payload. Don't use payload data for program logic.

You can monitor the duty cycle with the function `GetTXms()` after every transmission. SlimLoRa will return the duration in ms of the LAST transmission. You have to add this to a variable to your program to keep track the Duty Cycle every day. If you access SlimLoRa via `lora` object example `SlimLoRa lora = SlimLoRa(8);` you can also read (please don't write) the values `lora.slimTotalTXms` and `lora.slimLastTXms`. After one day remember to erase the slimTotalTXms with function `lora.ZeroTXms()` or `lora.slimTotalTXms = 0;`. I think I will remove the functions.

I have also made some values public. Because the application can do something cool stuff like: if you are close to a GW you can check with frame counter if you transmit verbose data. So every 20 - 30 uplinks you can send more data.

## Drift - Only room temperature tested. Cold or Heat untested

If your project relies on small battery, try to lower `SLIMLORA_DRIFT` from `2` to `1` or even to `0`. There is a danger to not receive downlinks. Verify the behaviour with cold and heat. Make sure your device / network can handle that. With MegaBrick and `SLIMLORA_DRIFT 1` I managed to join at with SF7 and receive downlinks in room temperature. Your device / network maybe is not capable of that. `3` and `2` seems to be ok with feather-32u4 for RX2 SF12 at 2s. Join is fine with `2`, `3`, `4`. With drift `1` join fails with feather-32u4, so the best for feather-32u4 is `2` or `3`. MegaBrick joins with `1` but fails with `0`. So best for MebaBrick is `1` or `2`.

| Drift |    SF7  |   SF12  |
|-------|---------|---------|
|   5   |   100ms | 1100ms  |
|   4   |    87ms |  918ms  |
|   3   |    66ms |  756ms  |
|   2   |    47ms |  558ms  |
|   1   |    27ms |  362ms  |

# Tips for your project

If you need to handle array data with EEPROM some helper function `getArrayEEPROM`, `setArrayEEPROM` and `printHex` are public to use them in your programm.

---

**Note:**

# Original message from Hendrik Hagendorn [some parts deleted]. #

This library evolved from a fun project [LoRa-ATtiny-Node](https://github.com/novag/LoRa-ATtiny-Node). It was never meant to run on more powerful UCs than an ATtiny85. This fork of the original library tries to support the Arduino ecosystem.

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

### Timer0
SlimLoRa uses the Timer0 on every call of the Join or SendData methods. Between these calls Timer0 can be used by the user for other purposes.
See also timing.h and timing.c.

## Supported hardware

Radios based on the SX1276 are supported (e.g. HopeRF RFM95).

## Release History

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
