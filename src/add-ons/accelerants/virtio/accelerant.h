/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef VIRTIO_GPU_ACCELERANT_H
#define VIRTIO_GPU_ACCELERANT_H


#include "virtio_info.h"


typedef struct accelerant_info {
	int					device;
	bool				is_clone;

	uint32				enabled_heads;
	int					current_head;

	area_id				shared_info_area;
	virtio_gpu_shared_info*	shared_info;

	struct {
		area_id				mode_list_area;
			// cloned list of standard display modes
		display_mode		*mode_list;
		uint16				current_mode;
	} heads[MAX_SCANOUTS];
} accelerant_info;

extern accelerant_info *gInfo;

extern status_t create_mode_list(int scanout);

#endif	/* VIRTIO_GPU_ACCELERANT_H */
