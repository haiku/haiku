/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	  Alexander von Gluck, kallisti5@unixzen.com
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "bios.h"
#include "display.h"
#include "utility.h"
#include "pll.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define TRACE_PLL
#ifdef TRACE_PLL
extern "C" void _sPrintf(const char *format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif


// For AtomBIOS PLLSet
union set_pixel_clock {
	SET_PIXEL_CLOCK_PS_ALLOCATION base;
	PIXEL_CLOCK_PARAMETERS v1;
	PIXEL_CLOCK_PARAMETERS_V2 v2;
	PIXEL_CLOCK_PARAMETERS_V3 v3;
	PIXEL_CLOCK_PARAMETERS_V5 v5;
	PIXEL_CLOCK_PARAMETERS_V6 v6;
};


static uint32
pll_compute_post_divider(uint32 targetClock)
{
	radeon_shared_info &info = *gInfo->shared_info;

	// if RADEON_PLL_USE_POST_DIV
	//	return pll->post_div;

	uint32 vco;
	if (info.device_chipset < (RADEON_R700 | 0x70)) {
		if (0) // TODO : RADEON_PLL_IS_LCD
			vco = PLL_MIN_DEFAULT / 10; // pll->lcd_pll_out_min;
		else
			vco = PLL_MIN_DEFAULT / 10; // pll->pll_out_min;
	} else {
		if (0) // TODO : RADEON_PLL_IS_LCD
			vco = PLL_MAX_DEFAULT / 10; // pll->lcd_pll_out_max;
		else
			vco = PLL_MAX_DEFAULT / 10; // pll->pll_out_min;
	}

	uint32 postDivider = vco / targetClock;
	uint32 tmp = vco % targetClock;

	if (info.device_chipset < (RADEON_R700 | 0x70)) {
		if (tmp)
			postDivider++;
	} else {
		if (!tmp)
			postDivider--;
	}

	if (postDivider > POST_DIV_LIMIT)
		postDivider = POST_DIV_LIMIT;
	else if (postDivider < POST_DIV_MIN)
		postDivider = POST_DIV_MIN;

	return postDivider;
}


status_t
pll_compute(pll_info *pll) {

	uint32 targetClock = pll->pixel_clock / 10;
		// to 10 kHz units

	pll->post_div = pll_compute_post_divider(targetClock);
	pll->reference_div = REF_DIV_MIN;
	pll->feedback_div = 0;
	pll->feedback_div_frac = 0;

	uint32 referenceFrequency = PLL_REFERENCE_DEFAULT / 10;

	// if RADEON_PLL_USE_REF_DIV
	//	ref_div = pll->reference_div;

	// if (pll->flags & RADEON_PLL_USE_FRAC_FB_DIV) {
	// 	avivo_get_fb_div(pll, targetClock, postDivider, referenceDivider,
	//  &feedbackDivider, &feedbackDividerFrac);
	// 	feedbackDividerFrac = (100 * feedbackDividerFrac) / pll->reference_freq;
	// 	if (frac_fb_div >= 5) {
	// 		frac_fb_div -= 5;
	// 		frac_fb_div = frac_fb_div / 10;
	// 		frac_fb_div++;
	// 	}
	// 	if (frac_fb_div >= 10) {
	// 		fb_div++;
	// 		frac_fb_div = 0;
	// 	}
	// } else {
		while (pll->reference_div <= REF_DIV_LIMIT) {
			// get feedback divider
			uint32 retroEncabulator = pll->post_div * pll->reference_div;

			retroEncabulator *= targetClock;
			pll->feedback_div = retroEncabulator / referenceFrequency;
			pll->feedback_div_frac
				= retroEncabulator % referenceFrequency;

			if (pll->feedback_div > FB_DIV_LIMIT)
				pll->feedback_div = FB_DIV_LIMIT;
			else if (pll->feedback_div < FB_DIV_MIN)
				pll->feedback_div = FB_DIV_MIN;

			if (pll->feedback_div_frac >= (referenceFrequency / 2))
				pll->feedback_div++;

			pll->feedback_div_frac = 0;
			if (pll->reference_div == 0
				|| pll->post_div == 0
				|| targetClock == 0) {
				TRACE("%s: Caught division by zero\n", __func__);
				return B_ERROR;
			}
			uint32 tmp = (referenceFrequency * pll->feedback_div)
				/ (pll->post_div * pll->reference_div);
			tmp = (tmp * 10000) / targetClock;

			if (tmp > (10000 + MAX_TOLERANCE))
				pll->reference_div++;
			else if (tmp >= (10000 - MAX_TOLERANCE))
				break;
			else
				pll->reference_div++;
		}
	// }

	if (pll->reference_div == 0 || pll->post_div == 0) {
		TRACE("%s: Caught division by zero of post or reference divider\n",
			__func__);
		return B_ERROR;
	}

	uint32 calculatedClock
		= (referenceFrequency * pll->feedback_div)
		+ (referenceFrequency * pll->feedback_div_frac)
		/ (pll->reference_div * pll->post_div);

	calculatedClock *= 10;
		// back to kHz for storage

	TRACE("%s: pixel clock: %" B_PRIu32 " gives:"
		" feedbackDivider = %" B_PRIu32 ".%" B_PRIu32
		"; referenceDivider = %" B_PRIu32 "; postDivider = %" B_PRIu32 "\n",
		__func__, pll->pixel_clock, pll->feedback_div, pll->feedback_div_frac,
		pll->reference_div, pll->post_div);

	if (pll->pixel_clock != calculatedClock) {
		TRACE("%s: pixel clock %" B_PRIu32 " was changed to %" B_PRIu32 "\n",
			__func__, pll->pixel_clock, calculatedClock);
		pll->pixel_clock = calculatedClock;
	}

	return B_OK;
}


union adjust_pixel_clock {
	ADJUST_DISPLAY_PLL_PS_ALLOCATION v1;
	ADJUST_DISPLAY_PLL_PS_ALLOCATION_V3 v3;
};


status_t
pll_adjust(pll_info *pll, uint8 crtcID)
{
	pll->flags |= PLL_PREFER_LOW_REF_DIV;

	// TODO : PLL flags
	radeon_shared_info &info = *gInfo->shared_info;

	uint32 pixelClock = pll->pixel_clock;
		// original as pixel_clock will be adjusted

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint32 encoderID = gConnector[connectorIndex]->encoder.objectID;
	uint32 encoder_mode = display_get_encoder_mode(connectorIndex);

	if (info.device_chipset >= (RADEON_R600 | 0x20)) {
		union adjust_pixel_clock args;

		uint8 tableMajor;
		uint8 tableMinor;

		int index = GetIndexIntoMasterTable(COMMAND, AdjustDisplayPll);

		if (atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor)
			!= B_OK) {
			return B_ERROR;
		}

		memset(&args, 0, sizeof(args));
		switch (tableMajor) {
			case 1:
				switch (tableMinor) {
					case 1:
					case 2:
						args.v1.usPixelClock
							= B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
						args.v1.ucTransmitterID = encoderID;
						args.v1.ucEncodeMode = encoder_mode;
						// TODO : SS and SS % > 0
						if (0) {
							args.v1.ucConfig
								|= ADJUST_DISPLAY_CONFIG_SS_ENABLE;
						}

						atom_execute_table(gAtomContext, index, (uint32*)&args);
						// get returned adjusted clock
						pll->pixel_clock
							= B_LENDIAN_TO_HOST_INT16(args.v1.usPixelClock);
						pll->pixel_clock *= 10;
						break;
					case 3:
						args.v3.sInput.usPixelClock
							= B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
						args.v3.sInput.ucTransmitterID = encoderID;
						args.v3.sInput.ucEncodeMode = encoder_mode;
						args.v3.sInput.ucDispPllConfig = 0;
						// TODO : SS and SS % > 0
						if (0) {
							args.v3.sInput.ucDispPllConfig
								|= DISPPLL_CONFIG_SS_ENABLE;
						}
						// TODO : if ATOM_DEVICE_DFP_SUPPORT
						// TODO : display port DP

						// TODO : is DP?
						args.v3.sInput.ucExtTransmitterID = 0;

						atom_execute_table(gAtomContext, index, (uint32*)&args);
						// get returned adjusted clock
						pll->pixel_clock
							= B_LENDIAN_TO_HOST_INT32(
								args.v3.sOutput.ulDispPllFreq);
						pll->pixel_clock *= 10;

						if (args.v3.sOutput.ucRefDiv) {
							pll->flags |= PLL_USE_FRAC_FB_DIV;
							pll->flags |= PLL_USE_REF_DIV;
							pll->reference_div = args.v3.sOutput.ucRefDiv;
						}
						if (args.v3.sOutput.ucPostDiv) {
							pll->flags |= PLL_USE_FRAC_FB_DIV;
							pll->flags |= PLL_USE_POST_DIV;
							pll->post_div = args.v3.sOutput.ucPostDiv;
						}
						break;
					default:
						TRACE("%s: ERROR: table version %" B_PRIu8 ".%" B_PRIu8
							" unknown\n", __func__, tableMajor, tableMinor);
						return B_ERROR;
				}
				break;
			default:
				TRACE("%s: ERROR: table version %" B_PRIu8 ".%" B_PRIu8
					" unknown\n", __func__, tableMajor, tableMinor);
				return B_ERROR;
		}
	}

	TRACE("%s: was: %" B_PRIu32 ", now: %" B_PRIu32 "\n", __func__,
		pixelClock, pll->pixel_clock);

	return B_OK;
}


