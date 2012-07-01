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

#define XTAL_MIN (114.285 - 2)
#define XTAL_MAX (114.285 + 2)

uint8 buffer[CYDEV_EEPROM_ROW_SIZE];

void Settings_Start(void) {
    float crystal_freq;
    EEPROM_Start();
    
    crystal_freq = ((float)swap32(*(reg32*)CYDEV_EE_BASE) / 0x01000000);
    
    if (crystal_freq < XTAL_MIN || crystal_freq > XTAL_MAX ) {
        // New radio! Si570_Start() will compute xtal
        // and it will immediately be stored in eeprom.
        Si570_Xtal = 0;
    } else {
        Si570_Xtal = *(reg32*)CYDEV_EE_BASE;
    }
    
}

void Settings_Main(void) {
    uint8 i;
    
    if (*(reg32*)CYDEV_EE_BASE != Si570_Xtal) {
        if (EEPROM_QueryWrite() != CYRET_STARTED) {
            for (i=4; i < CYDEV_EEPROM_ROW_SIZE; i++) buffer[i] = 0;
            *(uint32*)&buffer = Si570_Xtal;
            EEPROM_StartWrite(buffer, 0);
        }
    }

}
