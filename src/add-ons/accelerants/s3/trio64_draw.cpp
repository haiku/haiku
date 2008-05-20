/*
	Haiku S3 Trio64 driver adapted from the X.org S3 driver.

	Copyright 2001	Ani Joshi <ajoshi@unixbox.com>

	Copyright 2008 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2008
*/


#include "accel.h"
#include "trio64.h"



void 
Trio64_FillRectangle(engine_token* et, uint32 color, fill_rect_params* pList,
					 uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	gInfo.WaitQueue(3);
	WriteReg16(MULTIFUNC_CNTL, 0xa000);
	WriteReg32(FRGD_COLOR, color);
	WriteReg16(FRGD_MIX, FSS_FRGDCOL | 0x07);		// 7 = GXcopy rop

	while (count--) {
		int x = pList->left;
		int y = pList->top;
		int w = pList->right - x;
		int h = pList->bottom - y;

		gInfo.WaitQueue(5);
		WriteReg16(CUR_X, x);
		WriteReg16(CUR_Y, y);
		WriteReg16(CUR_WIDTH, w);
		WriteReg16(MULTIFUNC_CNTL, h);
		WriteReg16(CMD, CMD_RECT | DRAW | INC_X | INC_Y | WRTDATA);

		pList++;
	}
}


void 
Trio64_FillSpan(engine_token* et, uint32 color, uint16* pList, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	gInfo.WaitQueue(3);
	WriteReg16(MULTIFUNC_CNTL, 0xa000);
	WriteReg32(FRGD_COLOR, color);
	WriteReg16(FRGD_MIX, FSS_FRGDCOL | 0x07);		// 7 = GXcopy rop

	while (count--) {
		int y = *pList++;
		int x = *pList++;
		int w = *pList++ - x;

		// Some S3 chips display a line completely across the screen when a
		// span has zero width;  thus, the following if statement discards any
		// span with zero or negative width.

		if (w < 0)
			continue;

		// Draw the span as a rectangle with a height of 1 to avoid the
		// extra complexity of drawing a line.

		gInfo.WaitQueue(5);
		WriteReg16(CUR_X, x);
		WriteReg16(CUR_Y, y);
		WriteReg16(CUR_WIDTH, w);
		WriteReg16(MULTIFUNC_CNTL, 0);	// height is 1; but as computed it is 0
		WriteReg16(CMD, CMD_RECT | DRAW | INC_X | INC_Y | WRTDATA);
	}
}


void 
Trio64_InvertRectangle(engine_token* et, fill_rect_params* pList, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	gInfo.WaitQueue(2);
	WriteReg16(MULTIFUNC_CNTL, 0xa000);
	WriteReg16(FRGD_MIX, FSS_FRGDCOL | 0x00);		// 0 = GXinvert rop

	while (count--) {
		int x = pList->left;
		int y = pList->top;
		int w = pList->right - x;
		int h = pList->bottom - y;

		gInfo.WaitQueue(5);
		WriteReg16(CUR_X, x);
		WriteReg16(CUR_Y, y);
		WriteReg16(CUR_WIDTH, w);
		WriteReg16(MULTIFUNC_CNTL, h);
		WriteReg16(CMD, CMD_RECT | DRAW | INC_X | INC_Y | WRTDATA);

		pList++;
	}
}


void 
Trio64_ScreenToScreenBlit(engine_token* et, blit_params* pList, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	gInfo.WaitQueue(2);
	WriteReg16(MULTIFUNC_CNTL, 0xa000);
	WriteReg16(FRGD_MIX, FSS_BITBLT | 0x07);		// 7 = GXcopy rop

	while (count--) {
		int src_x = pList->src_left;
		int src_y = pList->src_top;
		int dest_x = pList->dest_left;
		int dest_y = pList->dest_top;
		int width = pList->width;
		int height = pList->height;

		if (src_x == dest_x &&  src_y == dest_y)
			continue;

		int cmd = CMD_BITBLT | DRAW | INC_X | INC_Y | WRTDATA;

		if (src_x < dest_x) {
			src_x += width;
			dest_x += width;
			cmd &= ~INC_X;
		}

		if (src_y < dest_y) {
			src_y += height;
			dest_y += height;
			cmd &= ~INC_Y;
		}

		gInfo.WaitQueue(7);
		WriteReg16(CUR_X, src_x);
		WriteReg16(CUR_Y, src_y);
		WriteReg16(DESTX_DIASTP, dest_x);
		WriteReg16(DESTY_AXSTP, dest_y);
		WriteReg16(CUR_WIDTH, width);
		WriteReg16(MULTIFUNC_CNTL, height);
		WriteReg16(CMD, cmd);

		pList ++;
	}
}
