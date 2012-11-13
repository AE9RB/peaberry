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
// 0: RX=norm TX=norm
// 1: RX=rev TX=norm
// 2: RX=norm TX=rev
// 3: RX=rev TX=rev
uint8 Audio_IQ_Channels;

// Use to detect change
uint8 Prev_IQ_Channels = 255;


#define SOF_CENTER 22000
#define FRAC_MIN (970 * 64)
#define FRAC_MAX (1015 * 64)
uint16 frac = FRAC_MIN + (FRAC_MAX - FRAC_MAX) / 2;

void Audio_Buffer_Sync(void) {
    static uint16 prev_pos;
    uint16 pos;
    uint16 x;
    static uint8 mult;

    pos = CY_GET_REG8(SyncSOF_FRAME_POS_LO__STATUS_REG);
    if (pos & 0x01) {
        pos += (uint16)CY_GET_REG8(SyncSOF_FRAME_POS_HI__STATUS_REG) << 8;
        
        if (pos > SOF_CENTER) {
            if (frac < FRAC_MAX) frac += (pos - SOF_CENTER) / 256;
        } else {
            if (frac > FRAC_MIN) frac -= (SOF_CENTER - pos) / 256;
        }
        
        if (mult >= 11) mult = 9;
        else mult++;

        if (prev_pos < pos) {
            x = (pos - prev_pos) * mult;
            if (x > 256) x = 256;
            if (frac < FRAC_MAX) frac += x;
        } else {
            x = (prev_pos - pos) * mult;
            if (x > 256) x = 256;
            if (frac > FRAC_MIN) frac -= x;
        }
        prev_pos = pos;
        
        CY_SET_REG8(SyncSOF_FRAC_HI__CONTROL_REG, frac >> 8);
        CY_SET_REG8(SyncSOF_FRAC_LO__CONTROL_REG, frac & 0xFF);
        
    }

}

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
        SPKR_Select(0);
    } else {
        SPKR_Select(1);
    }
}


void Audio_Main(void) {
    USBAudio_Main();
    PCM3060_Main();
    Audio_Buffer_Sync();
    
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
