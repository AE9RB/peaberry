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

// While there are crystals that get us the exact PCM3060 clock
// that we want, we still want to lock on to the USB sof 1kHz.
// We set the Cypress DLL close to the target and do fractional N
// by wiggling FASTCLK_PLL_P with a PWM. A counter watches
// the USB sof timing and constantly adjusts the PWM.
// In effect, we end up with crystal stability using only the
// PSoC 3 ILO internal LC circuit (assuming the host has a crystal).
// Special thanks to KF6SJ for finding FASTCLK_PLL_P.

uint8 fasterp, slowerp, chan1, chan2, initialized = 0;

// The allowed PWM range needs to account for manufacturing
// variances in the IMO.  Supposedly, +-0.25%, but there's more...
// When the MiniProg3 is attached to the device, the IMO runs at
// a slightly different rate (or the MiniProg3 clock is used).
// So do not rely on the debugger to find the center.

CY_ISR(isr_up) {
    uint16 c;
    c = CY_GET_REG16(SyncSOF_PWM_COMPARE1_LSB_PTR);
    if (c<1015) CY_SET_REG16(SyncSOF_PWM_COMPARE1_LSB_PTR, c+1);
}

CY_ISR(isr_dn) {
    uint16 c;
    c = CY_GET_REG16(SyncSOF_PWM_COMPARE1_LSB_PTR);
    if (c>970) CY_SET_REG16(SyncSOF_PWM_COMPARE1_LSB_PTR, c-1);
}

void SyncSOF_Start(void) {
    uint8 td1, td2;
    
    if (!initialized) {
        fasterp = FASTCLK_PLL_P;
        slowerp = FASTCLK_PLL_P - 1;

        SyncSOF_PWM_Start();
        SyncSOF_Counter_Start();


        chan1 = pup_DMA_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_FASTCLK_PLL_BASE));
        td1 = CyDmaTdAllocate();
        CyDmaTdSetConfiguration(td1, 1, td1, 0);
        CyDmaTdSetAddress(td1, LO16(&fasterp), LO16(&FASTCLK_PLL_P));
        CyDmaChSetInitialTd(chan1, td1);

        chan2 = pdn_DMA_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_FASTCLK_PLL_BASE));
        td2 = CyDmaTdAllocate();
        CyDmaTdSetConfiguration(td2, 1, td2, 0);
        CyDmaTdSetAddress(td2, LO16(&slowerp), LO16(&FASTCLK_PLL_P));
        CyDmaChSetInitialTd(chan2, td2);
        
        CyDmaChEnable(chan1, 1);
        CyDmaChEnable(chan2, 1);

        initialized = 1;
    }

    pup_isr_Start();
    pup_isr_SetVector(isr_up);

    pdn_isr_Start();
    pdn_isr_SetVector(isr_dn);
}

void SyncSOF_Stop(void) {
    pup_isr_Stop();
    pdn_isr_Stop();
}

// Ideal clock is 36.864 MHz. But really we want 18432 cycles/2 per USB frame.
// The deadzone was determined by experimentation on a dev kit. It is large enough
// to not change the divider too often but small enough to keep the PLL from drifting.
// The default center was found by logging USB/DMA buffer slips.

void SyncSOF_Slower(void) {
    uint16 p, c;
    c = SyncSOF_Counter_ReadCompare();
    if (c < 18426) return;
    p = SyncSOF_Counter_ReadPeriod();
    if ((p&0x0001) == (c&0x0001)) {
        SyncSOF_Counter_WritePeriod(p-1);
    } else {
        SyncSOF_Counter_WriteCompare(c-1);
    }
}

void SyncSOF_Faster(void) {
    uint16 p, c;
    p = SyncSOF_Counter_ReadPeriod();
    if (p > 18436) return;
    c = SyncSOF_Counter_ReadCompare();
    if ((p&0x0001) == (c&0x0001)) {
        SyncSOF_Counter_WriteCompare(c+1);
    } else {
        SyncSOF_Counter_WritePeriod(p+1);
    }
}
