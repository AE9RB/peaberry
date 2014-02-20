// Copyright 2013 David Turnbull AE9RB
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <peaberry.h>

void Band_Main(void) {
    static uint32 lo;
    uint32 i;
    uint8 relay;

    if (Current_LO != lo) {
        i = swap32(lo = Current_LO);
        
        // Watch for special IQ reversal frequencies
        switch (i) {
        case 0x10AAAAA8: Audio_IQ_Channels = 0; break; // 33.333333 MHz
        case 0x10B8E38A: Audio_IQ_Channels = 1; break; // 33.444444 MHz
        case 0x10C71C6D: Audio_IQ_Channels = 2; break; // 33.555555 MHz
        case 0x10D55550: Audio_IQ_Channels = 3; break; // 33.666666 MHz
        }

        // Band numbers for Elecraft T1
        if (i > 0X1B800000) T1_Band_Number = 12; // 55.000 MHz
        else if (i > 0xF800000) T1_Band_Number = 11; // 31.000 MHz
        else if (i > 0xD000000) T1_Band_Number = 10; // 26.000 MHz
        else if (i > 0xB000000) T1_Band_Number = 9; // 22.000 MHz
        else if (i > 0x9800000) T1_Band_Number = 8; // 19.000 MHz
        else if (i > 0x8000000) T1_Band_Number = 7; // 16.000 MHz
        else if (i > 0x6000000) T1_Band_Number = 6; // 12.000 MHz
        else if (i > 0x4800000) T1_Band_Number = 5; // 9.000 MHz
        else if (i > 0x3000000) T1_Band_Number = 4; // 6.000 MHz
        else if (i > 0x2800000) T1_Band_Number = 3; // 5.000 MHz
        else if (i > 0x1800000) T1_Band_Number = 2; // 3.000 MHz
        else if (i > 0x0C00000) T1_Band_Number = 1; // 1.500 MHz
        else T1_Band_Number = 0;
        
        // Set relay to correct filter
        if (i > 0xB000000) relay = 0; // 22.000 MHz
        else if (i > 0x8000000) relay = 1; // 16.000 MHz
        else if (i > 0x4800000) relay = 0; // 9.000 MHz
        else if (i > 0x2800000) relay = 1; // 5.000 MHz
        else if (i > 0x1800000) relay = 0; // 3.000 MHz
        else relay = 1;
        if (relay)
            Control_Write(Control_Read() & ~CONTROL_XK);
        else
            Control_Write(Control_Read() | CONTROL_XK);

        // Inhibit transmitter as necessary
        if (i > 0xC866666 && i < 0xDF99999) TX_Inhibit = 1;
        else TX_Inhibit = 0;
    }

}


