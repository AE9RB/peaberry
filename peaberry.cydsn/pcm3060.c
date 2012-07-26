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

// Delay a whole sample to swap endians on 24-bit words using DMA.
void LoadSwapOrder(uint8* a) {
    a[0] = 5;
    a[1] = 4;
    a[2] = 3;
    a[3] = 8;
    a[4] = 7;
    a[5] = 6;
    a[6] = 2;
    a[7] = 1;
    a[8] = 0;
}


uint8 RxI2S_Buff_Chan, RxI2S_Buff_TD[DMA_AUDIO_BUFS];
volatile uint8 RxI2S_Buff[USB_AUDIO_BUFS][I2S_BUF_SIZE], RxI2S_Swap[9], RxI2S_Move, RxI2S_DMA_Buf;

CY_ISR(RxI2S_DMA_done) {
    uint8 td, bufnum;
    td = DMAC_CH[RxI2S_Buff_Chan].basic_status[1] & 0x7Fu;
    bufnum = RxI2S_DMA_Buf + 1;
    if (bufnum >= DMA_AUDIO_BUFS) bufnum = 0;
    if (td != RxI2S_Buff_TD[bufnum]) {
        // resync, should only happen when debugging
        for (bufnum = 0; bufnum < DMA_AUDIO_BUFS; bufnum++) {
            if (td == RxI2S_Buff_TD[bufnum]) break;
        }
    }
    RxI2S_DMA_Buf = bufnum;
}

void DmaRxConfiguration(void)
{
    uint8 RxI2S_Swap_Chan, RxI2S_Stage_Chan, RxI2S_Stage_TD[9], RxI2S_Swap_TD[9];
	uint8 i, n, order[9];
    LoadSwapOrder(order);

    RxI2S_Stage_Chan = RxI2S_Stage_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
	for (i=0; i < 9; i++) RxI2S_Stage_TD[i]=CyDmaTdAllocate();
    for (i=0; i < 9; i++) {
	    n = i + 1;
	    if (n >= 9) n=0;
        CyDmaTdSetConfiguration(RxI2S_Stage_TD[i], 1, RxI2S_Stage_TD[n], RxI2S_Stage__TD_TERMOUT_EN );
	    CyDmaTdSetAddress(RxI2S_Stage_TD[i], LO16(I2S_RX_FIFO_0_PTR), LO16(&RxI2S_Swap[i]));
    }
	CyDmaChSetInitialTd(RxI2S_Stage_Chan, RxI2S_Stage_TD[0]);

    RxI2S_Swap_Chan = RxI2S_Swap_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
	for (i=0; i < 9; i++) RxI2S_Swap_TD[i]=CyDmaTdAllocate();
    for (i=0; i < 9; i++) {
        n = i + 1;
        if (n >= 9) n=0;
        CyDmaTdSetConfiguration(RxI2S_Swap_TD[i], 1, RxI2S_Swap_TD[n], RxI2S_Swap__TD_TERMOUT_EN);
        CyDmaTdSetAddress(RxI2S_Swap_TD[i], LO16(&RxI2S_Swap[order[i]]), LO16(&RxI2S_Move));
    }
	CyDmaChSetInitialTd(RxI2S_Swap_Chan, RxI2S_Swap_TD[0]);

    RxI2S_Buff_Chan = RxI2S_Buff_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
	for (i=0; i < DMA_AUDIO_BUFS; i++) RxI2S_Buff_TD[i] = CyDmaTdAllocate();
	for (i=0; i < DMA_AUDIO_BUFS; i++) {
	    CyDmaTdSetConfiguration(RxI2S_Buff_TD[i], I2S_BUF_SIZE/2, RxI2S_Buff_TD[i+1], TD_INC_DST_ADR | RxI2S_Buff__TD_TERMOUT_EN );	
	    CyDmaTdSetAddress(RxI2S_Buff_TD[i], LO16(&RxI2S_Move), LO16(RxI2S_Buff[i/2]));
        i++;
	 	n = i + 1;
		if (n >= DMA_AUDIO_BUFS) n=0;
	    CyDmaTdSetConfiguration(RxI2S_Buff_TD[i], I2S_BUF_SIZE/2, RxI2S_Buff_TD[n], TD_INC_DST_ADR | RxI2S_Buff__TD_TERMOUT_EN );	
	    CyDmaTdSetAddress(RxI2S_Buff_TD[i], LO16(&RxI2S_Move), LO16(RxI2S_Buff[i/2] + I2S_BUF_SIZE/2));
	}
	CyDmaChSetInitialTd(RxI2S_Buff_Chan, RxI2S_Buff_TD[0]);
	
    RxI2S_DMA_Buf = 0;
	RxI2S_done_isr_Start();
    RxI2S_done_isr_SetVector(RxI2S_DMA_done);

    CyDmaChEnable(RxI2S_Buff_Chan, 1u);
	CyDmaChEnable(RxI2S_Stage_Chan, 1u);
	CyDmaChEnable(RxI2S_Swap_Chan, 1u);
}


