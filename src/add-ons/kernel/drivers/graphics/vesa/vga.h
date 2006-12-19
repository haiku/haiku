/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VGA_H
#define _VGA_H


#include <vesa_info.h>


status_t vga_set_indexed_colors(uint8 first, uint8 *colors, uint16 count);
status_t vga_planar_blit(vesa_shared_info *info, uint8 *src, int32 srcBPR,
	int32 left, int32 top, int32 right, int32 bottom);

#endif	/* _VGA_H */