status_t
pll_set(uint8 pllID, uint32 pixelClock, uint8 crtcID)
{
	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	pll_info *pll = &gConnector[connectorIndex]->encoder.pll;

	pll->pixel_clock = pixelClock;
	pll->id = pllID;

	pll_adjust(pll, crtcID);
		// get any needed clock adjustments, set reference/post dividers, set flags
	pll_compute(pll);
		// compute dividers, set flags

	int index = GetIndexIntoMasterTable(COMMAND, SetPixelClock);
	union set_pixel_clock args;
	memset(&args, 0, sizeof(args));

	uint8 tableMajor;
	uint8 tableMinor;

	atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor);

	uint32 bpc = 8;
		// TODO : BPC == Digital Depth, EDID 1.4+ on digital displays
		// isn't in Haiku edid common code?

	switch (tableMinor) {
		case 1:
			args.v1.usPixelClock
				= B_HOST_TO_LENDIAN_INT16(pll->pixel_clock / 10);
			args.v1.usRefDiv = B_HOST_TO_LENDIAN_INT16(pll->reference_div);
			args.v1.usFbDiv = B_HOST_TO_LENDIAN_INT16(pll->feedback_div);
			args.v1.ucFracFbDiv = pll->feedback_div_frac;
			args.v1.ucPostDiv = pll->post_div;
			args.v1.ucPpll = pll->id;
			args.v1.ucCRTC = crtcID;
			args.v1.ucRefDivSrc = 1;
			break;
		case 2:
			args.v2.usPixelClock
				= B_HOST_TO_LENDIAN_INT16(pll->pixel_clock / 10);
			args.v2.usRefDiv = B_HOST_TO_LENDIAN_INT16(pll->reference_div);
			args.v2.usFbDiv = B_HOST_TO_LENDIAN_INT16(pll->feedback_div);
			args.v2.ucFracFbDiv = pll->feedback_div_frac;
			args.v2.ucPostDiv = pll->post_div;
			args.v2.ucPpll = pll->id;
			args.v2.ucCRTC = crtcID;
			args.v2.ucRefDivSrc = 1;
			break;
		case 3:
			args.v3.usPixelClock
				= B_HOST_TO_LENDIAN_INT16(pll->pixel_clock / 10);
			args.v3.usRefDiv = B_HOST_TO_LENDIAN_INT16(pll->reference_div);
			args.v3.usFbDiv = B_HOST_TO_LENDIAN_INT16(pll->feedback_div);
			args.v3.ucFracFbDiv = pll->feedback_div_frac;
			args.v3.ucPostDiv = pll->post_div;
			args.v3.ucPpll = pll->id;
			args.v3.ucMiscInfo = (pll->id << 2);
			// if (ss_enabled && (ss->type & ATOM_EXTERNAL_SS_MASK))
			// 	args.v3.ucMiscInfo |= PIXEL_CLOCK_MISC_REF_DIV_SRC;
			args.v3.ucTransmitterId
				= gConnector[connectorIndex]->encoder.objectID;
			args.v3.ucEncoderMode = display_get_encoder_mode(connectorIndex);
			break;
		case 5:
			args.v5.ucCRTC = crtcID;
			args.v5.usPixelClock
				= B_HOST_TO_LENDIAN_INT16(pll->pixel_clock / 10);
			args.v5.ucRefDiv = pll->reference_div;
			args.v5.usFbDiv = B_HOST_TO_LENDIAN_INT16(pll->feedback_div);
			args.v5.ulFbDivDecFrac
				= B_HOST_TO_LENDIAN_INT32(pll->feedback_div_frac * 100000);
			args.v5.ucPostDiv = pll->post_div;
			args.v5.ucMiscInfo = 0; /* HDMI depth, etc. */
			// if (ss_enabled && (ss->type & ATOM_EXTERNAL_SS_MASK))
			//	args.v5.ucMiscInfo |= PIXEL_CLOCK_V5_MISC_REF_DIV_SRC;
			switch (bpc) {
				case 8:
				default:
					args.v5.ucMiscInfo |= PIXEL_CLOCK_V5_MISC_HDMI_24BPP;
					break;
				case 10:
					args.v5.ucMiscInfo |= PIXEL_CLOCK_V5_MISC_HDMI_30BPP;
					break;
			}
			args.v5.ucTransmitterID
				= gConnector[connectorIndex]->encoder.objectID;
			args.v5.ucEncoderMode
				= display_get_encoder_mode(connectorIndex);
			args.v5.ucPpll = pllID;
			break;
		case 6:
			args.v6.ulDispEngClkFreq
				= B_HOST_TO_LENDIAN_INT32(crtcID << 24 | pll->pixel_clock / 10);
			args.v6.ucRefDiv = pll->reference_div;
			args.v6.usFbDiv = B_HOST_TO_LENDIAN_INT16(pll->feedback_div);
			args.v6.ulFbDivDecFrac
				= B_HOST_TO_LENDIAN_INT32(pll->feedback_div_frac * 100000);
			args.v6.ucPostDiv = pll->post_div;
			args.v6.ucMiscInfo = 0; /* HDMI depth, etc. */
			// if (ss_enabled && (ss->type & ATOM_EXTERNAL_SS_MASK))
			//	args.v6.ucMiscInfo |= PIXEL_CLOCK_V6_MISC_REF_DIV_SRC;
			switch (bpc) {
				case 8:
				default:
					args.v6.ucMiscInfo |= PIXEL_CLOCK_V6_MISC_HDMI_24BPP;
					break;
				case 10:
					args.v6.ucMiscInfo |= PIXEL_CLOCK_V6_MISC_HDMI_30BPP;
					break;
				case 12:
					args.v6.ucMiscInfo |= PIXEL_CLOCK_V6_MISC_HDMI_36BPP;
					break;
				case 16:
					args.v6.ucMiscInfo |= PIXEL_CLOCK_V6_MISC_HDMI_48BPP;
					break;
			}
			args.v6.ucTransmitterID
				= gConnector[connectorIndex]->encoder.objectID;
			args.v6.ucEncoderMode = display_get_encoder_mode(connectorIndex);
			args.v6.ucPpll = pllID;
			break;
		default:
			TRACE("%s: ERROR: table version %" B_PRIu8 ".%" B_PRIu8 " TODO\n",
				__func__, tableMajor, tableMinor);
			return B_ERROR;
	}

	TRACE("%s: set adjusted pixel clock %" B_PRIu32 " (was %" B_PRIu32 ")\n",
		__func__, pll->pixel_clock, pixelClock);

	return atom_execute_table(gAtomContext, index, (uint32 *)&args);
}
