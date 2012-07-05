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

CY_ISR(isr_up) {
    uint16 c;
    c = CY_GET_REG16(SyncSOF_PWM_COMPARE1_LSB_PTR);
    if (c<992) CY_SET_REG16(SyncSOF_PWM_COMPARE1_LSB_PTR, c+1);
}

CY_ISR(isr_dn) {
    uint16 c;
    c = CY_GET_REG16(SyncSOF_PWM_COMPARE1_LSB_PTR);
    if (c>983) CY_SET_REG16(SyncSOF_PWM_COMPARE1_LSB_PTR, c-1);
}

void SyncSOF_Start(void) {
    uint8 td1, td2;
    
    if (!initialized) {
        fasterp = FASTCLK_PLL_P;
        slowerp = FASTCLK_PLL_P - 1;

        SyncSOF_PWM_Start();
        SyncSOF_Counter_Start();

        pup_isr_Start();
        pup_isr_SetVector(isr_up);

        pdn_isr_Start();
        pdn_isr_SetVector(isr_dn);

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
        
        initialized = 1;
    }
    CyDmaChEnable(chan1, 1);
    CyDmaChEnable(chan2, 1);
}

void SyncSOF_Stop(void) {
    CyDmaChDisable(chan1);
    CyDmaChDisable(chan2);
}

// Ideal clock is 36.864 MHz. But really we want 18432 cycles/2 per USB frame.
// The counter starts with a range of 18435-18427 which has a deadzone suitable
// for PLL adjustment (found by experimentation). It is also minus 1 to compensate
// for the reset. Actual out of sync conditions can request the center be fine tuned.

void SyncSOF_Slower(void) {
    uint16 p, c;
    p = SyncSOF_Counter_ReadPeriod();
    if (p < 18428) return;
    c = SyncSOF_Counter_ReadCompare();
    if ((p&0x0001) == (c&0x0001)) {
        SyncSOF_Counter_WritePeriod(p-1);
    } else {
        SyncSOF_Counter_WriteCompare(c-1);
    }
}

void SyncSOF_Faster(void) {
    uint16 p, c;
    c = SyncSOF_Counter_ReadCompare();
    if (c > 18434) return;
    p = SyncSOF_Counter_ReadPeriod();
    if ((p&0x0001) == (c&0x0001)) {
        SyncSOF_Counter_WriteCompare(c+1);
    } else {
        SyncSOF_Counter_WritePeriod(p+1);
    }
}
