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

#define WINDOW 2
#define FRAC_MIN 970 
#define FRAC_MAX 1015 
uint16 frac = FRAC_MIN + (FRAC_MAX - FRAC_MAX) / 2;

uint8 la = 0;

void SyncSOF_Main(void) {
    static uint8 prev_pos, nsh, nsh2;
    uint8 pos;

    if (CY_GET_REG8(SyncSOF_FRAME_POS_READY__STATUS_REG)) {
        pos = CY_GET_REG8(SyncSOF_FRAME_POS__STATUS_REG);        
        
        if (nsh) nsh =- 1;
        else nsh = 2;
        if (prev_pos < pos) {
            if (0) ;
            else if (pos > 129 && frac < FRAC_MAX) frac += 2 + (nsh & 0x01);
            else if (pos > 127 && frac < FRAC_MAX) frac += 1 + (nsh & 0x01);
            else frac += 1;
        } else if (prev_pos > pos) {
            if (0) ;
            else if (pos < 126 && frac > FRAC_MIN) frac -= 2 + (nsh & 0x01);
            else if (pos < 128 && frac > FRAC_MIN) frac -= 1 + (nsh & 0x01);
            else frac -= 1;
        } else {
            if (nsh2) nsh2--;
            else {
                if (pos > 127 && frac < FRAC_MAX) frac += 1;
                if (pos < 128 && frac > FRAC_MIN) frac -= 1;
                nsh2 = 24;
            }
        }

        CY_SET_REG8(SyncSOF_FRAC_HI__CONTROL_REG, frac >> 8);
        CY_SET_REG8(SyncSOF_FRAC_LO__CONTROL_REG, frac & 0xFF);
        
        prev_pos = pos;
    }

}
