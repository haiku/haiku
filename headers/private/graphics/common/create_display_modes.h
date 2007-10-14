/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CREATE_DISPLAY_MODES_H
#define _CREATE_DISPLAY_MODES_H


#include <edid.h>

#include <Accelerant.h>
#include <GraphicsDefs.h>


typedef bool (*check_display_mode_hook)(display_mode *mode);

#ifdef __cplusplus
extern "C" {
#endif

area_id create_display_modes(const char *name, edid1_info *edid,
	const display_mode *initialModes, uint32 initialModeCount,
	const color_space *spaces, uint32 spacesCount,
	check_display_mode_hook hook, display_mode **_modes, uint32 *_count);

#ifdef __cplusplus
}
#endif

#endif	/* _CREATE_DISPLAY_MODES_H */
