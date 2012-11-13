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

#define PCM3060_ADDR 0x46
// Filter 0x30 enables pre-emphasis
#define PCM3060_SPKR_FILTER 0x00

static uint8 SwapOrderA[] = {5,4,3,8,7,6,2,1,0};
static uint8 SwapOrderB[] = {11,10,9,8,7,6,5,4,3,2,1,0};

volatile uint8 RxI2S_Buff[USB_AUDIO_BUFS][I2S_BUF_SIZE], RxI2S_Swap[12], RxI2S_Move;
uint8 RxI2S_Swap_Chan, RxI2S_Swap_TD[12];
uint8 RxI2S_Stage_Chan, RxI2S_Stage_TD[12];
uint8 RxI2S_Buff_Chan, RxI2S_Buff_TD;

void DmaRxSetup() {
    uint8 i;
    RxI2S_Stage_Chan = RxI2S_Stage_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    RxI2S_Swap_Chan = RxI2S_Swap_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    RxI2S_Buff_Chan = RxI2S_Buff_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    for (i=0; i < 12; i++) RxI2S_Stage_TD[i]=CyDmaTdAllocate();
    for (i=0; i < 12; i++) RxI2S_Swap_TD[i]=CyDmaTdAllocate();
    RxI2S_Buff_TD = CyDmaTdAllocate();
}

void DmaRxInit(uint8 reverse) {
    uint8 i, n, swapsize, *order;

    for (i=0; i<2; i++) {    
        CyDmaChSetRequest(RxI2S_Stage_Chan, CPU_TERM_CHAIN);
        CyDmaChSetRequest(RxI2S_Swap_Chan, CPU_TERM_CHAIN);
        CyDmaChSetRequest(RxI2S_Buff_Chan, CPU_TERM_CHAIN);
        CyDmaChEnable(RxI2S_Stage_Chan, 1u);
        CyDmaChEnable(RxI2S_Swap_Chan, 1u);
        CyDmaChEnable(RxI2S_Buff_Chan, 1u);
    }

    CyDmaChDisable(RxI2S_Stage_Chan);
    CyDmaChDisable(RxI2S_Swap_Chan);
    CyDmaChDisable(RxI2S_Buff_Chan);

    if (reverse) {
        order = SwapOrderA;
        swapsize = 9;
    } else {
        order = SwapOrderB;
        swapsize = 12;
    }

    for (i=0; i < swapsize; i++) {
        n = i + 1;
        if (n >= swapsize) n=0;
        CyDmaTdSetConfiguration(RxI2S_Stage_TD[i], 1, RxI2S_Stage_TD[n], RxI2S_Stage__TD_TERMOUT_EN );
        CyDmaTdSetAddress(RxI2S_Stage_TD[i], LO16(I2S_RX_FIFO_0_PTR), LO16(&RxI2S_Swap[i]));
    }
    CyDmaChSetInitialTd(RxI2S_Stage_Chan, RxI2S_Stage_TD[0]);

    for (i=0; i < swapsize; i++) {
        n = i + 1;
        if (n >= swapsize) n=0;
        CyDmaTdSetConfiguration(RxI2S_Swap_TD[i], 1, RxI2S_Swap_TD[n], RxI2S_Swap__TD_TERMOUT_EN);
        CyDmaTdSetAddress(RxI2S_Swap_TD[i], LO16(&RxI2S_Swap[order[i]]), LO16(&RxI2S_Move));
    }
    CyDmaChSetInitialTd(RxI2S_Swap_Chan, RxI2S_Swap_TD[0]);

    CyDmaTdSetConfiguration(RxI2S_Buff_TD, I2S_BUF_SIZE * USB_AUDIO_BUFS, RxI2S_Buff_TD, TD_INC_DST_ADR);    
    CyDmaTdSetAddress(RxI2S_Buff_TD, LO16(&RxI2S_Move), LO16(RxI2S_Buff[0]));
    CyDmaChSetInitialTd(RxI2S_Buff_Chan, RxI2S_Buff_TD);

    CyDmaChEnable(RxI2S_Buff_Chan, 1u);
    CyDmaChEnable(RxI2S_Swap_Chan, 1u);
    CyDmaChEnable(RxI2S_Stage_Chan, 1u);
}


uint8 TxZero = 0;
volatile uint8 TxI2S_Buff[USB_AUDIO_BUFS][I2S_BUF_SIZE], TxI2S_Swap[12], TxI2S_Stage;
uint8 TxI2S_Swap_Chan, TxI2S_Swap_TD[12];
uint8 TxI2S_Stage_Chan, TxI2S_Stage_TD[12];
uint8 TxI2S_Buff_Chan, TxI2S_Buff_TD;
uint8 TxI2S_Zero_Chan, TxI2S_Zero_TD;

