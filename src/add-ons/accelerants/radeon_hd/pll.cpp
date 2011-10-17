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

#define ERROR(x...) _sPrintf("radeon_hd: " x)


union firmware_info {
	ATOM_FIRMWARE_INFO info;
	ATOM_FIRMWARE_INFO_V1_2 info_12;
	ATOM_FIRMWARE_INFO_V1_3 info_13;
	ATOM_FIRMWARE_INFO_V1_4 info_14;
	ATOM_FIRMWARE_INFO_V2_1 info_21;
	ATOM_FIRMWARE_INFO_V2_2 info_22;
};


status_t
pll_limit_probe(pll_info *pll)
{
	int index = GetIndexIntoMasterTable(DATA, FirmwareInfo);
	uint8 tableMajor;
	uint8 tableMinor;
	uint16 tableOffset;

	if (atom_parse_data_header(gAtomContext, index, NULL,
		&tableMajor, &tableMinor, &tableOffset) != B_OK) {
		ERROR("%s: Couldn't parse data header\n", __func__);
		return B_ERROR;
	}

	union firmware_info *firmwareInfo
		= (union firmware_info *)(gAtomContext->bios + tableOffset);

	/* pixel clock limits */
	pll->referenceFreq
		= B_LENDIAN_TO_HOST_INT16(firmwareInfo->info.usReferenceClock) * 10;

	if (tableMinor < 2) {
		pll->pllOutMin
			= B_LENDIAN_TO_HOST_INT16(
				firmwareInfo->info.usMinPixelClockPLL_Output) * 10;
	} else {
		pll->pllOutMin
			= B_LENDIAN_TO_HOST_INT32(
				firmwareInfo->info_12.ulMinPixelClockPLL_Output);
	}

	pll->pllOutMax
		= B_LENDIAN_TO_HOST_INT32(
			firmwareInfo->info.ulMaxPixelClockPLL_Output);

	if (tableMinor >= 4) {
		pll->lcdPllOutMin
			= B_LENDIAN_TO_HOST_INT16(
				firmwareInfo->info_14.usLcdMinPixelClockPLL_Output) * 100;

		if (pll->lcdPllOutMin == 0)
			pll->lcdPllOutMin = pll->pllOutMin;

		pll->lcdPllOutMax
			= B_LENDIAN_TO_HOST_INT16(
				firmwareInfo->info_14.usLcdMaxPixelClockPLL_Output) * 100;

		if (pll->lcdPllOutMax == 0)
			pll->lcdPllOutMax = pll->pllOutMax;

	} else {
		pll->lcdPllOutMin = pll->pllOutMin;
		pll->lcdPllOutMax = pll->pllOutMax;
	}

	if (pll->pllOutMin == 0) {
		pll->pllOutMin = 64800;
			// Avivo+ limit
	}

	pll->minPostDiv = POST_DIV_MIN;
	pll->maxPostDiv = POST_DIV_LIMIT;
	pll->minRefDiv = REF_DIV_MIN;
	pll->maxRefDiv = REF_DIV_LIMIT;
	pll->minFeedbackDiv = FB_DIV_MIN;
	pll->maxFeedbackDiv = FB_DIV_LIMIT;

	pll->pllInMin = B_LENDIAN_TO_HOST_INT16(
		firmwareInfo->info.usMinPixelClockPLL_Input) * 10;
	pll->pllInMax = B_LENDIAN_TO_HOST_INT16(
		firmwareInfo->info.usMaxPixelClockPLL_Input) * 10;

	TRACE("%s: referenceFreq: %" B_PRIu16 "; pllOutMin: %" B_PRIu16 "; "
		" pllOutMax: %" B_PRIu16 "; pllInMin: %" B_PRIu16 ";"
		"pllInMax: %" B_PRIu16 "\n", __func__, pll->referenceFreq,
		pll->pllOutMin, pll->pllOutMax, pll->pllInMin, pll->pllInMax);

	return B_OK;
}


