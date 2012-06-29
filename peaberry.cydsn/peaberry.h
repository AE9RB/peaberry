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

#ifndef PEABERRY_H
#define PEABERRY_H

#include <device.h>

// Check and lock this in the main loop before using I2C
extern uint8 Lock_I2C;
#define LOCKI2C_UNLOCKED 0
#define LOCKI2C_SI570    1
#define LOCKI2C_PCM3060  2

// Control register bits
#define CONTROL_TX_ENABLE  0x01
#define CONTROL_RX_REVERSE 0x02
#define CONTROL_TX_REVERSE 0x04

// 48 24-bit stereo samples every 1 ms
#define I2S_BUF_SIZE (48u * 3 * 2)
// 48 12-bit mono samples every 1 ms
#define MIC_BUF_SIZE (48u * 2)
//  4 buffers is the minimum required to sync USB Audio
#define USB_AUDIO_BUFS 4

// Visibility into USBFS
extern uint8 USBFS_initVar;
extern uint8 USBFS_DmaTd[USBFS_MAX_EP];

// usbaudio.c
void USBAudio_SyncBufs(uint8 dma, uint8* use, uint8* eat, uint8* debounce, uint8 adjust);
void USBAudio_Main(void);

// syncsof.c
void SyncSOF_Start(void);
void SyncSOF_Stop(void);
void SyncSOF_Slower(void);
void SyncSOF_Faster(void);

// si570.c
extern volatile uint32 Si570_LO;
void Si570_Start(void);
void Si570_Main(void);

// pcm3060.c
void PCM3060_Start(void);
void PCM3060_Main(void);
uint8* PCM3060_TxBuf(void);
uint8* PCM3060_RxBuf(void);

// mic.c
void Mic_Start(void);
uint8* Mic_Buf(void);

#endif //PEABERRY_H
