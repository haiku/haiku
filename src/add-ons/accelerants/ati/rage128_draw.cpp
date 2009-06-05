/*
	Haiku ATI video driver adapted from the X.org ATI driver.

	Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario,
						 Precision Insight, Inc., Cedar Park, Texas, and
						 VA Linux Systems Inc., Fremont, California.

	Copyright 2009 Haiku, Inc.  All rights reserved.
	Distributed under the terms of the MIT license.

	Authors:
	Gerald Zajac 2009
*/


#include "accelerant.h"
#include "rage128.h"




void
Rage128_EngineFlush()
{
	// Flush all dirty data in the Pixel Cache to memory.

	OUTREGM(R128_PC_NGUI_CTLSTAT, R128_PC_FLUSH_ALL, R128_PC_FLUSH_ALL);

	for (int i = 0; i < R128_TIMEOUT; i++) {
		if ( ! (INREG(R128_PC_NGUI_CTLSTAT) & R128_PC_BUSY))
			break;
	}
}


void
Rage128_EngineReset()
{
	// Reset graphics card to known state.

	Rage128_EngineFlush();

	uint32 clockCntlIndex = INREG(R128_CLOCK_CNTL_INDEX);
	uint32 mclkCntl = GetPLLReg(R128_MCLK_CNTL);

	SetPLLReg(R128_MCLK_CNTL, mclkCntl | R128_FORCE_GCP | R128_FORCE_PIPE3D_CP);

	uint32 genResetCntl = INREG(R128_GEN_RESET_CNTL);

	OUTREG(R128_GEN_RESET_CNTL, genResetCntl | R128_SOFT_RESET_GUI);
	INREG(R128_GEN_RESET_CNTL);
	OUTREG(R128_GEN_RESET_CNTL, genResetCntl & ~R128_SOFT_RESET_GUI);
	INREG(R128_GEN_RESET_CNTL);

	SetPLLReg(R128_MCLK_CNTL, mclkCntl);
	OUTREG(R128_CLOCK_CNTL_INDEX, clockCntlIndex);
	OUTREG(R128_GEN_RESET_CNTL, genResetCntl);
}


void
Rage128_EngineInit(const DisplayModeEx& mode)
{
	// Initialize the acceleration hardware.

	SharedInfo& si = *gInfo.sharedInfo;

	TRACE("Rage128_EngineInit()  bits/pixel: %d\n", mode.bitsPerPixel);

	OUTREG(R128_SCALE_3D_CNTL, 0);
	Rage128_EngineReset();

	uint32 dataType = 0;

	switch (mode.bitsPerPixel) {
	case 8:
		dataType = 2;
		break;
	case 15:
		dataType = 3;
		break;
	case 16:
		dataType = 4;
		break;
	case 32:
		dataType = 6;
		break;
	default:
		TRACE("Unsupported color depth: %d bits/pixel\n", mode.bitsPerPixel);
	}

	gInfo.WaitForFifo(2);
	OUTREG(R128_DEFAULT_OFFSET, gInfo.sharedInfo->frameBufferOffset);
	OUTREG(R128_DEFAULT_PITCH, mode.timing.h_display / 8);

	gInfo.WaitForFifo(4);
	OUTREG(R128_AUX_SC_CNTL, 0);
	OUTREG(R128_DEFAULT_SC_BOTTOM_RIGHT, (R128_DEFAULT_SC_RIGHT_MAX
										| R128_DEFAULT_SC_BOTTOM_MAX));
	OUTREG(R128_SC_TOP_LEFT, 0);
	OUTREG(R128_SC_BOTTOM_RIGHT, (R128_DEFAULT_SC_RIGHT_MAX
								| R128_DEFAULT_SC_BOTTOM_MAX));

	si.r128_dpGuiMasterCntl = ((dataType << R128_GMC_DST_DATATYPE_SHIFT)
								| R128_GMC_CLR_CMP_CNTL_DIS
								| R128_GMC_AUX_CLIP_DIS);
	gInfo.WaitForFifo(1);
	OUTREG(R128_DP_GUI_MASTER_CNTL, (si.r128_dpGuiMasterCntl
									| R128_GMC_BRUSH_SOLID_COLOR
									| R128_GMC_SRC_DATATYPE_COLOR));

	gInfo.WaitForFifo(8);
	OUTREG(R128_DST_BRES_ERR, 0);
	OUTREG(R128_DST_BRES_INC, 0);
	OUTREG(R128_DST_BRES_DEC, 0);
	OUTREG(R128_DP_BRUSH_FRGD_CLR, 0xffffffff);
	OUTREG(R128_DP_BRUSH_BKGD_CLR, 0x00000000);
	OUTREG(R128_DP_SRC_FRGD_CLR, 0xffffffff);
	OUTREG(R128_DP_SRC_BKGD_CLR, 0x00000000);
	OUTREG(R128_DP_WRITE_MASK, 0xffffffff);

	gInfo.WaitForFifo(1);
	OUTREGM(R128_DP_DATATYPE, 0, R128_HOST_BIG_ENDIAN_EN);

	gInfo.WaitForIdle();
}


