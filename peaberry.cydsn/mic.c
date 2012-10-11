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

uint8 Mic_Buff[USB_AUDIO_BUFS][MIC_BUF_SIZE];

void Mic_Start(void)
{
    uint8 Mic_Buff_Chan, Mic_Buff_TD;
    uint8 Mic_Conv_Chan, Mic_Conv_TD;

    Mic_Buff_Chan = Mic_Buff_DmaInitialize(2, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_SRAM_BASE));
    Mic_Buff_TD = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(Mic_Buff_TD, MIC_BUF_SIZE * USB_AUDIO_BUFS, Mic_Buff_TD, TD_INC_DST_ADR );    
    CyDmaTdSetAddress(Mic_Buff_TD, LO16(Mic1216_Conv_u0__16BIT_F1_REG), LO16(Mic_Buff[0]));
    CyDmaChSetInitialTd(Mic_Buff_Chan, Mic_Buff_TD);
    
    Mic_Conv_Chan = Mic_Conv_DmaInitialize(2, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_PERIPH_BASE));
    Mic_Conv_TD = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(Mic_Conv_TD, 2, Mic_Conv_TD, Mic_Conv__TD_TERMOUT_EN );    
    CyDmaTdSetAddress(Mic_Conv_TD, LO16(&Microphone_DEC_SAMP_PTR), LO16(Mic1216_Conv_u0__16BIT_F0_REG));
    CyDmaChSetInitialTd(Mic_Conv_Chan, Mic_Conv_TD);

    CyDmaChEnable(Mic_Buff_Chan, 1u);
    CyDmaChEnable(Mic_Conv_Chan, 1u);
    
    Microphone_Start();
    Microphone_StartConvert();
}


uint8* Mic_Buf(void) {
    return Mic_Buff[SyncSOF_USB_Buffer()];
}
