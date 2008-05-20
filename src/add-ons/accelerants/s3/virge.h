/*
	Haiku S3 Virge driver adapted from the X.org S3 Virge driver.

	Copyright (C) 1994-1999 The XFree86 Project, Inc.  All Rights Reserved.

	Copyright 2007-2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2007-2008
*/


#ifndef __VIRGE_H__
#define __VIRGE_H__



#define CURSOR_BYTES	1024		// bytes used for cursor image in video memory

// Miscellaneous registers.
#define SUBSYS_STAT_REG		0x8504
#define SYSTEM_CONTROL_REG	0x83DA
#define DDC_REG				0xFF20

// Memory port controller registers.
#define FIFO_CONTROL_REG	0x8200

// Image write stuff.
#define SRC_BASE		0xA4D4
#define DEST_BASE		0xA4D8
#define CLIP_L_R		0xA4DC
#define CLIP_T_B		0xA4E0
#define DEST_SRC_STR	0xA4E4
#define MONO_PAT_0		0xA4E8
#define MONO_PAT_1		0xA4EC
#define PAT_BG_CLR		0xA4F0
#define PAT_FG_CLR		0xA4F4
#define CMD_SET			0xA500
#define RWIDTH_HEIGHT	0xA504
#define RSRC_XY			0xA508
#define RDEST_XY		0xA50C

// Command Register.
#define	CMD_OP_MSK		(0xf << 27)
#define	CMD_BITBLT		(0x0 << 27)
#define	CMD_RECT		((0x2 << 27) | 0x0100)
#define	CMD_LINE		(0x3 << 27)
#define	CMD_POLYFILL	(0x5 << 27)
#define	CMD_NOP			(0xf << 27)

#define	DRAW		0x0020

// Destination Color Format.
#define DST_8BPP	0x00
#define DST_16BPP	0x04
#define DST_24BPP	0x08

// X Positive, Y Positive (Bit BLT).
#define CMD_XP		0x02000000
#define CMD_YP		0x04000000


#define IN_SUBSYS_STAT() (ReadReg32(SUBSYS_STAT_REG))


static inline void VerticalRetraceWait()
{
	if (ReadCrtcReg(0x17) & 0x80) {
		int i = 0x10000;
		while ((ReadReg8(SYSTEM_CONTROL_REG) & 0x08) == 0x08 && i--) ;
		i = 0x10000;
		while ((ReadReg8(SYSTEM_CONTROL_REG) & 0x08) == 0x00 && i--) ;
	}
}


#endif // __VIRGE_H__
