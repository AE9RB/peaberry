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

// Full support for Elecraft T1 Automatic Antenna Tuner

uint8 T1_Tune_Request = 0;

void T1_Main(void) {
    static uint8 state = 0, timer, new_band, band, send, bits, band_request;
    static uint16 tune_timer; 
    static uint32 lo;
    uint32 i;

    if (Si570_LO != lo) {
        i = swap32(lo = Si570_LO);
        if (i > 0x1b800000) new_band = 12; // 55.000 MHz
        else if (i > 0xf800000) new_band = 11; // 31.000 MHz
        else if (i > 0xd000000) new_band = 10; // 26.000 MHz
        else if (i > 0xb000000) new_band = 9; // 22.000 MHz
        else if (i > 0x9800000) new_band = 8; // 19.000 MHz
        else if (i > 0x8000000) new_band = 7; // 16.000 MHz
        else if (i > 0x6000000) new_band = 6; // 12.000 MHz
        else if (i > 0x4800000) new_band = 5; // 9.000 MHz
        else if (i > 0x3000000) new_band = 4; // 6.000 MHz
        else if (i > 0x2800000) new_band = 3; // 5.000 MHz
        else if (i > 0x1800000) new_band = 2; // 3.000 MHz
        else if (i > 0x0c00000) new_band = 1; // 1.500 MHz
        else new_band = 0;
        if (new_band != band) band_request = 1;
    }

    if (tune_timer) {
        tune_timer--;
        if (!tune_timer) {
            Control_Write(Control_Read() & ~CONTROL_ATU_1);
        }
    }

    switch(state) {
    case 0: // idle
        if (Status_Read() & STATUS_ATU_0) timer++;
        else {
            if (timer >= 85 && timer <= 115) {
                state = 1;
                timer = 20;
                send = band = new_band;
                bits = 4;
                Control_Write(Control_Read() & ~CONTROL_ATU_0 | CONTROL_ATU_0_OE);
            }
            else {
                timer = 0;
                if (band_request || T1_Tune_Request) {
                    Control_Write(Control_Read() | CONTROL_ATU_1);
                    if (T1_Tune_Request) tune_timer = 1000;
                    else tune_timer = 10;
                    band_request = T1_Tune_Request = 0;
                }
            }
        }
        break;
    case 1: // data low
        timer--;
        if (!timer) {
            if (bits) {
                Control_Write(Control_Read() | CONTROL_ATU_0);
                if (send & 0x08) timer = 8;
                else timer = 3;
                send <<= 1;
                bits--;
                state = 2;
            } else {
                Control_Write(Control_Read() & ~(CONTROL_ATU_0 | CONTROL_ATU_0_OE));
                state = 0;
            }
        }
        break;
    case 2: // data high
        timer--;
        if (!timer) {
            if (bits) timer = 3;
            else timer = 10;
            Control_Write(Control_Read() & ~CONTROL_ATU_0);
            state = 1;
        }
        break;
    }   

}