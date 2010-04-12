/*
 * This file is a part of BeOS USBVision driver project.
 * Copyright (c) 2003 by Siarzuk Zharski <imker@gmx.li>
 *
 * This file may be used under the terms of the BSD License
 * Look into the file "License" for details.
 *
 * Skeletal part of this code was inherired from original BeOS sample code,
 * that is distributed under the terms of the Be Sample Code License.
 * Look into the file "Be License" for details.
 *
 * $Source: /cvsroot/sis4be/usbvision/include/nt100x.h,v $
 * $Author: zharik $
 * $Revision: 1.1 $
 * $Date: 2003/07/15 18:58:05 $
 *
 */

#ifndef _NT100X_H_
#define _NT100X_H_

#define READ_CMD_PREFIX 0x33C2
#define WRITE_CMD_PREFIX 0x3342

#define COMMAND_DATA_LENGTH 0x08
typedef struct{
  uint8  reg;
  uint8  data_length;
  uint8  data[COMMAND_DATA_LENGTH];
}xet_nt100x_reg;

#define NT_IOCTL_READ_REGISTER  B_DEVICE_OP_CODES_END + 1
#define NT_IOCTL_WRITE_REGISTER B_DEVICE_OP_CODES_END + 2

/*General Control Registers (Power, Restart EP, USB, IO­pins, Camera Control)*/
#define PWR_REG         0x00 /*0*/
#define CONFIG_REG      0x01 /*1*/
#define ADRS_REG        0x02 /*2*/
#define ALTER_REG       0x03 /*3*/
#define FORCE_ALTER_REG 0x04 /*4*/
#define STATUS_REG      0x05 /*5*/
#define IOPIN_REG       0x06 /*6*/
#define SER_MODE        0x07 /*7*/
#define SER_ADRS        0x08 /*8*/
#define SER_CONT        0x09 /*9*/
#define SER_DAT1        0x0a /*10*/
#define SER_DAT2        0x0b /*11*/
#define SER_DAT3        0x0c /*12*/
#define SER_DAT4        0x0d /*13*/

/*EEPROM Read/Write Registers*/
#define EE_DATA  0x0e /*14*/
#define EE_LSBAD 0x0f /*15*/
#define EE_CONT  0x10 /*16*/

/*17: 0x11*/

/*DRAM and Memory Buffers Setup Registers*/
#define DRM_CONT 0x12 /*18*/
#define DRM_PRM1 0x13 /*19*/
#define DRM_PRM2 0x14 /*20*/
#define DRM_PRM3 0x15 /*21*/
#define DRM_PRM4 0x16 /*22*/
#define DRM_PRM5 0x17 /*23*/
#define DRM_PRM6 0x18 /*24*/
#define DRM_PRM7 0x19 /*25*/
#define DRM_PRM8 0x1a /*26*/

/*Video Setup and Control Registers*/
#define VIN_REG1    0x1b /*27*/
#define VIN_REG2    0x1c /*28*/
#define LXSIZE_IN   0x1d /*29*/
#define MXSIZE_IN   0x1e /*30*/
#define LYSIZE_IN   0x1f /*31*/
#define MYSIZE_IN   0x20 /*32*/
#define LX_OFFST    0x21 /*33*/
#define MX_OFFST    0x22 /*34*/
#define LY_OFFST    0x23 /*35*/
#define MY_OFFST    0x24 /*36*/
#define FRM_RATE    0x25 /*37*/
#define LXSIZE_O    0x26 /*38*/
#define MXSIZE_O    0x27 /*39*/
#define LYSIZE_O    0x28 /*40*/
#define MYSIZE_O    0x29 /*41*/
#define FILT_CONT   0x2a /*42*/
#define VO_MODE     0x2b /*43*/
#define INTRA_CYC   0x2c /*44*/
#define STRIP_SZ    0x2d /*45*/
#define FORCE_INTRA 0x2e /*46*/
#define FORCE_UP    0x2f /*47*/
#define BUF_THR     0x30 /*48*/
#define DVI_YUV     0x31 /*49*/
#define AUDIO_CONT  0x32 /*50*/
#define AUD_PK_LEN  0x33 /*51*/
#define BLK_PK_LEN  0x34 /*52*/

/*USB WatchDog Register*/
#define WD_COUNT 0x35 /*53*/

/*54 0x36*/
/*55 0x37*/

/*Compression Ratio Management Registers*/
#define PCM_THR1   0x38 /*56*/
#define PCM_THR2   0x39 /*57*/
#define DIST_THR_I 0x3a /*58*/
#define DIST_THR_A 0x3b /*59*/
#define MAX_DIST_I 0x3c /*60*/
#define MAX_DIST_A 0x3d /*61*/
#define VID_BUF_   0x3e /*62*/
#define LFP_LSB    0x3f /*63*/
#define LFP_MSB    0x40 /*64*/
#define VID_LPF    0x41 /*65*/

#endif //_NT100X_H_
