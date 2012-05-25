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

#define PCM3060_ADDR 0x46
#define PCM3060_REG 0x40
uint8 pcm3060_enable[2];


void main()
{
    pcm3060_enable[0] = 0x40;
    pcm3060_enable[1] = 0xC0;

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    CyGlobalIntEnable;
    
    USBFS_Start(0, USBFS_5V_OPERATION);

    I2S_Start();

    I2C_Start();
    I2C_MasterClearStatus();
    I2C_MasterWriteBuf(PCM3060_ADDR, (uint8 *) pcm3060_enable, 2, I2C_MODE_COMPLETE_XFER);
    while(0u == (I2C_MasterStatus() & I2C_MSTAT_WR_CMPLT)) {}
    
    //while(!USBFS_GetConfiguration());
    

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
