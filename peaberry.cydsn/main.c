/* ========================================
 *
 * Copyright 2012 David Turnbull
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF DAVID TURNBULL.
 *
 * ========================================
*/
#include <peaberry.h>

uint8 Lock_I2C = LOCKI2C_UNLOCKED;

void main()
{
    CyGlobalIntEnable;
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
    SyncSOF_Start();
    while(!USBFS_GetConfiguration());
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
        
        USBAudio_Main();
        PCM3060_Main();
        Si570_Main();
            
    }
}

/* [] END OF FILE */
