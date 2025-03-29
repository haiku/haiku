/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef FRAMEBUFFER_PRIVATE_H
#define FRAMEBUFFER_PRIVATE_H


#include <Drivers.h>
#include <Accelerant.h>
#include <PCI.h>


#define DEVICE_NAME				"framebuffer"
#define ACCELERANT_NAME	"framebuffer.accelerant"


struct framebuffer_info {
	uint32			cookie_magic;
	int32			open_count;
	int32			id;
	struct vesa_shared_info* shared_info;
	area_id			shared_area;

	addr_t			frame_buffer;
};

extern status_t framebuffer_init(framebuffer_info& info);
extern void framebuffer_uninit(framebuffer_info& info);

#endif	/* FRAMEBUFFER_PRIVATE_H */
