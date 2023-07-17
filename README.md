# SlimLoRa Arduino library for EU868 (WIP, does not fully tested yet)

---

# Tested

- [x] Feather 32u4 EU868 region.
- [x] Join on helium with SF10. Success everytime on 1st window.
- [x] SetPower

# Untested

- [ ] ADR.
- [ ] Store session in EEPROM.
- [ ] Only once tested SF7, SF8, SF9 and succeded, but I will verify again.

# TODO (PR's welcome)

- [ ] Add pin mappings infrastucture for other connections.
- [x] Battery Level to DevStatusAns.
- [ ] Join on TTN fails. Maybe my gateway is not normal. Should try with others.
- [ ] Test join with SF11, SF12 on Helium and TTN.
- [ ] Change SetPower style to LoRaWAN style.
- [ ] Respect Dwell MAC command
- [ ] Respect Duty Cycle
- [ ] Respect Join Back-off (not faster than 36 seconds)

# Maybe good ideas

- [ ] 32-bit Frame Counter.

# I can't test it.

- [ ] Add regions. Only works with EU868.

# Evaluation / mystery stuff

- SF10 indoors working, outdoors not?

---

**Note:**

Original message from Hendrik Hagendorn.

This library evolved from a fun project [LoRa-ATtiny-Node](https://github.com/novag/LoRa-ATtiny-Node). It was never meant to run on more powerful UCs than an ATtiny85. This fork of the original library tries to support the Arduino ecosystem.

It is currently still being adapted to the Arduino ecosystem and is **not ready for use**!

If you are looking for a stable and proven LoRaWAN library, SlimLoRa is most likely not for you! Go for alternatives like the popular LMIC.

SlimLoRa originated from a code base by Ideentron B.V.. Therefore its structure is similar to the TinyLoRa library.
SlimLoRa is very different from LMIC. This does not mean it's better, it's different. E.g. SlimLoRa has no eventing system.

Since it was a library optmized for the ATtiny85 many things have to be adjusted to work with the Arduino standard libraries. I do not have a lot of experience with Arduinos, so hints about best practices and contributions in general are appreciated.

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
