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

#ifndef PEABERRY_H
#define PEABERRY_H

#include <device.h>

// Control register bits
#define CONTROL_LO_DIV_BY_8  0x01
#define CONTROL_RX_REVERSE   0x02
#define CONTROL_TX_ENABLE    0x04

// 48 24-bit stereo samples every 1 ms
#define I2S_BUF_SIZE (48u * 3 * 2)
// 48 12-bit mono samples every 1 ms
#define MIC_BUF_SIZE (48u * 2)
// Four buffers is the minimum required to sync USB Audio.
#define USB_AUDIO_BUFS 4
// The four buffers are treated as eight half-buffers by the DelSig DMA
// so we can avoid any race conditions where the DelSig catches up
// while we are preparing for the next USB transfer.
#define DMA_AUDIO_BUFS (USB_AUDIO_BUFS * 2)

// Visibility into USBFS
extern uint8 USBFS_initVar;
extern uint8 USBFS_DmaTd[USBFS_MAX_EP];

// main.c
extern uint8 TX_Request, Locked_I2C;
#define LOCKI2C_UNLOCKED 0
#define LOCKI2C_SI570    1
#define LOCKI2C_PCM3060  2
uint32 swap32(uint32) CYREENTRANT;

// usbaudio.c
void USBAudio_SyncBufs(uint8 dma, uint8* use, int8* distance);
void USBAudio_Main(void);
void USBAudio_Start(void);
uint8 USBAudio_Volume(void);

// syncsof.c
void SyncSOF_Start(void);
void SyncSOF_Stop(void);
void SyncSOF_Slower(void);
void SyncSOF_Faster(void);

// si570.c
#define SI570_STARTUP_FREQ 56.32
extern volatile uint32 Si570_Xtal, Si570_LO;
extern uint8 Si570_Buf[], Si570_Factory[], Si570_OLD[];
void Si570_Start(void);
void Si570_Main(void);
void Si570_Fake_Reset(void);

// pcm3060.c
void PCM3060_Start(void);
void PCM3060_Main(void);
uint8* PCM3060_TxBuf(void);
uint8* PCM3060_RxBuf(void);

// mic.c
void Mic_Start(void);
uint8* Mic_Buf(void);

// settings.c
void Settings_Start(void);
void Settings_Main(void);


#endif //PEABERRY_H
