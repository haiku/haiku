/*
	Haiku S3 Savage driver adapted from the X.org Savage driver.

	Copyright (C) 1994-2000 The XFree86 Project, Inc.  All Rights Reserved.
	Copyright (c) 2003-2006, X.Org Foundation

	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2006-2008
*/

#ifndef __SAVAGE_H__
#define __SAVAGE_H__



#define CURSOR_BYTES	1024		// bytes used for cursor image in video memory

#define	ADVANCED_FUNC_CTRL		0x850C
#define SYSTEM_CONTROL_REG		0x83DA


// Stream Processor 1.

// Primary Stream 1 Frame Buffer Address 0.
#define PRI_STREAM_FBUF_ADDR0	0x81c0
// Primary Stream 1 Frame Buffer Address 1.
#define PRI_STREAM_FBUF_ADDR1	0x81c4
// Primary Stream 1 Stride.
#define PRI_STREAM_STRIDE		0x81c8

// Stream Processor 2.

// Primary Stream 2 Frame Buffer Address 0.
#define PRI_STREAM2_FBUF_ADDR0	0x81b0
// Primary Stream 2 Frame Buffer Address 1.
#define PRI_STREAM2_FBUF_ADDR1	0x81b4
// Primary Stream 2 Stride.
#define PRI_STREAM2_STRIDE		0x81b8

#define S3_GLOBAL_GBD_REG		0x816C	// global bitmap descriptor register

#define MEMORY_CTRL0_REG		0xCA
#define MEM_PS1					0x10	// CRCA_4 :Primary stream 1
#define MEM_PS2					0x20	// CRCA_5 :Primary stream 2

#define SRC_BASE				0xa4d4
#define DEST_BASE				0xa4d8
#define CLIP_L_R				0xa4dc
#define CLIP_T_B				0xa4e0
#define DEST_SRC_STR			0xa4e4
#define MONO_PAT_0				0xa4e8
#define MONO_PAT_1				0xa4ec

#define DISABLE_BLOCK_WRITE_2D	0x10	// CR88_4 =1 : disable block write

#define STATUS_WORD0		(ReadReg32(0x48C00))
#define ALT_STATUS_WORD0	(ReadReg32(0x48C60))
#define MAXFIFO				0x7f00

// BCI definitions.
//=================

#define TILE_FORMAT_LINEAR	0

#define BCI_ENABLE			8		// savage4, MX, IX, 3D
#define BCI_ENABLE_TWISTER	0		// twister, prosavage, DDR, supersavage, 2000

#define S3_BIG_ENDIAN		4
#define S3_LITTLE_ENDIAN	0
#define S3_BD64				1

#define BCI_BD_BW_DISABLE	0x10000000
#define BCI_BUFFER_OFFSET	0x10000

#define BCI_GET_PTR		vuint32* bci_ptr = ((uint32*)(gInfo.regs + BCI_BUFFER_OFFSET))
#define BCI_SEND(dw)	(*bci_ptr++ = ((uint32)(dw)))

#define BCI_CMD_NOP					0x40000000
#define BCI_CMD_RECT				0x48000000
#define BCI_CMD_RECT_XP				0x01000000
#define BCI_CMD_RECT_YP				0x02000000
#define BCI_CMD_GET_ROP(cmd)		(((cmd) >> 16) & 0xFF)
#define BCI_CMD_SET_ROP(cmd, rop)	((cmd) |= ((rop & 0xFF) << 16))

#define BCI_CMD_SEND_COLOR			0x00008000
#define BCI_CMD_DEST_PBD_NEW		0x00000C00
#define BCI_CMD_SRC_SOLID			0x00000000
#define BCI_CMD_SRC_SBD_COLOR_NEW	0x00000140

#define BCI_W_H(w, h)				((((h) << 16) | (w)) & 0x0FFF0FFF)
#define BCI_X_Y(x, y)				((((y) << 16) | (x)) & 0x0FFF0FFF)


static inline void VerticalRetraceWait()
{
	if (ReadCrtcReg(0x17) & 0x80) {
		int i = 0x10000;
		while ((ReadReg8(SYSTEM_CONTROL_REG) & 0x08) == 0x08 && i--) ;
		i = 0x10000;
		while ((ReadReg8(SYSTEM_CONTROL_REG) & 0x08) == 0x00 && i--) ;
	}
}


#endif // __SAVAGE_H__
