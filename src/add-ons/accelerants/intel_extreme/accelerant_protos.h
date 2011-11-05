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
status_t intel_init_accelerant(int fd);
ssize_t intel_accelerant_clone_info_size(void);
void intel_get_accelerant_clone_info(void* data);
status_t intel_clone_accelerant(void* data);
void intel_uninit_accelerant(void);
status_t intel_get_accelerant_device_info(accelerant_device_info* info);
sem_id intel_accelerant_retrace_semaphore(void);

// modes & constraints
uint32 intel_accelerant_mode_count(void);
status_t intel_get_mode_list(display_mode* dm);
status_t intel_propose_display_mode(display_mode* target,
	const display_mode* low, const display_mode* high);
status_t intel_set_display_mode(display_mode* mode);
status_t intel_get_display_mode(display_mode* currentMode);
status_t intel_get_edid_info(void* info, size_t size, uint32* _version);
status_t intel_get_frame_buffer_config(frame_buffer_config* config);
status_t intel_get_pixel_clock_limits(display_mode* mode, uint32* low,
	uint32* high);
status_t intel_move_display(uint16 hDisplayStart, uint16 vDisplayStart);
status_t intel_get_timing_constraints(display_timing_constraints* constraints);
void intel_set_indexed_colors(uint count, uint8 first, uint8* colorData,
	uint32 flags);

// DPMS
uint32 intel_dpms_capabilities(void);
uint32 intel_dpms_mode(void);
status_t intel_set_dpms_mode(uint32 flags);

// cursor
status_t intel_set_cursor_shape(uint16 width, uint16 height, uint16 hotX,
	uint16 hotY, uint8* andMask, uint8* xorMask);
void intel_move_cursor(uint16 x, uint16 y);
void intel_show_cursor(bool isVisible);

// accelerant engine
uint32 intel_accelerant_engine_count(void);
status_t intel_acquire_engine(uint32 capabilities, uint32 maxWait,
			sync_token* syncToken, engine_token** _engineToken);
status_t intel_release_engine(engine_token* engineToken, sync_token* syncToken);
void intel_wait_engine_idle(void);
status_t intel_get_sync_token(engine_token* engineToken, sync_token* syncToken);
status_t intel_sync_to_token(sync_token* syncToken);

// 2D acceleration
void intel_screen_to_screen_blit(engine_token* engineToken,
	blit_params* list, uint32 count);
void intel_fill_rectangle(engine_token* engineToken, uint32 color,
	fill_rect_params* list, uint32 count);
void intel_invert_rectangle(engine_token* engineToken, fill_rect_params* list,
	uint32 count);
void intel_fill_span(engine_token* engineToken, uint32 color, uint16* list,
	uint32 count);

// overlay
uint32 intel_overlay_count(const display_mode* mode);
const uint32* intel_overlay_supported_spaces(const display_mode* mode);
uint32 intel_overlay_supported_features(uint32 colorSpace);
const overlay_buffer* intel_allocate_overlay_buffer(color_space space,
	uint16 width, uint16 height);
status_t intel_release_overlay_buffer(const overlay_buffer* buffer);
status_t intel_get_overlay_constraints(const display_mode* mode,
	const overlay_buffer* buffer, overlay_constraints* constraints);
overlay_token intel_allocate_overlay(void);
status_t intel_release_overlay(overlay_token overlayToken);
status_t intel_configure_overlay(overlay_token overlayToken,
	const overlay_buffer* buffer, const overlay_window* window,
	const overlay_view* view);
status_t i965_configure_overlay(overlay_token overlayToken,
	const overlay_buffer* buffer, const overlay_window* window,
	const overlay_view* view);

#ifdef __cplusplus
}
#endif

#endif	/* ACCELERANT_PROTOS_H */
