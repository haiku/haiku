/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 */

#ifndef GENERIC_H
#define GENERIC_H

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
status_t ACQUIRE_ENGINE(uint32 capabilities, uint32 max_wait, sync_token *st, engine_token **et);
status_t RELEASE_ENGINE(engine_token *et, sync_token *st);
void WAIT_ENGINE_IDLE(void);
status_t GET_SYNC_TOKEN(engine_token *et, sync_token *st);
status_t SYNC_TO_TOKEN(sync_token *st);

void SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *list, uint32 count);
void FILL_RECTANGLE(engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
void INVERT_RECTANGLE(engine_token *et, fill_rect_params *list, uint32 count);

void FILL_SPAN(engine_token *et, uint32 color, uint16 *list, uint32 count);

#endif
