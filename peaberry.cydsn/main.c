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
    I2C_Start();
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
    PCM3060_Start();
    Si570_Start();
    SyncSOF_Start();

    for(;;) {
	
        // monitor VBUS to comply with USB spec
        if (USBFS_VBusPresent()) {
            if(!USBFS_initVar) {
                USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
            }
        } else {
            if(USBFS_initVar) {
                USBFS_Stop();
            }
        }
        
        USBAudio_Main();
        PCM3060_Main();
        Si570_Main();
            
    }
}

/* [] END OF FILE */
