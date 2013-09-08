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

#define SYNC_SOF_CENTER 22000
#define SYNC_FRAC_MIN (FracN_DEFAULT-1000)
#define SYNC_FRAC_MAX (FracN_DEFAULT+1000)
#define SYNC_I_GUARD 3000
#define SYNC_P_GAIN 0.0001
#define SYNC_I_GAIN 0.001
#define SYNC_D_GAIN 0.5

void Sync_Start(void) {
    FracN_Start(P_DMA);
    SyncSOF_Start();
    // A pause is needed for the PLL to settle
    CyDelay(1);
}

void Sync_Main(void) {
    static uint16 frac = FracN_DEFAULT;
    static int16 prev_error = 0, int_error = 0;
    int16 cur_error, diff, p_term, i_term, d_term;
    uint16 pos;
    pos = CY_GET_REG8(SyncSOF_FRAME_POS_LO__STATUS_REG);
    if (pos & 0x01) {
        pos += (uint16)CY_GET_REG8(SyncSOF_FRAME_POS_HI__STATUS_REG) << 8;

        cur_error = pos - SYNC_SOF_CENTER;
        int_error += cur_error;
        if (int_error < -SYNC_I_GUARD)
            int_error = -SYNC_I_GUARD;
        else if (int_error > SYNC_I_GUARD)
            int_error = SYNC_I_GUARD;
            
        diff = cur_error - prev_error;
        p_term = cur_error * SYNC_P_GAIN;
        i_term = int_error * SYNC_I_GAIN;
        d_term = diff * SYNC_D_GAIN;
        prev_error = cur_error;
        int_error *= 0.9;
        
        frac += p_term + i_term + d_term;

        if (frac > SYNC_FRAC_MAX) frac = SYNC_FRAC_MAX;
        if (frac < SYNC_FRAC_MIN) frac = SYNC_FRAC_MIN;
        FracN_Set(frac);
    }
}

