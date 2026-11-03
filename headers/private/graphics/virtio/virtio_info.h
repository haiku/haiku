/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VIRTIO_INFO_H
#define VIRTIO_INFO_H


#include <Drivers.h>
#include <Accelerant.h>

#include <edid.h>


#define MAX_SCANOUTS 16


struct virtio_gpu_shared_info {
	struct {
		area_id			mode_list_area;		// area containing display mode list
		uint32			mode_count;
		display_mode	current_mode;
		uint32			bytes_per_row;

		area_id			frame_buffer_area;	// area of frame buffer
		uint8*			frame_buffer;
			// pointer to frame buffer (visible by all apps!)

		edid1_raw		edid_raw;
		bool			has_edid;
		uint32			dpms_capabilities;
	} display_info[MAX_SCANOUTS];

	char			name[32];
	uint32			vram_size;
};

//----------------- ioctl() interface ----------------

// list ioctls
enum {
	VIRTIO_GPU_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	VIRTIO_GPU_GET_DEVICE_NAME,
	VIRTIO_GPU_SET_DISPLAY_MODE,
	VIRTIO_GPU_GET_ENABLED_SCANOUTS,
};


struct virtio_gpu_set_display_mode_params {
	uint32 scanout;
	display_mode mode;
};


#endif	/* VIRTIO_INFO_H */
