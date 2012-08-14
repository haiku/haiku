/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	  Alexander von Gluck, kallisti5@unixzen.com
 */


#include "pll.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "accelerant_protos.h"
#include "accelerant.h"
#include "bios.h"
#include "connector.h"
#include "display.h"
#include "displayport.h"
#include "encoder.h"
#include "utility.h"


#define TRACE_PLL
#ifdef TRACE_PLL
extern "C" void _sPrintf(const char* format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


status_t
pll_limit_probe(pll_info* pll)
{
	uint8 tableMajor;
	uint8 tableMinor;
	uint16 tableOffset;

	int index = GetIndexIntoMasterTable(DATA, FirmwareInfo);
	if (atom_parse_data_header(gAtomContext, index, NULL,
		&tableMajor, &tableMinor, &tableOffset) != B_OK) {
		ERROR("%s: Couldn't parse data header\n", __func__);
		return B_ERROR;
	}

	TRACE("%s: table %" B_PRIu8 ".%" B_PRIu8 "\n", __func__,
		tableMajor, tableMinor);

	union atomFirmwareInfo {
		ATOM_FIRMWARE_INFO info;
		ATOM_FIRMWARE_INFO_V1_2 info_12;
		ATOM_FIRMWARE_INFO_V1_3 info_13;
		ATOM_FIRMWARE_INFO_V1_4 info_14;
		ATOM_FIRMWARE_INFO_V2_1 info_21;
		ATOM_FIRMWARE_INFO_V2_2 info_22;
	};
	union atomFirmwareInfo* firmwareInfo
		= (union atomFirmwareInfo*)(gAtomContext->bios + tableOffset);

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
				firmwareInfo->info_12.ulMinPixelClockPLL_Output) * 10;
	}

	pll->pllOutMax
		= B_LENDIAN_TO_HOST_INT32(
			firmwareInfo->info.ulMaxPixelClockPLL_Output) * 10;

	if (tableMinor >= 4) {
		pll->lcdPllOutMin
			= B_LENDIAN_TO_HOST_INT16(
				firmwareInfo->info_14.usLcdMinPixelClockPLL_Output) * 1000;

		if (pll->lcdPllOutMin == 0)
			pll->lcdPllOutMin = pll->pllOutMin;

		pll->lcdPllOutMax
			= B_LENDIAN_TO_HOST_INT16(
				firmwareInfo->info_14.usLcdMaxPixelClockPLL_Output) * 1000;

		if (pll->lcdPllOutMax == 0)
			pll->lcdPllOutMax = pll->pllOutMax;

	} else {
		pll->lcdPllOutMin = pll->pllOutMin;
		pll->lcdPllOutMax = pll->pllOutMax;
	}

	if (pll->pllOutMin == 0) {
		pll->pllOutMin = 64800 * 10;
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


status_t
pll_ppll_ss_probe(pll_info* pll, uint32 ssID)
{
	uint8 tableMajor;
	uint8 tableMinor;
	uint16 headerOffset;
	uint16 headerSize;

	int index = GetIndexIntoMasterTable(DATA, PPLL_SS_Info);
	if (atom_parse_data_header(gAtomContext, index, &headerSize,
		&tableMajor, &tableMinor, &headerOffset) != B_OK) {
		ERROR("%s: Couldn't parse data header\n", __func__);
		return B_ERROR;
	}

	struct _ATOM_SPREAD_SPECTRUM_INFO *ss_info
		= (struct _ATOM_SPREAD_SPECTRUM_INFO*)((uint16*)gAtomContext->bios
		+ headerOffset);

	int indices = (headerSize - sizeof(ATOM_COMMON_TABLE_HEADER))
		/ sizeof(ATOM_SPREAD_SPECTRUM_ASSIGNMENT);

	int i;
	for (i = 0; i < indices; i++) {
		if (ss_info->asSS_Info[i].ucSS_Id == ssID) {
			pll->ssPercentage = B_LENDIAN_TO_HOST_INT16(
				ss_info->asSS_Info[i].usSpreadSpectrumPercentage);
			pll->ssType = ss_info->asSS_Info[i].ucSpreadSpectrumType;
			pll->ssStep = ss_info->asSS_Info[i].ucSS_Step;
			pll->ssDelay = ss_info->asSS_Info[i].ucSS_Delay;
			pll->ssRange = ss_info->asSS_Info[i].ucSS_Range;
			pll->ssReferenceDiv
				= ss_info->asSS_Info[i].ucRecommendedRef_Div;
			return B_OK;
		}
	}

	pll->ssPercentage = 0;
	pll->ssType = 0;
	pll->ssStep = 0;
	pll->ssDelay = 0;
	pll->ssRange = 0;
	pll->ssReferenceDiv = 0;
	return B_ERROR;
}


status_t
pll_asic_ss_probe(pll_info* pll, uint32 ssID)
{
	uint8 tableMajor;
	uint8 tableMinor;
	uint16 headerOffset;
	uint16 headerSize;

	int index = GetIndexIntoMasterTable(DATA, ASIC_InternalSS_Info);
	if (atom_parse_data_header(gAtomContext, index, &headerSize,
		&tableMajor, &tableMinor, &headerOffset) != B_OK) {
		ERROR("%s: Couldn't parse data header\n", __func__);
		return B_ERROR;
	}

	union asicSSInfo {
		struct _ATOM_ASIC_INTERNAL_SS_INFO info;
		struct _ATOM_ASIC_INTERNAL_SS_INFO_V2 info_2;
		struct _ATOM_ASIC_INTERNAL_SS_INFO_V3 info_3;
	};

	union asicSSInfo *ss_info
		= (union asicSSInfo*)((uint16*)gAtomContext->bios + headerOffset);

	int i;
	int indices;
	switch (tableMajor) {
		case 1:
			indices = (headerSize - sizeof(ATOM_COMMON_TABLE_HEADER))
				/ sizeof(ATOM_ASIC_SS_ASSIGNMENT);

			for (i = 0; i < indices; i++) {
				if (ss_info->info.asSpreadSpectrum[i].ucClockIndication
					!= ssID) {
					continue;
				}
				TRACE("%s: ss match found\n", __func__);
				if (pll->pixelClock > B_LENDIAN_TO_HOST_INT32(
					ss_info->info.asSpreadSpectrum[i].ulTargetClockRange)) {
					TRACE("%s: pixelClock > targetClockRange!\n", __func__);
					continue;
				}

				pll->ssPercentage = B_LENDIAN_TO_HOST_INT16(
					ss_info->info.asSpreadSpectrum[i].usSpreadSpectrumPercentage
					);

				pll->ssType
					= ss_info->info.asSpreadSpectrum[i].ucSpreadSpectrumMode;
				pll->ssRate = B_LENDIAN_TO_HOST_INT16(
					ss_info->info.asSpreadSpectrum[i].usSpreadRateInKhz);
				return B_OK;
			}
			break;
		case 2:
			indices = (headerSize - sizeof(ATOM_COMMON_TABLE_HEADER))
				/ sizeof(ATOM_ASIC_SS_ASSIGNMENT_V2);

			for (i = 0; i < indices; i++) {
				if (ss_info->info_2.asSpreadSpectrum[i].ucClockIndication
					!= ssID) {
					continue;
				}
				TRACE("%s: ss match found\n", __func__);
				if (pll->pixelClock > B_LENDIAN_TO_HOST_INT32(
					ss_info->info_2.asSpreadSpectrum[i].ulTargetClockRange)) {
					TRACE("%s: pixelClock > targetClockRange!\n", __func__);
					continue;
				}

				pll->ssPercentage = B_LENDIAN_TO_HOST_INT16(
					ss_info
						->info_2.asSpreadSpectrum[i].usSpreadSpectrumPercentage
					);

				pll->ssType
					= ss_info->info_2.asSpreadSpectrum[i].ucSpreadSpectrumMode;
				pll->ssRate = B_LENDIAN_TO_HOST_INT16(
					ss_info->info_2.asSpreadSpectrum[i].usSpreadRateIn10Hz);
				return B_OK;
			}
			break;
		case 3:
			indices = (headerSize - sizeof(ATOM_COMMON_TABLE_HEADER))
				/ sizeof(ATOM_ASIC_SS_ASSIGNMENT_V3);

			for (i = 0; i < indices; i++) {
				if (ss_info->info_3.asSpreadSpectrum[i].ucClockIndication
					!= ssID) {
					continue;
				}
				TRACE("%s: ss match found\n", __func__);
				if (pll->pixelClock > B_LENDIAN_TO_HOST_INT32(
					ss_info->info_3.asSpreadSpectrum[i].ulTargetClockRange)) {
					TRACE("%s: pixelClock > targetClockRange!\n", __func__);
					continue;
				}

				pll->ssPercentage = B_LENDIAN_TO_HOST_INT16(
					ss_info
						->info_3.asSpreadSpectrum[i].usSpreadSpectrumPercentage
					);

				pll->ssType
					= ss_info->info_3.asSpreadSpectrum[i].ucSpreadSpectrumMode;
				pll->ssRate = B_LENDIAN_TO_HOST_INT16(
					ss_info->info_3.asSpreadSpectrum[i].usSpreadRateIn10Hz);
				return B_OK;
			}
			break;
		default:
			ERROR("%s: Unknown SS table version!\n", __func__);
			return B_ERROR;
	}

	pll->ssPercentage = 0;
	pll->ssType = 0;
	pll->ssRate = 0;

	ERROR("%s: No potential spread spectrum data found!\n", __func__);
	return B_ERROR;
}


void
pll_compute_post_divider(pll_info* pll)
{
	if ((pll->flags & PLL_USE_POST_DIV) != 0) {
		TRACE("%s: using AtomBIOS post divider\n", __func__);
		return;
	}

	uint32 vco;
	if ((pll->flags & PLL_PREFER_MINM_OVER_MAXP) != 0) {
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

	if ((pll->flags & PLL_PREFER_MINM_OVER_MAXP) != 0) {
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
pll_compute(pll_info* pll)
{
	pll_compute_post_divider(pll);

	uint32 targetClock = pll->pixelClock;

	pll->feedbackDiv = 0;
	pll->feedbackDivFrac = 0;
	uint32 referenceFrequency = pll->referenceFreq;

	if ((pll->flags & PLL_USE_REF_DIV) != 0) {
		TRACE("%s: using AtomBIOS reference divider\n", __func__);
	} else {
		TRACE("%s: using minimum reference divider\n", __func__);
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


void
pll_setup_flags(pll_info* pll, uint8 crtcID)
{
	radeon_shared_info &info = *gInfo->shared_info;
	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint32 encoderFlags = gConnector[connectorIndex]->encoder.flags;

	uint32 dceVersion = (info.dceMajor * 100) + info.dceMinor;

	if (dceVersion >= 302 && pll->pixelClock > 200000)
		pll->flags |= PLL_PREFER_HIGH_FB_DIV;
	else
		pll->flags |= PLL_PREFER_LOW_REF_DIV;

	if (info.chipsetID < RADEON_RV770)
		pll->flags |= PLL_PREFER_MINM_OVER_MAXP;

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

	if ((info.chipsetFlags & CHIP_APU) != 0) {
		// Use fractional feedback on APU's
		pll->flags |= PLL_USE_FRAC_FB_DIV;
	}
}


status_t
pll_adjust(pll_info* pll, display_mode* mode, uint8 crtcID)
{
	radeon_shared_info &info = *gInfo->shared_info;

	uint32 pixelClock = pll->pixelClock;
		// original as pixel_clock will be adjusted

	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	connector_info* connector = gConnector[connectorIndex];

	uint32 encoderID = connector->encoder.objectID;
	uint32 encoderMode = display_get_encoder_mode(connectorIndex);
	uint32 encoderFlags = connector->encoder.flags;

	uint32 externalEncoderID = 0;
	if (connector->encoderExternal.isDPBridge)
		externalEncoderID = connector->encoderExternal.objectID;

	if (info.dceMajor >= 3) {

		uint8 tableMajor;
		uint8 tableMinor;

		int index = GetIndexIntoMasterTable(COMMAND, AdjustDisplayPll);
		if (atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor)
			!= B_OK) {
			ERROR("%s: Couldn't find AtomBIOS PLL adjustment\n", __func__);
			return B_ERROR;
		}

		TRACE("%s: table %" B_PRIu8 ".%" B_PRIu8 "\n", __func__,
			tableMajor, tableMinor);

		// Prepare arguments for AtomBIOS call
		union adjustPixelClock {
			ADJUST_DISPLAY_PLL_PS_ALLOCATION v1;
			ADJUST_DISPLAY_PLL_PS_ALLOCATION_V3 v3;
		};
		union adjustPixelClock args;
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

						// Handle DP adjustments
						if (encoderMode == ATOM_ENCODER_MODE_DP
							|| encoderMode == ATOM_ENCODER_MODE_DP_MST) {
							TRACE("%s: encoderMode is DP\n", __func__);
							args.v3.sInput.ucDispPllConfig
								|= DISPPLL_CONFIG_COHERENT_MODE;
							/* 16200 or 27000 */
							uint32 dpLinkSpeed
								= dp_get_link_rate(connectorIndex, mode);
							args.v3.sInput.usPixelClock
								= B_HOST_TO_LENDIAN_INT16(dpLinkSpeed / 10);
						} else if ((encoderFlags & ATOM_DEVICE_DFP_SUPPORT)
							!= 0) {
							#if 0
							if (encoderMode == ATOM_ENCODER_MODE_HDMI) {
								/* deep color support */
								args.v3.sInput.usPixelClock =
									cpu_to_le16((mode->clock * bpc / 8) / 10);
							}
							#endif
							if (pixelClock > 165000) {
								args.v3.sInput.ucDispPllConfig
									|= DISPPLL_CONFIG_DUAL_LINK;
							}
							if (1) {	// dig coherent mode?
								args.v3.sInput.ucDispPllConfig
									|= DISPPLL_CONFIG_COHERENT_MODE;
							}
						}

						args.v3.sInput.ucExtTransmitterID = externalEncoderID;

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


status_t
pll_set(display_mode* mode, uint8 crtcID)
{
	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	pll_info* pll = &gConnector[connectorIndex]->encoder.pll;

	pll->pixelClock = mode->timing.pixel_clock;

	pll_setup_flags(pll, crtcID);
		// set up any special flags
	pll_adjust(pll, mode, crtcID);
		// get any needed clock adjustments, set reference/post dividers
	pll_compute(pll);
		// compute dividers

	radeon_shared_info &info = *gInfo->shared_info;
	pll->ssType = 0;

	switch (display_get_encoder_mode(connectorIndex)) {
		case ATOM_ENCODER_MODE_DP_MST:
		case ATOM_ENCODER_MODE_DP:
			if (info.dceMajor >= 4)
				pll_asic_ss_probe(pll, ASIC_INTERNAL_SS_ON_DP);
			else {
				// TODO: DP Clock == 1.62Ghz?
				pll_ppll_ss_probe(pll, ATOM_DP_SS_ID1);
			}
			break;
		case ATOM_ENCODER_MODE_LVDS:
			if (info.dceMajor >= 4)
				pll_asic_ss_probe(pll, gInfo->lvdsSpreadSpectrumID);
			else
				pll_ppll_ss_probe(pll, gInfo->lvdsSpreadSpectrumID);
			break;
		case ATOM_ENCODER_MODE_DVI:
			if (info.dceMajor >= 4)
				pll_asic_ss_probe(pll, ASIC_INTERNAL_SS_ON_TMDS);
			break;
		case ATOM_ENCODER_MODE_HDMI:
			if (info.dceMajor >= 4)
				pll_asic_ss_probe(pll, ASIC_INTERNAL_SS_ON_HDMI);
			break;
	}

	display_crtc_ss(pll, ATOM_DISABLE);
		// disable ss

	uint8 tableMajor;
	uint8 tableMinor;

	int index = GetIndexIntoMasterTable(COMMAND, SetPixelClock);
	atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor);

	TRACE("%s: table %" B_PRIu8 ".%" B_PRIu8 "\n", __func__,
		tableMajor, tableMinor);

	uint32 bitsPerColor = 8;
		// TODO: Digital Depth, EDID 1.4+ on digital displays
		// isn't in Haiku edid common code?

	// Prepare arguments for AtomBIOS call
	union setPixelClock {
		SET_PIXEL_CLOCK_PS_ALLOCATION base;
		PIXEL_CLOCK_PARAMETERS v1;
		PIXEL_CLOCK_PARAMETERS_V2 v2;
		PIXEL_CLOCK_PARAMETERS_V3 v3;
		PIXEL_CLOCK_PARAMETERS_V5 v5;
		PIXEL_CLOCK_PARAMETERS_V6 v6;
	};
	union setPixelClock args;
	memset(&args, 0, sizeof(args));

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
			switch (bitsPerColor) {
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
			args.v5.ucPpll = pll->id;
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
			switch (bitsPerColor) {
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
			args.v6.ucPpll = pll->id;
			break;
		default:
			TRACE("%s: ERROR: table version %" B_PRIu8 ".%" B_PRIu8 " TODO\n",
				__func__, tableMajor, tableMinor);
			return B_ERROR;
	}

	TRACE("%s: set adjusted pixel clock %" B_PRIu32 " (was %" B_PRIu32 ")\n",
		__func__, pll->pixelClock, mode->timing.pixel_clock);

	status_t result = atom_execute_table(gAtomContext, index, (uint32*)&args);

	//display_crtc_ss(pll, ATOM_ENABLE);
	// Not yet, lets avoid this.

	return result;
}


status_t
pll_external_set(uint32 clock)
{
	TRACE("%s: set external pll clock to %" B_PRIu32 "\n", __func__, clock);

	if (clock == 0)
		ERROR("%s: Warning: default display clock is 0?\n", __func__);

	// also known as PLL display engineering
	uint8 tableMajor;
	uint8 tableMinor;

	int index = GetIndexIntoMasterTable(COMMAND, SetPixelClock);
	atom_parse_cmd_header(gAtomContext, index, &tableMajor, &tableMinor);

	TRACE("%s: table %" B_PRIu8 ".%" B_PRIu8 "\n", __func__,
		tableMajor, tableMinor);

	union setPixelClock {
		SET_PIXEL_CLOCK_PS_ALLOCATION base;
		PIXEL_CLOCK_PARAMETERS v1;
		PIXEL_CLOCK_PARAMETERS_V2 v2;
		PIXEL_CLOCK_PARAMETERS_V3 v3;
		PIXEL_CLOCK_PARAMETERS_V5 v5;
		PIXEL_CLOCK_PARAMETERS_V6 v6;
	};
	union setPixelClock args;
	memset(&args, 0, sizeof(args));

	radeon_shared_info &info = *gInfo->shared_info;
	uint32 dceVersion = (info.dceMajor * 100) + info.dceMinor;
	switch (tableMajor) {
		case 1:
			switch(tableMinor) {
				case 5:
					// If the default DC PLL clock is specified,
					// SetPixelClock provides the dividers.
					args.v5.ucCRTC = ATOM_CRTC_INVALID;
					args.v5.usPixelClock = B_HOST_TO_LENDIAN_INT16(clock);
					args.v5.ucPpll = ATOM_DCPLL;
					break;
				case 6:
					// If the default DC PLL clock is specified,
					// SetPixelClock provides the dividers.
					args.v6.ulDispEngClkFreq = B_HOST_TO_LENDIAN_INT32(clock);
					if (dceVersion == 601)
						args.v6.ucPpll = ATOM_EXT_PLL1;
					else if (dceVersion >= 600)
						args.v6.ucPpll = ATOM_PPLL0;
					else
						args.v6.ucPpll = ATOM_DCPLL;
					break;
				default:
					ERROR("%s: Unknown table version %" B_PRIu8
						".%" B_PRIu8 "\n", __func__, tableMajor, tableMinor);
			}
			break;
		default:
			ERROR("%s: Unknown table version %" B_PRIu8
						".%" B_PRIu8 "\n", __func__, tableMajor, tableMinor);
	}
	return B_OK;
}


void
pll_external_init()
{
	radeon_shared_info &info = *gInfo->shared_info;

	if (info.dceMajor >= 6) {
		pll_external_set(gInfo->displayClockFrequency);
	} else if (info.dceMajor >= 4) {
		// Create our own pseudo pll
		pll_info pll;
		bool ssPresent = pll_asic_ss_probe(&pll, ASIC_INTERNAL_SS_ON_DCPLL)
			== B_OK ? true : false;
		if (ssPresent)
			display_crtc_ss(&pll, ATOM_DISABLE);
		pll_external_set(gInfo->displayClockFrequency);
		#if 0
		if (ssPresent)
			display_crtc_ss(&pll, ATOM_ENABLE);
		#endif
	}
}


status_t
pll_pick(uint32 connectorIndex)
{
	pll_info* pll = &gConnector[connectorIndex]->encoder.pll;
	radeon_shared_info &info = *gInfo->shared_info;

	bool linkB = gConnector[connectorIndex]->encoder.linkEnumeration
		== GRAPH_OBJECT_ENUM_ID2 ? true : false;

	if (info.dceMajor == 6 && info.dceMinor == 1) {
		// DCE 6.1 APU
		if (gConnector[connectorIndex]->encoder.objectID
			== ENCODER_OBJECT_ID_INTERNAL_UNIPHY && !linkB) {
			pll->id = ATOM_PPLL2;
			return B_OK;
		}
		// TODO: check for used PLL1 and use PLL2?
		pll->id = ATOM_PPLL1;
		return B_OK;
	} else if (info.dceMajor >= 4) {
		if (connector_is_dp(connectorIndex)) {
			if (gInfo->dpExternalClock) {
				pll->id = ATOM_PPLL_INVALID;
				return B_OK;
			} else if (info.dceMajor >= 6) {
				pll->id = ATOM_PPLL1;
				return B_OK;
			} else if (info.dceMajor >= 5) {
				pll->id = ATOM_DCPLL;
				return B_OK;
			}
		}
		pll->id = ATOM_PPLL1;
		return B_OK;
	}

	// TODO: Should return the CRTCID here.
	pll->id = ATOM_PPLL1;
	return B_OK;
}
