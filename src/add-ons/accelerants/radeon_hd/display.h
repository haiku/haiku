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


#endif /* RADEON_HD_DISPLAY_H */
