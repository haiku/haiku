/*
	Copyright 2010 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac
*/


#include "accelerant.h"
#include "3dfx.h"


// Constants for the DST_FORMAT register based upon the color depth.

static const uint32 fmtColorDepth[] = {
	0x10000,		// 1 byte/pixel
	0x30000,		// 2 bytes/pixel
	0x50000,		// 3 bytes/pixel (not used)
	0x50000			// 4 bytes/pixel
};


void
TDFX_FillRectangle(engine_token* et, uint32 color, fill_rect_params* list, 
	uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	DisplayModeEx& mode = gInfo.sharedInfo->displayMode;
	uint32 fmt = mode.bytesPerRow | fmtColorDepth[mode.bytesPerPixel - 1];

	TDFX_WaitForFifo(3);
	OUTREG32(DST_FORMAT, fmt);
	OUTREG32(COLOR_BACK, color);
	OUTREG32(COLOR_FORE, color);

	while (count--) {
		int x = list->left;
		int y = list->top;
		int w = list->right - x + 1;
		int h = list->bottom - y + 1;

		TDFX_WaitForFifo(3);
		OUTREG32(DST_SIZE, w | (h << 16));
		OUTREG32(DST_XY,   x | (y << 16));
		OUTREG32(CMD_2D, RECTANGLE_FILL | CMD_2D_GO | (ROP_COPY << 24));

		list++;
	}
}


void
TDFX_FillSpan(engine_token* et, uint32 color, uint16* list, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	DisplayModeEx& mode = gInfo.sharedInfo->displayMode;
	uint32 fmt = mode.bytesPerRow | fmtColorDepth[mode.bytesPerPixel - 1];

	TDFX_WaitForFifo(3);
	OUTREG32(DST_FORMAT, fmt);
	OUTREG32(COLOR_BACK, color);
	OUTREG32(COLOR_FORE, color);

	while (count--) {
		int y = *list++;
		int x = *list++;
		int w = *list++ - x + 1;

		if (w <= 0)
			continue;	// discard span with zero or negative width

		TDFX_WaitForFifo(3);
		OUTREG32(DST_SIZE, w | (1 << 16));
		OUTREG32(DST_XY,   x | (y << 16));
		OUTREG32(CMD_2D, RECTANGLE_FILL | CMD_2D_GO | (ROP_COPY << 24));
	}
}


void
TDFX_InvertRectangle(engine_token* et, fill_rect_params* list, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	DisplayModeEx& mode = gInfo.sharedInfo->displayMode;
	uint32 fmt = mode.bytesPerRow | fmtColorDepth[mode.bytesPerPixel - 1];

	TDFX_WaitForFifo(1);
	OUTREG32(DST_FORMAT, fmt);

	while (count--) {
		int x = list->left;
		int y = list->top;
		int w = list->right - x + 1;
		int h = list->bottom - y + 1;

		TDFX_WaitForFifo(3);
		OUTREG32(DST_SIZE, w | (h << 16));
		OUTREG32(DST_XY,   x | (y << 16));
		OUTREG32(CMD_2D, RECTANGLE_FILL | CMD_2D_GO | (ROP_INVERT << 24));

		list++;
	}
}


void
TDFX_ScreenToScreenBlit(engine_token* et, blit_params* list, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	DisplayModeEx& mode = gInfo.sharedInfo->displayMode;
	uint32 fmt = mode.bytesPerRow | fmtColorDepth[mode.bytesPerPixel - 1];

	TDFX_WaitForFifo(2);
	OUTREG32(DST_FORMAT, fmt);
	OUTREG32(SRC_FORMAT, fmt);

	while (count--) {
		int src_x = list->src_left;
		int src_y = list->src_top;
		int dest_x = list->dest_left;
		int dest_y = list->dest_top;
		int width = list->width;
		int height = list->height;

		uint32 cmd = SCRN_TO_SCRN_BLIT | CMD_2D_GO | (ROP_COPY << 24);

		if (src_x <= dest_x) {
			cmd |= X_RIGHT_TO_LEFT;
			src_x += width;
			dest_x += width;
		}

		if (src_y <= dest_y) {
			cmd |= Y_BOTTOM_TO_TOP;
			src_y += height;
			dest_y += height;
		}

		TDFX_WaitForFifo(4);
		OUTREG32(SRC_XY, src_x | (src_y << 16));
		OUTREG32(DST_SIZE, (width + 1) | ((height + 1) << 16));
		OUTREG32(DST_XY, dest_x | (dest_y << 16));
		OUTREG32(CMD_2D, cmd);

		list++;
	}
}
