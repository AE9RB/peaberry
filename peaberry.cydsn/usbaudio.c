/* ========================================
 *
 * Copyright 2012 David Turnbull
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF DAVID TURNBULL.
 *
 * ========================================
*/

#include <peaberry.h>

#define RX_ENDPOINT              2
#define MIC_ENDPOINT             4
#define TX_INTERFACE             3
#define TX_ENDPOINT              3
#define SPKR_INTERFACE           6
#define SPKR_ENDPOINT            5

// For the unused speaker or transmit USB data.
volatile uint8 Void_Buff[I2S_BUF_SIZE];

// Using a minimum of four DMA buffers with fine tuning of the SOF sync,
// we can reduce overruns and underruns so they almost never happen.
void USBAudio_SyncBufs(uint8 dma, uint8* use, uint8* eat, uint8* debounce, uint8 adjust) {
    uint8 i;
    if (*debounce) (*debounce)--;
    if (!*eat && *use == dma) {
        *use += USB_AUDIO_BUFS - 1;
        if (*use >= USB_AUDIO_BUFS) *use -= USB_AUDIO_BUFS;
        *debounce = USB_AUDIO_BUFS;
        if (adjust) SyncSOF_Slower();
    } else {
        if (*eat) {
            i = dma;
            if (i != *use) {
                if (++i == USB_AUDIO_BUFS) i = 0;
                if (i != *use) return;
            }
            *eat = 0;
        }
        if (++*use == USB_AUDIO_BUFS) *use = 0;
        if ((*use == dma)) {
            *use += USB_AUDIO_BUFS - 1;
            if (*use >= USB_AUDIO_BUFS) *use -= USB_AUDIO_BUFS;
            if (!*debounce) {
                *eat = 1;
                if (adjust) SyncSOF_Faster();
            }
        }
    }    
}


void USBAudio_Main(void) {
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
		USBFS_LoadInEP(MIC_ENDPOINT, Mic_Buf(), MIC_BUF_SIZE);
		USBFS_LoadInEP(MIC_ENDPOINT, 0, MIC_BUF_SIZE);
	}

}
