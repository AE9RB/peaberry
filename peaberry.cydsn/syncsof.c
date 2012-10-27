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

#define FRAC_MIN (970 * 1)
#define FRAC_MAX (1015 * 1)
uint16 frac = FRAC_MIN + (FRAC_MAX - FRAC_MAX) / 2;

void SyncSOF_Main(void) {
    static uint8 nsh, up, dn, x;
    static uint16 prev_pos;
    uint16 pos;
    static int8 dir;

    pos = CY_GET_REG8(SyncSOF_FRAME_POS_LO__STATUS_REG);
    if (pos & 0x01) {
        pos += (uint16)CY_GET_REG8(SyncSOF_FRAME_POS_HI__STATUS_REG) << 8;

        if (nsh) nsh--;
        
        if (!nsh && pos > 30000 && frac < FRAC_MAX) {frac += 2; nsh = 5;}
        else if (!nsh && pos < 15000 && frac > FRAC_MIN) {frac -= 2; nsh = 5;}
        else if (prev_pos < pos) {
            if (frac < FRAC_MAX) frac += 1;
        } else if (prev_pos > pos) {
            if (frac > FRAC_MIN) frac -= 1;
        }
        
        CY_SET_REG8(SyncSOF_FRAC_HI__CONTROL_REG, frac >> 8);
        CY_SET_REG8(SyncSOF_FRAC_LO__CONTROL_REG, frac & 0xFF);
        
        prev_pos = pos;
    }

}