uint8 TxI2S_Buff_Chan, TxI2S_Buff_TD[DMA_AUDIO_BUFS], TxBufCountdown = 0, TxZero = 0;
volatile uint8 TxI2S_Buff[USB_AUDIO_BUFS][I2S_BUF_SIZE], TxI2S_Swap[9], TxI2S_Stage, TxI2S_DMA_Buf;

CY_ISR(TxI2S_DMA_done) {
    uint8 td, bufnum;
    if (TxBufCountdown) TxBufCountdown--;
    td = DMAC_CH[TxI2S_Buff_Chan].basic_status[1] & 0x7Fu;
    bufnum = TxI2S_DMA_Buf + 1;
    if (bufnum >= DMA_AUDIO_BUFS) bufnum = 0;
    if (td != TxI2S_Buff_TD[bufnum]) {
        // resync, should only happen when debugging
        for (bufnum = 0; bufnum < DMA_AUDIO_BUFS; bufnum++) {
            if (td == TxI2S_Buff_TD[bufnum]) break;
        }
    }
    TxI2S_DMA_Buf = bufnum;

}

void DmaTxConfiguration(void) {
    uint8 TxI2S_Swap_Chan, TxI2S_Swap_TD[9], TxI2S_Stage_Chan, TxI2S_Stage_TD[9];
	uint8 i, n, order[9], TxI2S_Zero_Chan, TxI2S_Zero_TD;
    LoadSwapOrder(order);
	
    TxI2S_Swap_Chan = TxI2S_Swap_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    for (i=0; i < 9; i++) TxI2S_Swap_TD[i]=CyDmaTdAllocate();
    for (i=0; i < 9; i++) {
	 	n = i + 1;
		if (n >= 9) n=0;
	    CyDmaTdSetConfiguration(TxI2S_Swap_TD[i], 1, TxI2S_Swap_TD[n], TxI2S_Swap__TD_TERMOUT_EN);
	    CyDmaTdSetAddress(TxI2S_Swap_TD[i], LO16(&TxI2S_Swap[order[i]]), LO16(I2S_TX_FIFO_0_PTR));
    }
	CyDmaChSetInitialTd(TxI2S_Swap_Chan, TxI2S_Swap_TD[0]);
    
    TxI2S_Stage_Chan = TxI2S_Stage_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    for (i=0; i < 9; i++) TxI2S_Stage_TD[i]=CyDmaTdAllocate();
    for (i=0; i < 9; i++) {
	 	n = i + 1;
		if (n >= 9) n=0;
	    CyDmaTdSetConfiguration(TxI2S_Stage_TD[i], 1, TxI2S_Stage_TD[n], TxI2S_Stage__TD_TERMOUT_EN);
	    CyDmaTdSetAddress(TxI2S_Stage_TD[i], LO16(&TxI2S_Stage), LO16(TxI2S_Swap + i));
    }
	CyDmaChSetInitialTd(TxI2S_Stage_Chan, TxI2S_Stage_TD[8]);

    TxI2S_Buff_Chan = TxI2S_Buff_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
	for (i=0; i < DMA_AUDIO_BUFS; i++) TxI2S_Buff_TD[i] = CyDmaTdAllocate();
	for (i=0; i < DMA_AUDIO_BUFS; i++) {
	    CyDmaTdSetConfiguration(TxI2S_Buff_TD[i], I2S_BUF_SIZE/2, TxI2S_Buff_TD[i+1], (TD_INC_SRC_ADR | TxI2S_Buff__TD_TERMOUT_EN) );	
	    CyDmaTdSetAddress(TxI2S_Buff_TD[i], LO16(TxI2S_Buff[i/2]), LO16(&TxI2S_Stage));
        i++;
	 	n = i + 1;
		if (n >= DMA_AUDIO_BUFS) n=0;
	    CyDmaTdSetConfiguration(TxI2S_Buff_TD[i], I2S_BUF_SIZE/2, TxI2S_Buff_TD[n], (TD_INC_SRC_ADR | TxI2S_Buff__TD_TERMOUT_EN) );	
	    CyDmaTdSetAddress(TxI2S_Buff_TD[i], LO16(TxI2S_Buff[i/2] + I2S_BUF_SIZE/2), LO16(&TxI2S_Stage));
	}
	CyDmaChSetInitialTd(TxI2S_Buff_Chan, TxI2S_Buff_TD[0]);

    TxI2S_Zero_Chan = TxI2S_Zero_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    TxI2S_Zero_TD = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(TxI2S_Zero_TD, I2S_BUF_SIZE * USB_AUDIO_BUFS, TxI2S_Zero_TD, TD_INC_DST_ADR );	
    CyDmaTdSetAddress(TxI2S_Zero_TD, LO16(&TxZero), LO16(TxI2S_Buff[0]));
	CyDmaChSetInitialTd(TxI2S_Zero_Chan, TxI2S_Zero_TD);

    TxI2S_DMA_Buf = 0;
    TxI2S_done_isr_Start();
    TxI2S_done_isr_SetVector(TxI2S_DMA_done);

    CyDmaChEnable(TxI2S_Zero_Chan, 1u);
    CyDmaChEnable(TxI2S_Buff_Chan, 1u);
	CyDmaChEnable(TxI2S_Stage_Chan, 1u);
	CyDmaChEnable(TxI2S_Swap_Chan, 1u);
}


