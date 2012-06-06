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

#define STARTUP_LO 0x713D0A07 // 14.08 MHz in byte reversed 11.21 bits
#define SI570_STARTUP_FREQ 56.32 // Si570 default output (LO*4)
#define SI570_ADDR 0x55
#define SI570_DCO_MIN 4850.0
#define SI570_DCO_MAX 5670.0
#define SI570_DCO_CENTER ((SI570_DCO_MIN + SI570_DCO_MAX) / 2)

volatile uint32 Si570_LO = STARTUP_LO;
uint32 Current_LO = STARTUP_LO;
float Si570_Xtal;
uint8 Si570_Buf[7];

void Si570_Start(void) {
    uint8 hsdiv, n1;
    uint16 rfreqint;
    uint32 rfreqfrac;
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
    Si570_Xtal = (SI570_STARTUP_FREQ * hsdiv * n1) / rfreq;
}

void Si570_Main(void) {
    static uint8 n1, hsdiv, state = 0;
    static float fout, dco;
    uint8 i;
    uint16 rfreqint;
    uint32 rfreqfrac;
    float testdco;

    switch (state) {
        case 0: // idle
            if (Current_LO != Si570_LO) {
                Current_LO = Si570_LO;
                Si570_Buf[0] = ((uint8*)&Current_LO)[3];
                Si570_Buf[1] = ((uint8*)&Current_LO)[2];
                Si570_Buf[2] = ((uint8*)&Current_LO)[1];
                Si570_Buf[3] = ((uint8*)&Current_LO)[0];
                fout = (float)*(uint32*)Si570_Buf;
                fout = fout / 0x800000 * 4;
                dco = SI570_DCO_MAX;
                state = 4;
            }
            break;
        case 12: // freeze DSPLL
            Si570_Buf[0] = 135;
            Si570_Buf[1] = 0x10;
            I2C_MasterWriteBuf(SI570_ADDR, Si570_Buf, 2, I2C_MODE_COMPLETE_XFER);
            state++;
            break;
        case 14: // write new DSPLL config
            testdco = dco / Si570_Xtal;
            // masks are probably overkill, don't trust floats
            rfreqint = (uint16)testdco & 0x3FF;
            rfreqfrac = (uint32)((testdco - rfreqint) * 0x10000000) & 0x0FFFFFFF;
            Si570_Buf[0] = 7;
            i = hsdiv - 4;
            Si570_Buf[1] = i << 5;
            i = n1 - 1;
            Si570_Buf[1] |= i >> 2;
            Si570_Buf[2] = i << 6;
            Si570_Buf[2] |= rfreqint >> 4;
            *(uint32*)&Si570_Buf[3] = rfreqfrac;
            Si570_Buf[3] |= rfreqint << 4;
            I2C_MasterWriteBuf(SI570_ADDR, Si570_Buf, 7, I2C_MODE_COMPLETE_XFER);
            state++;
            break;
        case 16: // release DSPLL
            Si570_Buf[0] = 135;
            Si570_Buf[1] = 0x20;
            I2C_MasterWriteBuf(SI570_ADDR, Si570_Buf, 2, I2C_MODE_COMPLETE_XFER);
            state++;
            break;
        case 13: // waiting on I2C
        case 15:
        case 17:
            if (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT) {
                state++;
            }
            break;
        case 18: // done
            state = 0;
            break;
        case 8:  // invalid for HS_DIV
        case 10:
            state++;
            //nobreak
        default: // try one hsdiv
            i = SI570_DCO_CENTER / (fout * state);
            if (i > 1 && (i&1)) i++;
            testdco = fout * state * i;
            if (testdco > SI570_DCO_MIN && testdco < dco) {
                dco = testdco;
                n1 = i;
                hsdiv = state;
            }
            state++;
            break;            
    }

}
