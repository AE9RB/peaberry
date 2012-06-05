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

#define SI570_ADDR 0x55
#define STARTUP_LO 0x713D0A07 // 14.08 MHz in byte reversed 11.21 bits
#define STARTUP_FREQ 56.32 // Si570 default output (LO*4)

volatile uint32 Si570_LO = STARTUP_LO;
uint32 Current_LO = STARTUP_LO;
float Si570_Xtal;
volatile uint8 Si570_State = 0;
uint8 Si570_Buf[6];

void Si570_Start(void) {

    
    uint8 hsdiv, n1;
    uint32 rfreqint, rfreqfrac;
    float rfreq;
    
    // reload Si570 default registers so we can calc xtal
    Si570_Buf[0] = 135;
    Si570_Buf[1] = 1;
    I2C_MasterWriteBuf(SI570_ADDR, Si570_Buf, 2, I2C_MODE_COMPLETE_XFER);
    while(0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)) {}

    // read all the registers
    Si570_Buf[0] = 7;
    I2C_MasterWriteBuf(SI570_ADDR, Si570_Buf, 1, I2C_MODE_NO_STOP);
    while(0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)) {}
    I2C_MasterReadBuf(SI570_ADDR, Si570_Buf, 6, I2C_MODE_REPEAT_START);
    while(0u == (I2C_MasterStatus() & I2C_MSTAT_RD_CMPLT)) {}

    // obtain xtal calibration
    hsdiv = (Si570_Buf[0] >> 5) + 4;
    n1 = (((Si570_Buf[0] & 0x1F) << 2) | (Si570_Buf[1] >> 6)) + 1;
    rfreqint = (((uint16)Si570_Buf[1] & 0x3F) << 4) | (Si570_Buf[2] >> 4);
    rfreqfrac = ((uint32*)&Si570_Buf[2])[0] & 0x0FFFFFFF;
    rfreq = rfreqint + (float)rfreqfrac / 0x10000000;
    Si570_Xtal = (STARTUP_FREQ * hsdiv * n1) / rfreq;
}

void Si570_Main(void) {
    if (Current_LO != Si570_LO) {
        Current_LO = Si570_LO;
        //TODO calc change
    }
}