uint8* PCM3060_TxBuf(void) {
    static uint8 debounce = 0, use = 0;
    USBAudio_SyncBufs(TxI2S_DMA_Buf, &use, &debounce);
    return TxI2S_Buff[use];
}

uint8* PCM3060_RxBuf(void) {
    static uint8 debounce = 0, use = 0;
    USBAudio_SyncBufs(RxI2S_DMA_Buf, &use, &debounce);
    return RxI2S_Buff[use];
}


void PCM3060_Start(void) {
	uint8 pcm3060_cmd[2], i, state = 0;

    DmaTxConfiguration();
    DmaRxConfiguration();
    
    I2S_Start();
    I2S_EnableTx();
    I2S_EnableRx();
    
    
    while (state < 2) {
        switch (state) {
        case 0: // Take PCM3060 out of sleep mode
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
}


void PCM3060_Main(void) {
    // 0-54 volume is mute, 53 = state mute, 54 = user mute
    static uint8 state = 0, volume = 0xFF, pcm3060_cmd[3];
    uint8 i;
    
    // Watch for TX/RX switching and ask the PCM3060 to mute
    // so we only toggle the transmit register while fully muted.
    // This also manages the speaker volume.
    switch (state) {
    case 0:
        if (!Locked_I2C) {
            if (Control_Read() & CONTROL_TX_ENABLE) {
                if (!TX_Request) {
                    state = 10;
                    volume = 53;
                    Locked_I2C = 1;
                } else if (volume == 53) {
                    state = 30;
                    volume = 0xFF;
                    Locked_I2C = 1;
                }
            } else { // not transmitting
                if (TX_Request) {
                    state = 20;
                    volume = 53;
                    Locked_I2C = 1;
                } else {
                    i = USBAudio_Volume();
                    if (volume != i) {
                        state = 30;
                        volume = i;
                        Locked_I2C = 1;
                    }
                }
            }
        }
        break;
    case 10:
    case 20:
    case 30:
    case 40:
    	pcm3060_cmd[0] = 0x41;
        pcm3060_cmd[1] = volume;
        pcm3060_cmd[2] = volume;
        I2C_MasterWriteBuf(PCM3060_ADDR, pcm3060_cmd, 3, I2C_MODE_COMPLETE_XFER);
        state++;
        break;
    case 11:
    case 21:
    case 31:
    case 41:
        i = I2C_MasterStatus();
        if (i & I2C_MSTAT_ERR_XFER) {
            state--;
        } else if (i & I2C_MSTAT_WR_CMPLT) {
            // Volume only moves 0.5dB every 8 samples
            // 67buf = 100dB / 0.5dB * 8samples / 24samples/buf
            TxBufCountdown = 67;
            Locked_I2C = 0;
            state++;
        }
        break;    
    case 12:
    case 22:
    case 32:
    case 42:
        // wait on PCM3060 to process full volume change
        if (!TxBufCountdown) state++;
        break;
    case 13:
        Control_Write(Control_Read() & ~CONTROL_TX_ENABLE);
        state = 0;
        break;
    case 23:
        Control_Write(Control_Read() | CONTROL_TX_ENABLE);
        state = 0;
        break;
    case 33:
        state = 0;
        break;
    }
}

