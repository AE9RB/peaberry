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

uint8 TX_Request = 0;

void TX_Main(void) {
    static uint8 state = 0, beat, blink;
    uint8 i;
    
    i = Status_Read() & STATUS_BEAT;
    if (beat != i) {
        beat = i;
        switch (state) {
            case 0: // receiving
                if (TX_Request) {
                    state = 1;
                    Control_Write(Control_Read() & ~CONTROL_RX);
                }
                break;
            case 1:
                Control_Write(Control_Read() | CONTROL_TX);
                state = 2;
                break;
            case 2:
                Control_Write(Control_Read() & ~CONTROL_AMP);
                state = 10;
                blink = 1;
                break;
            case 10: // transmitting
                if (TX_Request) {
                    blink--;
                    if (!blink) {
                        i = Control_Read();
                        if (i & CONTROL_LED) blink = 100;
                        else blink = 75;
                        Control_Write(i ^ CONTROL_LED);
                    }
                } else {
                    Control_Write(Control_Read() & ~(CONTROL_TX | CONTROL_LED) | CONTROL_AMP);
                    state = 11;
                }
                break;
            case 11:
                Control_Write(Control_Read() | CONTROL_RX);
                state = 0;
                break;
        }
    }       
    
}