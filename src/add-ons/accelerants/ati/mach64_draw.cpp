/*
	Haiku ATI video driver adapted from the X.org ATI driver.

	Copyright 1992,1993,1994,1995,1996,1997 by Kevin E. Martin, Chapel Hill, North Carolina.
	Copyright 1997 through 2004 by Marc Aurele La France (TSI @ UQV), tsi@xfree86.org

	Copyright 2009 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2009
*/


#include "accelerant.h"
#include "mach64.h"



void
Mach64_EngineReset()
{
	// Reset engine and then enable it.

	uint32 genTestCntl = INREG(GEN_TEST_CNTL) & ~GUI_ENGINE_ENABLE;
	OUTREG(GEN_TEST_CNTL, genTestCntl);
	OUTREG(GEN_TEST_CNTL, genTestCntl | GUI_ENGINE_ENABLE);

	// Ensure engine is not locked up by clearing any FIFO errors.

	OUTREG(BUS_CNTL, INREG(BUS_CNTL) | BUS_HOST_ERR_ACK | BUS_FIFO_ERR_ACK);
}


void
Mach64_EngineInit(const DisplayModeEx& mode)
{
	// Initialize the drawing environment and clear the display.

	Mach64_EngineReset();

	gInfo.WaitForIdle();

	OUTREG(MEM_VGA_WP_SEL, 0x00010000);
	OUTREG(MEM_VGA_RP_SEL, 0x00010000);

	uint32 dpPixWidth = 0;
	uint32 dpChainMask = 0;

	switch (mode.bitsPerPixel) {
	case 8:
		dpPixWidth = HOST_8BPP | SRC_8BPP | DST_8BPP;
		dpChainMask = DP_CHAIN_8BPP;
		break;
	case 15:
		dpPixWidth = HOST_15BPP | SRC_15BPP | DST_15BPP;
		dpChainMask = DP_CHAIN_15BPP;
		break;
	case 16:
		dpPixWidth = HOST_16BPP | SRC_16BPP | DST_16BPP;
		dpChainMask = DP_CHAIN_16BPP;
		break;
	case 32:
		dpPixWidth = HOST_32BPP | SRC_32BPP | DST_32BPP;
		dpChainMask = DP_CHAIN_32BPP;
		break;
	}

	dpPixWidth |= BYTE_ORDER_LSB_TO_MSB;	// set for little-endian byte order

	gInfo.WaitForFifo(3);
	OUTREG(DP_PIX_WIDTH, dpPixWidth);
	OUTREG(DP_CHAIN_MASK, dpChainMask);

	OUTREG(CONTEXT_MASK, 0xffffffff);

	gInfo.WaitForFifo(7);
	OUTREG(DST_OFF_PITCH, (mode.timing.h_display / 8) << 22);
	OUTREG(DST_Y_X, 0);
	OUTREG(DST_HEIGHT, 0);
	OUTREG(DST_BRES_ERR, 0);
	OUTREG(DST_BRES_INC, 0);
	OUTREG(DST_BRES_DEC, 0);
	OUTREG(DST_CNTL, DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM | DST_LAST_PEL);

	gInfo.WaitForFifo(6);
	OUTREG(SRC_OFF_PITCH, (mode.timing.h_display / 8) << 22);
	OUTREG(SRC_Y_X, 0);
	OUTREG(SRC_HEIGHT1_WIDTH1, 0x10001);
	OUTREG(SRC_Y_X_START, 0);
	OUTREG(SRC_HEIGHT2_WIDTH2, 0x10001);
	OUTREG(SRC_CNTL, SRC_LINE_X_LEFT_TO_RIGHT);

	gInfo.WaitForFifo(7);
	OUTREGM(HOST_CNTL, 0, HOST_BYTE_ALIGN);
	OUTREG(PAT_REG0, 0);
	OUTREG(PAT_REG1, 0);
	OUTREG(PAT_CNTL, 0);

	OUTREG(SC_LEFT_RIGHT, ((mode.timing.h_display << 16) | 0 ));
	OUTREG(SC_TOP_BOTTOM, ((mode.timing.v_display << 16) | 0 ));

	gInfo.WaitForFifo(9);
	OUTREG(DP_BKGD_CLR, 0);
	OUTREG(DP_FRGD_CLR, 0xffffffff);
	OUTREG(DP_WRITE_MASK, 0xffffffff);
	OUTREG(DP_MIX, (MIX_SRC << 16) | MIX_DST);
	OUTREG(DP_SRC, FRGD_SRC_FRGD_CLR);

	OUTREG(CLR_CMP_CLR, 0);
	OUTREG(CLR_CMP_MASK, 0xffffffff);
	OUTREG(CLR_CMP_CNTL, 0);

	gInfo.WaitForIdle();
}


