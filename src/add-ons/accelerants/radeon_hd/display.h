/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_DISPLAY_H
#define RADEON_HD_DISPLAY_H


status_t init_registers(register_info* reg, uint8 crtid);
status_t detect_crt_ranges(uint32 crtid);
status_t detect_displays();
void debug_displays();

void display_crtc_lock(uint8 crtc_id, int command);
void display_crtc_blank(uint8 crtc_id, int command);
void display_crtc_scale(uint8 crtc_id, display_mode *mode);
void display_crtc_fb_set_legacy(uint8 crtc_id, display_mode *mode);
void display_crtc_fb_set_dce1(uint8 crtc_id, display_mode *mode);
void display_crtc_set(uint8 crtc_id, display_mode *mode);
void display_crtc_set_dtd(uint8 crtc_id, display_mode *mode);
void display_crtc_power(uint8 crt_id, int command);


#endif /* RADEON_HD_DISPLAY_H */
