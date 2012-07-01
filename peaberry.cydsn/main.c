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

uint8 Lock_I2C = LOCKI2C_UNLOCKED;

void main()
{
    CyGlobalIntEnable;
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
    SyncSOF_Start();
    while(!USBFS_GetConfiguration());
    Settings_Start();
    I2C_Start();
    Si570_Start();
    Mic_Start();
    PCM3060_Start();

    for(;;) {
	
        // monitor VBUS to comply with USB spec
        if (USBFS_VBusPresent()) {
            if(!USBFS_initVar) {
                USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
                SyncSOF_Start();
            }
        } else {
            if(USBFS_initVar) {
                SyncSOF_Stop();
                USBFS_Stop();
            }
        }
        
        Settings_Main();
        USBAudio_Main();
        PCM3060_Main();
        Si570_Main();
            
    }
}

uint32 swap32(uint32 original) CYREENTRANT {
    uint8 *r, *o;
    uint32 ret;
    r = (void*)&ret;
    o = (void*)&original;
    r[0] = o[3];
    r[1] = o[2];
    r[2] = o[1];
    r[3] = o[0];
    return ret;
}

