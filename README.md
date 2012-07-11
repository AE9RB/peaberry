# AE9RB Peaberry SDR

An amateur HF transceiver designed for the Free and Open Source community.

## Features

 * Single full-speed USB interface for audio and control.
 * 48kHz 24-bit radio interface.
 * 48kHz 24-bit stereo speaker and 12-bit microphone.
 * All DelSig clocks synchronized to USB SOF.
 * 1 watt transmitter.
 * DG8SAQ/PE0FKO compatible control interface.
 * Open Source firmware.  Apache 2.0 license.

## Windows Driver

The audio interfaces use standard OS drivers. The control interface
requires a driver on Windows. The PE0FKO libusb drivers work but require
a small change to the inf file.  (&MI_00 added to DeviceID)

https://github.com/downloads/ham21/peaberry/Windows-USB-Driver-1.2.5.0.zip

## Firmware

The project was built with PSoC Creator 2.0:
http://www.cypress.com/

It is normal to get errors about missing files until you have built
the project for the first time.  This is because the generated files
have been excluded from the git repository.

## Schematic

https://github.com/downloads/ham21/peaberry/SchematicV1.pdf
