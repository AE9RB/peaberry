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

uint8 Mic_Buff_Chan, Mic_Buff_TD[DMA_AUDIO_BUFS];
volatile uint8 Mic_Buff[USB_AUDIO_BUFS][MIC_BUF_SIZE], Mic_DMA_TD;

CY_ISR(Mic_DMA_done) {
    Mic_DMA_TD = DMAC_CH[Mic_Buff_Chan].basic_status[1] & 0x7Fu;
}

void Mic_Start(void)
{
	uint8 i, n;

    Mic_Buff_Chan = Mic_Buff_DmaInitialize(2, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
	for (i=0; i < DMA_AUDIO_BUFS; i++) Mic_Buff_TD[i] = CyDmaTdAllocate();
	for (i=0; i < DMA_AUDIO_BUFS; i++) {
	    CyDmaTdSetConfiguration(Mic_Buff_TD[i], MIC_BUF_SIZE/2, Mic_Buff_TD[i+1], TD_SWAP_EN | TD_INC_DST_ADR | Mic_Buff__TD_TERMOUT_EN );	
	    CyDmaTdSetAddress(Mic_Buff_TD[i], LO16(&Microphone_DEC_SAMP_PTR), LO16(Mic_Buff[i/2]));
        i++;
	 	n = i + 1;
		if (n >= DMA_AUDIO_BUFS) n=0;
	    CyDmaTdSetConfiguration(Mic_Buff_TD[i], MIC_BUF_SIZE/2, Mic_Buff_TD[n], TD_SWAP_EN | TD_INC_DST_ADR | Mic_Buff__TD_TERMOUT_EN );	
	    CyDmaTdSetAddress(Mic_Buff_TD[i], LO16(&Microphone_DEC_SAMP_PTR), LO16(Mic_Buff[i/2] + MIC_BUF_SIZE/2));
	}
	CyDmaChSetInitialTd(Mic_Buff_Chan, Mic_Buff_TD[0]);
	
    Mic_DMA_TD = Mic_Buff_TD[0];
	Mic_done_isr_Start();
    Mic_done_isr_SetVector(Mic_DMA_done);

    CyDmaChEnable(Mic_Buff_Chan, 1u);
    
    Microphone_Start();
    Microphone_StartConvert();
}

//TODO Use DMA and synthesized hardware to perform buffer math
// in the meantime, we divide the work into five
// calls so as to be kind to the main loop.
uint8* Mic_Buf(void) {
    static uint8 debounce = 0, use = 0;
    static uint16 *buf;
    static uint8 pos;
    
    uint8 dma, td, i;
    int16 b;
    
    if (pos) {
        for (i = MIC_BUF_SIZE / 8; i; i--) {
            pos--;
            b = buf[pos];
            // DelSig will overflow (see datasheet)
            if (b & 0xF000) b = 2047;
            // changed to signed
            else b -= 2048;
            // rotate into USB bit order
            buf[pos] = (b << 12) | (b >> 4);
        }
        if (pos) return 0;
        return buf;
    }

    td = Mic_DMA_TD; // volatile
    for (dma = 0; dma < DMA_AUDIO_BUFS; dma++) {
        if (td == Mic_Buff_TD[dma]) break;
    }
    USBAudio_SyncBufs(dma, &use, &debounce, 0);
    buf = (uint16*)Mic_Buff[use];
    pos = MIC_BUF_SIZE / 2;
    return 0;
}
