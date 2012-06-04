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
#include <si570.h>

#define STARTUP_FREQ 0x713D0A07 // 14.08 MHz in 11.21 bits MHz

volatile uint8 changing = 0;
volatile uint32 Si570_Freq = STARTUP_FREQ;
uint32 Si570_Last_Freq = STARTUP_FREQ;

void Si570_Start(void) {
    //TODO read xtal
}

void Si570_Main(void) {
    if (Si570_Last_Freq != Si570_Freq) {
        Si570_Last_Freq = Si570_Freq;
        //TODO calc change
    }
}
