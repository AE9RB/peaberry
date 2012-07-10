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


void USBAudio_Start(void) {
    USBFS_ReadOutEP(TX_ENDPOINT, Void_Buff, I2S_BUF_SIZE);
    USBFS_ReadOutEP(SPKR_ENDPOINT, Void_Buff, I2S_BUF_SIZE);
}

uint8 USBAudio_TX_Enabled = 0, USBAudio_RX_Enabled = 0, USBAudio_SPKR_Enabled = 0;

void USBAudio_Main(void) {
    void *i;
    
    if (USBFS_GetInterfaceSetting(TX_INTERFACE) == 1) {
        if (!USBAudio_TX_Enabled) {
            USBFS_EnableOutEP(TX_ENDPOINT);
            USBAudio_TX_Enabled = 1;
        }
    } else {
        if (USBAudio_TX_Enabled) {
            USBFS_DisableOutEP(TX_ENDPOINT);
            USBAudio_TX_Enabled = 0;
        }
    }
    if (USBFS_GetInterfaceSetting(SPKR_INTERFACE) == 1) {
        if (!USBAudio_SPKR_Enabled) {
            USBFS_EnableOutEP(SPKR_ENDPOINT);
            USBAudio_SPKR_Enabled = 1;
        }
    } else {
        if (USBAudio_SPKR_Enabled) {
            USBFS_DisableOutEP(SPKR_ENDPOINT);
            USBAudio_SPKR_Enabled = 0;
        }
    }

    USBAudio_RX_Enabled = (USBFS_GetInterfaceSetting(RX_INTERFACE) == 1);

    // Not sure why this is needed, but HDSDR fails without it.
    if(USBFS_IsConfigurationChanged() != 0u) {
        if (USBAudio_TX_Enabled) USBFS_EnableOutEP(TX_ENDPOINT);
        if (USBAudio_SPKR_Enabled) USBFS_EnableOutEP(SPKR_ENDPOINT);
    }

    
    if (USBFS_GetEPState(TX_ENDPOINT) == USBFS_OUT_BUFFER_FULL)
    {
        if (TX_Enabled) {
            USBFS_ReadOutEP(TX_ENDPOINT, PCM3060_TxBuf(), I2S_BUF_SIZE);
            USBFS_EnableOutEP(TX_ENDPOINT);
        } else {
            USBFS_ReadOutEP(TX_ENDPOINT, Void_Buff, I2S_BUF_SIZE);
            USBFS_EnableOutEP(TX_ENDPOINT);
        }
    }

    if (USBFS_GetEPState(SPKR_ENDPOINT) == USBFS_OUT_BUFFER_FULL)
    {
        if (TX_Enabled) {
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
        if (i = Mic_Buf()) {
		    USBFS_LoadInEP(MIC_ENDPOINT, i, MIC_BUF_SIZE);
		    USBFS_LoadInEP(MIC_ENDPOINT, 0, MIC_BUF_SIZE);
        }
	}

}
