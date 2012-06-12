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
#include <pcm3060.h>

#define PCM3060_ADDR 0x46
#define PCM3060_REG 0x40

// 48 24-bit stereo samples every 1 ms, triple buffered
#define I2S_BUF_SIZE                (48u * 3 * 2)
#define I2S_NUM_BUFS                3u
#define RxI2S_ENDPOINT              2u
#define TxI2S_INTERFACE             3u
#define TxI2S_ENDPOINT              3u

uint8 RxI2S_Buff_Chan, RxI2S_Buff_TD[I2S_NUM_BUFS];
volatile uint8 RxI2S_Buff[I2S_NUM_BUFS][I2S_BUF_SIZE], RxI2S_Swap[9], RxI2S_Move;
volatile uint8 RxI2S_In_Progress_TD = 0u;

CY_ISR(RxI2S_DMA_done) {
    // Equivalent to the following but avoiding compiler warnings:
	// CyDmaChStatus(RxI2S_Buff_Chan, &RxI2S_In_Progress_TD, 0);
    RxI2S_In_Progress_TD = DMAC_CH[RxI2S_Buff_Chan].basic_status[1] & 0x7Fu;
}

void DmaRxConfiguration(void)
{
    uint8 RxI2S_Swap_Chan, RxI2S_Stage_Chan, RxI2S_Stage_TD, RxI2S_Swap_TD[9];
	uint8 i, n;

    RxI2S_Stage_Chan = RxI2S_Stage_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
	RxI2S_Stage_TD=CyDmaTdAllocate();
	CyDmaTdSetConfiguration(RxI2S_Stage_TD, 9, RxI2S_Stage_TD, TD_INC_DST_ADR );
	CyDmaTdSetAddress(RxI2S_Stage_TD, LO16((uint32)I2S_RX_FIFO_0_PTR), LO16((uint32)RxI2S_Swap));
	CyDmaChSetInitialTd(RxI2S_Stage_Chan, RxI2S_Stage_TD);

    RxI2S_Swap_Chan = RxI2S_Swap_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
	for (i=0 ;i<9; i++) {RxI2S_Swap_TD[i]=CyDmaTdAllocate();}
	CyDmaTdSetConfiguration(RxI2S_Swap_TD[0], 1, RxI2S_Swap_TD[1], RxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(RxI2S_Swap_TD[0], LO16((uint32)RxI2S_Swap + 5), LO16((uint32)&RxI2S_Move));
	CyDmaTdSetConfiguration(RxI2S_Swap_TD[1], 1, RxI2S_Swap_TD[2], RxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(RxI2S_Swap_TD[1], LO16((uint32)RxI2S_Swap + 4), LO16((uint32)&RxI2S_Move));
	CyDmaTdSetConfiguration(RxI2S_Swap_TD[2], 1, RxI2S_Swap_TD[3], RxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(RxI2S_Swap_TD[2], LO16((uint32)RxI2S_Swap + 3), LO16((uint32)&RxI2S_Move));
	CyDmaTdSetConfiguration(RxI2S_Swap_TD[3], 1, RxI2S_Swap_TD[4], RxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(RxI2S_Swap_TD[3], LO16((uint32)RxI2S_Swap + 8), LO16((uint32)&RxI2S_Move));
	CyDmaTdSetConfiguration(RxI2S_Swap_TD[4], 1, RxI2S_Swap_TD[5], RxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(RxI2S_Swap_TD[4], LO16((uint32)RxI2S_Swap + 7), LO16((uint32)&RxI2S_Move));
	CyDmaTdSetConfiguration(RxI2S_Swap_TD[5], 1, RxI2S_Swap_TD[6], RxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(RxI2S_Swap_TD[5], LO16((uint32)RxI2S_Swap + 6), LO16((uint32)&RxI2S_Move));
	CyDmaTdSetConfiguration(RxI2S_Swap_TD[6], 1, RxI2S_Swap_TD[7], RxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(RxI2S_Swap_TD[6], LO16((uint32)RxI2S_Swap + 2), LO16((uint32)&RxI2S_Move));
	CyDmaTdSetConfiguration(RxI2S_Swap_TD[7], 1, RxI2S_Swap_TD[8], RxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(RxI2S_Swap_TD[7], LO16((uint32)RxI2S_Swap + 1), LO16((uint32)&RxI2S_Move));
	CyDmaTdSetConfiguration(RxI2S_Swap_TD[8], 1, RxI2S_Swap_TD[0], RxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(RxI2S_Swap_TD[8], LO16((uint32)RxI2S_Swap + 0), LO16((uint32)&RxI2S_Move));
	CyDmaChSetInitialTd(RxI2S_Swap_Chan, RxI2S_Swap_TD[0]);

    RxI2S_Buff_Chan = RxI2S_Buff_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
	for (i=0; i< I2S_NUM_BUFS; i++) {
	    RxI2S_Buff_TD[i] = CyDmaTdAllocate();
	}
	for (i=0; i< I2S_NUM_BUFS; i++) {
	 	n = i + 1;
		if (n >= I2S_NUM_BUFS) {n=0;}
	    CyDmaTdSetConfiguration(RxI2S_Buff_TD[i], I2S_BUF_SIZE, RxI2S_Buff_TD[n], TD_INC_DST_ADR | RxI2S_Buff__TD_TERMOUT_EN );	
	    CyDmaTdSetAddress(RxI2S_Buff_TD[i], LO16((uint32)&RxI2S_Move), LO16((uint32)RxI2S_Buff[i]));
	}
	CyDmaChSetInitialTd(RxI2S_Buff_Chan, RxI2S_Buff_TD[0u]);
		
	RxI2S_done_isr_Start();
    RxI2S_done_isr_SetVector(RxI2S_DMA_done);

	CyDmaChEnable(RxI2S_Stage_Chan, 1u);
	CyDmaChEnable(RxI2S_Swap_Chan, 1u);
    CyDmaChEnable(RxI2S_Buff_Chan, 1u);
}


uint8 TxI2S_Buff_Chan, TxI2S_Buff_TD[I2S_NUM_BUFS];
volatile uint8 TxI2S_Buff[I2S_NUM_BUFS][I2S_BUF_SIZE], TxI2S_Swap[9], TxI2S_Stage;
volatile uint8 TxI2S_In_Progress_TD = 0u;

CY_ISR(TxI2S_DMA_done) {
    // Equivalent to the following but avoiding compiler warnings:
	//CyDmaChStatus(TxI2S_Buff_Chan, &TxI2S_In_Progress_TD, 0);
    TxI2S_In_Progress_TD = DMAC_CH[TxI2S_Buff_Chan].basic_status[1] & 0x7Fu;
}

void DmaTxConfiguration(void) {
    uint8 TxI2S_Swap_Chan, TxI2S_Swap_TD[9], TxI2S_Stage_Chan, TxI2S_Stage_TD[9];
	uint8 i, n;
	
    TxI2S_Swap_Chan = TxI2S_Swap_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_PERIPH_BASE));
    for (i=0 ;i<9; i++) {TxI2S_Swap_TD[i]=CyDmaTdAllocate();}
	CyDmaTdSetConfiguration(TxI2S_Swap_TD[0], 1, TxI2S_Swap_TD[1], TxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Swap_TD[0], LO16(TxI2S_Swap + 5), LO16(I2S_TX_FIFO_0_PTR));
	CyDmaTdSetConfiguration(TxI2S_Swap_TD[1], 1, TxI2S_Swap_TD[2], TxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Swap_TD[1], LO16(TxI2S_Swap + 4), LO16(I2S_TX_FIFO_0_PTR));
	CyDmaTdSetConfiguration(TxI2S_Swap_TD[2], 1, TxI2S_Swap_TD[3], TxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Swap_TD[2], LO16(TxI2S_Swap + 3), LO16(I2S_TX_FIFO_0_PTR));
	CyDmaTdSetConfiguration(TxI2S_Swap_TD[3], 1, TxI2S_Swap_TD[4], TxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Swap_TD[3], LO16(TxI2S_Swap + 8), LO16(I2S_TX_FIFO_0_PTR));
	CyDmaTdSetConfiguration(TxI2S_Swap_TD[4], 1, TxI2S_Swap_TD[5], TxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Swap_TD[4], LO16(TxI2S_Swap + 7), LO16(I2S_TX_FIFO_0_PTR));
	CyDmaTdSetConfiguration(TxI2S_Swap_TD[5], 1, TxI2S_Swap_TD[6], TxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Swap_TD[5], LO16(TxI2S_Swap + 6), LO16(I2S_TX_FIFO_0_PTR));
	CyDmaTdSetConfiguration(TxI2S_Swap_TD[6], 1, TxI2S_Swap_TD[7], TxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Swap_TD[6], LO16(TxI2S_Swap + 2), LO16(I2S_TX_FIFO_0_PTR));
	CyDmaTdSetConfiguration(TxI2S_Swap_TD[7], 1, TxI2S_Swap_TD[8], TxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Swap_TD[7], LO16(TxI2S_Swap + 1), LO16(I2S_TX_FIFO_0_PTR));
	CyDmaTdSetConfiguration(TxI2S_Swap_TD[8], 1, TxI2S_Swap_TD[0], TxI2S_Swap__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Swap_TD[8], LO16(TxI2S_Swap + 0), LO16(I2S_TX_FIFO_0_PTR));
	CyDmaChSetInitialTd(TxI2S_Swap_Chan, TxI2S_Swap_TD[0]);
    
    TxI2S_Stage_Chan = TxI2S_Stage_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    for (i=0 ;i<9; i++) {TxI2S_Stage_TD[i]=CyDmaTdAllocate();}
	CyDmaTdSetConfiguration(TxI2S_Stage_TD[0], 1, TxI2S_Stage_TD[1], TxI2S_Stage__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Stage_TD[0], LO16(TxI2S_Stage), LO16(TxI2S_Swap + 0));
	CyDmaTdSetConfiguration(TxI2S_Stage_TD[1], 1, TxI2S_Stage_TD[2], TxI2S_Stage__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Stage_TD[1], LO16(TxI2S_Stage), LO16(TxI2S_Swap + 1));
	CyDmaTdSetConfiguration(TxI2S_Stage_TD[2], 1, TxI2S_Stage_TD[3], TxI2S_Stage__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Stage_TD[2], LO16(TxI2S_Stage), LO16(TxI2S_Swap + 2));
	CyDmaTdSetConfiguration(TxI2S_Stage_TD[3], 1, TxI2S_Stage_TD[4], TxI2S_Stage__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Stage_TD[3], LO16(TxI2S_Stage), LO16(TxI2S_Swap + 3));
	CyDmaTdSetConfiguration(TxI2S_Stage_TD[4], 1, TxI2S_Stage_TD[5], TxI2S_Stage__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Stage_TD[4], LO16(TxI2S_Stage), LO16(TxI2S_Swap + 4));
	CyDmaTdSetConfiguration(TxI2S_Stage_TD[5], 1, TxI2S_Stage_TD[6], TxI2S_Stage__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Stage_TD[5], LO16(TxI2S_Stage), LO16(TxI2S_Swap + 5));
	CyDmaTdSetConfiguration(TxI2S_Stage_TD[6], 1, TxI2S_Stage_TD[7], TxI2S_Stage__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Stage_TD[6], LO16(TxI2S_Stage), LO16(TxI2S_Swap + 6));
	CyDmaTdSetConfiguration(TxI2S_Stage_TD[7], 1, TxI2S_Stage_TD[8], TxI2S_Stage__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Stage_TD[7], LO16(TxI2S_Stage), LO16(TxI2S_Swap + 7));
	CyDmaTdSetConfiguration(TxI2S_Stage_TD[8], 1, TxI2S_Stage_TD[0], TxI2S_Stage__TD_TERMOUT_EN );
	CyDmaTdSetAddress(TxI2S_Stage_TD[8], LO16(TxI2S_Stage), LO16(TxI2S_Swap + 8));
	CyDmaChSetInitialTd(TxI2S_Stage_Chan, TxI2S_Stage_TD[8]);

    TxI2S_Buff_Chan = TxI2S_Buff_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
	for (i=0; i< I2S_NUM_BUFS; i++) {
	    TxI2S_Buff_TD[i] = CyDmaTdAllocate();
	}
	for (i=0; i< I2S_NUM_BUFS; i++) {
	 	n = i + 1;
		if (n >= I2S_NUM_BUFS) {n=0;}
	    CyDmaTdSetConfiguration(TxI2S_Buff_TD[i], I2S_BUF_SIZE, TxI2S_Buff_TD[n], (TD_INC_SRC_ADR | TxI2S_Buff__TD_TERMOUT_EN) );	
	    CyDmaTdSetAddress(TxI2S_Buff_TD[i], LO16(TxI2S_Buff[i]), LO16(TxI2S_Stage));
	}
	CyDmaChSetInitialTd(TxI2S_Buff_Chan, TxI2S_Buff_TD[0]);

    TxI2S_done_isr_Start();
    TxI2S_done_isr_SetVector(TxI2S_DMA_done);

    CyDmaChEnable(TxI2S_Buff_Chan, 1u);
	CyDmaChEnable(TxI2S_Stage_Chan, 1u);
	CyDmaChEnable(TxI2S_Swap_Chan, 1u);
}


void PCM3060_Start(void) {
	uint8 pcm3060_cmd[2];

    DmaRxConfiguration();
    DmaTxConfiguration();
    
    I2S_Start();
    I2S_EnableTx();
    I2S_EnableRx();

    // Take PCM3060 out of sleep mode
	pcm3060_cmd[0] = 0x40;
    pcm3060_cmd[1] = 0xC0;
    I2C_MasterWriteBuf(PCM3060_ADDR, pcm3060_cmd, 2, I2C_MODE_COMPLETE_XFER);
    while(0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)) {}
}


// The buffers will fire an interrupt after a transfer is complete.
// We collect the current td in the interrupt, which is actually
// the next td. This searches and backs up for the actual buf num.
uint8 FindPrevBufNum(uint8 td_prev, uint8* td_list) {
    uint8 i;
	for (i=0; i < I2S_NUM_BUFS; i++) {
		if (td_list[i] == td_prev) break;
	}
	if (i==0) i = I2S_NUM_BUFS-1;
	else i--;
    return i;
}


void PCM3060_Main(void) {
    static uint8 RxI2S_Last_TD = 0;
    uint8 td, i;
    
    //uint8 sta = I2S_ReadTxStatus();
    //TODO having problems with the I2S module sending random bits
    //while USB IN EP2 is sending data.  There are no underflows and
    //the buffers remain all zeros.  I can see the bits on a scope.
    //I've tried pulling all of USBFS_LoadInEP in here for isolation
    //and found the following critical line causes the spurious data.
    //USBFS_ARB_EP1_CFG_PTR[ri] |= USBFS_ARB_EPX_CFG_IN_DATA_RDY;    
    
    if(USBFS_IsConfigurationChanged() != 0u && USBFS_GetConfiguration() != 0u) {
        if (USBFS_GetInterfaceSetting(TxI2S_INTERFACE) == 1) {
            //TODO this is temporary until I finish tx/spkr switching
            USBFS_ReadOutEP(TxI2S_ENDPOINT, TxI2S_Buff[0], I2S_BUF_SIZE);
            USBFS_EnableOutEP(TxI2S_ENDPOINT);
        }
    }
    
    if (USBFS_GetEPState(TxI2S_ENDPOINT) == USBFS_OUT_BUFFER_FULL)
    {
        i = FindPrevBufNum(TxI2S_In_Progress_TD, TxI2S_Buff_TD);
		// Automatic DMA has a strange interface.
        USBFS_ReadOutEP(TxI2S_ENDPOINT, TxI2S_Buff[i], I2S_BUF_SIZE);
        USBFS_EnableOutEP(TxI2S_ENDPOINT);
    }

	if (USBFS_GetEPState(RxI2S_ENDPOINT) == USBFS_IN_BUFFER_EMPTY) {
		td = RxI2S_In_Progress_TD; // copy the volatile
		if (td != RxI2S_Last_TD) {
            i = FindPrevBufNum(td, RxI2S_Buff_TD);
			// Automatic DMA has a strange interface.
			USBFS_LoadInEP(RxI2S_ENDPOINT, RxI2S_Buff[i], I2S_BUF_SIZE);
			USBFS_LoadInEP(RxI2S_ENDPOINT, 0, I2S_BUF_SIZE);
			RxI2S_Last_TD = td;
		}
	}
}

