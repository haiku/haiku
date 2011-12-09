/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_PLL_H
#define RADEON_HD_PLL_H


#include <SupportDefs.h>


#define MAX_TOLERANCE 10

#define PLL_MIN_DEFAULT 16000
#define PLL_MAX_DEFAULT 400000
#define PLL_REFERENCE_DEFAULT 27000

/* limited by the number of bits available */
#define FB_DIV_MIN 4
#define FB_DIV_LIMIT 2048
#define REF_DIV_MIN 2
#define REF_DIV_LIMIT 1024
#define POST_DIV_MIN 2
#define POST_DIV_LIMIT 127

/* pll flags */
#define PLL_USE_BIOS_DIVS        (1 << 0)
#define PLL_NO_ODD_POST_DIV      (1 << 1)
#define PLL_USE_REF_DIV          (1 << 2)
#define PLL_LEGACY               (1 << 3)
#define PLL_PREFER_LOW_REF_DIV   (1 << 4)
#define PLL_PREFER_HIGH_REF_DIV  (1 << 5)
#define PLL_PREFER_LOW_FB_DIV    (1 << 6)
#define PLL_PREFER_HIGH_FB_DIV   (1 << 7)
#define PLL_PREFER_LOW_POST_DIV  (1 << 8)
#define PLL_PREFER_HIGH_POST_DIV (1 << 9)
#define PLL_USE_FRAC_FB_DIV      (1 << 10)
#define PLL_PREFER_CLOSEST_LOWER (1 << 11)
#define PLL_USE_POST_DIV         (1 << 12)
#define PLL_IS_LCD               (1 << 13)
#define PLL_PREFER_MINM_OVER_MAXP (1 << 14)


struct pll_info {
	/* pixel clock to be programmed (kHz)*/
	uint32 pixelClock;

	/* flags for the current clock */
	uint32 flags;

	/* pll id */
	uint32 id;

	/* reference frequency */
	uint32 referenceFreq;

	/* fixed dividers */
	uint32 postDiv;
	uint32 referenceDiv;
	uint32 feedbackDiv;
	uint32 feedbackDivFrac;

	/* pll in/out limits */
	uint32 pllInMin;
	uint32 pllInMax;
	uint32 pllOutMin;
	uint32 pllOutMax;
	uint32 lcdPllOutMin;
	uint32 lcdPllOutMax;
	uint32 bestVco;

	/* divider limits */
	uint32 minRefDiv;
	uint32 maxRefDiv;
	uint32 minPostDiv;
	uint32 maxPostDiv;
	uint32 minFeedbackDiv;
	uint32 maxFeedbackDiv;
	uint32 minFeedbackDivFrac;
	uint32 maxFeedbackDivFrac;
};


status_t pll_adjust(pll_info *pll, uint8 crtcID);
status_t pll_compute(pll_info *pll);
void pll_setup_flags(pll_info *pll, uint8 crtcID);
status_t pll_limit_probe(pll_info *pll);
status_t pll_set(uint8 pllID, uint32 pixelClock, uint8 crtcID);


#endif /* RADEON_HD_PLL_H */
