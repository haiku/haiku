/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef ACCELERANT_PROTOS_H
#define ACCELERANT_PROTOS_H


#include <Accelerant.h>
#include "video_overlay.h"


#ifdef __cplusplus
extern "C" {
#endif

void spin(bigtime_t delay);

// general
status_t radeon_init_accelerant(int fd);
void radeon_uninit_accelerant(void);

// modes & constraints
uint32 radeon_accelerant_mode_count(void);
status_t radeon_get_mode_list(display_mode *dm);
status_t radeon_set_display_mode(display_mode *mode);
status_t radeon_get_display_mode(display_mode *currentMode);
status_t radeon_get_frame_buffer_config(frame_buffer_config *config);
status_t radeon_get_pixel_clock_limits(display_mode *mode, uint32 *low, uint32 *high);

// accelerant engine
status_t radeon_acquire_engine(uint32 capabilities, uint32 maxWait,
			sync_token *syncToken, engine_token **_engineToken);
status_t radeon_release_engine(engine_token *engineToken, sync_token *syncToken);


#ifdef __cplusplus
}
#endif

#endif	/* ACCELERANT_PROTOS_H */
