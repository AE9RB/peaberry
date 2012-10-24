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

#include "`$INSTANCE_NAME`_api.h"
#include <device.h>


// Use the enable macro instead of this start function:
// SyncSOF_Enable(up_DMA, down_DMA);
void `$INSTANCE_NAME`_Start(uint8 dmaUpChan, uint8 dmaDownChan) {
    uint8 td;

    CY_SET_REG8(`$INSTANCE_NAME`_Counter2__CONTROL_AUX_CTL_REG, 0x20);
    CY_SET_REG8(`$INSTANCE_NAME`_Counter1__CONTROL_AUX_CTL_REG, 0x20);
    CY_SET_REG8(`$INSTANCE_NAME`_Counter0__CONTROL_AUX_CTL_REG, 0x20);
    
    td = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(td, 1, td, 0);
    CyDmaTdSetAddress(td, LO16(`$INSTANCE_NAME`_PLL_HI__STATUS_REG), LO16(&FASTCLK_PLL_P));
    CyDmaChSetInitialTd(dmaUpChan, td);
    CyDmaChEnable(dmaUpChan, 1);

    td = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(td, 1, td, 0);
    CyDmaTdSetAddress(td, LO16(`$INSTANCE_NAME`_PLL_LO__STATUS_REG), LO16(&FASTCLK_PLL_P));
    CyDmaChSetInitialTd(dmaDownChan, td);
    CyDmaChEnable(dmaDownChan, 1);

}

// Returns the buffer number 0-1 which is to be used for USB DMA
uint8 `$INSTANCE_NAME`_USB_Buffer(void) {
    return CY_GET_REG8(`$INSTANCE_NAME`_BUFFER__STATUS_REG);
}

// SOF(millisecond) countdown timer for use with 
// PCM3060 volume changes during tx/rx switching
void `$INSTANCE_NAME`_SetCountdown(uint8 ms) {
    CY_SET_REG8(`$INSTANCE_NAME`_DelayCountdown_u0__A0_REG, ms);
}

// Returns current countdown value, stops at 0
uint8 `$INSTANCE_NAME`_GetCountdown(void) {
    return CY_GET_REG8(`$INSTANCE_NAME`_DelayCountdown_u0__A0_REG);
}
