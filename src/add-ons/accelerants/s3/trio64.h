/*
	Haiku S3 Trio64 driver adapted from the X.org S3 driver.

	Copyright 2001  Ani Joshi <ajoshi@unixbox.com>

	Copyright 2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2008
*/


#ifndef __TRIO64_H__
#define __TRIO64_H__


// Note that the cursor normally needs only 1024 bytes; however, if 1024 bytes
// are used, some of the Trio64 chips draw a short white horizontal line below
// and to the right of the cursor.  Setting the number of bytes to 2048 solves
// the problem.

#define CURSOR_BYTES	2048		// see comment above


// Command Registers.
#define ADVFUNC_CNTL	0x4ae8
#define SUBSYS_STAT		0x42e8
#define SUBSYS_CNTL		0x42e8
#define CUR_Y			0x82e8
#define CUR_X			0x86e8
#define DESTY_AXSTP		0x8ae8
#define DESTX_DIASTP	0x8ee8
#define CUR_WIDTH		0x96e8
#define CMD				0x9ae8
#define GP_STAT			0x9ae8
#define FRGD_COLOR		0xa6e8
#define WRT_MASK		0xaae8
#define FRGD_MIX		0xbae8
#define MULTIFUNC_CNTL	0xbee8

// Command register bits.
#define CMD_RECT		0x4000
#define CMD_BITBLT		0xc000
#define INC_Y			0x0080
#define INC_X			0x0020
#define DRAW			0x0010
#define WRTDATA			0x0001

// Foreground mix register.
#define FSS_FRGDCOL		0x0020
#define FSS_BITBLT		0x0060

#define GP_BUSY			0x0200

#define SCISSORS_T		0x1000
#define SCISSORS_L		0x2000
#define SCISSORS_B		0x3000
#define SCISSORS_R		0x4000


#endif // __TRIO64_H__
