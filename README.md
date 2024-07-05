# SlimLoRa Arduino library for EU868 (Basic functionality)

---

![CI](https://github.com/clavisound/SlimLoRa/actions/workflows/main.yml/badge.svg)

# Tested

- [x] Feather 32u4 EU868 region.
- [x] Join SF9 on TTN and power 0dBm in different room. Success on 1st window.
- [x] Join SF8 on TTN and power 0dBm in same room. Success on 1st window.
- [x] Join SF7 on TTN and power 0dBm in different room. Success on 1st window.
- [x] Join SF10 with Helium. Success everytime on 1st window but not in first attempt.
Megabrick testing:
- [ ] FAIL with SF7 and power 13dBm in different room and an antenna.
- [x] Join with SF9 and power 13dBm in different room and an antenna.
- [x] ADR works. After join with SF9 in different room from gateway (RSSI -85), TTN sends SF7 (or SF8) ADR command and SlimLoRa conforms.
- [x] Session restore works with Device address, AppSKey and NetworkKey. After Join, there is no need to rejoin if the device is powered off.
- [x] ADR_ACK_LIMIT works.
- [x] SetPower
- [x] Added arduino eeprom style store and restore. I recommend `ARDUINO_EEPROM == 1` in `SlimLoRa.h`. When you use `avr/eeprom.h` style and you compile with different options, or if you change part of your sketch relative to EEPROM (EEMEM) the address of the data are changing places! This is a "[bug](https://arduino.stackexchange.com/a/93879/59046)" on avr/eeprom.h. With avr style if you change your sketch, maybe you need to re-join. With arduino eeprom style you don't need to re-join.

Solutions with avr style.
Solution #1: Erase ALL the EEPROM after uploading the new firmware.
Solution #2: Hint you can track the EEPROM addresses with: `avr-objdump -D` on .eemem section.
Solution #3: Don't enable keep session.
Solution #4: use arduino style eeprom in `SlimLoRa.h`

- [x] Deep Sleep

# Semi-Working

- [x] Duty Cycle. Added GetTXms function to return the TOTAL duration of ALL transmissions. At SF7 1bytes reports 45-48ms vs 46ms [theoretical](https://avbentem.github.io/airtime-calculator/ttn/eu868/1) at SF8 reports 84ms vs 82ms (theoretical). SF7 5 bytes 52ms vs 51.5ms (theoretical). Application HAVE to read the value of GetTXms() after every transmission to check if the the Duty Cycle is respected. I decided to not respect Duty Cycle on SlimLoRa, since if the device is going to Deep Sleep and wakes up via a accelerometer on AVR MCU's freezes the timer0. I think the solution is the RTC.
- [x] Power to the people. Several values made public. Take care to not write them or you may loose access to the network. Instead of using getters I selected to make public some variables.

# Untested

- [ ] Added battery Level to DevStatusAns, but need to tested it. How?
- [ ] Test join with SF7 SF8 SF9 on Helium.
- [ ] ABP.
- [ ] Something funny with RX counter happening? It must be on par with server?

# TODO's (PR's welcome) - In order of importance.

- [ ] Add pin mappings infrastucture for other boards.
- [ ] Make DevNonce random.
- [ ] Confirmed Uplink
- [ ] Confirmed Downlink
- [ ] Add compile options for battery status (unable to measure, connected to external power)
- [ ] Respect Dwell MAC command (only for US902?)
- [ ] Change SetPower style to LoRaWAN style.
- [ ] Random delay for TX.

# Undoable on AVR and Deep Sleep

- [x] Respect Duty Cycle
Since AVR on Deep Sleep freezes the timer0. SlimLoRa is unable to know about time.
- [x] Respect Join Back-off (not faster than 36 seconds). Must handle by application. In case of Deep Sleep SlimLoRa can't keep track the time.

# Maybe good ideas

- [ ] 32-bit Frame Counter.

# I can't test it.

- [ ] Add regions. Only works with EU868.

# How to use it (mini-tutorial)

Download the library and extract to Arduino/libraries folder. Rename SlimLoRa-master or SlimLoRa-VERSION to SlimLoRa. Read the details on examples. If something goes bad erase ALL the EEPROM and re-try join.

SlimLoRa changes the data of payload. Don't use payload data for program logic.

You can monitor the duty cycle with the function `GetTXms()` after every transmission. SlimLoRa will return the duration in ms of the LAST transmission. You have to add this to a variable to your program to keep track the Duty Cycle every day. If you access SlimLoRa via `lora` object example `SlimLoRa lora = SlimLoRa(8);` you can also read (please don't write) the values `lora.slimTotalTXms` and `lora.slimLastTXms`. After one day remember to erase the slimTotalTXms with function `lora.ZeroTXms()` or `lora.slimTotalTXms = 0;`. I think I will remove the functions.

I have also made some values public. Because the application can do something cool stuff like: if you are close to a GW you can check with frame counter if you transmit verbose data. So every 20 - 30 uplinks you can send more data.

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
