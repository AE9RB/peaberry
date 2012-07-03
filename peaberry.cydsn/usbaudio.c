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
#define MIC_ENDPOINT             4
#define TX_INTERFACE             3
#define TX_ENDPOINT              3
#define SPKR_INTERFACE           6
#define SPKR_ENDPOINT            5

// For the unused speaker or transmit USB data.
volatile uint8 Void_Buff[I2S_BUF_SIZE];

// Using only four DMA buffers with fine tuning of the SOF sync,
// we can reduce overruns and underruns so they almost never happen.
void USBAudio_SyncBufs(uint8 dma, uint8* use, uint8* debounce, uint8 adjust) {
    if (*debounce) (*debounce)--;
    if (dma == *use) {
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


void USBAudio_Main(void) {
    void *i;
    
    if(USBFS_IsConfigurationChanged() != 0u) {
        if (USBFS_GetInterfaceSetting(TX_INTERFACE) == 1) {
            if(USBFS_DmaTd[TX_ENDPOINT] == DMA_INVALID_TD) {
                USBFS_ReadOutEP(TX_ENDPOINT, Void_Buff, I2S_BUF_SIZE);
            }
            USBFS_EnableOutEP(TX_ENDPOINT);
        }
        if (USBFS_GetInterfaceSetting(SPKR_INTERFACE) == 1) {
            if(USBFS_DmaTd[SPKR_ENDPOINT] == DMA_INVALID_TD) {
                USBFS_ReadOutEP(SPKR_ENDPOINT, Void_Buff, I2S_BUF_SIZE);
            }
            USBFS_EnableOutEP(SPKR_ENDPOINT);
        }
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
        if (i = Mic_Buf()) {
		    USBFS_LoadInEP(MIC_ENDPOINT, i, MIC_BUF_SIZE);
		    USBFS_LoadInEP(MIC_ENDPOINT, 0, MIC_BUF_SIZE);
        }
	}

}
