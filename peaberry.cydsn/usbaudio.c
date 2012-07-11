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

#define RX_ENDPOINT              2
#define RX_INTERFACE             2
#define TX_INTERFACE             3
#define TX_ENDPOINT              3
#define MIC_ENDPOINT             4
#define MIC_INTERFACE            5
#define SPKR_INTERFACE           6
#define SPKR_ENDPOINT            5

// For the unused speaker or transmit USB data.
volatile uint8 Void_Buff[I2S_BUF_SIZE];

// Using only four USB buffers with fine tuning of the SOF sync,
// we can reduce overruns and underruns so they almost never happen.
void USBAudio_SyncBufs(uint8 dma, uint8* use, uint8* debounce, uint8 adjust) {
    uint8 dma_adjusted;
    if (dma & 0x01) {
        dma /= 2;
        dma_adjusted = dma + 1;
    } else {
        dma /= 2;
        dma_adjusted = dma;
    }
    if (dma_adjusted >= USB_AUDIO_BUFS) dma_adjusted -= USB_AUDIO_BUFS;
    if (*debounce) (*debounce)--;
    if (dma == *use || dma_adjusted == *use) {
        *use += USB_AUDIO_BUFS - 1;
        if (*use >= USB_AUDIO_BUFS) *use -= USB_AUDIO_BUFS;
        if (adjust) SyncSOF_Slower();
        *debounce = USB_AUDIO_BUFS;
        return;
    }
    if (++*use == USB_AUDIO_BUFS) *use = 0;
    if (*use == dma) {
        *use += USB_AUDIO_BUFS - 1;
        if (*use >= USB_AUDIO_BUFS) *use -= USB_AUDIO_BUFS;
        if (!*debounce) {
            *debounce = USB_AUDIO_BUFS;
            if (adjust) SyncSOF_Faster();
        }
    }
}

extern uint8 USBFS_currentVolume[];
extern uint8 USBFS_minimumVolume[];
extern uint8 USBFS_maximumVolume[];
extern uint8 USBFS_currentMute;

// Returns volume in the PCM3060 range of 54-255 where 54 is mute.
uint8 USBAudio_Volume(void) {
    // cache results of expensive division
    static uint16 prev=0;
    static uint8 volume=0;
    uint16 i;
    if (*(uint16*)USBFS_currentVolume != prev) {
        prev = *(uint16*)USBFS_currentVolume;
        ((uint8*)&i)[1] = USBFS_currentVolume[0];
        ((uint8*)&i)[0] = USBFS_currentVolume[1];
        if (i== 0x8000) {
            volume = 54;
        } else {
            volume = 176 + ((i + 0x2780) >> 7);
        }
    }
    if (USBFS_currentMute) return 54;
    return volume;
}

void USBAudio_Start(void) {
    // -39.5dB to 0.0dB volume range
    USBFS_minimumVolume[0] = 0x80;
    USBFS_minimumVolume[1] = 0xD8;
    USBFS_maximumVolume[0] = 0;
    USBFS_maximumVolume[1] = 0;
    USBFS_currentVolume[0] = 0;
    USBFS_currentVolume[1] = 0;
    USBFS_ReadOutEP(TX_ENDPOINT, Void_Buff, I2S_BUF_SIZE);
    USBFS_ReadOutEP(SPKR_ENDPOINT, Void_Buff, I2S_BUF_SIZE);
}


void USBAudio_Main(void) {
    static uint8 TX_Enabled = 0, SPKR_Enabled = 0;
    
    if (USBFS_GetInterfaceSetting(TX_INTERFACE) == 1) {
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
    if (USBFS_GetInterfaceSetting(SPKR_INTERFACE) == 1) {
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

    if(USBFS_IsConfigurationChanged()) {
        USBFS_EnableOutEP(TX_ENDPOINT);
        USBFS_EnableOutEP(SPKR_ENDPOINT);
    }

    
    if (USBFS_GetEPState(TX_ENDPOINT) == USBFS_OUT_BUFFER_FULL)
    {
        if (Control_Read() & CONTROL_TX_ENABLE) {
            USBFS_ReadOutEP(TX_ENDPOINT, PCM3060_TxBuf(), I2S_BUF_SIZE);
            USBFS_EnableOutEP(TX_ENDPOINT);
        } else {
            USBFS_ReadOutEP(TX_ENDPOINT, Void_Buff, I2S_BUF_SIZE);
            USBFS_EnableOutEP(TX_ENDPOINT);
        }
    }

    if (USBFS_GetEPState(SPKR_ENDPOINT) == USBFS_OUT_BUFFER_FULL)
    {
        if (Control_Read() & CONTROL_TX_ENABLE) {
            USBFS_ReadOutEP(SPKR_ENDPOINT, Void_Buff, I2S_BUF_SIZE);
            USBFS_EnableOutEP(SPKR_ENDPOINT);
        } else {
            USBFS_ReadOutEP(SPKR_ENDPOINT, PCM3060_TxBuf(), I2S_BUF_SIZE);
            USBFS_EnableOutEP(SPKR_ENDPOINT);
        }
    }

	if (USBFS_GetEPState(RX_ENDPOINT) == USBFS_IN_BUFFER_EMPTY) {
		USBFS_LoadInEP(RX_ENDPOINT, PCM3060_RxBuf(), I2S_BUF_SIZE);
		USBFS_LoadInEP(RX_ENDPOINT, 0, I2S_BUF_SIZE);
	}

	if (USBFS_GetEPState(MIC_ENDPOINT) == USBFS_IN_BUFFER_EMPTY) {
	    USBFS_LoadInEP(MIC_ENDPOINT, Mic_Buf(), MIC_BUF_SIZE);
	    USBFS_LoadInEP(MIC_ENDPOINT, 0, MIC_BUF_SIZE);
	}

}