void
Rage128_FillRectangle(engine_token *et, uint32 color, fill_rect_params *pList, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	gInfo.WaitForFifo(3);
	OUTREG(R128_DP_GUI_MASTER_CNTL, (gInfo.sharedInfo->r128_dpGuiMasterCntl
									| R128_GMC_BRUSH_SOLID_COLOR
									| R128_GMC_SRC_DATATYPE_COLOR
									| R128_ROP3_P));	// use GXcopy for rop

	OUTREG(R128_DP_BRUSH_FRGD_CLR, color);
	OUTREG(R128_DP_CNTL, R128_DST_X_LEFT_TO_RIGHT | R128_DST_Y_TOP_TO_BOTTOM);

	while (count--) {
		int x = pList->left;
		int y = pList->top;
		int w = pList->right - x + 1;
		int h = pList->bottom - y + 1;

		gInfo.WaitForFifo(2);
		OUTREG(R128_DST_Y_X, (y << 16) | x);
		OUTREG(R128_DST_WIDTH_HEIGHT, (w << 16) | h);

		pList++;
	}
}


void
Rage128_FillSpan(engine_token *et, uint32 color, uint16 *pList, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	gInfo.WaitForFifo(2);
	OUTREG(R128_DP_GUI_MASTER_CNTL, (gInfo.sharedInfo->r128_dpGuiMasterCntl
									| R128_GMC_BRUSH_SOLID_COLOR
									| R128_GMC_SRC_DATATYPE_COLOR
									| R128_ROP3_P));	// use GXcopy for rop

	OUTREG(R128_DP_BRUSH_FRGD_CLR, color);

	while (count--) {
		int y = *pList++;
		int x = *pList++;
		int w = *pList++ - x + 1;

		if (w <= 0)
			continue;	// discard span with zero or negative width

		gInfo.WaitForFifo(2);
		OUTREG(R128_DST_Y_X, (y << 16) | x);
		OUTREG(R128_DST_WIDTH_HEIGHT, (w << 16) | 1);
	}
}


void
Rage128_InvertRectangle(engine_token *et, fill_rect_params *pList, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	gInfo.WaitForFifo(2);
	OUTREG(R128_DP_GUI_MASTER_CNTL, (gInfo.sharedInfo->r128_dpGuiMasterCntl
									| R128_GMC_BRUSH_NONE
									| R128_GMC_SRC_DATATYPE_COLOR
									| R128_DP_SRC_SOURCE_MEMORY
									| R128_ROP3_Dn));	// use GXinvert for rop

	OUTREG(R128_DP_CNTL, R128_DST_X_LEFT_TO_RIGHT | R128_DST_Y_TOP_TO_BOTTOM);

	while (count--) {
		int x = pList->left;
		int y = pList->top;
		int w = pList->right - x + 1;
		int h = pList->bottom - y + 1;

		gInfo.WaitForFifo(2);
		OUTREG(R128_DST_Y_X, (y << 16) | x);
		OUTREG(R128_DST_WIDTH_HEIGHT, (w << 16) | h);

		pList++;
	}
}


void
Rage128_ScreenToScreenBlit(engine_token *et, blit_params *pList, uint32 count)
{
	(void)et;		// avoid compiler warning for unused arg

	gInfo.WaitForFifo(1);
	OUTREG(R128_DP_GUI_MASTER_CNTL, (gInfo.sharedInfo->r128_dpGuiMasterCntl
									| R128_GMC_BRUSH_NONE
									| R128_GMC_SRC_DATATYPE_COLOR
									| R128_DP_SRC_SOURCE_MEMORY
									| R128_ROP3_S));	// use GXcopy for rop

	while (count--) {
		int cmd = 0;
		int src_x = pList->src_left;
		int src_y = pList->src_top;
		int dest_x = pList->dest_left;
		int dest_y = pList->dest_top;
		int width = pList->width;
		int height = pList->height;

		if (dest_x <= src_x) {
			cmd |= R128_DST_X_LEFT_TO_RIGHT;
		} else {
			src_x += width;
			dest_x += width;
		}

		if (dest_y <= src_y) {
			cmd |= R128_DST_Y_TOP_TO_BOTTOM;
		} else {
			src_y += height;
			dest_y += height;
		}

		gInfo.WaitForFifo(4);
		OUTREG(R128_DP_CNTL, cmd);
		OUTREG(R128_SRC_Y_X, (src_y << 16) | src_x);
		OUTREG(R128_DST_Y_X, (dest_y << 16) | dest_x);
		OUTREG(R128_DST_HEIGHT_WIDTH, ((height + 1) << 16) | (width + 1));

		pList ++;
	}
}
