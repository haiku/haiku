/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_PLL_H
#define RADEON_HD_PLL_H


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


status_t pll_compute(uint32 pixelClock, uint32 *dotclockOut,
	uint32 *referenceOut, uint32 *feedbackOut, uint32 *feedbackFracOut,
	uint32 *postOut);
status_t pll_set(uint8 pll_id, uint32 pixelClock, uint8 crtc_id);


#endif /* RADEON_HD_PLL_H */
