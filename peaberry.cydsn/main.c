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

void main()
{
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    CyGlobalIntEnable;
    USBFS_Start(0, USBFS_5V_OPERATION);
    while(!USBFS_GetConfiguration());

    /* CyGlobalIntEnable; */ /* Uncomment this line to enable global interrupts. */
    for(;;)
    {
	    if(USBFS_IsConfigurationChanged() != 0u) {
            uint8 conf = USBFS_GetConfiguration();
            if (conf != 0u) {
                uint8 intset = USBFS_GetInterfaceSetting(1u);
                if (intset == 1u) {
                    // USBFS_EnableOutEP(2u);
                } else {
                    // USBFS_DisableOutEP(2u);
                }
            }    
        }

    }
}

/* [] END OF FILE */
