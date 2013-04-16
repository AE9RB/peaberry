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

// Constants from our USBFS enumeration
#define RX_ENDPOINT     2
#define RX_INTERFACE    2
#define TX_ENDPOINT     3
#define TX_INTERFACE    3

// IQ Reversal option
// 0: RX=norm TX=norm
// 1: RX=rev TX=norm
// 2: RX=norm TX=rev
// 3: RX=rev TX=rev
uint8 Audio_IQ_Channels;

// This implements similar logic as USBFS_LoadInEP.
void Audio_USB_LoadInEP(uint8 epNumber, uint8 *pData, uint16 length)
{
    uint8 ri, td_config;
    uint8 *p;

    ri = ((epNumber - USBFS_EP1) << USBFS_EPX_CNTX_ADDR_SHIFT);
    p = (uint8 *)&USBFS_ARB_RW1_DR_PTR[ri];
    if (Audio_IQ_Channels & 0x01) {
        td_config = TD_TERMIN_EN | TD_INC_SRC_ADR | TD_SWAP_EN;
    } else {
        td_config = TD_TERMIN_EN | TD_INC_SRC_ADR | TD_SWAP_EN | TD_SWAP_SIZE4;
    }
    CY_SET_REG8(&USBFS_SIE_EP1_CNT0_PTR[ri], (length >> 8u) | (USBFS_EP[epNumber].epToggle));
    CY_SET_REG8(&USBFS_SIE_EP1_CNT1_PTR[ri],  length & 0xFFu);
    CyDmaChDisable(USBFS_DmaChan[epNumber]);
    CyDmaTdSetConfiguration(USBFS_DmaTd[epNumber], length, USBFS_DmaTd[epNumber], td_config);
    CyDmaTdSetAddress(USBFS_DmaTd[epNumber],  LO16((uint32)pData), LO16((uint32)p));
    CyDmaClearPendingDrq(USBFS_DmaChan[epNumber]);
    CyDmaChSetInitialTd(USBFS_DmaChan[epNumber], USBFS_DmaTd[epNumber]);
    CyDmaChEnable(USBFS_DmaChan[epNumber], 1);
    USBFS_EP[epNumber].apiEpState = USBFS_NO_EVENT_PENDING;
    USBFS_ARB_EP1_CFG_PTR[ri] |= USBFS_ARB_EPX_CFG_IN_DATA_RDY;
}

// This implements similar logic as USBFS_ReadOutEP.
void Audio_USB_ReadOutEP(uint8 epNumber, uint8 *pData, uint16 length)
{
    uint8 ri, td_config;
    uint8 *p;

    ri = ((epNumber - USBFS_EP1) << USBFS_EPX_CNTX_ADDR_SHIFT);
    p = (uint8 *)&USBFS_ARB_RW1_DR_PTR[ri];
    if (Audio_IQ_Channels & 0x02) {
        td_config = TD_TERMIN_EN | TD_INC_DST_ADR | TD_SWAP_EN;
    } else {
        td_config = TD_TERMIN_EN | TD_INC_DST_ADR | TD_SWAP_EN | TD_SWAP_SIZE4;
    }
    CyDmaChDisable(USBFS_DmaChan[epNumber]);
    CyDmaTdSetConfiguration(USBFS_DmaTd[epNumber], length, USBFS_DmaTd[epNumber], td_config);
    CyDmaTdSetAddress(USBFS_DmaTd[epNumber],  LO16((uint32)p), LO16((uint32)pData));
    CyDmaClearPendingDrq(USBFS_DmaChan[epNumber]);
    CyDmaChSetInitialTd(USBFS_DmaChan[epNumber], USBFS_DmaTd[epNumber]);
    CyDmaChEnable(USBFS_DmaChan[epNumber], 1);
}

uint8 TX_Enabled;

void Audio_Start(void) {
    if(USBFS_DmaTd[RX_ENDPOINT] == DMA_INVALID_TD)
        USBFS_InitEP_DMA(RX_ENDPOINT, PCM3060_RxBuf());
    if(USBFS_DmaTd[TX_ENDPOINT] == DMA_INVALID_TD)
        USBFS_InitEP_DMA(TX_ENDPOINT, PCM3060_TxBuf());
    TX_Enabled = 0;
    Audio_USB_ReadOutEP(TX_ENDPOINT, PCM3060_TxBuf(), 0);
}

void Audio_Main(void) {

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
        Audio_USB_ReadOutEP(TX_ENDPOINT, PCM3060_TxBuf(), I2S_BUF_SIZE);
        USBFS_EnableOutEP(TX_ENDPOINT);
    }

    if (USBFS_GetEPState(RX_ENDPOINT) == USBFS_IN_BUFFER_EMPTY) {
		Audio_USB_LoadInEP(RX_ENDPOINT, PCM3060_RxBuf(), I2S_BUF_SIZE);
	}
}
