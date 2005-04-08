/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VGA_H
#define VGA_H


/* VGA ports */

// misc stuff
#define VGA_INPUT_STATUS_0		0x3c2
#define VGA_INPUT_STATUS_1		0x3da

// sequencer registers
#define VGA_SEQUENCER_INDEX		0x3c4
#define VGA_SEQUENCER_DATA		0x3c5

// graphics registers
#define VGA_GRAPHICS_INDEX 		0x3ce
#define VGA_GRAPHICS_DATA 		0x3cf

// CRTC registers
#define VGA_CRTC_INDEX			0x3d4
#define VGA_CRTC_DATA			0x3d5

// attribute registers
#define VGA_ATTRIBUTE_READ		0x3c1
#define VGA_ATTRIBUTE_WRITE		0x3c0

// color registers
#define VGA_COLOR_STATE			0x3c7
#define VGA_COLOR_READ_MODE		0x3c7
#define VGA_COLOR_WRITE_MODE	0x3c8
#define VGA_COLOR_DATA			0x3c9
#define VGA_COLOR_MASK			0x3c6

#endif	/* VGA_H */
