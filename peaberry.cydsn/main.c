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

void main_init(void) {
    CyGlobalIntEnable;
    SyncSOF_Start();
    FracN_Start(P_DMA);
    I2C_Start();
    Settings_Init();
    Si570_Init();
    PCM3060_Init();
}

void main_start(void) {
    Audio_Start();
    PCM3060_Start();
}

void main_stop(void) {
    TX_Request = 0;
    PCM3060_Stop();
}

void main()
{
    main_init();
    
    Control_Write(Control_Read() & ~CONTROL_LED);

    for(;;) {
        if (USBFS_VBusPresent()) {
            if(!USBFS_initVar) {
                USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
                main_start();
            }
        } else {
            if(USBFS_initVar) {
                main_stop();
                USBFS_Stop();
            }
        }
        
        Settings_Main();
        Audio_Main();
        Si570_Main();
        TX_Main();
            
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
    
    Control_Write(Control_Read() & ~CONTROL_TX | CONTROL_LED | CONTROL_AMP | CONTROL_RX);
    Morse_Main(msg);
    
    while(1) {
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