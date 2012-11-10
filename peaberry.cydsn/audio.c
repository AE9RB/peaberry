// Copyright 2012 David Turnbull AE9RB
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

// IQ Reveral option
// 0,1: RX=jumper TX=norm  000 001
// 2,3: RX=jumper TX=rev   010 011
// 4: RX=norm TX=norm      100
// 5: RX=rev TX=norm       101
// 6: RX=norm TX=rev       110
// 7: RX=rev TX=rev        111
uint8 Audio_IQ_Channels = 0;

// Use to detect change
uint8 Prev_IQ_Channels = 255;


void Audio_Reset(void) {
    Mic_Init();
    PCM3060_Init();
    Mic_Start();
    PCM3060_Start();
    Audio_Set_Speaker();
}

void Audio_Start(void) {
    Mic_Setup();
    PCM3060_Setup();
    Audio_Reset();
}

void Audio_Set_Speaker(void) {
    if (TX_Request) {
        SPKR_DisconnectAll();
    } else if (Audio_IQ_Channels &0x02) {
        SPKR_Select(1);
    } else {
        SPKR_Select(0);
    }
}

void Audio_Main(void) {
    USBAudio_Main();
    PCM3060_Main();
    
    switch (Si570_LO) {
        case 0xA8AAAA10: Audio_IQ_Channels = 0; break; // 33.333333 MHz
        case 0x8AE3B810: Audio_IQ_Channels = 1; break; // 33.444444 MHz
        case 0x6D1CC710: Audio_IQ_Channels = 2; break; // 33.555555 MHz
        case 0x5055D510: Audio_IQ_Channels = 3; break; // 33.666666 MHz
    }
    
    if (Prev_IQ_Channels != Audio_IQ_Channels) {
        Prev_IQ_Channels = Audio_IQ_Channels;
        Audio_Reset();
    }
    
}
