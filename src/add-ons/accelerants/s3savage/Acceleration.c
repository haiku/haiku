/*
	Haiku S3 Savage driver adapted from the X.org Savage driver.

	Copyright (C) 1994-2000 The XFree86 Project, Inc.  All Rights Reserved.
	Copyright (c) 2003-2006, X.Org Foundation

	Copyright 2007 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2006-2007
*/


#include "GlobalData.h"
#include "AccelerantPrototypes.h"
#include "savage.h"



void 
FILL_RECTANGLE (engine_token *et, uint32 color, fill_rect_params *pList, uint32 count)
{
	int cmd = BCI_CMD_RECT | BCI_CMD_RECT_XP | BCI_CMD_RECT_YP
			| BCI_CMD_DEST_PBD_NEW | BCI_CMD_SRC_SOLID | BCI_CMD_SEND_COLOR;

	(void)et;		// avoid compiler warning for unused arg

//	TRACE(("FILL_RECTANGLE, color: %X  count: %d\n", color, count));

	BCI_CMD_SET_ROP(cmd, 0xCC);		// use GXcopy for rop

	while (count--) {
		int x = pList->left;
		int y = pList->top;
		int w = pList->right - x + 1;
		int h = pList->bottom - y + 1;

		BCI_GET_PTR;

		si->WaitQueue(7);

		BCI_SEND(cmd);
		BCI_SEND(si->GlobalBD.bd2.LoPart);
		BCI_SEND(si->GlobalBD.bd2.HiPart);

		BCI_SEND(color);
		BCI_SEND(BCI_X_Y(x, y));
		BCI_SEND(BCI_W_H(w, h));

		pList++;
	}
}


void 
FILL_SPAN(engine_token *et, uint32 color, uint16 *pList, uint32 count)
{
	int cmd = BCI_CMD_RECT | BCI_CMD_RECT_XP | BCI_CMD_RECT_YP
			| BCI_CMD_DEST_PBD_NEW | BCI_CMD_SRC_SOLID | BCI_CMD_SEND_COLOR;

	(void)et;		// avoid compiler warning for unused arg

//	TRACE(("FILL_SPAN, count: %d\n", count));

	BCI_CMD_SET_ROP(cmd, 0xCC);		// use GXcopy for rop

	while (count--) {
		int y = *pList++;
		int x = *pList++;
		int w = *pList++ - x + 1;

		BCI_GET_PTR;

		// The MediaPlayer in Zeta 1.21 displays a window which has 2 zero width
		// spans which the Savage chips display as a line completely across the
		// screen;  thus, the following if statement discards any span with zero
		// or negative width.
			
		if (w <= 0)
			continue;

		si->WaitQueue(7);

		BCI_SEND(cmd);
		BCI_SEND(si->GlobalBD.bd2.LoPart);
		BCI_SEND(si->GlobalBD.bd2.HiPart);

		BCI_SEND(color);
		BCI_SEND(BCI_X_Y(x, y));
		BCI_SEND(BCI_W_H(w, 1));
	}
}


void 
INVERT_RECTANGLE(engine_token *et, fill_rect_params *pList, uint32 count)
{
	int cmd = BCI_CMD_RECT | BCI_CMD_RECT_XP | BCI_CMD_RECT_YP
			| BCI_CMD_DEST_PBD_NEW;

	(void)et;		// avoid compiler warning for unused arg

//	TRACE(("INVERT_RECTANGLE, count: %d\n", count));

	BCI_CMD_SET_ROP(cmd, 0x55);		// use GXinvert for rop

	while (count--) {
		int x = pList->left;
		int y = pList->top;
		int w = pList->right - x + 1;
		int h = pList->bottom - y + 1;

		BCI_GET_PTR;

		si->WaitQueue(7);

		BCI_SEND(cmd);
		BCI_SEND(si->GlobalBD.bd2.LoPart);
		BCI_SEND(si->GlobalBD.bd2.HiPart);

		BCI_SEND(BCI_X_Y(x, y));
		BCI_SEND(BCI_W_H(w, h));

		pList++;
	}
}


void 
SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *pList, uint32 count)
{
//	TRACE(("SCREEN_TO_SCREEN_BLIT\n"));

	(void)et;		// avoid compiler warning for unused arg

	while (count--) {
		int cmd;
		int src_x = pList->src_left;
		int src_y = pList->src_top;
		int dest_x = pList->dest_left;
		int dest_y = pList->dest_top;
		int width = pList->width;
		int height = pList->height;

		BCI_GET_PTR;

		cmd = BCI_CMD_RECT | BCI_CMD_DEST_PBD_NEW | BCI_CMD_SRC_SBD_COLOR_NEW;
		BCI_CMD_SET_ROP(cmd, 0xCC);		// use GXcopy for rop

		if (dest_x <= src_x) {
			cmd |= BCI_CMD_RECT_XP;
		} else {
			src_x += width;
			dest_x += width;
		}

		if (dest_y <= src_y) {
			cmd |= BCI_CMD_RECT_YP;
		} else {
			src_y += height;
			dest_y += height;
		}

		si->WaitQueue(9);

		BCI_SEND(cmd);

		BCI_SEND(si->GlobalBD.bd2.LoPart);
		BCI_SEND(si->GlobalBD.bd2.HiPart);

		BCI_SEND(si->GlobalBD.bd2.LoPart);
		BCI_SEND(si->GlobalBD.bd2.HiPart);

		BCI_SEND(BCI_X_Y(src_x, src_y));
		BCI_SEND(BCI_X_Y(dest_x, dest_y));
		BCI_SEND(BCI_W_H(width + 1, height + 1));

		pList ++;
	}
}

