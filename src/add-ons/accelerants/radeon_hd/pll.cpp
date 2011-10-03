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
			vco = PLL_MIN_DEFAULT; // pll->lcd_pll_out_min;
		else
			vco = PLL_MIN_DEFAULT; // pll->pll_out_min;
	} else {
		if (0) // TODO : RADEON_PLL_IS_LCD
			vco = PLL_MAX_DEFAULT; // pll->lcd_pll_out_max;
		else
			vco = PLL_MAX_DEFAULT; // pll->pll_out_min;
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
pll_compute(uint32 pixelClock, uint32 *dotclockOut, uint32 *referenceOut,
	uint32 *feedbackOut, uint32 *feedbackFracOut, uint32 *postOut)
{
	uint32 targetClock = pixelClock / 10;
	uint32 postDivider = pll_compute_post_divider(targetClock);
	uint32 referenceDivider = REF_DIV_MIN;
	uint32 feedbackDivider = 0;
	uint32 feedbackDividerFrac = 0;

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
		while (referenceDivider <= REF_DIV_LIMIT) {
			// get feedback divider
			uint32 retroEncabulator = postDivider * referenceDivider;

			retroEncabulator *= targetClock;
			feedbackDivider = retroEncabulator / PLL_REFERENCE_DEFAULT;
			feedbackDividerFrac = retroEncabulator % PLL_REFERENCE_DEFAULT;

			if (feedbackDivider > FB_DIV_LIMIT)
				feedbackDivider = FB_DIV_LIMIT;
			else if (feedbackDivider < FB_DIV_MIN)
				feedbackDivider = FB_DIV_MIN;

			if (feedbackDividerFrac >= (PLL_REFERENCE_DEFAULT / 2))
				feedbackDivider++;

			feedbackDividerFrac = 0;
			if (referenceDivider == 0 || postDivider == 0 || targetClock == 0) {
				TRACE("%s: Caught division by zero\n",
					__func__);
				return B_ERROR;
			}
			uint32 tmp = (PLL_REFERENCE_DEFAULT * feedbackDivider)
				/ (postDivider * referenceDivider);
			tmp = (tmp * 10000) / targetClock;

			if (tmp > (10000 + MAX_TOLERANCE))
				referenceDivider++;
			else if (tmp >= (10000 - MAX_TOLERANCE))
				break;
			else
				referenceDivider++;
		}
	// }

	if (referenceDivider == 0 || postDivider == 0) {
		TRACE("%s: Caught division by zero of post or reference divider\n",
			__func__);
		return B_ERROR;
	}

	*dotclockOut = ((PLL_REFERENCE_DEFAULT * feedbackDivider * 10)
		+ (PLL_REFERENCE_DEFAULT * feedbackDividerFrac))
		/ (referenceDivider * postDivider * 10);

	*feedbackOut = feedbackDivider;
	*feedbackFracOut = feedbackDividerFrac;
	*referenceOut = referenceDivider;
	*postOut = postDivider;

	TRACE("%s: pixel clock: %" B_PRIu32 " gives:"
		" feedbackDivider = %" B_PRIu32 ".%" B_PRIu32
		"; referenceDivider = %" B_PRIu32 "; postDivider = %" B_PRIu32
		"; dotClock = %" B_PRIu32 "\n", __func__, pixelClock, feedbackDivider,
		feedbackDividerFrac, referenceDivider, postDivider, *dotclockOut);

	return B_OK;
}


status_t
pll_set(uint8 pll_id, uint32 pixelClock, uint8 crtc_id)
{
	uint32 dotclock = 0;
	uint32 reference = 0;
	uint32 feedback = 0;
	uint32 feedbackFrac = 0;
	uint32 post = 0;

	pll_compute(pixelClock, &dotclock, &reference, &feedback,
		&feedbackFrac, &post);

	int index = GetIndexIntoMasterTable(COMMAND, SetPixelClock);
	union set_pixel_clock args;
	memset(&args, 0, sizeof(args));

	uint8 frev;
	uint8 crev;
	atom_parse_cmd_header(gAtomContext, index, &frev, &crev);

	uint32 connector_index = gDisplay[crtc_id]->connector_index;

	switch (crev) {
		case 1:
			args.v1.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
			args.v1.usRefDiv = B_HOST_TO_LENDIAN_INT16(reference);
			args.v1.usFbDiv = B_HOST_TO_LENDIAN_INT16(feedback);
			args.v1.ucFracFbDiv = feedbackFrac;
			args.v1.ucPostDiv = post;
			args.v1.ucPpll = pll_id;
			args.v1.ucCRTC = crtc_id;
			args.v1.ucRefDivSrc = 1;
			break;
		case 2:
			args.v2.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
			args.v2.usRefDiv = B_HOST_TO_LENDIAN_INT16(reference);
			args.v2.usFbDiv = B_HOST_TO_LENDIAN_INT16(feedback);
			args.v2.ucFracFbDiv = feedbackFrac;
			args.v2.ucPostDiv = post;
			args.v2.ucPpll = pll_id;
			args.v2.ucCRTC = crtc_id;
			args.v2.ucRefDivSrc = 1;
			break;
		case 3:
			args.v3.usPixelClock = B_HOST_TO_LENDIAN_INT16(pixelClock / 10);
			args.v3.usRefDiv = B_HOST_TO_LENDIAN_INT16(reference);
			args.v3.usFbDiv = B_HOST_TO_LENDIAN_INT16(feedback);
			args.v3.ucFracFbDiv = feedbackFrac;
			args.v3.ucPostDiv = post;
			args.v3.ucPpll = pll_id;
			args.v3.ucMiscInfo = (pll_id << 2);
			// if (ss_enabled && (ss->type & ATOM_EXTERNAL_SS_MASK))
			// 	args.v3.ucMiscInfo |= PIXEL_CLOCK_MISC_REF_DIV_SRC;
			args.v3.ucTransmitterId
				= gConnector[connector_index]->encoder_object_id;
			args.v3.ucEncoderMode = display_get_encoder_mode(connector_index);
			break;
		default:
			TRACE("%s: ERROR: table version %d.%d TODO\n", __func__,
				frev, crev);
			return B_ERROR;
	}

	TRACE("%s: setting pixel clock %" B_PRIu32 "\n", __func__, pixelClock);

	atom_execute_table(gAtomContext, index, (uint32 *)&args);

	return B_OK;
}
