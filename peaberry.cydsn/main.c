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

// A compliant USB device is required to monitor
// vbus voltage and shut down if it disappears.
void main_usb_vbus(void) {
    if (USBFS_VBusPresent()) {
        if(!USBFS_initVar) {
            USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
            Audio_Start();
            PCM3060_Start();
        }
    } else {
        if(USBFS_initVar) {
            TX_Request = 0;
            PCM3060_Stop();
            USBFS_Stop();
        }
    }
}

void main()
{
    uint8 i, beat, beater = 0;

    CyDelay(100); //TODO move this to bootloader
    CyGlobalIntEnable;
    Sync_Start();
    I2C_Start();
    Settings_Init();
    Si570_Init();
    PCM3060_Init();
    
    Control_Write(Control_Read() & ~CONTROL_LED);

    for(;;) {
        // USB Audio is very high priority
        Audio_Main();
        Sync_Main();
        // Everything else runs twice per millisecond
        // Keep T1 first for timing accuracy
        i = Status_Read() & STATUS_BEAT;
        if (beat != i) {
            switch(beater++) {
            case 0:
                T1_Main();
                break;
            case 1:
                TX_Main();
                break;
            case 2:
                Settings_Main();
                break;
            case 3:
                Si570_Main();
                break;
            case 4:
                Band_Main();
                break;
            default:
                main_usb_vbus();
                beater = 0;
                beat = i;
            }
        }
            
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

void ERROR(char* msg) {
    uint8 i, beat;
    uint16 timer = 0;
    
    if(USBFS_initVar) USBFS_Stop();
    Control_Write(Control_Read() & ~CONTROL_TX | CONTROL_LED | CONTROL_AMP | CONTROL_RX);
    Morse_Main(msg);
    
    for(;;) {
        i = Status_Read() & STATUS_BEAT;
        if (beat != i) {
            beat = i;
            if (!timer--) {
                timer = 480; // 5 WPM
                Morse_Main(0);
            }
        }    
    }
}