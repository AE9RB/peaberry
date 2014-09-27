// Copyright 2013 David Turnbull AE9RB
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

#include <device.h>

#define CONTROL_LED   0x01
#define STATUS_BOOT   0x01

#define MORSE_DOT  1
#define MORSE_DASH 3
#define MORSE_CHAR 3
#define MORSE_WORD 7

#define MCODE_L(x) (((x&0x0000000FLU)?1:0)\
+((x&0x000000F0LU)?1:0)\
+((x&0x00000F00LU)?1:0)\
+((x&0x0000F000LU)?1:0)\
+((x&0x000F0000LU)?1:0)\
+((x&0x00F00000LU)?1:0))
#define MCODE_C(x) (((x&0x00000002LU)?1:0)\
+((x&0x00000020LU)?2:0)\
+((x&0x00000200LU)?4:0)\
+((x&0x00002000LU)?8:0)\
+((x&0x00020000LU)?16:0)\
+((x&0x00200000LU)?32:0))
#define MCODE_6(x) (((x&0x00F00000LU)?4:0)\
+ ((((x&0x00F00000LU)?1:0) & ((x&0x00000002LU)?0:1))?2:0))
#define MCODE(d)((uint8) (d==0)?7:MCODE_L(0x##d##LU)\
+ (MCODE_C(0x##d##LU) << 8 - MCODE_L(0x##d##LU))\
- MCODE_6(0x##d##LU))

const uint8 code MORSE_BOOT[] = {
    /* 0x42 B */ MCODE(3111),
    /* 0x4F O */ MCODE(333),
    /* 0x4F O */ MCODE(333),
    /* 0x54 T */ MCODE(3),
    0
};

const uint8 code MORSE_XTAL[] = {
    /* 0x58 X */ MCODE(3113),
    /* 0x54 T */ MCODE(3),
    /* 0x41 A */ MCODE(13),
    /* 0x4C L */ MCODE(1311),
    0
};

uint8 *message;

CY_ISR(morse_interrupt)
{
    static uint8 pos = 0, state = 0, codes, len, timer;

    switch (state) {
    case 0:
        codes = message[pos++];
        len = codes & 0x07;
        if (!codes) {
            pos = 0;
            timer = MORSE_WORD - MORSE_CHAR - 2;
            len = 0;
            state = 3;
            break;
        }
    case 1:
        if (codes & 0x80) timer = MORSE_DASH;
        else timer = MORSE_DOT;
        codes <<= 1;
        len--;
        Control_Write(Control_Read() & ~CONTROL_LED);
        state = 2;
    case 2:
        if (!timer) {
            state = 3;
            if (!len) timer = MORSE_CHAR - 1;
            else timer = MORSE_DOT - 1;
            Control_Write(Control_Read() | CONTROL_LED);
        }
        else {
            timer--;
            break;
        }
    case 3:
        if (!timer) {
            if (!len) state = 0;
            else state = 1;
        }
        else timer--;
    }
}
    
void main()
{
    message = MORSE_BOOT;
    if (Status_Read() & STATUS_BOOT) {
        if (CyXTAL_ReadStatus()) message = MORSE_XTAL;
        else Bootloader_Start();
    }
    morse_isr_StartEx(&morse_interrupt);
    Morse_Counter_Start();
    CyGlobalIntEnable;
    Bootloader_Start();
    for(;;) {}
}

/* [] END OF FILE */
