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

uint8 Mic_Buff_Chan, Mic_Buff_TD[USB_AUDIO_BUFS];
volatile uint8 Mic_Buff[USB_AUDIO_BUFS][MIC_BUF_SIZE], Mic_DMA_TD;

CY_ISR(Mic_DMA_done) {
    Mic_DMA_TD = DMAC_CH[Mic_Buff_Chan].basic_status[1] & 0x7Fu;
}

void Mic_Start(void)
{
	uint8 i, n;

    Mic_Buff_Chan = Mic_Buff_DmaInitialize(2, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
	for (i=0; i < USB_AUDIO_BUFS; i++) Mic_Buff_TD[i] = CyDmaTdAllocate();
	for (i=0; i < USB_AUDIO_BUFS; i++) {
	 	n = i + 1;
		if (n >= USB_AUDIO_BUFS) n=0;
	    CyDmaTdSetConfiguration(Mic_Buff_TD[i], MIC_BUF_SIZE, Mic_Buff_TD[n], TD_INC_DST_ADR | Mic_Buff__TD_TERMOUT_EN );	
	    CyDmaTdSetAddress(Mic_Buff_TD[i], LO16(&Microphone_DEC_SAMP_PTR), LO16(Mic_Buff[i]));
	}
	CyDmaChSetInitialTd(Mic_Buff_Chan, Mic_Buff_TD[0]);
	
    Mic_DMA_TD = Mic_Buff_TD[0];
	Mic_done_isr_Start();
    Mic_done_isr_SetVector(Mic_DMA_done);

    CyDmaChEnable(Mic_Buff_Chan, 1u);
    
    Microphone_Start();
    Microphone_StartConvert();
}

uint8* Mic_Buf(void) {
    static uint8 eat = 0, debounce = 0, use = 0;
    uint8 dma, td;
    td = Mic_DMA_TD; // volatile
    for (dma=0;dma<USB_AUDIO_BUFS;dma++) {
        if (td == Mic_Buff_TD[dma]) break;
    }
    USBAudio_SyncBufs(dma, &use, &eat, &debounce, 0);
    return Mic_Buff[use];
}
