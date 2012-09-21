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

// Setings bits
#define IQ_GEN_TX     0x01
#define IQ_GEN_DIV    0x02

void `$INSTANCE_NAME`_Start(void) {
    uint8 i;
    i = CY_GET_REG8(`$INSTANCE_NAME`_Counter__CONTROL_AUX_CTL_REG);
    CY_SET_REG8(`$INSTANCE_NAME`_Counter__CONTROL_AUX_CTL_REG, i | 0x20);
}

void `$INSTANCE_NAME`_Stop(void) {
    uint8 i;
    i = CY_GET_REG8(`$INSTANCE_NAME`_Counter__CONTROL_AUX_CTL_REG);
    CY_SET_REG8(`$INSTANCE_NAME`_Counter__CONTROL_AUX_CTL_REG, i & ~0x20);
}

void `$INSTANCE_NAME`_SetTransmit(uint8 tx) {
    uint8 i;
    i = CY_GET_REG8(`$INSTANCE_NAME`_Settings__CONTROL_REG);
    if (tx) {
        CY_SET_REG8(`$INSTANCE_NAME`_Settings__CONTROL_REG, i | IQ_GEN_TX );
    } else {
        CY_SET_REG8(`$INSTANCE_NAME`_Settings__CONTROL_REG, i & ~IQ_GEN_TX );
    }
}

uint8 `$INSTANCE_NAME`_GetTransmit(void) {
    return CY_GET_REG8(`$INSTANCE_NAME`_Settings__CONTROL_REG) & IQ_GEN_TX;
}

void `$INSTANCE_NAME`_SetDivider(uint8 tx) {
    uint8 i;
    i = CY_GET_REG8(`$INSTANCE_NAME`_Settings__CONTROL_REG);
    if (tx) {
        CY_SET_REG8(`$INSTANCE_NAME`_Settings__CONTROL_REG, i | IQ_GEN_DIV );
    } else {
        CY_SET_REG8(`$INSTANCE_NAME`_Settings__CONTROL_REG, i & ~IQ_GEN_DIV );
    }
}

uint8 `$INSTANCE_NAME`_GetDivider(void) {
    return CY_GET_REG8(`$INSTANCE_NAME`_Settings__CONTROL_REG) & IQ_GEN_DIV;
}