void
Mach64_FillRectangle(engine_token *et, uint32 color, fill_rect_params *pList, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	gInfo.WaitForFifo(4);
	OUTREG(DP_SRC, BKGD_SRC_BKGD_CLR | FRGD_SRC_FRGD_CLR | MONO_SRC_ONE);
	OUTREG(DP_FRGD_CLR, color);
	OUTREG(DP_MIX, (MIX_SRC << 16) | MIX_DST);
	OUTREG(DST_CNTL, DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM | DST_LAST_PEL);

	while (count--) {
		int x = pList->left;
		int y = pList->top;
		int w = pList->right - x + 1;
		int h = pList->bottom - y + 1;

		gInfo.WaitForFifo(2);
		OUTREG(DST_Y_X, (x << 16) | y);
		OUTREG(DST_HEIGHT_WIDTH, (w << 16) | h);

		pList++;
	}
}


void
Mach64_FillSpan(engine_token *et, uint32 color, uint16 *pList, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	gInfo.WaitForFifo(4);
	OUTREG(DP_SRC, BKGD_SRC_BKGD_CLR | FRGD_SRC_FRGD_CLR | MONO_SRC_ONE);
	OUTREG(DP_FRGD_CLR, color);
	OUTREG(DP_MIX, (MIX_SRC << 16) | MIX_DST);
	OUTREG(DST_CNTL, DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM | DST_LAST_PEL);

	while (count--) {
		int y = *pList++;
		int x = *pList++;
		int w = *pList++ - x + 1;

		if (w <= 0)
			continue;	// discard span with zero or negative width

		gInfo.WaitForFifo(2);
		OUTREG(DST_Y_X, (x << 16) | y);
		OUTREG(DST_HEIGHT_WIDTH, (w << 16) | 1);
	}
}


void
Mach64_InvertRectangle(engine_token *et, fill_rect_params *pList, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	gInfo.WaitForFifo(3);
	OUTREG(DP_SRC, BKGD_SRC_BKGD_CLR | FRGD_SRC_FRGD_CLR | MONO_SRC_ONE);
	OUTREG(DP_MIX, MIX_NOT_DST << 16);
	OUTREG(DST_CNTL, DST_X_LEFT_TO_RIGHT | DST_Y_TOP_TO_BOTTOM | DST_LAST_PEL);

	while (count--) {
		int x = pList->left;
		int y = pList->top;
		int w = pList->right - x + 1;
		int h = pList->bottom - y + 1;

		gInfo.WaitForFifo(2);
		OUTREG(DST_Y_X, (x << 16) | y);
		OUTREG(DST_HEIGHT_WIDTH, (w << 16) | h);

		pList++;
	}
}


void
Mach64_ScreenToScreenBlit(engine_token *et, blit_params *pList, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	gInfo.WaitForFifo(2);
	OUTREG(DP_SRC, FRGD_SRC_BLIT);
	OUTREG(DP_MIX, MIX_SRC << 16);

	while (count--) {
		int cmd = DST_LAST_PEL;
		int src_x = pList->src_left;
		int src_y = pList->src_top;
		int dest_x = pList->dest_left;
		int dest_y = pList->dest_top;
		int width = pList->width;
		int height = pList->height;

		if (dest_x <= src_x) {
			cmd |= DST_X_LEFT_TO_RIGHT;
		} else {
			src_x += width;
			dest_x += width;
		}

		if (dest_y <= src_y) {
			cmd |= DST_Y_TOP_TO_BOTTOM;
		} else {
			src_y += height;
			dest_y += height;
		}

		gInfo.WaitForFifo(5);
		OUTREG(DST_CNTL, cmd);
		OUTREG(SRC_Y_X, (src_x << 16) | src_y);
		OUTREG(SRC_HEIGHT1_WIDTH1, ((width + 1) << 16) | (height + 1));
		OUTREG(DST_Y_X, (dest_x << 16) | dest_y);
		OUTREG(DST_HEIGHT_WIDTH, ((width + 1) << 16) | (height + 1));

		pList ++;
	}
}
