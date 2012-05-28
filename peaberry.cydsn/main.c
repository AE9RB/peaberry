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

#define PCM3060_ADDR 0x46
#define PCM3060_REG 0x40

/* DMA Configuration for RxI2S */
#define RxI2S_BYTES_PER_BURST       (1u)
#define RxI2S_REQUEST_PER_BURST     (1u)
#define RxI2S_SRC_BASE              (CYDEV_PERIPH_BASE)
#define RxI2S_DST_BASE              (CYDEV_SRAM_BASE)
#define RxI2S_BUF_SIZE              (48u * 3 * 2)
#define RxI2S_NUM_BUFS              3u
#define RxI2S_ENDPOINT              2u

uint8 RxI2S_Chan;
uint8 RxI2S_TD[RxI2S_NUM_BUFS];
volatile uint8 RxI2S_Buff[RxI2S_NUM_BUFS][RxI2S_BUF_SIZE];
volatile uint8 RxI2S_In_Progress_TD = 0u;

CY_ISR(RxI2S_DMA_done) {
	CyDmaChStatus(RxI2S_Chan, &RxI2S_In_Progress_TD, 0);
}

void DmaRxConfiguration(void)
{
	uint8 i, n;

	RxI2S_done_isr_Start();
    RxI2S_done_isr_SetVector(RxI2S_DMA_done);

    RxI2S_Chan = RxI2S_DmaInitialize(RxI2S_BYTES_PER_BURST, RxI2S_REQUEST_PER_BURST, \
                                     HI16(RxI2S_SRC_BASE), HI16(RxI2S_DST_BASE));

	for (i=0; i< RxI2S_NUM_BUFS; i++) {
	    RxI2S_TD[i] = CyDmaTdAllocate();
	}

	for (i=0; i< RxI2S_NUM_BUFS; i++) {
	 	n = i + 1;
		if (n >= RxI2S_NUM_BUFS) {n=0;}
	    CyDmaTdSetConfiguration(RxI2S_TD[i], RxI2S_BUF_SIZE, RxI2S_TD[n], TD_INC_DST_ADR | RxI2S__TD_TERMOUT_EN );	
	    CyDmaTdSetAddress(RxI2S_TD[i], LO16((uint32)I2S_RX_FIFO_0_PTR), LO16((uint32)RxI2S_Buff[i]));
	}

	CyDmaChSetInitialTd(RxI2S_Chan, RxI2S_TD[0u]);
    CyDmaChEnable(RxI2S_Chan, 1u);
}

void main()
{
	uint8 RxI2S_This_TD, RxI2S_Last_TD = 0;
	uint8 i, *buf;
	int c;

	uint8 pcm3060_enable[2];
	pcm3060_enable[0] = 0x40;
    pcm3060_enable[1] = 0xC0;

    CyGlobalIntEnable;
    
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);

    DmaRxConfiguration();

    I2S_Start();
    CyDelay(50);
    I2S_EnableRx();

    I2C_Start();
    I2C_MasterClearStatus();
    I2C_MasterWriteBuf(PCM3060_ADDR, (uint8 *) pcm3060_enable, 2, I2C_MODE_COMPLETE_XFER);
    while(0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)) {}
    
    for(;;) {
		if (USBFS_GetEPState(RxI2S_ENDPOINT) != USBFS_NO_EVENT_PENDING) {
			RxI2S_This_TD = RxI2S_In_Progress_TD; // use a copy of the volatile
			if (RxI2S_This_TD != RxI2S_Last_TD) {
				// Find the buffer for the current TD
				for (i=0; i < RxI2S_NUM_BUFS; i++) {
					if (RxI2S_TD[i] == RxI2S_This_TD) break;
				}
				// Current TD is being changed, use the previous buffer
				if (i==0) i = RxI2S_NUM_BUFS-1;
				else i--;
				buf = RxI2S_Buff[i];
				//TODO This would-be reversal is messed up.
				//     Am I missing header bytes?
				for (c=0; c < RxI2S_BUF_SIZE; c += 3) {
					buf[c+2] = buf[c];
				}
				//TODO Automatic DMA has a strange interface.
				USBFS_LoadInEP(RxI2S_ENDPOINT, buf, RxI2S_BUF_SIZE);
				USBFS_LoadInEP(RxI2S_ENDPOINT, 0, RxI2S_BUF_SIZE);
				
				RxI2S_Last_TD = RxI2S_This_TD;
			}

		}
	
	    if(USBFS_IsConfigurationChanged() != 0u) {
			//TODO
        }
    }
}

/* [] END OF FILE */
