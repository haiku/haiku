/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef FRAMEBUFFER_ACCELERANT_H
#define FRAMEBUFFER_ACCELERANT_H


#include "vesa_info.h"


typedef struct accelerant_info {
	int					device;
	bool				is_clone;

	area_id				shared_info_area;
	vesa_shared_info	*shared_info;

	area_id				mode_list_area;
		// cloned list of standard display modes
	display_mode		*mode_list;
	uint16				current_mode;
} accelerant_info;

extern accelerant_info *gInfo;

extern status_t create_mode_list(void);

#endif	/* FRAMEBUFFER_ACCELERANT_H */
