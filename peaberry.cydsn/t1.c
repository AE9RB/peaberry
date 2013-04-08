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

#define T1_STATE_IDLE 0

uint8 T1_Band, T1_Tune_Timer;
uint32 T1_LO;

// type 0 is wakeup for bandswitch, type 1 engages auto-tune
void T1_Tune(uint8 type) {
    uint16 timer;
    if (type) timer = 1000;
    else timer = 10;
    if (timer > T1_Tune_Timer) T1_Tune_Timer = timer;
    Control_Write(Control_Read() | CONTROL_ATU_1);
}

void T1_LO_Watch(void) {
    uint8 band;
    uint32 i;

    if (Si570_LO != T1_LO) {
        T1_LO = Si570_LO;
        i = swap32(Si570_LO);
        if (i > 0x1b800000) band = 0; // 55.000 MHz
        else if (i > 0xf800000) band = 11; // 31.000 MHz
        else if (i > 0xd000000) band = 10; // 26.000 MHz
        else if (i > 0xb000000) band = 9; // 22.000 MHz
        else if (i > 0x9800000) band = 8; // 19.000 MHz
        else if (i > 0x8000000) band = 7; // 16.000 MHz
        else if (i > 0x6000000) band = 6; // 12.000 MHz
        else if (i > 0x4800000) band = 5; // 9.000 MHz
        else if (i > 0x3000000) band = 4; // 6.000 MHz
        else if (i > 0x2800000) band = 3; // 5.000 MHz
        else if (i > 0x1800000) band = 2; // 3.000 MHz
        else if (i > 0x0c00000) band = 1; // 1.500 MHz
        else band = 0;
        if (T1_Band != band) {
            T1_Band = band;
            T1_Tune(0);
        }
    }
}

void T1_Main(void) {
    static uint8 T1_State = 0;

    if (T1_Tune_Timer) {
        T1_Tune_Timer--;
        if (!T1_Tune_Timer) {
            Control_Write(Control_Read() & ~CONTROL_ATU_1);
        }
    }
    T1_LO_Watch();
    
    switch(T1_State) {
    case T1_STATE_IDLE:
        break;
    }

}