void
pll_compute_post_divider(pll_info *pll)
{
	radeon_shared_info &info = *gInfo->shared_info;

	if ((pll->flags & PLL_USE_POST_DIV) != 0) {
		TRACE("%s: using AtomBIOS post divider\n", __func__);
		return;
	}

	uint32 vco;
	if (info.device_chipset < (RADEON_R700 | 0x70)) {
		if ((pll->flags & PLL_IS_LCD) != 0)
			vco = pll->lcdPllOutMin;
		else
			vco = pll->pllOutMin;
	} else {
		if ((pll->flags & PLL_IS_LCD) != 0)
			vco = pll->lcdPllOutMax;
		else
			vco = pll->pllOutMin;
	}

	TRACE("%s: vco = %" B_PRIu32 "\n", __func__, vco);

	uint32 postDivider = vco / pll->pixelClock;
	uint32 tmp = vco % pll->pixelClock;

	if (info.device_chipset < (RADEON_R700 | 0x70)) {
		if (tmp)
			postDivider++;
	} else {
		if (!tmp)
			postDivider--;
	}

	if (postDivider > pll->maxPostDiv)
		postDivider = pll->maxPostDiv;
	else if (postDivider < pll->minPostDiv)
		postDivider = pll->minPostDiv;

	pll->postDiv = postDivider;
	TRACE("%s: postDiv = %" B_PRIu32 "\n", __func__, postDivider);
}


status_t
pll_compute(pll_info *pll)
{
	pll_compute_post_divider(pll);

	uint32 targetClock = pll->pixelClock;

	pll->feedbackDiv = 0;
	pll->feedbackDivFrac = 0;
	uint32 referenceFrequency = pll->referenceFreq;

	if ((pll->flags & PLL_USE_REF_DIV) != 0) {
		TRACE("%s: using AtomBIOS reference divider\n", __func__);
		return B_OK;
	} else {
		pll->referenceDiv = pll->minRefDiv;
	}

	if ((pll->flags & PLL_USE_FRAC_FB_DIV) != 0) {
		TRACE("%s: using AtomBIOS fractional feedback divider\n", __func__);

		uint32 tmp = pll->postDiv * pll->referenceDiv;
		tmp *= targetClock;
		pll->feedbackDiv = tmp / pll->referenceFreq;
		pll->feedbackDivFrac = tmp % pll->referenceFreq;

		if (pll->feedbackDiv > pll->maxFeedbackDiv)
			pll->feedbackDiv = pll->maxFeedbackDiv;
		else if (pll->feedbackDiv < pll->minFeedbackDiv)
			pll->feedbackDiv = pll->minFeedbackDiv;

		pll->feedbackDivFrac
			= (100 * pll->feedbackDivFrac) / pll->referenceFreq;

		if (pll->feedbackDivFrac >= 5) {
			pll->feedbackDivFrac -= 5;
			pll->feedbackDivFrac /= 10;
			pll->feedbackDivFrac++;
		}
		if (pll->feedbackDivFrac >= 10) {
			pll->feedbackDiv++;
			pll->feedbackDivFrac = 0;
		}
	} else {
		TRACE("%s: performing fractional feedback calculations\n", __func__);

		while (pll->referenceDiv <= pll->maxRefDiv) {
			// get feedback divider
			uint32 retroEncabulator = pll->postDiv * pll->referenceDiv;

			retroEncabulator *= targetClock;
			pll->feedbackDiv = retroEncabulator / referenceFrequency;
			pll->feedbackDivFrac
				= retroEncabulator % referenceFrequency;

			if (pll->feedbackDiv > pll->maxFeedbackDiv)
				pll->feedbackDiv = pll->maxFeedbackDiv;
			else if (pll->feedbackDiv < pll->minFeedbackDiv)
				pll->feedbackDiv = pll->minFeedbackDiv;

			if (pll->feedbackDivFrac >= (referenceFrequency / 2))
				pll->feedbackDiv++;

			pll->feedbackDivFrac = 0;

			if (pll->referenceDiv == 0
				|| pll->postDiv == 0
				|| targetClock == 0) {
				TRACE("%s: Caught division by zero!\n", __func__);
				TRACE("%s: referenceDiv %" B_PRIu32 "\n",
					__func__, pll->referenceDiv);
				TRACE("%s: postDiv      %" B_PRIu32 "\n",
					__func__, pll->postDiv);
				TRACE("%s: targetClock  %" B_PRIu32 "\n",
					__func__, targetClock);
				return B_ERROR;
			}
			uint32 tmp = (referenceFrequency * pll->feedbackDiv)
				/ (pll->postDiv * pll->referenceDiv);
			tmp = (tmp * 1000) / targetClock;

			if (tmp > (1000 + (MAX_TOLERANCE / 10)))
				pll->referenceDiv++;
			else if (tmp >= (1000 - (MAX_TOLERANCE / 10)))
				break;
			else
				pll->referenceDiv++;
		}
	}

	if (pll->referenceDiv == 0 || pll->postDiv == 0) {
		TRACE("%s: Caught division by zero of post or reference divider\n",
			__func__);
		return B_ERROR;
	}

	uint32 calculatedClock
		= ((referenceFrequency * pll->feedbackDiv * 10)
		+ (referenceFrequency * pll->feedbackDivFrac))
		/ (pll->referenceDiv * pll->postDiv * 10);

	TRACE("%s: pixel clock: %" B_PRIu32 " gives:"
		" feedbackDivider = %" B_PRIu32 ".%" B_PRIu32
		"; referenceDivider = %" B_PRIu32 "; postDivider = %" B_PRIu32 "\n",
		__func__, pll->pixelClock, pll->feedbackDiv, pll->feedbackDivFrac,
		pll->referenceDiv, pll->postDiv);

	if (pll->pixelClock != calculatedClock) {
		TRACE("%s: pixel clock %" B_PRIu32 " was changed to %" B_PRIu32 "\n",
			__func__, pll->pixelClock, calculatedClock);
		pll->pixelClock = calculatedClock;
	}

	return B_OK;
}


