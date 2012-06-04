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
#include <device.h>
#include <pcm3060.h>

void main()
{
    CyGlobalIntEnable;
    I2C_Start();
    USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
    PCM3060_Start();

    for(;;) {
        PCM3060_Main();
	
        //TODO monitor VBUS to comply with USB spec
    
	    if(USBFS_IsConfigurationChanged() != 0u) {
			//TODO out endpoints
        }
    }
}

/* [] END OF FILE */
