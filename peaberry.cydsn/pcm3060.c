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

#include <device.h>
#include <main.h>
#include <pcm3060.h>
#include <syncsof.h>

#define PCM3060_ADDR 0x46
#define PCM3060_REG 0x40

// 48 24-bit stereo samples every 1 ms
#define I2S_BUF_SIZE             (48u * 3 * 2)
#define I2S_RX_BUFS              3u
#define I2S_TX_BUFS              4u
#define RX_ENDPOINT              2u
#define TX_INTERFACE             3u
#define TX_ENDPOINT              3u
#define SPKR_INTERFACE           6u
#define SPKR_ENDPOINT            5u

volatile uint8 Void_Buff[I2S_BUF_SIZE];

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


uint8 RxI2S_Buff_Chan, RxI2S_Buff_TD[I2S_RX_BUFS];
volatile uint8 RxI2S_Buff[I2S_RX_BUFS][I2S_BUF_SIZE], RxI2S_Swap[9], RxI2S_Move;
volatile uint8 RxI2S_Use_Buf = I2S_RX_BUFS - 1;

CY_ISR(RxI2S_DMA_done) {
    uint8 i, td;
    td = DMAC_CH[RxI2S_Buff_Chan].basic_status[1] & 0x7Fu;
    for (i=0;i<I2S_RX_BUFS;i++) {
        if (td == RxI2S_Buff_TD[i]) break;
    }
    //TODO true sync
    if (i==0) {
        RxI2S_Use_Buf = I2S_RX_BUFS-1;
    } else {
        RxI2S_Use_Buf = i - 1;
    }
    //RxI2S_DMA_Buf = i;
}

void DmaRxConfiguration(void)
{
    uint8 RxI2S_Swap_Chan, RxI2S_Stage_Chan, RxI2S_Stage_TD, RxI2S_Swap_TD[9];
	uint8 i, n, order[9];
    LoadSwapOrder(order);

    RxI2S_Stage_Chan = RxI2S_Stage_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
	RxI2S_Stage_TD=CyDmaTdAllocate();
	CyDmaTdSetConfiguration(RxI2S_Stage_TD, 9, RxI2S_Stage_TD, TD_INC_DST_ADR );
	CyDmaTdSetAddress(RxI2S_Stage_TD, LO16((uint32)I2S_RX_FIFO_0_PTR), LO16((uint32)RxI2S_Swap));
	CyDmaChSetInitialTd(RxI2S_Stage_Chan, RxI2S_Stage_TD);

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
	for (i=0; i < I2S_RX_BUFS; i++) RxI2S_Buff_TD[i] = CyDmaTdAllocate();
	for (i=0; i < I2S_RX_BUFS; i++) {
	 	n = i + 1;
		if (n >= I2S_RX_BUFS) n=0;
	    CyDmaTdSetConfiguration(RxI2S_Buff_TD[i], I2S_BUF_SIZE, RxI2S_Buff_TD[n], TD_INC_DST_ADR | RxI2S_Buff__TD_TERMOUT_EN );	
	    CyDmaTdSetAddress(RxI2S_Buff_TD[i], LO16((uint32)&RxI2S_Move), LO16((uint32)RxI2S_Buff[i]));
	}
	CyDmaChSetInitialTd(RxI2S_Buff_Chan, RxI2S_Buff_TD[0u]);
	
	RxI2S_done_isr_Start();
    RxI2S_done_isr_SetVector(RxI2S_DMA_done);

    CyDmaChEnable(RxI2S_Buff_Chan, 1u);
	CyDmaChEnable(RxI2S_Stage_Chan, 1u);
	CyDmaChEnable(RxI2S_Swap_Chan, 1u);
}


uint8 TxI2S_Buff_Chan, TxI2S_Buff_TD[I2S_TX_BUFS];
volatile uint8 TxI2S_Buff[I2S_TX_BUFS][I2S_BUF_SIZE], TxI2S_Swap[9], TxI2S_Stage;
volatile uint8 TxI2S_DMA_Buf = 0;

CY_ISR(TxI2S_DMA_done) {
    uint8 i, td;
    td = DMAC_CH[TxI2S_Buff_Chan].basic_status[1] & 0x7Fu;
    for (i=0;i<I2S_TX_BUFS;i++) {
        if (td == TxI2S_Buff_TD[i]) break;
    }
    TxI2S_DMA_Buf = i;
}

