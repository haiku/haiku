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

	int				device;
	bool			is_clone;
};

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
extern void setup_ring_buffer(ring_buffer &ringBuffer, const char *name);
extern void write_to_ring(ring_buffer &ring, uint32 value);
extern void ring_command_complete(ring_buffer &ring);

// modes.cpp
extern status_t create_mode_list(void);

// memory.cpp
extern void intel_free_memory(uint32 handle);
extern status_t intel_allocate_memory(size_t size, uint32& handle, uint32& offset);

#endif	/* INTEL_EXTREME_ACCELERANT_H */
