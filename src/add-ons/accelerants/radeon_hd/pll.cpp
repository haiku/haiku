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

// Pixel Clock Storage
// kHz			Value			Result
//	Haiku:		104000 khz		104 Mhz
//	Linux:		104000 khz		104 Mhz
//	AtomBIOS:	10400 * 10 khz	104 Mhz
// Ghz
//	Haiku:		162000 * 10 khz	1.62 Ghz
//	Linux:		162000 * 10 khz	1.62 Ghz
//	AtomBIOS:	16200  * 10 Khz	0.162 * 10 Ghz


/* The PLL allows to synthesize a clock signal with a range of frequencies
 * based on a single input reference clock signal. It uses several dividers
 * to create a rational factor multiple of the input frequency.
 *
 * The reference clock signal frequency is pll_info::referenceFreq (in kHz).
 * It is then, one after another...
 *   (1) divided by the (integer) reference divider (pll_info::referenceDiv).
 *   (2) multiplied by the fractional feedback divider, which sits in the
 *       PLL's feedback loop and thus multiplies the frequency. It allows
 *       using a rational number factor of the form "x.y", with
 *       x = pll_info::feedbackDiv and y = pll_info::feedbackDivFrac.
 *   (3) divided by the (integer) post divider (pll_info::postDiv).
 *   Allowed ranges are given in the pll_info min/max values.
 *
 *   The resulting output pixel clock frequency is then:
 *
 *                            feedbackDiv + (feedbackDivFrac/10)
 *   f_out = referenceFreq * ------------------------------------
 *                                  referenceDiv * postDiv
 */


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

	TRACE("%s: referenceFreq: %" B_PRIu32 "; pllOutMin: %" B_PRIu32 "; "
		" pllOutMax: %" B_PRIu32 "; pllInMin: %" B_PRIu32 ";"
		"pllInMax: %" B_PRIu32 "\n", __func__, pll->referenceFreq,
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
		pll->ssEnabled = false;
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
			pll->ssEnabled = true;
			return B_OK;
		}
	}

	pll->ssEnabled = false;
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
		pll->ssEnabled = false;
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
				if (pll->pixelClock / 10 > B_LENDIAN_TO_HOST_INT32(
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
				pll->ssPercentageDiv = 100;
				pll->ssEnabled = true;
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
				if (pll->pixelClock / 10 > B_LENDIAN_TO_HOST_INT32(
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
				pll->ssPercentageDiv = 100;
				pll->ssEnabled = true;
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
				if (pll->pixelClock / 10 > B_LENDIAN_TO_HOST_INT32(
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

				if ((ss_info->info_3.asSpreadSpectrum[i].ucSpreadSpectrumMode
					& SS_MODE_V3_PERCENTAGE_DIV_BY_1000_MASK) != 0)
					pll->ssPercentageDiv = 1000;
				else
					pll->ssPercentageDiv = 100;

				if (ssID == ASIC_INTERNAL_ENGINE_SS
					|| ssID == ASIC_INTERNAL_MEMORY_SS)
					pll->ssRate /= 100;

				pll->ssEnabled = true;
				return B_OK;
			}
			break;
		default:
			ERROR("%s: Unknown SS table version!\n", __func__);
			pll->ssEnabled = false;
			return B_ERROR;
	}

	ERROR("%s: No potential spread spectrum data found!\n", __func__);
	pll->ssEnabled = false;
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
			vco = pll->pllOutMax;
	} else {
		if ((pll->flags & PLL_IS_LCD) != 0)
			vco = pll->lcdPllOutMax;
		else
			vco = pll->pllOutMin;
	}

	TRACE("%s: vco = %" B_PRIu32 "\n", __func__, vco);

	uint32 postDivider = vco / pll->adjustedClock;
	uint32 tmp = vco % pll->adjustedClock;

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


/*! Compute values for the fractional feedback divider to match the desired
 *  pixel clock frequency as closely as possible. Reference and post divider
 *  values are already filled in (if used).
 */
status_t
pll_compute(pll_info* pll)
{
	radeon_shared_info &info = *gInfo->shared_info;

	pll_compute_post_divider(pll);

	const uint32 targetClock = pll->adjustedClock;

	pll->feedbackDiv = 0;
	pll->feedbackDivFrac = 0;

	if ((pll->flags & PLL_USE_REF_DIV) != 0) {
		TRACE("%s: using AtomBIOS reference divider\n", __func__);
	} else {
		TRACE("%s: using minimum reference divider\n", __func__);
		pll->referenceDiv = pll->minRefDiv;
	}

	if ((pll->flags & PLL_USE_FRAC_FB_DIV) != 0) {
		TRACE("%s: using AtomBIOS fractional feedback divider\n", __func__);

		const uint32 numerator = pll->postDiv * pll->referenceDiv
			* targetClock;
		pll->feedbackDiv = numerator / pll->referenceFreq;
		pll->feedbackDivFrac = numerator % pll->referenceFreq;

		if (pll->feedbackDiv > pll->maxFeedbackDiv)
			pll->feedbackDiv = pll->maxFeedbackDiv;
		else if (pll->feedbackDiv < pll->minFeedbackDiv)
			pll->feedbackDiv = pll->minFeedbackDiv;

		// Put first 2 digits after the decimal point into feedbackDivFrac
		pll->feedbackDivFrac
			= (100 * pll->feedbackDivFrac) / pll->referenceFreq;

		// Now round it to one digit
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
			pll->feedbackDiv = retroEncabulator / pll->referenceFreq;
			pll->feedbackDivFrac
				= retroEncabulator % pll->referenceFreq;

			if (pll->feedbackDiv > pll->maxFeedbackDiv)
				pll->feedbackDiv = pll->maxFeedbackDiv;
			else if (pll->feedbackDiv < pll->minFeedbackDiv)
				pll->feedbackDiv = pll->minFeedbackDiv;

			if (pll->feedbackDivFrac >= (pll->referenceFreq / 2))
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
			uint32 tmp = (pll->referenceFreq * pll->feedbackDiv)
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
		= ((pll->referenceFreq * pll->feedbackDiv * 10)
		+ (pll->referenceFreq * pll->feedbackDivFrac))
		/ (pll->referenceDiv * pll->postDiv * 10);

	TRACE("%s: Calculated pixel clock of %" B_PRIu32 " based on:\n", __func__,
		calculatedClock);
	TRACE("%s:   referenceFrequency: %" B_PRIu32 "; "
		"referenceDivider: %" B_PRIu32 "\n", __func__, pll->referenceFreq,
		pll->referenceDiv);
	TRACE("%s:   feedbackDivider: %" B_PRIu32 "; "
		"feedbackDividerFrac: %" B_PRIu32 "\n", __func__, pll->feedbackDiv,
		pll->feedbackDivFrac);
	TRACE("%s:   postDivider: %" B_PRIu32 "\n", __func__, pll->postDiv);

	if (pll->adjustedClock != calculatedClock) {
		TRACE("%s: pixel clock %" B_PRIu32 " was changed to %" B_PRIu32 "\n",
			__func__, pll->adjustedClock, calculatedClock);
		pll->pixelClock = calculatedClock;
	}

	// Calcuate needed SS data on DCE4
	if (info.dceMajor >= 4 && pll->ssEnabled) {
		if (pll->ssPercentageDiv == 0) {
			// Avoid div by 0, shouldn't happen but be mindful of it
			TRACE("%s: ssPercentageDiv is less than 0, aborting SS calcualation",
				__func__);
			pll->ssEnabled = false;
			return B_OK;
		}
		uint32 amount = ((pll->feedbackDiv * 10) + pll->feedbackDivFrac);
		amount *= pll->ssPercentage;
		amount /= pll->ssPercentageDiv * 100;
		pll->ssAmount = (amount / 10) & ATOM_PPLL_SS_AMOUNT_V2_FBDIV_MASK;
		pll->ssAmount |= ((amount - (amount / 10))
			<< ATOM_PPLL_SS_AMOUNT_V2_NFRAC_SHIFT) & ATOM_PPLL_SS_AMOUNT_V2_NFRAC_MASK;

		uint32 centerSpreadMultiplier = 2;
		if ((pll->ssType & ATOM_PPLL_SS_TYPE_V2_CENTRE_SPREAD) != 0)
			centerSpreadMultiplier = 4;
		pll->ssStep = (centerSpreadMultiplier * amount * pll->referenceDiv
			* (pll->ssRate * 2048)) / (125 * 25 * pll->referenceFreq / 100);
	}

	return B_OK;
}


void
pll_setup_flags(pll_info* pll, uint8 crtcID)
{
	radeon_shared_info &info = *gInfo->shared_info;
	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	uint32 connectorFlags = gConnector[connectorIndex]->flags;

	uint32 dceVersion = (info.dceMajor * 100) + info.dceMinor;

	TRACE("%s: CRTC: %" B_PRIu8 ", PLL: %" B_PRIu8 "\n", __func__,
		crtcID, pll->id);

	if (dceVersion >= 302 && pll->pixelClock > 200000)
		pll->flags |= PLL_PREFER_HIGH_FB_DIV;
	else
		pll->flags |= PLL_PREFER_LOW_REF_DIV;

	if (info.chipsetID < RADEON_RV770)
		pll->flags |= PLL_PREFER_MINM_OVER_MAXP;

	if ((connectorFlags & ATOM_DEVICE_LCD_SUPPORT) != 0) {
		pll->flags |= PLL_IS_LCD;

		// use reference divider for spread spectrum
		TRACE("%s: Spread Spectrum is %" B_PRIu32 "%%\n", __func__,
			pll->ssPercentage);
		if (pll->ssPercentage > 0) {
			if (pll->ssReferenceDiv > 0) {
				TRACE("%s: using Spread Spectrum reference divider. "
					"refDiv was: %" B_PRIu32 ", now: %" B_PRIu32 "\n",
					__func__, pll->referenceDiv, pll->ssReferenceDiv);
				pll->flags |= PLL_USE_REF_DIV;
				pll->referenceDiv = pll->ssReferenceDiv;

				// TODO: IS AVIVO+?
				pll->flags |= PLL_USE_FRAC_FB_DIV;
			}
		}
	}

	if ((connectorFlags & ATOM_DEVICE_TV_SUPPORT) != 0)
		pll->flags |= PLL_PREFER_CLOSEST_LOWER;

	if ((info.chipsetFlags & CHIP_APU) != 0) {
		// Use fractional feedback on APU's
		pll->flags |= PLL_USE_FRAC_FB_DIV;
	}
}


/**
 * pll_adjust - Ask AtomBIOS if it wants to make adjustments to our pll
 *
 * Returns B_OK on successful execution.
 */
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
	uint32 connectorFlags = connector->flags;

	uint32 externalEncoderID = 0;
	pll->adjustedClock = pll->pixelClock;
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
						if (pll->ssPercentage > 0) {
							args.v1.ucConfig
								|= ADJUST_DISPLAY_CONFIG_SS_ENABLE;
						}

						atom_execute_table(gAtomContext, index, (uint32*)&args);
						// get returned adjusted clock
						pll->adjustedClock
							= B_LENDIAN_TO_HOST_INT16(args.v1.usPixelClock);
						pll->adjustedClock *= 10;
						break;
					case 3:
						args.v3.sInput.usPixelClock
							= B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
						args.v3.sInput.ucTransmitterID = encoderID;
						args.v3.sInput.ucEncodeMode = encoderMode;
						args.v3.sInput.ucDispPllConfig = 0;
						if (pll->ssPercentage > 0) {
							args.v3.sInput.ucDispPllConfig
								|= DISPPLL_CONFIG_SS_ENABLE;
						}

						// Handle DP adjustments
						if (encoderMode == ATOM_ENCODER_MODE_DP
							|| encoderMode == ATOM_ENCODER_MODE_DP_MST) {
							TRACE("%s: encoderMode is DP\n", __func__);
							args.v3.sInput.ucDispPllConfig
								|= DISPPLL_CONFIG_COHERENT_MODE;
							/* 162000 or 270000 */
							uint32 dpLinkSpeed
								= dp_get_link_rate(connectorIndex, mode);
							/* 16200 or 27000 */
							args.v3.sInput.usPixelClock
								= B_HOST_TO_LENDIAN_INT16(dpLinkSpeed / 10);
						} else if ((connectorFlags & ATOM_DEVICE_DFP_SUPPORT)
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
						pll->adjustedClock = B_LENDIAN_TO_HOST_INT32(
								args.v3.sOutput.ulDispPllFreq);
						pll->adjustedClock *= 10;
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
		pixelClock, pll->adjustedClock);

	return B_OK;
}


/*
 * pll_set - Calculate and set a pll on the crtc provided based on the mode.
 *
 * Returns B_OK on successful execution
 */
status_t
pll_set(display_mode* mode, uint8 crtcID)
{
	uint32 connectorIndex = gDisplay[crtcID]->connectorIndex;
	pll_info* pll = &gConnector[connectorIndex]->encoder.pll;
	uint32 dp_clock = gConnector[connectorIndex]->dpInfo.linkRate;
	pll->ssEnabled = false;

	pll->pixelClock = mode->timing.pixel_clock;

	radeon_shared_info &info = *gInfo->shared_info;

	// Probe for PLL spread spectrum info;
	pll->ssPercentage = 0;
	pll->ssType = 0;
	pll->ssStep = 0;
	pll->ssDelay = 0;
	pll->ssRange = 0;
	pll->ssReferenceDiv = 0;

	switch (display_get_encoder_mode(connectorIndex)) {
		case ATOM_ENCODER_MODE_DP_MST:
		case ATOM_ENCODER_MODE_DP:
			if (info.dceMajor >= 4)
				pll_asic_ss_probe(pll, ASIC_INTERNAL_SS_ON_DP);
			else {
				if (dp_clock == 162000) {
					pll_ppll_ss_probe(pll, ATOM_DP_SS_ID2);
					if (!pll->ssEnabled)
						pll_ppll_ss_probe(pll, ATOM_DP_SS_ID1);
				} else
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

	pll_setup_flags(pll, crtcID);
		// set up any special flags
	pll_adjust(pll, mode, crtcID);
		// get any needed clock adjustments, set reference/post dividers
	pll_compute(pll);
		// compute dividers and spread spectrum

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
			if (pll->ssPercentage > 0
				&& (pll->ssType & ATOM_EXTERNAL_SS_MASK) != 0) {
				args.v3.ucMiscInfo |= PIXEL_CLOCK_MISC_REF_DIV_SRC;
			}
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
			if (pll->ssPercentage > 0
				&& (pll->ssType & ATOM_EXTERNAL_SS_MASK) != 0) {
				args.v5.ucMiscInfo |= PIXEL_CLOCK_V5_MISC_REF_DIV_SRC;
			}
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
			if (pll->ssPercentage > 0
				&& (pll->ssType & ATOM_EXTERNAL_SS_MASK) != 0) {
				args.v6.ucMiscInfo |= PIXEL_CLOCK_V6_MISC_REF_DIV_SRC;
			}
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

	if (pll->ssEnabled)
		display_crtc_ss(pll, ATOM_ENABLE);
	else
		display_crtc_ss(pll, ATOM_DISABLE);

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
					args.v5.usPixelClock = B_HOST_TO_LENDIAN_INT16(clock / 10);
					args.v5.ucPpll = ATOM_DCPLL;
					break;
				case 6:
					// If the default DC PLL clock is specified,
					// SetPixelClock provides the dividers.
					args.v6.ulDispEngClkFreq
						= B_HOST_TO_LENDIAN_INT32(clock / 10);
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


/**
 * pll_external_init - Sets external default pll to sane value
 *
 * Takes the AtomBIOS ulDefaultDispEngineClkFreq and applies it
 * back to the card's external PLL clock via SetPixelClock
 */
void
pll_external_init()
{
	radeon_shared_info &info = *gInfo->shared_info;

	if (info.dceMajor >= 6) {
		pll_external_set(gInfo->displayClockFrequency);
	} else if (info.dceMajor >= 4) {
		// Create our own pseudo pll
		pll_info pll;
		pll.pixelClock = gInfo->displayClockFrequency;

		pll_asic_ss_probe(&pll, ASIC_INTERNAL_SS_ON_DCPLL);
		if (pll.ssEnabled)
			display_crtc_ss(&pll, ATOM_DISABLE);
		pll_external_set(pll.pixelClock);
		if (pll.ssEnabled)
			display_crtc_ss(&pll, ATOM_ENABLE);
	}
}


/**
 * pll_usage_mask - Calculate which PLL's are in use
 *
 * Returns the mask of which PLL's are in use
 */
uint32
pll_usage_mask()
{
	uint32 pllMask = 0;
	for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
		if (gConnector[id]->valid == true) {
			pll_info* pll = &gConnector[id]->encoder.pll;
			if (pll->id != ATOM_PPLL_INVALID)
				pllMask |= (1 << pll->id);
		}
	}
	return pllMask;
}


/**
 * pll_usage_count - Find number of connectors attached to a PLL
 *
 * Returns the count of connectors using specified PLL
 */
uint32
pll_usage_count(uint32 pllID)
{
	uint32 pllCount = 0;
	for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
		if (gConnector[id]->valid == true) {
			pll_info* pll = &gConnector[id]->encoder.pll;
			if (pll->id == pllID)
				pllCount++;
		}
	}

	return pllCount;
}


/**
 * pll_shared_dp - Find any existing PLL's used for DP connectors
 *
 * Returns the PLL shared by other DP connectors
 */
uint32
pll_shared_dp()
{
	for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
		if (gConnector[id]->valid == true) {
			if (connector_is_dp(id)) {
				pll_info* pll = &gConnector[id]->encoder.pll;
				return pll->id;
			}
		}
	}
	return ATOM_PPLL_INVALID;
}


/**
 * pll_next_available - Find the next available PLL
 *
 * Returns the next available PLL
 */
uint32
pll_next_available()
{
	radeon_shared_info &info = *gInfo->shared_info;
	uint32 dceVersion = (info.dceMajor * 100) + info.dceMinor;

	uint32 pllMask = pll_usage_mask();

	if (dceVersion == 802 || dceVersion == 601) {
		if (!(pllMask & (1 << ATOM_PPLL0)))
			return ATOM_PPLL0;
	}

	if (!(pllMask & (1 << ATOM_PPLL1)))
		return ATOM_PPLL1;
	if (dceVersion != 601) {
		if (!(pllMask & (1 << ATOM_PPLL2)))
			return ATOM_PPLL2;
	}
	// TODO: If this starts happening, we likely need to
	// add the sharing of PLL's with identical clock rates
	// (see radeon_atom_pick_pll in drm)
	ERROR("%s: Unable to find a PLL! (0x%" B_PRIX32 ")\n", __func__, pllMask);
	return ATOM_PPLL_INVALID;
}


status_t
pll_pick(uint32 connectorIndex)
{
	pll_info* pll = &gConnector[connectorIndex]->encoder.pll;
	radeon_shared_info &info = *gInfo->shared_info;
	uint32 dceVersion = (info.dceMajor * 100) + info.dceMinor;

	bool linkB = gConnector[connectorIndex]->encoder.linkEnumeration
		== GRAPH_OBJECT_ENUM_ID2 ? true : false;

	pll->id = ATOM_PPLL_INVALID;

	// DCE 6.1 APU, UNIPHYA requires PLL2
	if (gConnector[connectorIndex]->encoder.objectID
		== ENCODER_OBJECT_ID_INTERNAL_UNIPHY && !linkB) {
		pll->id = ATOM_PPLL2;
		return B_OK;
	}

	if (connector_is_dp(connectorIndex)) {
		// If DP external clock, set to invalid except on DCE 6.1
		if (gInfo->dpExternalClock && !(dceVersion == 601)) {
			pll->id = ATOM_PPLL_INVALID;
			return B_OK;
		}

		// DCE 6.1+, we can share DP PLLs. See if any other DP connectors
		// have been assigned a PLL yet.
		if (dceVersion >= 601) {
			pll->id = pll_shared_dp();
			if (pll->id != ATOM_PPLL_INVALID)
				return B_OK;
			// Continue through to pll_next_available
		} else if (dceVersion == 600) {
			pll->id = ATOM_PPLL0;
			return B_OK;
		} else if (info.dceMajor >= 5) {
			pll->id = ATOM_DCPLL;
			return B_OK;
		}
	}

	if (info.dceMajor >= 4) {
		pll->id = pll_next_available();
		return B_OK;
	}

	// TODO: Should return the CRTCID here.
	pll->id = ATOM_PPLL1;
	return B_OK;
}
