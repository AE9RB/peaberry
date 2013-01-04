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

// Connects audio output to SPKR as needed
void Audio_Set_Speaker(void) {
    
    //SPKR_Select(0); //DEBUG TEST
    //return; //DEBUG TEST

    if (TX_Request || B96_Enabled) {
        SPKR_DisconnectAll();
    } else if (Audio_IQ_Channels &0x02) {
        SPKR_Select(0);
    } else {
        SPKR_Select(1);
    }
}
    
// Returns volume in the PCM3060 range of 54-255 where 54 is mute.
uint8 Audio_Volume(void) {
    // cache results of expensive division
    static uint16 prev = 0;
    static uint8 volume = 255;
    uint16 i;
    uint8 save_cs;
    save_cs = CyEnterCriticalSection();
    i = *(uint16*)USBFS_currentVolume;
    CyExitCriticalSection(save_cs);
    if (i != prev) {
        prev = i;
        ((uint8*)&i)[1] = ((uint8*)&prev)[0];
        ((uint8*)&i)[0] = ((uint8*)&prev)[1];
        if (i== 0x8000) {
            volume = 54;
        } else {
            volume = 176 + ((i + 0x2780) >> 7);
        }
    }
    if (USBFS_currentMute) return 54;
    return volume;
}

uint8 TX_Enabled, SPKR_Enabled;

void Audio_USB_Start(void) {
    // -39.5dB to 0.0dB volume range
    USBFS_minimumVolume[0] = 0x80;
    USBFS_minimumVolume[1] = 0xD8;
    USBFS_maximumVolume[0] = 0;
    USBFS_maximumVolume[1] = 0;
    USBFS_currentVolume[0] = 0;
    USBFS_currentVolume[1] = 0;
    TX_Enabled = SPKR_Enabled = 0;
    // Start endpoints with bogus destination
    USBFS_ReadOutEP(TX_ENDPOINT, PCM3060_VoidBuf(), 0);
    if (!B96_Enabled) USBFS_ReadOutEP(SPKR_ENDPOINT, PCM3060_VoidBuf(), 0);
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
    
    if (!B96_Enabled) {
        if (USBFS_GetInterfaceSetting(SPKR_INTERFACE)) {
            if (!SPKR_Enabled) {
                USBFS_EnableOutEP(SPKR_ENDPOINT);
                SPKR_Enabled = 1;
            }
        } else {
            if (SPKR_Enabled) {
                USBFS_DisableOutEP(SPKR_ENDPOINT);
                SPKR_Enabled = 0;
            }
        }
    }
    
    if(USBFS_IsConfigurationChanged()) {
        USBFS_EnableOutEP(TX_ENDPOINT);
        if (!B96_Enabled) USBFS_EnableOutEP(SPKR_ENDPOINT);
    }

    
    if (USBFS_GetEPState(TX_ENDPOINT) == USBFS_OUT_BUFFER_FULL)
    {
        if (B96_Enabled || IQGen_GetTransmit()) {
            USBFS_ReadOutEP(TX_ENDPOINT, PCM3060_TxBuf(), I2S_Buf_Size);
            USBFS_EnableOutEP(TX_ENDPOINT);
        } else {
            USBFS_ReadOutEP(TX_ENDPOINT, PCM3060_VoidBuf(), I2S_Buf_Size);
            USBFS_EnableOutEP(TX_ENDPOINT);
        }
    }

    if (USBFS_GetEPState(RX_ENDPOINT) == USBFS_IN_BUFFER_EMPTY) {
		USBFS_LoadInEP(RX_ENDPOINT, PCM3060_RxBuf(), I2S_Buf_Size);
		USBFS_LoadInEP(RX_ENDPOINT, 0, I2S_Buf_Size);
	}

	if (USBFS_GetEPState(MIC_ENDPOINT) == USBFS_IN_BUFFER_EMPTY) {
	    USBFS_LoadInEP(MIC_ENDPOINT, Mic_Buf(), MIC_BUF_SIZE);
	    USBFS_LoadInEP(MIC_ENDPOINT, 0, MIC_BUF_SIZE);
	}

    if (!B96_Enabled) {
        if (USBFS_GetEPState(SPKR_ENDPOINT) == USBFS_OUT_BUFFER_FULL)
        {
            if (IQGen_GetTransmit()) {
                USBFS_ReadOutEP(SPKR_ENDPOINT, PCM3060_VoidBuf(), I2S_Buf_Size);
                USBFS_EnableOutEP(SPKR_ENDPOINT);
            } else {
                USBFS_ReadOutEP(SPKR_ENDPOINT, PCM3060_TxBuf(), I2S_Buf_Size);
                USBFS_EnableOutEP(SPKR_ENDPOINT);
            }
        }

    }

}
