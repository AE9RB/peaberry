// ========================================
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
// ========================================

#include "cytypes.h"
#include "cyfitter.h"

// 15860 is the computed value for 36.864 MHz
#define `$INSTANCE_NAME`_DEFAULT 15860

// Start the fractional PLL using this macro with the name of the DMA block.
// The 8051 doesn't like function pointers so this help keep app code clean.
#define `$INSTANCE_NAME`_Start(dmaP) {`$INSTANCE_NAME`_Start2( \
    dmaP##_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_FASTCLK_PLL_BASE)) \
);}

// Use FracN_Start in application code, not this start2
void `$INSTANCE_NAME`_Start2(uint8);

// Set the fraction to a new value
void `$INSTANCE_NAME`_Set(uint16);
