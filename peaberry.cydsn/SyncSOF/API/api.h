// ========================================
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
// ========================================

#include "cytypes.h"
#include "cyfitter.h"

void `$INSTANCE_NAME`_Start(uint8, uint8);
uint8 `$INSTANCE_NAME`_USB_Buffer(void);
void `$INSTANCE_NAME`_SetCountdown(uint8);
uint8 `$INSTANCE_NAME`_GetCountdown(void);

#define `$INSTANCE_NAME`_Enable(dmaUp, dmaDown) {`$INSTANCE_NAME`_Start( \
    dmaUp##_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_FASTCLK_PLL_BASE)), \
    dmaDown##_DmaInitialize(1, 1, HI16(CYDEV_PERIPH_BASE), HI16(CYDEV_FASTCLK_PLL_BASE))  \
);}
