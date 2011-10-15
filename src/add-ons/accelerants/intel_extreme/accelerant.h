/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef INTEL_EXTREME_ACCELERANT_H
#define INTEL_EXTREME_ACCELERANT_H


#include "intel_extreme.h"

#include <edid.h>
#include <video_overlay.h>


struct overlay {
	overlay_buffer	buffer;
	addr_t			buffer_base;
	uint32			buffer_offset;
	addr_t			state_base;
	uint32			state_offset;
};

struct overlay_frame {
	int16			h_start;
	int16			v_start;
	uint16			width;
	uint16			height;
};

struct accelerant_info {
	uint8			*registers;
	area_id			regs_area;

	intel_shared_info *shared_info;
	area_id			shared_info_area;

	display_mode	*mode_list;		// cloned list of standard display modes
	area_id			mode_list_area;

	struct overlay_registers *overlay_registers;
	overlay			*current_overlay;
	overlay_view	last_overlay_view;
	overlay_frame	last_overlay_frame;
	uint32			last_horizontal_overlay_scale;
	uint32			last_vertical_overlay_scale;
	uint32			overlay_position_buffer_offset;

	edid1_info		edid_info;
	bool			has_edid;

	// limited 3D support for overlay on i965
	addr_t			context_base;
	uint32			context_offset;
	bool			context_set;

	int				device;
	uint8			head_mode;
	bool			is_clone;

	// LVDS panel mode passed from the bios/startup.
	display_mode	lvds_panel_mode;
};

#define HEAD_MODE_A_ANALOG		0x01
#define HEAD_MODE_B_DIGITAL		0x02
#define HEAD_MODE_CLONE			0x03
#define HEAD_MODE_LVDS_PANEL	0x08

extern accelerant_info *gInfo;

// register access

inline uint32
read32(uint32 encodedRegister)
{
	return *(volatile uint32 *)(gInfo->registers
		+ gInfo->shared_info->register_blocks[REGISTER_BLOCK(encodedRegister)]
		+ REGISTER_REGISTER(encodedRegister));
}

inline void
write32(uint32 encodedRegister, uint32 value)
{
	*(volatile uint32 *)(gInfo->registers
		+ gInfo->shared_info->register_blocks[REGISTER_BLOCK(encodedRegister)]
		+ REGISTER_REGISTER(encodedRegister)) = value;
}


// dpms.cpp
extern void enable_display_plane(bool enable);
extern void set_display_power_mode(uint32 mode);

// engine.cpp
extern void uninit_ring_buffer(ring_buffer &ringBuffer);
extern void setup_ring_buffer(ring_buffer &ringBuffer, const char *name);

// modes.cpp
extern void wait_for_vblank(void);
extern void set_frame_buffer_base(void);
extern void save_lvds_mode(void);
extern status_t create_mode_list(void);

// memory.cpp
extern void intel_free_memory(uint32 base);
extern status_t intel_allocate_memory(size_t size, uint32 flags, uint32 &base);

#endif	/* INTEL_EXTREME_ACCELERANT_H */