union adjust_pixel_clock {
	ADJUST_DISPLAY_PLL_PS_ALLOCATION v1;
	ADJUST_DISPLAY_PLL_PS_ALLOCATION_V3 v3;
};


void
pll_setup_flags(pll_info *pll, uint8 crtcID)
{
	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint32 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	pll->flags |= PLL_PREFER_LOW_REF_DIV;

	if ((encoderFlags & ATOM_DEVICE_LCD_SUPPORT) != 0) {
		pll->flags |= PLL_IS_LCD;

		// TODO: Spread Spectrum PLL
		// use reference divider for spread spectrum
		if (0) { // SS enabled
			if (0) { // if we have a SS reference divider
				pll->flags |= PLL_USE_REF_DIV;
				//pll->reference_div = ss->refdiv;
				pll->flags |= PLL_USE_FRAC_FB_DIV;
			}
		}
	}

	if ((encoderFlags & ATOM_DEVICE_TV_SUPPORT) != 0)
		pll->flags |= PLL_PREFER_CLOSEST_LOWER;
}


status_t
pll_adjust(pll_info *pll, uint8 crtcID)
{
	// TODO: PLL flags
	radeon_shared_info &info = *gInfo->shared_info;

	uint32 pixelClock = pll->pixelClock;
		// original as pixel_clock will be adjusted

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint32 encoderID = gConnector[connectorIndex]->encoder.objectID;
	uint32 encoderMode = display_get_encoder_mode(connectorIndex);

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
						args.v1.ucEncodeMode = encoderMode;
						// TODO: SS and SS % > 0
						if (0) {
							args.v1.ucConfig
								|= ADJUST_DISPLAY_CONFIG_SS_ENABLE;
						}

						atom_execute_table(gAtomContext, index, (uint32*)&args);
						// get returned adjusted clock
						pll->pixelClock
							= B_LENDIAN_TO_HOST_INT16(args.v1.usPixelClock);
						pll->pixelClock *= 10;
						break;
					case 3:
						args.v3.sInput.usPixelClock
							= B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
						args.v3.sInput.ucTransmitterID = encoderID;
						args.v3.sInput.ucEncodeMode = encoderMode;
						args.v3.sInput.ucDispPllConfig = 0;
						// TODO: SS and SS % > 0
						if (0) {
							args.v3.sInput.ucDispPllConfig
								|= DISPPLL_CONFIG_SS_ENABLE;
						}
						// TODO: if ATOM_DEVICE_DFP_SUPPORT
						// TODO: display port DP

						// TODO: is DP?
						args.v3.sInput.ucExtTransmitterID = 0;

						atom_execute_table(gAtomContext, index, (uint32*)&args);
						// get returned adjusted clock
						pll->pixelClock
							= B_LENDIAN_TO_HOST_INT32(
								args.v3.sOutput.ulDispPllFreq);
						pll->pixelClock *= 10;
							// convert to kHz for storage

						if (args.v3.sOutput.ucRefDiv) {
							pll->flags |= PLL_USE_FRAC_FB_DIV;
							pll->flags |= PLL_USE_REF_DIV;
							pll->referenceDiv = args.v3.sOutput.ucRefDiv;
						}
						if (args.v3.sOutput.ucPostDiv) {
							pll->flags |= PLL_USE_FRAC_FB_DIV;
							pll->flags |= PLL_USE_POST_DIV;
							pll->postDiv = args.v3.sOutput.ucPostDiv;
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
		pixelClock, pll->pixelClock);

	return B_OK;
}


union set_pixel_clock {
	SET_PIXEL_CLOCK_PS_ALLOCATION base;
	PIXEL_CLOCK_PARAMETERS v1;
	PIXEL_CLOCK_PARAMETERS_V2 v2;
	PIXEL_CLOCK_PARAMETERS_V3 v3;
	PIXEL_CLOCK_PARAMETERS_V5 v5;
	PIXEL_CLOCK_PARAMETERS_V6 v6;
};


