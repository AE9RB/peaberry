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

#include <peaberry.h>

// MCODE is an 8-bit-per-chararcter morse code encoding macro.
// Dots are represented as 1s; dashs are 3s. The length is stored in the three
// lowest bits. Dots are encoded as binary 0; dashes are 1; left aligned.
// For example, letter Q is encoded as "1101" plus a filler "0" then "100".
// The astute bit twiddler will notice that bit 0x40 is part of both
// the code and length; a stored length of 0 is interpreted as 6.
// Use MCODE(0) to indicate a character with no valid morse code.
// These invalid characters will have a stored length of 7.
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

const uint8 code MCODES[] = {
    /* 0x22 " */ MCODE(131131),
    /* 0x23 # */ MCODE(0),
    /* 0x24 $ */ MCODE(0),
    /* 0x25 % */ MCODE(0),
    /* 0x26 & */ MCODE(0),
    /* 0x27 ' */ MCODE(133331),
    /* 0x28 ( */ MCODE(31331),
    /* 0x29 ) */ MCODE(313313),
    /* 0x2A * */ MCODE(0),
    /* 0x2B + */ MCODE(13131),
    /* 0x2C , */ MCODE(331133),
    /* 0x2D - */ MCODE(311113),
    /* 0x2E . */ MCODE(131313),
    /* 0x2F / */ MCODE(31131),
    /* 0x30 0 */ MCODE(33333),
    /* 0x31 1 */ MCODE(13333),
    /* 0x32 2 */ MCODE(11333),
    /* 0x33 3 */ MCODE(11133),
    /* 0x34 4 */ MCODE(11113),
    /* 0x35 5 */ MCODE(11111),
    /* 0x36 6 */ MCODE(31111),
    /* 0x37 7 */ MCODE(33111),
    /* 0x38 8 */ MCODE(33311),
    /* 0x39 9 */ MCODE(33331),
    /* 0x3A : */ MCODE(333111),
    /* 0x3B ; */ MCODE(0),
    /* 0x3C < */ MCODE(0),
    /* 0x3D = */ MCODE(31113),
    /* 0x3E > */ MCODE(0),
    /* 0x3F ? */ MCODE(113311),
    /* 0x40 @ */ MCODE(133131),
    /* 0x41 A */ MCODE(13),
    /* 0x42 B */ MCODE(3111),
    /* 0x43 C */ MCODE(3131),
    /* 0x44 D */ MCODE(311),
    /* 0x45 E */ MCODE(1),
    /* 0x46 F */ MCODE(1131),
    /* 0x47 G */ MCODE(331),
    /* 0x48 H */ MCODE(1111),
    /* 0x49 I */ MCODE(11),
    /* 0x4A J */ MCODE(1333),
    /* 0x4B K */ MCODE(313),
    /* 0x4C L */ MCODE(1311),
    /* 0x4D M */ MCODE(33),
    /* 0x4E N */ MCODE(31),
    /* 0x4F O */ MCODE(333),
    /* 0x50 P */ MCODE(1331),
    /* 0x51 Q */ MCODE(3313),
    /* 0x52 R */ MCODE(131),
    /* 0x53 S */ MCODE(111),
    /* 0x54 T */ MCODE(3),
    /* 0x55 U */ MCODE(113),
    /* 0x56 V */ MCODE(1113),
    /* 0x57 W */ MCODE(133),
    /* 0x58 X */ MCODE(3113),
    /* 0x59 Y */ MCODE(3133),
    /* 0x5A Z */ MCODE(3311),
};

#define MORSE_DOT (1)
#define MORSE_DASH (3)
#define MORSE_CHAR (3)
#define MORSE_WORD (7 - MORSE_CHAR)

// Example usage:
// Morse_Main("Repeating Message ");for(;;){sleep(240);Morse_Main(0);}
void Morse_Main(char* msg) {
    static uint8 pos, codes, len, state, timer, *message;
    uint8 i;

    if (msg) {
        state = pos = 0;
        message = msg;
    }
    else switch (state) {
    case 0:
        if (!message[pos]) pos = 0;
        i = message[pos++];
        if (i >= 0x61 && i <= 0x7A) i -= 32;
        if (i < 0x22 || i > 0x5A) codes = 7;
        else codes = MCODES[i-0x22];
        len = codes & 0x07;
        if (len==0) len = 6;
        if (codes==7) {
            timer = MORSE_WORD - 2;
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
