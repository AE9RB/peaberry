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

uint8 T1_Tune_Request;

void T1_Main(void) {
    static uint8 state = 0, timer, band, send, bits, band_request, band_request_timer;
    static uint16 tune_timer; 
    
    if (tune_timer) {
        tune_timer--;
        if (!tune_timer) {
            Control_Write(Control_Read() & ~CONTROL_ATU_1);
        }
    }

    if (band_request_timer) band_request_timer--;
    else if (Band_Number != band) {
        band_request = 1;
        band_request_timer = 200;
    }

    switch(state) {
    case 0: // idle
        if (Status_Read() & STATUS_ATU_0) timer++;
        else {
            if (timer >= 85 && timer <= 115) {
                state = 1;
                timer = 20;
                send = band = Band_Number;
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