# AE9RB Peaberry SDR

An amateur HF transceiver designed by the Free and Open Source community.

Kits available from http://AE9RB.com/

## Features

 * Single full-speed USB interface for audio and control.
 * 48kHz 24-bit radio interface.
 * 48kHz 24-bit stereo speaker and 12-bit microphone.
 * All DelSig clocks synchronized to USB SOF.
 * 1 watt transmitter.
 * DG8SAQ/PE0FKO compatible control interface.
 * Open Source firmware.  Apache 2.0 license.

## Building Firmware

The project was built with PSoC Creator 2.2:
http://www.cypress.com/

The Debug target will not build, only the Release target will fit on the chip.

It is normal to get errors about missing files until you have built
the project for the first time.  This is because the generated files
have been excluded from the git repository. 

## Schematic and Windows Driver

http://ae9rb.com/forum/viewtopic.php?f=2&t=38
