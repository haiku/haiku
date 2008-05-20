/*
	Haiku S3 Virge driver adapted from the X.org Virge driver.

	Copyright (C) 1994-1999 The XFree86 Project, Inc.	All Rights Reserved.

	Copyright 2007 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2007
*/


#include "accel.h"
#include "virge.h"



static inline void WaitForSync()
{
	while ((IN_SUBSYS_STAT() & 0x2000) == 0) ;
}



void 
Virge_FillRectangle(engine_token* et, uint32 color, fill_rect_params* pList, uint32 count)
{
	int rop = 0xF0;
	int cmd = DRAW | rop << 17 | CMD_RECT | CMD_XP | CMD_YP ;

	(void)et;		// avoid compiler warning for unused arg

	cmd |= gInfo.sharedInfo->commonCmd;

	while (count--) {
		int x = pList->left;
		int y = pList->top;
		int w = pList->right - x + 1;
		int h = pList->bottom - y + 1;

		gInfo.WaitQueue(4);
		WriteReg32(PAT_FG_CLR, color);
		WriteReg32(RWIDTH_HEIGHT, ((w - 1) << 16) | h);
		WriteReg32(RDEST_XY, (x << 16) | y);
		WriteReg32(CMD_SET, cmd);
		WaitForSync();

		pList++;
	}
}


void 
Virge_FillSpan(engine_token* et, uint32 color, uint16* pList, uint32 count)
{
	int rop = 0xF0;
	int cmd = DRAW | rop << 17 | CMD_RECT | CMD_XP | CMD_YP ;

	(void)et;		// avoid compiler warning for unused arg

	cmd |= gInfo.sharedInfo->commonCmd;

	while (count--) {
		int y = *pList++;
		int x = *pList++;
		int w = *pList++ - x + 1;

		// The MediaPlayer in Zeta 1.21 displays a window which has 2 zero width
		// spans which the Virge chips display as a line completely across the
		// screen;  thus, the following if statement discards any span with zero
		// or negative width.

		if (w <= 0)
			continue;

		// Draw the span as a rectangle with a height of 1 to avoid the
		// extra complexity of drawing a line.

		gInfo.WaitQueue(4);
		WriteReg32(PAT_FG_CLR, color);
		WriteReg32(RWIDTH_HEIGHT, ((w - 1) << 16) | 1);
		WriteReg32(RDEST_XY, (x << 16) | y);
		WriteReg32(CMD_SET, cmd);
		WaitForSync();
	}
}


void 
Virge_InvertRectangle(engine_token* et, fill_rect_params* pList, uint32 count)
{
	int rop = 0x55;			// use GXinvert for rop
	int cmd = DRAW | rop << 17 | CMD_RECT | CMD_XP | CMD_YP;

	(void)et;		// avoid compiler warning for unused arg

	cmd |= gInfo.sharedInfo->commonCmd;

	while (count--) {
		int x = pList->left;
		int y = pList->top;
		int w = pList->right - x + 1;
		int h = pList->bottom - y + 1;

		gInfo.WaitQueue(3);
		WriteReg32(RWIDTH_HEIGHT, ((w - 1) << 16) + h);
		WriteReg32(RDEST_XY, (x << 16) | y);
		WriteReg32(CMD_SET, cmd);
		WaitForSync();

		pList++;
	}
}


void 
Virge_ScreenToScreenBlit(engine_token* et, blit_params* pList, uint32 count)
{
	int rop = 0xCC;		// use GXcopy for rop
	int cmd = DRAW | rop << 17 | CMD_BITBLT | CMD_XP | CMD_YP;

	(void)et;		// avoid compiler warning for unused arg

	cmd |= gInfo.sharedInfo->commonCmd;

	while (count--) {
		int src_x = pList->src_left;
		int src_y = pList->src_top;
		int dest_x = pList->dest_left;
		int dest_y = pList->dest_top;
		int width = pList->width;
		int height = pList->height;

		if (src_x == dest_x &&  src_y == dest_y)
			continue;

		cmd |= CMD_XP | CMD_YP;		// restore these flags in case removed on last iteration

		if (src_x < dest_x) {
			src_x += width;
			dest_x += width;
			cmd &= ~CMD_XP;
		}

		if (src_y < dest_y) {
			src_y += height;
			dest_y += height;
			cmd &= ~CMD_YP;
		}

		gInfo.WaitQueue(4);
		WriteReg32(RWIDTH_HEIGHT, ((width) << 16) | (height + 1));
		WriteReg32(RSRC_XY, (src_x << 16) | src_y);
		WriteReg32(RDEST_XY, (dest_x << 16) | dest_y);
		WriteReg32(CMD_SET, cmd);
		WaitForSync();

		pList ++;
	}
}
