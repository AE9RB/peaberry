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
#include <main.h>
#include <si570.h>

#define STARTUP_LO 0x713D0A07 // 56.32 MHz in byte reversed 11.21 bits (14.080)
#define MAX_LO 160.0 // maximum for CMOS Si570
#define MIN_LO 5.0 // 5.0 MHz is minimum with additional divide by 2
#define SI570_STARTUP_FREQ 56.32 // Si570 default output (LO*4)
#define SI570_ADDR 0x55
#define SI570_DCO_MIN 4850.0
#define SI570_DCO_MAX 5670.0
#define SI570_DCO_CENTER ((SI570_DCO_MIN + SI570_DCO_MAX) / 2)

extern uint8 Lock_I2C;
volatile uint32 Si570_LO = STARTUP_LO;
float Si570_Xtal;
uint8 Si570_Buf[7];

void Si570_Start(void) {
    uint8 hsdiv, n1, i, state = 0;
    uint16 rfreqint;
    uint32 rfreqfrac;
    float rfreq;
    
    while (state < 6) {
        switch (state) {
        case 0: // reload Si570 default registers
            Si570_Buf[0] = 135;
            Si570_Buf[1] = 1;
            I2C_MasterWriteBuf(SI570_ADDR, Si570_Buf, 2, I2C_MODE_COMPLETE_XFER);
            state++;
            break;
        case 2: // prepare to read registers
            Si570_Buf[0] = 7;
            I2C_MasterWriteBuf(SI570_ADDR, Si570_Buf, 1, I2C_MODE_NO_STOP);
            state++;
            break;
        case 1:
        case 3:
            i = I2C_MasterStatus();
            if (i & I2C_MSTAT_ERR_XFER) {
                state--;
            } else if (i & I2C_MSTAT_WR_CMPLT) {
                state++;
            }
            break;
        case 4: // do read registers
            I2C_MasterReadBuf(SI570_ADDR, Si570_Buf, 6, I2C_MODE_REPEAT_START);
            state++;
            break;
        case 5:
            i = I2C_MasterStatus();
            if (i & I2C_MSTAT_ERR_XFER) {
                state = 2;
            } else if (i & I2C_MSTAT_RD_CMPLT) {
                state++;
            }
            break;
        }
    }
    // calculate xtal calibration
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
    static uint32 Current_LO = STARTUP_LO;
    uint8 i;
    uint16 rfreqint;
    uint32 rfreqfrac;
    float testdco;

    switch (state) {
    case 0: // idle
        if (Current_LO != Si570_LO && Lock_I2C == LOCKI2C_UNLOCKED) {
            i = CyEnterCriticalSection();
            Si570_Buf[0] = ((uint8*)&Si570_LO)[3];
            Si570_Buf[1] = ((uint8*)&Si570_LO)[2];
            Si570_Buf[2] = ((uint8*)&Si570_LO)[1];
            Si570_Buf[3] = ((uint8*)&Si570_LO)[0];
            CyExitCriticalSection(i);
            fout = (float)*(uint32*)Si570_Buf;
            fout /= 0x200000;
            if (fout < MIN_LO) fout = MIN_LO;
            if (fout > MAX_LO) fout = MAX_LO;
            Lock_I2C = LOCKI2C_SI570;
            Current_LO = Si570_LO;
            dco = SI570_DCO_MAX;
            state = 4;
        }
        break;
    case 12: // done with math
        if (dco == SI570_DCO_MAX) {
            // no valid dividers found, should never happen
            state = 18;
        } else {
            // freeze DSPLL
            Si570_Buf[0] = 135;
            Si570_Buf[1] = 0x20;
            I2C_MasterWriteBuf(SI570_ADDR, Si570_Buf, 2, I2C_MODE_COMPLETE_XFER);
            state++;
        }
        break;
    case 14: // write new DSPLL config
        testdco = dco / Si570_Xtal;
        rfreqint = testdco;
        rfreqfrac = (testdco - rfreqint) * 0x10000000;
        // don't trust floats
        if (rfreqfrac > 0x0FFFFFFF) rfreqfrac = 0x0FFFFFFF;
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
        Si570_Buf[1] = 0x40;
        I2C_MasterWriteBuf(SI570_ADDR, Si570_Buf, 2, I2C_MODE_COMPLETE_XFER);
        state++;
        break;
    case 13: // waiting on I2C
    case 15:
    case 17:
        i = I2C_MasterStatus();
        if (i & I2C_MSTAT_ERR_XFER) {
            state--;
        } else if (i & I2C_MSTAT_WR_CMPLT) {
            state++;
        }
        break;
    case 18: // all done
        Lock_I2C = LOCKI2C_UNLOCKED;
        state = 0;
        break;
    case 8:  // invalid for HS_DIV
    case 10:
        state++;
        //nobreak
    default: // try one HS_DIV at a time
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
