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

#ifndef PEABERRY_H
#define PEABERRY_H

#include <device.h>

// Status register bits
#define STATUS_KEY_0  0x01
#define STATUS_KEY_1  0x02
#define STATUS_BOOT   0x04
#define STATUS_BEAT   0x08
#define STATUS_ATU_0  0x10

// Control register bits
#define CONTROL_LED      0x01
#define CONTROL_RX       0x02
#define CONTROL_TX       0x04
#define CONTROL_AMP      0x08
#define CONTROL_XK       0x10
#define CONTROL_ATU_0    0x20
#define CONTROL_ATU_0_OE 0x40
#define CONTROL_ATU_1    0x80

// Max buffer size for 1ms
#define I2S_BUF_SIZE (96u * 2 * 2)

// Unvisible stuff from Cypress that they expect us to use and don't export
uint8 USBFS_InitControlRead(void);
uint8 USBFS_InitControlWrite(void);
extern volatile T_USBFS_TD USBFS_currentTD;
extern volatile T_USBFS_EP_CTL_BLOCK USBFS_EP[];
extern uint8 USBFS_initVar;
extern uint8 USBFS_DmaTd[USBFS_MAX_EP];
extern uint8 USBFS_DmaChan[USBFS_MAX_EP];

// main.c
uint32 swap32(uint32) CYREENTRANT;
void ERROR(char* msg);

// morse.c
void Morse_Main(char* msg);

// audio.c
extern uint8 Audio_IQ_Channels;
void Audio_Start(void);
void Audio_Main(void);

// sync.c
void Sync_Start(void);
void Sync_Main(void);

// band.c
void Band_Main(void);

// si570.c
#define SI570_STARTUP_FREQ 56.32
extern volatile uint32 Si570_Xtal, Si570_LO;
extern uint8 Si570_Buf[], Si570_Factory[], Si570_OLD[];
void Si570_Init(void);
void Si570_Main(void);
void Si570_Fake_Reset(void);

// pcm3060.c
void PCM3060_Init(void);
void PCM3060_Start(void);
void PCM3060_Stop(void);
uint8* PCM3060_TxBuf(void);
uint8* PCM3060_RxBuf(void);

// settings.c
void Settings_Init(void);
void Settings_Main(void);

// tx.c
extern uint8 TX_Request;
void TX_Main(void);

// t1.c
extern uint8 T1_Band_Number;
extern uint8 T1_Tune_Request;
void T1_Main(void);

#endif //PEABERRY_H
