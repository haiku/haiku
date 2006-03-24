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


typedef struct accelerant_info {
	vuint8			*regs;
	area_id			regs_area;

	intel_shared_info *shared_info;
	area_id			shared_info_area;

	display_mode	*mode_list;		// cloned list of standard display modes
	area_id			mode_list_area;

	int				device;
	bool			is_clone;
} accelerant_info;

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

// modes.cpp
extern status_t create_mode_list(void);

#endif	/* INTEL_EXTREME_ACCELERANT_H */
