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

int8 nd(use, dma) {
    use *= DMA_USB_RATIO;
    if (dma < use) dma += DMA_AUDIO_BUFS;
    return use - dma;
}

// Using only four USB buffers with fine tuning of the SOF sync,
// we can eliminate all overruns and underruns.
void USBAudio_SyncBufs(uint8 dma, uint8* use, int8* distance) {
    static uint8 hold_timer=0;
    static uint8* hold_on;
    uint8 dma_adjusted, buf;
    int8 delta_distance, new_distance;
    
    buf = dma / DMA_USB_RATIO;
    if (dma & (DMA_USB_RATIO-1)) {
        dma_adjusted = buf + 1;
    } else {
        dma_adjusted = buf;
    }
    if (dma_adjusted >= USB_AUDIO_BUFS) dma_adjusted -= USB_AUDIO_BUFS;
    if (buf == *use || dma_adjusted == *use) {
        // underrun
        *use += USB_AUDIO_BUFS / 2;
        if (*use >= USB_AUDIO_BUFS) *use -= USB_AUDIO_BUFS;
        *distance = nd(*use, dma);
        return;
    }
    if (++*use == USB_AUDIO_BUFS) *use = 0;
    if (*use == buf) {
        // overrun
        *use += USB_AUDIO_BUFS / 2;
        if (*use >= USB_AUDIO_BUFS) *use -= USB_AUDIO_BUFS;
        *distance = nd(*use, dma);
        return;
    }
    // fine tune sof timer based on buffer slip
    new_distance = nd(*use, dma);
    if (!hold_timer || hold_on == use) {
        hold_on = use; // only adjust from one source at a time
        hold_timer = 10; // must be greater than number of interfaces
        delta_distance = *distance - new_distance;
        if (!delta_distance) return;
        if (delta_distance > 0) {
            SyncSOF_Slower();
        } else {
            SyncSOF_Faster();
        }
    } else {
        if (hold_timer) hold_timer--;
    }
    *distance = new_distance;
}

extern uint8 USBFS_currentVolume[];
extern uint8 USBFS_minimumVolume[];
extern uint8 USBFS_maximumVolume[];
extern uint8 USBFS_currentMute;

// Returns volume in the PCM3060 range of 54-255 where 54 is mute.
uint8 USBAudio_Volume(void) {
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

uint8 TX_Enabled, SPKR_Enabled, RX_Enabled, MIC_Enabled;
uint8 tx_reset, rx_reset, mic_reset;

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
    tx_reset = rx_reset = mic_reset = 1;
    TX_Enabled = SPKR_Enabled = RX_Enabled = MIC_Enabled = 0;
}

void USBAudio_Main(void) {
    uint8 outbound;
    
    outbound = TX_Enabled || SPKR_Enabled;
    
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
    
    if (!outbound && (TX_Enabled || SPKR_Enabled)) tx_reset = 1;

    if(USBFS_IsConfigurationChanged()) {
        USBFS_EnableOutEP(TX_ENDPOINT);
        USBFS_EnableOutEP(SPKR_ENDPOINT);
    }

    if (USBFS_GetInterfaceSetting(MIC_INTERFACE) == 1) {
        if (!MIC_Enabled) {
            MIC_Enabled = 1;
            mic_reset = 1;
        }
    } else {
        if (MIC_Enabled) {
            MIC_Enabled = 0;
        }
    }

    if (USBFS_GetInterfaceSetting(RX_INTERFACE) == 1) {
        if (!RX_Enabled) {
            RX_Enabled = 1;
            rx_reset = 1;
        }
    } else {
        if (RX_Enabled) {
            RX_Enabled = 0;
        }
    }

    
    if (USBFS_GetEPState(TX_ENDPOINT) == USBFS_OUT_BUFFER_FULL)
    {
        if (Control_Read() & CONTROL_TX_ENABLE) {
            USBFS_ReadOutEP(TX_ENDPOINT, PCM3060_TxBuf(&tx_reset), I2S_BUF_SIZE);
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
            USBFS_ReadOutEP(SPKR_ENDPOINT, PCM3060_TxBuf(&tx_reset), I2S_BUF_SIZE);
            USBFS_EnableOutEP(SPKR_ENDPOINT);
        }
    }

	if (USBFS_GetEPState(RX_ENDPOINT) == USBFS_IN_BUFFER_EMPTY) {
		USBFS_LoadInEP(RX_ENDPOINT, PCM3060_RxBuf(&rx_reset), I2S_BUF_SIZE);
		USBFS_LoadInEP(RX_ENDPOINT, 0, I2S_BUF_SIZE);
	}

	if (USBFS_GetEPState(MIC_ENDPOINT) == USBFS_IN_BUFFER_EMPTY) {
	    USBFS_LoadInEP(MIC_ENDPOINT, Mic_Buf(&mic_reset), MIC_BUF_SIZE);
	    USBFS_LoadInEP(MIC_ENDPOINT, 0, MIC_BUF_SIZE);
	}

}
