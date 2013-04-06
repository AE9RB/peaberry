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

#define PCM3060_I2C_ADDR 0x46

volatile uint8 RxI2S[USB_AUDIO_BUFS][I2S_BUF_SIZE], RxI2S_Stage;
volatile uint8 TxI2S[USB_AUDIO_BUFS][I2S_BUF_SIZE], TxI2S_Stage, TxI2S_Zero = 0;

uint8 RxI2S_Stage_TD[3], RxI2S_Buff_TD[USB_AUDIO_BUFS];

void DmaRxInit() {
    uint8 i;
    RxI2S_Stage_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    RxI2S_Buff_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    for (i=0; i < 3; i++) RxI2S_Stage_TD[i]=CyDmaTdAllocate();
    for (i=0; i < USB_AUDIO_BUFS; i++) RxI2S_Buff_TD[i]=CyDmaTdAllocate();
}

void DmaRxStart(void) {
    uint8 i, n;

    for (i=0; i < 3; i++) {
        if (i==2) {
            CyDmaTdSetConfiguration(RxI2S_Stage_TD[i], 1, RxI2S_Stage_TD[0], 0 );
        } else {
            CyDmaTdSetConfiguration(RxI2S_Stage_TD[i], 1, RxI2S_Stage_TD[i+1], RxI2S_Stage__TD_TERMOUT_EN );
        }
        CyDmaTdSetAddress(RxI2S_Stage_TD[i], LO16(I2S_RX_FIFO_0_PTR), LO16(&RxI2S_Stage));
    }
    CyDmaClearPendingDrq(RxI2S_Stage_DmaHandle);
    CyDmaChSetInitialTd(RxI2S_Stage_DmaHandle, RxI2S_Stage_TD[0]);

    for (i=0; i < USB_AUDIO_BUFS; i++) {
        n = i + 1;
        if (n >= USB_AUDIO_BUFS) n=0;
        CyDmaTdSetConfiguration(RxI2S_Buff_TD[i], I2S_BUF_SIZE, RxI2S_Buff_TD[n], TD_INC_DST_ADR);
        CyDmaTdSetAddress(RxI2S_Buff_TD[i], LO16(&RxI2S_Stage), LO16((uint32)RxI2S[i]));
    }
    CyDmaClearPendingDrq(RxI2S_Buff_DmaHandle);
    CyDmaChSetInitialTd(RxI2S_Buff_DmaHandle, RxI2S_Buff_TD[0]);

    CyDmaChEnable(RxI2S_Buff_DmaHandle, 1u);
    CyDmaChEnable(RxI2S_Stage_DmaHandle, 1u);
}


uint8 TxI2S_Stage_TD[3], TxI2S_Buff_TD[USB_AUDIO_BUFS], TxI2S_Zero_TD[USB_AUDIO_BUFS];

void DmaTxInit() {
    uint8 i;
    TxI2S_Buff_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    TxI2S_Stage_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    TxI2S_Zero_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    for (i=0; i < 3; i++) TxI2S_Stage_TD[i]=CyDmaTdAllocate();
    for (i=0; i < USB_AUDIO_BUFS; i++) TxI2S_Buff_TD[i]=CyDmaTdAllocate();
    for (i=0; i < USB_AUDIO_BUFS; i++) TxI2S_Zero_TD[i]=CyDmaTdAllocate();
}

