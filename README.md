# AE9RB Peaberry SDR

An amateur HF transceiver designed by the Free and Open Source community.

Kits available from http://AE9RB.com/

## Features

 * Single full-speed USB interface for audio and control.
 * 1 watt transmitter.
 * DG8SAQ/PE0FKO compatible control interface.
 * Open Source firmware.  Apache 2.0 license.

## Building Firmware

The project was built with PSoC Creator 3.1 (3.1.0.1570):
http://www.cypress.com/

It is normal to get errors about missing files until you have built
the project for the first time.  This is because the generated files
have been excluded from the git repository. 

The bootloader project must be built before you can build the peaberry project.

The bootloader component of the bootloader project must be V1.10.
