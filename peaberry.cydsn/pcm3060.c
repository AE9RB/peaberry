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

// 48 24-bit stereo samples every 1 ms
#define RxI2S_BUF_SIZE              (48u * 3 * 2)
#define RxI2S_NUM_BUFS              3u
#define RxI2S_ENDPOINT              2u


uint8 RxI2S_Last_TD = 0;
uint8 RxI2S_Swap_Chan, RxI2S_Stage_Chan, RxI2S_Buff_Chan;
uint8 RxI2S_Stage_TD, RxI2S_Swap_TD[9], RxI2S_Buff_TD[RxI2S_NUM_BUFS];
volatile uint8 RxI2S_Swap[9], RxI2S_Move, RxI2S_Buff[RxI2S_NUM_BUFS][RxI2S_BUF_SIZE];
volatile uint8 RxI2S_In_Progress_TD = 0u;

CY_ISR(RxI2S_DMA_done) {
	CyDmaChStatus(RxI2S_Buff_Chan, &RxI2S_In_Progress_TD, 0);
}

void DmaRxConfiguration(void)
{
	uint8 i, n;
	
	RxI2S_done_isr_Start();
    RxI2S_done_isr_SetVector(RxI2S_DMA_done);

    RxI2S_Stage_Chan = RxI2S_Stage_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    RxI2S_Swap_Chan = RxI2S_Swap_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));
    RxI2S_Buff_Chan = RxI2S_Buff_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_SRAM_BASE));

	RxI2S_Stage_TD=CyDmaTdAllocate();
	CyDmaTdSetConfiguration(RxI2S_Stage_TD, 9, RxI2S_Stage_TD, TD_INC_DST_ADR );
	CyDmaTdSetAddress(RxI2S_Stage_TD, LO16((uint32)I2S_RX_FIFO_0_PTR), LO16((uint32)RxI2S_Swap));
	CyDmaChSetInitialTd(RxI2S_Stage_Chan, RxI2S_Stage_TD);

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

	for (i=0; i< RxI2S_NUM_BUFS; i++) {
	    RxI2S_Buff_TD[i] = CyDmaTdAllocate();
	}

	for (i=0; i< RxI2S_NUM_BUFS; i++) {
	 	n = i + 1;
		if (n >= RxI2S_NUM_BUFS) {n=0;}
	    CyDmaTdSetConfiguration(RxI2S_Buff_TD[i], RxI2S_BUF_SIZE, RxI2S_Buff_TD[n], TD_INC_DST_ADR | RxI2S_Buff__TD_TERMOUT_EN );	
	    CyDmaTdSetAddress(RxI2S_Buff_TD[i], LO16((uint32)&RxI2S_Move), LO16((uint32)RxI2S_Buff[i]));
	}

	CyDmaChSetInitialTd(RxI2S_Buff_Chan, RxI2S_Buff_TD[0u]);
	
	
	CyDmaChEnable(RxI2S_Stage_Chan, 1u);
	CyDmaChEnable(RxI2S_Swap_Chan, 1u);
    CyDmaChEnable(RxI2S_Buff_Chan, 1u);
}


void PCM3060_Start(void) {
	uint8 pcm3060_cmd[2];

    DmaRxConfiguration();
    
    I2S_Start();
    I2S_EnableRx();

    // Take PCM3060 out of sleep mode
	pcm3060_cmd[0] = 0x40;
    pcm3060_cmd[1] = 0xC0;
    I2C_MasterWriteBuf(PCM3060_ADDR, (uint8 *) pcm3060_cmd, 2, I2C_MODE_COMPLETE_XFER);
    while(0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)) {}
}

void PCM3060_Main(void) {
    uint8 td, i;

	if (USBFS_GetEPState(RxI2S_ENDPOINT) != USBFS_NO_EVENT_PENDING) {
		td = RxI2S_In_Progress_TD; // copy the volatile
		if (td != RxI2S_Last_TD) {
			// Find the buffer for the current TD
			for (i=0; i < RxI2S_NUM_BUFS; i++) {
				if (RxI2S_Buff_TD[i] == td) break;
			}
			// Current TD is being changed, use the previous buffer
			if (i==0) i = RxI2S_NUM_BUFS-1;
			else i--;
			// Automatic DMA has a strange interface.
			USBFS_LoadInEP(RxI2S_ENDPOINT, RxI2S_Buff[i], RxI2S_BUF_SIZE);
			USBFS_LoadInEP(RxI2S_ENDPOINT, 0, RxI2S_BUF_SIZE);
			
			RxI2S_Last_TD = td;
		}
	}
}
