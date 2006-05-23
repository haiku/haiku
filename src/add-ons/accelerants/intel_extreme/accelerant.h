/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef INTEL_EXTREME_ACCELERANT_H
#define INTEL_EXTREME_ACCELERANT_H


#include "intel_extreme.h"

#include <video_overlay.h>


struct overlay {
	overlay_buffer	buffer;
	uint32			buffer_handle;
	uint32			buffer_offset;
};

struct overlay_frame {
	int16			h_start;
	int16			v_start;
	uint16			width;
	uint16			height;
};

struct accelerant_info {
	vuint8			*regs;
	area_id			regs_area;

	intel_shared_info *shared_info;
	area_id			shared_info_area;

	display_mode	*mode_list;		// cloned list of standard display modes
	area_id			mode_list_area;

	uint32			frame_buffer_handle;

	struct overlay_registers *overlay_registers;
	overlay			*current_overlay;
	overlay_view	last_overlay_view;
	overlay_frame	last_overlay_frame;
	uint32			last_horizontal_overlay_scale;
	uint32			last_vertical_overlay_scale;
	uint32			overlay_position_buffer_offset;

	hardware_status	*status;
	uint8			*cursor_memory;

	int				device;
	uint8			head_mode;
	bool			is_clone;
};

#define HEAD_MODE_A_ANALOG	0x01
#define HEAD_MODE_B_DIGITAL	0x02
#define HEAD_MODE_CLONE		0x03

extern accelerant_info *gInfo;

// register access

inline uint32
read32(uint32 offset)
{
	return *(volatile uint32 *)(gInfo->regs + offset);
}

inline void
write32(uint32 offset, uint32 value)
{
	*(volatile uint32 *)(gInfo->regs + offset) = value;
}


// dpms.cpp
extern void enable_display_plane(bool enable);
extern void set_display_power_mode(uint32 mode);

// engine.cpp
extern void uninit_ring_buffer(ring_buffer &ringBuffer);
extern void setup_ring_buffer(ring_buffer &ringBuffer, const char *name);

// modes.cpp
extern void wait_for_vblank(void);
extern status_t create_mode_list(void);

// memory.cpp
extern void intel_free_memory(uint32 handle);
extern status_t intel_allocate_memory(size_t size, uint32& handle, uint32& offset);

#endif	/* INTEL_EXTREME_ACCELERANT_H */
