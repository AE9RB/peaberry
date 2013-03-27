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

// IQ Reveral option
// 0: RX=norm TX=norm
// 1: RX=rev TX=norm
// 2: RX=norm TX=rev
// 3: RX=rev TX=rev
uint8 Audio_IQ_Channels;

// Use to detect change
uint8 Prev_IQ_Channels = 255;

#define SOF_CENTER 22000
#define FRAC_MIN (FracN_DEFAULT-75)
#define FRAC_MAX (FracN_DEFAULT+75)

void Audio_Buffer_Sync(void) {
    static uint16 frac = FracN_DEFAULT;
    static uint16 prev_pos;
    uint16 pos;

    pos = CY_GET_REG8(SyncSOF_FRAME_POS_LO__STATUS_REG);
    if (pos & 0x01) {
        pos += (uint16)CY_GET_REG8(SyncSOF_FRAME_POS_HI__STATUS_REG) << 8;

        if (pos > SOF_CENTER) {
            frac += (pos - SOF_CENTER) / 128;
        } else {
            frac -= (SOF_CENTER - pos) / 128;
        }
        
        if (prev_pos < pos) {
            frac += (pos - prev_pos) / 2;
        } else {
            frac -= (prev_pos - pos) / 2;
        }
        prev_pos = pos;
        
        if (frac > FRAC_MAX) frac = FRAC_MAX;
        if (frac < FRAC_MIN) frac = FRAC_MIN;
        FracN_Set(frac);
    }

}

void Audio_Reset(void) {
    PCM3060_Init();
    PCM3060_Start();
}

void Audio_Start(void) {
    PCM3060_Setup();
    Audio_Reset();
}

uint8 TX_Enabled, SPKR_Enabled;

void Audio_USB_Start(void) {
    TX_Enabled = SPKR_Enabled = 0;
    USBFS_ReadOutEP(TX_ENDPOINT, PCM3060_TxBuf(), 0);
}

void Audio_Detect_Change() {
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

void Audio_Main(void) {
    Audio_Buffer_Sync();
    Audio_Detect_Change();
    
    if (USBFS_GetInterfaceSetting(TX_INTERFACE)) {
        if (!TX_Enabled) {
            USBFS_EnableOutEP(TX_ENDPOINT);
            TX_Enabled = 1;
        }
    } else {
        if (TX_Enabled) {
            USBFS_DisableOutEP(TX_ENDPOINT);
            TX_Enabled = 0;
        }
    }
    
    if(USBFS_IsConfigurationChanged()) {
        USBFS_EnableOutEP(TX_ENDPOINT);
    }

    
    if (USBFS_GetEPState(TX_ENDPOINT) == USBFS_OUT_BUFFER_FULL) {
        USBFS_ReadOutEP(TX_ENDPOINT, PCM3060_TxBuf(), I2S_Buf_Size);
        USBFS_EnableOutEP(TX_ENDPOINT);
    }

    if (USBFS_GetEPState(RX_ENDPOINT) == USBFS_IN_BUFFER_EMPTY) {
		USBFS_LoadInEP(RX_ENDPOINT, PCM3060_RxBuf(), I2S_Buf_Size);
		USBFS_LoadInEP(RX_ENDPOINT, 0, I2S_Buf_Size);
	}

}
