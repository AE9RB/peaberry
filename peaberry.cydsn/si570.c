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

#define STARTUP_LO 0x713D0A07 // 56.32 MHz in byte reversed 11.21 bits (14.080)
#define MAX_LO 160.0 // maximum for CMOS Si570
#define MIN_LO 2.5 // minimum with additional divide by 4
#define DIV_LO 10.0 // Tunes 600m-160m without violating Si570 spec.
#define SI570_ADDR 0x55
#define SI570_DCO_MIN 4850.0
#define SI570_DCO_MAX 5670.0
#define SI570_DCO_CENTER ((SI570_DCO_MIN + SI570_DCO_MAX) / 2)

volatile uint32 Si570_Xtal, Si570_LO = STARTUP_LO;
// [0-1] for commands, [2-8] retain registers
uint8 Si570_Buf[8];
// A copy of the factory registers used for cfgsr calibration.
uint8 Si570_Factory[6];
// Emulate old technique of setting of frequency by reg writes
uint8 Si570_OLD[6];

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
            I2C_MasterReadBuf(SI570_ADDR, Si570_Buf + 2, 6, I2C_MODE_REPEAT_START);
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
    if (!Si570_Xtal) {
        // no eeprom setting, calculate xtal calibration
        hsdiv = (Si570_Buf[2] >> 5) + 4;
        n1 = (((Si570_Buf[2] & 0x1F) << 2) | (Si570_Buf[3] >> 6)) + 1;
        rfreqint = (((uint16)Si570_Buf[3] & 0x3F) << 4) | (Si570_Buf[4] >> 4);
        rfreqfrac = ((uint32*)&Si570_Buf[4])[0] & 0x0FFFFFFF;
        rfreq = rfreqint + (float)rfreqfrac / 0x10000000;
        Si570_Xtal = swap32((uint32)(SI570_STARTUP_FREQ * hsdiv * n1 / rfreq * 0x01000000));
    }
    for (i = 0; i < 6; i++) Si570_Factory[i] = Si570_Buf[i+2];
    Si570_OLD[0]=0;
}

// This method of setting frequency is strongly discouraged.
// It depends on client software managing the calibration data.
uint32 FreqFromOLD() {
    uint8 hsdiv, n1;
    uint16 rfreqint;
    uint32 rfreqfrac;
    float rfreq;

    hsdiv = (Si570_OLD[0] >> 5) + 4;
    n1 = (((Si570_OLD[0] & 0x1F) << 2) | (Si570_OLD[1] >> 6)) + 1;
    rfreqint = (((uint16)Si570_OLD[1] & 0x3F) << 4) | (Si570_OLD[2] >> 4);
    rfreqfrac = ((uint32*)&Si570_OLD[2])[0] & 0x0FFFFFFF;
    rfreq = rfreqint + (float)rfreqfrac / 0x10000000;
    
    Si570_OLD[0]=0;
    // Client software typically uses a fixed xtal freq of 114.285.
    return swap32((uint32)(114.285 * rfreq / (hsdiv * n1) * 0x200000));
}

// CFGSR requests a reset in order to determine the xtal frequency.
// This enables its calibration tab to fully work.
void Si570_Fake_Reset(void) {
    uint8 i;
    for (i = 0; i < 6; i++) Si570_Buf[i+2] = Si570_Factory[i];
}

void Si570_Main(void) {
    static uint8 n1, hsdiv, state = 0;
    static float fout, dco;
    static uint32 Current_LO = STARTUP_LO;
    static uint8 Divide_Lower = 0;
    uint8 i;
    uint16 rfreqint;
    uint32 rfreqfrac;
    float testdco;

    switch (state) {
    case 0: // idle
        if (Si570_OLD[0]) Si570_LO = FreqFromOLD();
        if (Current_LO != Si570_LO && !Locked_I2C) {
            i = CyEnterCriticalSection();
            fout = (float)swap32(Si570_LO) / 0x200000;
            Current_LO = Si570_LO;
            CyExitCriticalSection(i);
            if (fout < MIN_LO) fout = MIN_LO;
            if (fout > MAX_LO) fout = MAX_LO;
            Divide_Lower = (fout < DIV_LO);
            if (Divide_Lower) fout *= 4;
            Locked_I2C = 1;
            dco = SI570_DCO_MAX;
            state = 4;
        }
        break;
    case 12: // done with math
        // freeze DSPLL
        Si570_Buf[0] = 135;
        Si570_Buf[1] = 0x20;
        I2C_MasterWriteBuf(SI570_ADDR, Si570_Buf, 2, I2C_MODE_COMPLETE_XFER);
        state++;
        break;
    case 14: // write new DSPLL config
        i = CyEnterCriticalSection();
        testdco = dco / ((float)swap32(Si570_Xtal) / 0x01000000);
        CyExitCriticalSection(i);
        rfreqint = testdco;
        rfreqfrac = (testdco - rfreqint) * 0x10000000;
        // don't trust floats
        if (rfreqfrac > 0x0FFFFFFF) rfreqfrac = 0x0FFFFFFF;
        Si570_Buf[1] = 7;
        i = hsdiv - 4;
        Si570_Buf[2] = i << 5;
        i = n1 - 1;
        Si570_Buf[2] |= i >> 2;
        Si570_Buf[3] = i << 6;
        Si570_Buf[3] |= rfreqint >> 4;
        *(uint32*)&Si570_Buf[4] = rfreqfrac;
        Si570_Buf[4] |= rfreqint << 4;
        I2C_MasterWriteBuf(SI570_ADDR, Si570_Buf + 1, 7, I2C_MODE_COMPLETE_XFER);
        state++;
        break;
    case 16: // release DSPLL
        Si570_Buf[0] = 135;
        Si570_Buf[1] = 0x40;
        I2C_MasterWriteBuf(SI570_ADDR, Si570_Buf, 2, I2C_MODE_COMPLETE_XFER);
        i = CY_GET_REG8(IQGen_Settings__CONTROL_REG);
        if (Divide_Lower) {
            CY_SET_REG8(IQGen_Settings__CONTROL_REG, i & ~IQ_GEN_DIV_MASK | 0x07 );
        } else {
            CY_SET_REG8(IQGen_Settings__CONTROL_REG, i & ~IQ_GEN_DIV_MASK | 0x01 );
        }
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
        Locked_I2C = 0;
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
