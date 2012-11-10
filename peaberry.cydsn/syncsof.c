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

#define FRAC_MIN (970 * 64)
#define FRAC_MAX (1015 * 64)
uint16 frac = FRAC_MIN + (FRAC_MAX - FRAC_MAX) / 2;

void SyncSOF_Main(void) {
    static uint16 prev_pos;
    uint16 pos;
    uint16 x;

    pos = CY_GET_REG8(SyncSOF_FRAME_POS_LO__STATUS_REG);
    if (pos & 0x01) {
        pos += (uint16)CY_GET_REG8(SyncSOF_FRAME_POS_HI__STATUS_REG) << 8;

        if (prev_pos < pos) {
            x = pos - prev_pos;
            x *= 12;
            if (x > 128) x = 128;
            if (frac < FRAC_MAX) {
                if (pos > 32000) frac += 128;
                frac += x;
            }
        } else if (prev_pos > pos) {
            x = prev_pos - pos;
            x *= 12;
            if (x > 128) x = 128;
            if (frac > FRAC_MIN) {
                if (pos < 12000) frac -= 128;
                frac -= x;
            }
        }
        
        CY_SET_REG8(SyncSOF_FRAC_HI__CONTROL_REG, frac >> 8);
        CY_SET_REG8(SyncSOF_FRAC_LO__CONTROL_REG, frac & 0xFF);
        
        prev_pos = pos;
    }

}
