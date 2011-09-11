/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VALIDATE_DISPLAY_MODE_H
#define _VALIDATE_DISPLAY_MODE_H


#include <Accelerant.h>

#include "edid.h"

struct timing_constraints {
	uint16	resolution;

	uint16	min_before_sync;
	uint16	max_sync_start;
	uint16	min_sync_length;
	uint16	max_sync_length;
	uint16	min_after_sync;
	uint16	max_total;
};

struct display_constraints {
	uint16	min_h_display;
	uint16	max_h_display;
	uint16	min_v_display;
	uint16	max_v_display;

	uint32	min_pixel_clock;
	uint32	max_pixel_clock;

	timing_constraints	horizontal_timing;
	timing_constraints	vertical_timing;
};


#ifdef __cplusplus
extern "C" {
#endif


bool sanitize_display_mode(display_mode& mode,
	const display_constraints& constraints, const edid1_info* edidInfo);
bool is_display_mode_within_bounds(display_mode& mode, const display_mode& low,
	const display_mode& high);


#ifdef __cplusplus
}
#endif


#endif	/* _VALIDATE_DISPLAY_MODE_H */
