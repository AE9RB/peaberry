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

// RX/TX switching must be synchronized with PCM3060
uint8 TX_Request = 0;
// Check and lock this in the main loop before using I2C
uint8 Locked_I2C = 0;

uint8 B96_Enabled;
uint16 I2S_Buf_Size;

void main()
{
    B96_Enabled = !(Status_Read() & B96_EN);
    if (B96_Enabled) {
        I2S_Buf_Size = I2S_B96_SIZE;
        I2S_Clock_SetDivider(2);
    } else {
        I2S_Buf_Size = I2S_B48_SIZE;
        I2S_Clock_SetDivider(5);
    }
    
    CyGlobalIntEnable;

    SyncSOF_Enable(pup_DMA, pdn_DMA);
    IQGen_Start();
    
    USBFS_Start(B96_Enabled, USBFS_DWR_VDDD_OPERATION);
    while(!USBFS_GetConfiguration());
    Audio_USB_Start();

    Settings_Start();
    
    I2C_Start();
    Si570_Start();
    
    Audio_Start();

    for(;;) {
	
        // monitor VBUS to comply with USB spec
        if (USBFS_VBusPresent()) {
            if(!USBFS_initVar) {
                USBFS_Start(B96_Enabled, USBFS_DWR_VDDD_OPERATION);
                Audio_USB_Start();
            }
        } else {
            if(USBFS_initVar) {
                USBFS_Stop();
            }
        }
        
        Settings_Main();
        Audio_Main();
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

