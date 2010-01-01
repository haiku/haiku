/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_FRAME_BUFFER_CONSOLE_H
#define KERNEL_FRAME_BUFFER_CONSOLE_H


#include <console.h>


struct kernel_args;


#define FRAME_BUFFER_CONSOLE_MODULE_NAME "console/frame_buffer/v1"
#define FRAME_BUFFER_BOOT_INFO "frame_buffer/v1"

struct frame_buffer_boot_info {
	area_id	area;
	addr_t	physical_frame_buffer;
	addr_t	frame_buffer;
	int32	width;
	int32	height;
	int32	depth;
	int32	bytes_per_row;
	uint8	vesa_capabilities;
};

#ifdef __cplusplus
extern "C" {
#endif

bool		frame_buffer_console_available(void);
status_t	frame_buffer_update(addr_t baseAddress, int32 width, int32 height,
				int32 depth, int32 bytesPerRow);
status_t	frame_buffer_console_init(struct kernel_args* args);
status_t	frame_buffer_console_init_post_modules(struct kernel_args* args);

status_t	_user_frame_buffer_update(addr_t baseAddress, int32 width,
				int32 height, int32 depth, int32 bytesPerRow);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_FRAME_BUFFER_CONSOLE_H */
