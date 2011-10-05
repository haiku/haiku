/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_ENCODER_H
#define RADEON_HD_ENCODER_H


void encoder_assign_crtc(uint8 crt_id);
void encoder_mode_set(uint8 id, uint32 pixelClock);
void encoder_analog_setup(uint8 id, uint32 pixelClock, int command);
void encoder_output_lock(bool lock);


#endif /* RADEON_HD_ENCODER_H */
