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

// Thanks to KF6SJ for the idea of fractional N.
// You can see another implementation in his Simple SDR.
// http://www.simplecircuits.com/SimpleSDR.html

uint8 p1, p2;

// The standard PLL can only get us to 36.923 MHz.
// This will lower the clock to about 36.869.
// It is closer to the center of 36.864 but not likely
// cause USB audio buffer underrun.
void FracN_Start(void) {
    uint8 chan1, chan2, td1, td2;
    
    p1 = FASTCLK_PLL_P;
    p2 = FASTCLK_PLL_P - 1;
    
    //TODO add support for USB audio synchronization
    //TODO work out the math and find least amount of DMA work
    //FracN_Clock_1_SetDividerValue(101); // prime
    //FracN_Clock_2_SetDividerValue(1720);

    chan1 = FracN_DMA_1_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_FASTCLK_PLL_BASE));
    td1 = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(td1, 1, td1, 0);
    CyDmaTdSetAddress(td1, LO16(&p1), LO16(&FASTCLK_PLL_P));
    CyDmaChSetInitialTd(chan1, td1);
    CyDmaChEnable(chan1, 1);

    chan2 = FracN_DMA_2_DmaInitialize(1, 1, HI16(CYDEV_SRAM_BASE), HI16(CYDEV_FASTCLK_PLL_BASE));
    td2 = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(td2, 1, td2, 0);
    CyDmaTdSetAddress(td2, LO16(&p2), LO16(&FASTCLK_PLL_P));
    CyDmaChSetInitialTd(chan2, td2);
    CyDmaChEnable(chan2, 1);

}