void DmaTxSetup() {
    uint8 i;
    TxI2S_Swap_Chan = TxI2S_Swap_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    TxI2S_Buff_Chan = TxI2S_Buff_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    TxI2S_Stage_Chan = TxI2S_Stage_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    TxI2S_Zero_Chan = TxI2S_Zero_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    for (i=0; i < 12; i++) TxI2S_Swap_TD[i]=CyDmaTdAllocate();
    for (i=0; i < 12; i++) TxI2S_Stage_TD[i]=CyDmaTdAllocate();
    TxI2S_Buff_TD = CyDmaTdAllocate();
    TxI2S_Zero_TD = CyDmaTdAllocate();
}

void DmaTxInit(uint8 reverse) {
    uint8 i, n, swapsize, start, *order;

    for (i=0; i<2; i++) {    
        CyDmaChSetRequest(TxI2S_Swap_Chan, CPU_TERM_CHAIN);
        CyDmaChSetRequest(TxI2S_Stage_Chan, CPU_TERM_CHAIN);
        CyDmaChSetRequest(TxI2S_Buff_Chan, CPU_TERM_CHAIN);
        CyDmaChSetRequest(TxI2S_Zero_Chan, CPU_TERM_CHAIN);
        CyDmaChEnable(TxI2S_Swap_Chan, 1u);
        CyDmaChEnable(TxI2S_Stage_Chan, 1u);
        CyDmaChEnable(TxI2S_Buff_Chan, 1u);
        CyDmaChEnable(TxI2S_Zero_Chan, 1u);
    }
    
    CyDmaChDisable(TxI2S_Swap_Chan);
    CyDmaChDisable(TxI2S_Stage_Chan);
    CyDmaChDisable(TxI2S_Buff_Chan);
    CyDmaChDisable(TxI2S_Zero_Chan);

    if (reverse) {
        order = SwapOrderA;
        swapsize = 9;
        start = 3;
    } else {
        order = SwapOrderB;
        swapsize = 12;
        start = 9;
    }
    
    for (i=0; i < swapsize; i++) {
        n = i + 1;
        if (n >= swapsize) n=0;
        CyDmaTdSetConfiguration(TxI2S_Swap_TD[i], 1, TxI2S_Swap_TD[n], TxI2S_Swap__TD_TERMOUT_EN);
        CyDmaTdSetAddress(TxI2S_Swap_TD[i], LO16(&TxI2S_Swap[order[i]]), LO16(I2S_TX_FIFO_0_PTR));
    }
    CyDmaChSetInitialTd(TxI2S_Swap_Chan, TxI2S_Swap_TD[start]);

    for (i=0; i < swapsize; i++) {
        n = i + 1;
        if (n >= swapsize) n=0;
        CyDmaTdSetConfiguration(TxI2S_Stage_TD[i], 1, TxI2S_Stage_TD[n], TxI2S_Stage__TD_TERMOUT_EN);
        CyDmaTdSetAddress(TxI2S_Stage_TD[i], LO16(&TxI2S_Stage), LO16(&TxI2S_Swap[i]));
    }
    CyDmaChSetInitialTd(TxI2S_Stage_Chan, TxI2S_Stage_TD[swapsize-1]);

    CyDmaTdSetConfiguration(TxI2S_Buff_TD, I2S_BUF_SIZE * USB_AUDIO_BUFS, TxI2S_Buff_TD, TD_INC_SRC_ADR | TxI2S_Buff__TD_TERMOUT_EN);    
    CyDmaTdSetAddress(TxI2S_Buff_TD, LO16(TxI2S_Buff[0]), LO16(&TxI2S_Stage));
    CyDmaChSetInitialTd(TxI2S_Buff_Chan, TxI2S_Buff_TD);

    CyDmaTdSetConfiguration(TxI2S_Zero_TD, I2S_BUF_SIZE * USB_AUDIO_BUFS, TxI2S_Zero_TD, TD_INC_DST_ADR );    
    CyDmaTdSetAddress(TxI2S_Zero_TD, LO16(&TxZero), LO16(TxI2S_Buff[0]));
    CyDmaChSetInitialTd(TxI2S_Zero_Chan, TxI2S_Zero_TD);

    CyDmaChEnable(TxI2S_Buff_Chan, 1u);
    CyDmaChSetRequest(TxI2S_Stage_Chan, CPU_REQ);
    CyDmaChEnable(TxI2S_Stage_Chan, 1u);
    CyDmaChSetRequest(TxI2S_Swap_Chan, CPU_REQ);
    CyDmaChEnable(TxI2S_Swap_Chan, 1u);
    CyDmaChEnable(TxI2S_Zero_Chan, 1u);

}


