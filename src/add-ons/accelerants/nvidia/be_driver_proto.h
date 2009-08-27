/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Modified by Rudolf Cornelissen 8/2009.
*/

#if !defined(GENERIC_H)
#define GENERIC_H

#include <Accelerant.h>
#include <video_overlay.h>

#define DEBUG 1

status_t INIT_ACCELERANT(int fd);
ssize_t ACCELERANT_CLONE_INFO_SIZE(void);
void GET_ACCELERANT_CLONE_INFO(void *data);
status_t CLONE_ACCELERANT(void *data);
void UNINIT_ACCELERANT(void);
status_t GET_ACCELERANT_DEVICE_INFO(accelerant_device_info *adi);
sem_id ACCELERANT_RETRACE_SEMAPHORE(void);

uint32 ACCELERANT_MODE_COUNT(void);
status_t GET_MODE_LIST(display_mode *dm);
status_t PROPOSE_DISPLAY_MODE(display_mode *target, const display_mode *low, const display_mode *high);
status_t SET_DISPLAY_MODE(display_mode *mode_to_set);
status_t GET_DISPLAY_MODE(display_mode *current_mode);
#ifdef __HAIKU__
status_t GET_EDID_INFO(void* info, size_t size, uint32* _version);
status_t GET_PREFERRED_DISPLAY_MODE(display_mode* preferredMode);
#endif
status_t GET_FRAME_BUFFER_CONFIG(frame_buffer_config *a_frame_buffer);
status_t GET_PIXEL_CLOCK_LIMITS(display_mode *dm, uint32 *low, uint32 *high);
status_t MOVE_DISPLAY(uint16 h_display_start, uint16 v_display_start);
status_t GET_TIMING_CONSTRAINTS(display_timing_constraints *dtc);
void SET_INDEXED_COLORS(uint count, uint8 first, uint8 *color_data, uint32 flags);

uint32 DPMS_CAPABILITIES(void);
uint32 DPMS_MODE(void);
status_t SET_DPMS_MODE(uint32 dpms_flags);

status_t SET_CURSOR_SHAPE(uint16 width, uint16 height, uint16 hot_x, uint16 hot_y, uint8 *andMask, uint8 *xorMask);
void MOVE_CURSOR(uint16 x, uint16 y);
void SHOW_CURSOR(bool is_visible);

uint32 ACCELERANT_ENGINE_COUNT(void);
status_t ACQUIRE_ENGINE_PIO(uint32 capabilities, uint32 max_wait, sync_token *st, engine_token **et);
status_t ACQUIRE_ENGINE_DMA(uint32 capabilities, uint32 max_wait, sync_token *st, engine_token **et);
status_t RELEASE_ENGINE(engine_token *et, sync_token *st);
void WAIT_ENGINE_IDLE(void);
status_t GET_SYNC_TOKEN(engine_token *et, sync_token *st);
status_t SYNC_TO_TOKEN(sync_token *st);

/* PIO acceleration */
void SCREEN_TO_SCREEN_BLIT_PIO(engine_token *et, blit_params *list, uint32 count);
void SCREEN_TO_SCREEN_TRANSPARENT_BLIT_PIO(engine_token *et, uint32 transparent_colour, blit_params *list, uint32 count);
void SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT_PIO(engine_token *et, scaled_blit_params *list, uint32 count);
void FILL_RECTANGLE_PIO(engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
void INVERT_RECTANGLE_PIO(engine_token *et, fill_rect_params *list, uint32 count);
void FILL_SPAN_PIO(engine_token *et, uint32 color, uint16 *list, uint32 count);

/* video_overlay */
uint32 OVERLAY_COUNT(const display_mode *dm);
const uint32 *OVERLAY_SUPPORTED_SPACES(const display_mode *dm);
uint32 OVERLAY_SUPPORTED_FEATURES(uint32 a_color_space);
const overlay_buffer *ALLOCATE_OVERLAY_BUFFER(color_space cs, uint16 width, uint16 height);
status_t RELEASE_OVERLAY_BUFFER(const overlay_buffer *ob);
status_t GET_OVERLAY_CONSTRAINTS(const display_mode *dm, const overlay_buffer *ob, overlay_constraints *oc);
overlay_token ALLOCATE_OVERLAY(void);
status_t RELEASE_OVERLAY(overlay_token ot);
status_t CONFIGURE_OVERLAY(overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov);

status_t create_mode_list(void);

#endif