status_t
pll_set(uint8 pllID, uint32 pixelClock, uint8 crtcID)
{
	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	pll_info *pll = &gConnector[connectorIndex]->encoder.pll;

	pll->pixelClock = pixelClock;
	pll->id = pllID;

	pll_setup_flags(pll, crtcID);
		// set up any special flags
	pll_adjust(pll, crtcID);
		// get any needed clock adjustments, set reference/post dividers
	pll_compute(pll);
		// compute dividers

	int index = GetIndexIntoMasterTable(COMMAND, SetPixelClock);
	union set_pixel_clock args;
	memset(&args, 0, sizeof(args));

	uint8 tableMajor;
	uint8 tableMinor;

	atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor);

	uint32 bitsPerChannel = 8;
		// TODO: Digital Depth, EDID 1.4+ on digital displays
		// isn't in Haiku edid common code?

	switch (tableMinor) {
		case 1:
			args.v1.usPixelClock
				= B_HOST_TO_LENDIAN_INT16(pll->pixelClock / 10);
			args.v1.usRefDiv = B_HOST_TO_LENDIAN_INT16(pll->referenceDiv);
			args.v1.usFbDiv = B_HOST_TO_LENDIAN_INT16(pll->feedbackDiv);
			args.v1.ucFracFbDiv = pll->feedbackDivFrac;
			args.v1.ucPostDiv = pll->postDiv;
			args.v1.ucPpll = pll->id;
			args.v1.ucCRTC = crtcID;
			args.v1.ucRefDivSrc = 1;
			break;
		case 2:
			args.v2.usPixelClock
				= B_HOST_TO_LENDIAN_INT16(pll->pixelClock / 10);
			args.v2.usRefDiv = B_HOST_TO_LENDIAN_INT16(pll->referenceDiv);
			args.v2.usFbDiv = B_HOST_TO_LENDIAN_INT16(pll->feedbackDiv);
			args.v2.ucFracFbDiv = pll->feedbackDivFrac;
			args.v2.ucPostDiv = pll->postDiv;
			args.v2.ucPpll = pll->id;
			args.v2.ucCRTC = crtcID;
			args.v2.ucRefDivSrc = 1;
			break;
		case 3:
			args.v3.usPixelClock
				= B_HOST_TO_LENDIAN_INT16(pll->pixelClock / 10);
			args.v3.usRefDiv = B_HOST_TO_LENDIAN_INT16(pll->referenceDiv);
			args.v3.usFbDiv = B_HOST_TO_LENDIAN_INT16(pll->feedbackDiv);
			args.v3.ucFracFbDiv = pll->feedbackDivFrac;
			args.v3.ucPostDiv = pll->postDiv;
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
				= B_HOST_TO_LENDIAN_INT16(pll->pixelClock / 10);
			args.v5.ucRefDiv = pll->referenceDiv;
			args.v5.usFbDiv = B_HOST_TO_LENDIAN_INT16(pll->feedbackDiv);
			args.v5.ulFbDivDecFrac
				= B_HOST_TO_LENDIAN_INT32(pll->feedbackDivFrac * 100000);
			args.v5.ucPostDiv = pll->postDiv;
			args.v5.ucMiscInfo = 0; /* HDMI depth, etc. */
			// if (ss_enabled && (ss->type & ATOM_EXTERNAL_SS_MASK))
			//	args.v5.ucMiscInfo |= PIXEL_CLOCK_V5_MISC_REF_DIV_SRC;
			switch (bitsPerChannel) {
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
				= B_HOST_TO_LENDIAN_INT32(crtcID << 24 | pll->pixelClock / 10);
			args.v6.ucRefDiv = pll->referenceDiv;
			args.v6.usFbDiv = B_HOST_TO_LENDIAN_INT16(pll->feedbackDiv);
			args.v6.ulFbDivDecFrac
				= B_HOST_TO_LENDIAN_INT32(pll->feedbackDivFrac * 100000);
			args.v6.ucPostDiv = pll->postDiv;
			args.v6.ucMiscInfo = 0; /* HDMI depth, etc. */
			// if (ss_enabled && (ss->type & ATOM_EXTERNAL_SS_MASK))
			//	args.v6.ucMiscInfo |= PIXEL_CLOCK_V6_MISC_REF_DIV_SRC;
			switch (bitsPerChannel) {
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
		__func__, pll->pixelClock, pixelClock);

	return atom_execute_table(gAtomContext, index, (uint32*)&args);
}