uint8* PCM3060_TxBuf(void) {
    return TxI2S_Buff[SyncSOF_USB_Buffer()];
}

uint8* PCM3060_RxBuf(void) {
    return RxI2S_Buff[SyncSOF_USB_Buffer()];
}

void PCM3060_Setup(void) {
    uint8 pcm3060_cmd[2], i, state = 0;
    
    // Take PCM3060 out of sleep mode
    while (state < 2) {
        switch (state) {
        case 0:
            pcm3060_cmd[0] = 0x40;
            pcm3060_cmd[1] = 0xC0;
            I2C_MasterWriteBuf(PCM3060_ADDR, pcm3060_cmd, 2, I2C_MODE_COMPLETE_XFER);
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

    DmaTxSetup();
    DmaRxSetup();
    I2S_Start();
}

void PCM3060_Init(void) {
    I2S_DisableRx();
    I2S_DisableTx();
    DmaRxInit(Audio_IQ_Channels & 0x01);
    DmaTxInit(Audio_IQ_Channels & 0x02);
}


void PCM3060_Start(void) {
    I2S_EnableRx();
    I2S_EnableTx();
}


void PCM3060_Main(void) {
    // 0-54 volume is mute, 53 = state mute, 54 = user mute
    static uint8 state = 0, volume = 0, filter = 0, pcm3060_cmd[3];
    uint8 i;
    
    // Watch for TX/RX switching and ask the PCM3060 to mute
    // so we only toggle the transmit register while fully muted.
    // This also manages the speaker volume.
    switch (state) {
    case 0:
        if (!Locked_I2C) {
            if (IQGen_GetTransmit()) {
                if (!TX_Request) {
                    state = 10;
                    volume = 53;
                    filter = PCM3060_SPKR_FILTER;
                    Locked_I2C = 1;
                } else if (volume == 53) {
                    state = 30;
                    volume = 0xFF;
                    filter = 0;
                    Locked_I2C = 1;
                }
            } else { // not transmitting
                if (TX_Request) {
                    state = 20;
                    volume = 53;
                    filter = 0;
                    Locked_I2C = 1;
                } else {
                    i = USBAudio_Volume();
                    if (volume != i) {
                        state = 30;
                        volume = i;
                        filter = PCM3060_SPKR_FILTER;
                        Locked_I2C = 1;
                    }
                }
            }
        }
        break;
    case 10:
    case 20:
    case 30:
        pcm3060_cmd[0] = 0x41;
        pcm3060_cmd[1] = volume;
        pcm3060_cmd[2] = volume;
        I2C_MasterWriteBuf(PCM3060_ADDR, pcm3060_cmd, 3, I2C_MODE_COMPLETE_XFER);
        state++;
        break;
    case 11:
    case 21:
    case 31:
        i = I2C_MasterStatus();
        if (i & I2C_MSTAT_ERR_XFER) {
            state--;
        } else if (i & I2C_MSTAT_WR_CMPLT) {
            // Volume only moves 0.5dB every 8 samples
            // 34buf = 100dB / 0.5dB * 8samples / 48samples/ms
            SyncSOF_SetCountdown(34);
            state++;
        }
        break;    
    case 12:
    case 22:
    case 32:
        // wait on PCM3060 to process full volume change
        if (!SyncSOF_GetCountdown()) state++;
        break;
    case 13:
    case 23:
    case 33:
        pcm3060_cmd[0] = 0x45;
        pcm3060_cmd[1] = filter;
        I2C_MasterWriteBuf(PCM3060_ADDR, pcm3060_cmd, 2, I2C_MODE_COMPLETE_XFER);
        state++;
        break;
    case 14:
    case 24:
    case 34:
        i = I2C_MasterStatus();
        if (i & I2C_MSTAT_ERR_XFER) {
            state--;
        } else if (i & I2C_MSTAT_WR_CMPLT) {
            Locked_I2C = 0;
            state++;
        }
        break;    
    case 15:
        IQGen_SetTransmit(0);
        Audio_Set_Speaker();
        state = 0;
        break;
    case 25:
        IQGen_SetTransmit(1);
        Audio_Set_Speaker();
        state = 0;
        break;
    case 35:
        state = 0;
        break;
    }
}