void DmaTxStart(void) {
    uint8 i, n;
    
    for (i=0; i < 3; i++) {
        if (i==2) {
            CyDmaTdSetConfiguration(TxI2S_Stage_TD[i], 1, TxI2S_Stage_TD[0], 0 );
            CyDmaTdSetAddress(TxI2S_Stage_TD[i], LO16(&TxI2S_Zero), LO16(I2S_TX_FIFO_0_PTR));
        } else {
            CyDmaTdSetConfiguration(TxI2S_Stage_TD[i], 1, TxI2S_Stage_TD[i+1], TxI2S_Stage__TD_TERMOUT_EN );
            CyDmaTdSetAddress(TxI2S_Stage_TD[i], LO16(&TxI2S_Stage), LO16(I2S_TX_FIFO_0_PTR));
        }
    }
    CyDmaClearPendingDrq(TxI2S_Stage_DmaHandle);
    CyDmaChSetInitialTd(TxI2S_Stage_DmaHandle, TxI2S_Stage_TD[0]);
    
    for (i=0; i < USB_AUDIO_BUFS; i++) {
        n = i + 1;
        if (n >= USB_AUDIO_BUFS) {
            n=0;
            CyDmaTdSetConfiguration(TxI2S_Buff_TD[i], I2S_BUF_SIZE, TxI2S_Buff_TD[n], TD_INC_SRC_ADR | TxI2S_Buff__TD_TERMOUT_EN);    
        } else {
            CyDmaTdSetConfiguration(TxI2S_Buff_TD[i], I2S_BUF_SIZE, TxI2S_Buff_TD[n], TD_INC_SRC_ADR);    
        }
        CyDmaTdSetAddress(TxI2S_Buff_TD[i], LO16(TxI2S[i]), LO16(&TxI2S_Stage));
        CyDmaTdSetConfiguration(TxI2S_Zero_TD[i], I2S_BUF_SIZE, TxI2S_Zero_TD[n], TD_INC_DST_ADR );
        CyDmaTdSetAddress(TxI2S_Zero_TD[i], LO16(&TxI2S_Zero), LO16(TxI2S[i]));
    }
    CyDmaClearPendingDrq(TxI2S_Buff_DmaHandle);
    CyDmaChSetInitialTd(TxI2S_Buff_DmaHandle, TxI2S_Buff_TD[0]);
    CyDmaClearPendingDrq(TxI2S_Zero_DmaHandle);
    CyDmaChSetInitialTd(TxI2S_Zero_DmaHandle, TxI2S_Zero_TD[0]);

    CyDmaChEnable(TxI2S_Zero_DmaHandle, 1u);
    CyDmaChSetRequest(TxI2S_Buff_DmaHandle, CPU_REQ);
    CyDmaChEnable(TxI2S_Buff_DmaHandle, 1u);
    CyDmaChEnable(TxI2S_Stage_DmaHandle, 1u);
}

uint8* PCM3060_TxBuf(void) {
    return TxI2S[SyncSOF_USB_Buffer()];
}

uint8* PCM3060_RxBuf(void) {
    return RxI2S[SyncSOF_USB_Buffer()];
}

void PCM3060_SetRegister(uint8 reg, uint8 val) {
    uint8 pcm3060_cmd[2], i, state = 0;

    while (state < 2) {
        switch (state) {
        case 0:
            pcm3060_cmd[0] = reg;
            pcm3060_cmd[1] = val;
            I2C_MasterWriteBuf(PCM3060_I2C_ADDR, pcm3060_cmd, 2, I2C_MODE_COMPLETE_XFER);
            state++;
            break;
        case 1:
            i = I2C_MasterStatus();
            if (i & I2C_MSTAT_ERR_XFER) {
                state--;
            } else if (i & I2C_MSTAT_WR_CMPLT) {
                state++;
            }
            break;
        }
    }
}

void PCM3060_Init(void) {
    I2S_Start();
    DmaRxInit();
    DmaTxInit();
}

void PCM3060_Start(void) {
    PCM3060_SetRegister(0x40, 0xC0); // Wakeup
    DmaRxStart();
    DmaTxStart();
    I2S_EnableRx();
    I2S_EnableTx();
}


void PCM3060_Stop(void) {
    PCM3060_SetRegister(0x40, 0xF0); // Sleep
    I2S_DisableRx();
    I2S_DisableTx();
    CyDmaChDisable(RxI2S_Stage_DmaHandle);
    CyDmaChDisable(RxI2S_Buff_DmaHandle);
    CyDmaChDisable(TxI2S_Stage_DmaHandle);
    CyDmaChDisable(TxI2S_Buff_DmaHandle);
    CyDmaChDisable(TxI2S_Zero_DmaHandle);
    I2S_ClearRxFIFO();
    I2S_ClearTxFIFO();
}

