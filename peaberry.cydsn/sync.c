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

#define SOF_CENTER 22000
#define FRAC_MIN (FracN_DEFAULT-200)
#define FRAC_MAX (FracN_DEFAULT+200)

void Sync_Start(void) {
    FracN_Start(P_DMA);
    SyncSOF_Start();
    // A pause is needed for the PLL to settle
    CyDelay(1);
}

// This is a simplified PID control. It works quite well. The full
// PID algorithm will be implemented some day for fun and learning.
void Sync_Main(void) {
    static uint16 frac = FracN_DEFAULT;
    static uint16 prev_pos;
    uint16 pos, i;

    pos = CY_GET_REG8(SyncSOF_FRAME_POS_LO__STATUS_REG);
    if (pos & 0x01) {
        pos += (uint16)CY_GET_REG8(SyncSOF_FRAME_POS_HI__STATUS_REG) << 8;

        if (pos > SOF_CENTER) {
            frac += (pos - SOF_CENTER) / 128;
        } else {
            frac -= (SOF_CENTER - pos) / 128;
        }
        
        if (prev_pos < pos) {
            i = pos - prev_pos;
            if (i < 8000) frac += i / 2;
        } else {
            i = prev_pos - pos;
            if (i < 8000) frac -= i / 2;
        }
        prev_pos = pos;
        
        if (frac > FRAC_MAX) frac = FRAC_MAX;
        if (frac < FRAC_MIN) frac = FRAC_MIN;
        FracN_Set(frac);
    }

}