void DmaTxConfiguration(void) {
    uint8 TxI2S_Swap_Chan, TxI2S_Swap_TD[9], TxI2S_Stage_Chan, TxI2S_Stage_TD[9];
	uint8 i, n, order[9];
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
	for (i=0; i < I2S_TX_BUFS; i++) TxI2S_Buff_TD[i] = CyDmaTdAllocate();
	for (i=0; i < I2S_TX_BUFS; i++) {
	 	n = i + 1;
		if (n >= I2S_TX_BUFS) {n=0;}
	    CyDmaTdSetConfiguration(TxI2S_Buff_TD[i], I2S_BUF_SIZE, TxI2S_Buff_TD[n], (TD_INC_SRC_ADR | TxI2S_Buff__TD_TERMOUT_EN) );	
	    CyDmaTdSetAddress(TxI2S_Buff_TD[i], LO16(TxI2S_Buff[i]), LO16(&TxI2S_Stage));
	}
	CyDmaChSetInitialTd(TxI2S_Buff_Chan, TxI2S_Buff_TD[0]);

    TxI2S_done_isr_Start();
    TxI2S_done_isr_SetVector(TxI2S_DMA_done);

    CyDmaChEnable(TxI2S_Buff_Chan, 1u);
	CyDmaChEnable(TxI2S_Stage_Chan, 1u);
	CyDmaChEnable(TxI2S_Swap_Chan, 1u);
}



uint8 SyncTxBufs(void) {
    //static uint8 under=0, over=0;
    static uint8 TxI2S_Eat_Bufs = 0, TxI2S_Debounce = 0, TxI2S_Use_Buf = 0;
    uint8 thisDMA, i;
    thisDMA = TxI2S_DMA_Buf;
    if (TxI2S_Debounce) TxI2S_Debounce--;
    if (!TxI2S_Eat_Bufs && TxI2S_Use_Buf == thisDMA) {
        // underrun
        TxI2S_Use_Buf += I2S_TX_BUFS - 1;
        if (TxI2S_Use_Buf >= I2S_TX_BUFS) TxI2S_Use_Buf -= I2S_TX_BUFS;
        TxI2S_Debounce = I2S_TX_BUFS;
        //SyncSOF_Slower();
        //under++;
    } else {
        if (TxI2S_Eat_Bufs) {
            i = thisDMA;
            if (i != TxI2S_Use_Buf) {
                if (++i == I2S_TX_BUFS) i = 0;
                if (i != TxI2S_Use_Buf) return TxI2S_Use_Buf;
            }
            TxI2S_Eat_Bufs = 0;
        }
        if (++TxI2S_Use_Buf == I2S_TX_BUFS) TxI2S_Use_Buf = 0;
        if ((TxI2S_Use_Buf == thisDMA)) {
            // overrun, or we need to back up one more for previous underrun
            TxI2S_Use_Buf += I2S_TX_BUFS - 1;
            if (TxI2S_Use_Buf >= I2S_TX_BUFS) TxI2S_Use_Buf -= I2S_TX_BUFS;
            if (!TxI2S_Debounce) {
                TxI2S_Eat_Bufs = 1;
                //SyncSOF_Faster();
                //over++;
            }
        }
    }    
    return TxI2S_Use_Buf;
}

extern uint8 USBFS_DmaTd[USBFS_MAX_EP];

void PCM3060_Main(void) {
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
            USBFS_ReadOutEP(TX_ENDPOINT, TxI2S_Buff[SyncTxBufs()], I2S_BUF_SIZE);
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
            USBFS_ReadOutEP(SPKR_ENDPOINT, TxI2S_Buff[SyncTxBufs()], I2S_BUF_SIZE);
            USBFS_EnableOutEP(SPKR_ENDPOINT);
        }
    }

	if (USBFS_GetEPState(RX_ENDPOINT) == USBFS_IN_BUFFER_EMPTY) {
		USBFS_LoadInEP(RX_ENDPOINT, RxI2S_Buff[RxI2S_Use_Buf], I2S_BUF_SIZE);
		USBFS_LoadInEP(RX_ENDPOINT, 0, I2S_BUF_SIZE);
	}
}

void PCM3060_Start(void) {
	uint8 pcm3060_cmd[2];

    DmaRxConfiguration();
    DmaTxConfiguration();
    
    I2S_Start();
    I2S_EnableTx();
    I2S_EnableRx();

    // Take PCM3060 out of sleep mode
    //TODO error handling
	pcm3060_cmd[0] = 0x40;
    pcm3060_cmd[1] = 0xC0;
    I2C_MasterWriteBuf(PCM3060_ADDR, pcm3060_cmd, 2, I2C_MODE_COMPLETE_XFER);
    while(0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)) {}
}
