/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_FRAME_BUFFER_CONSOLE_H
#define KERNEL_FRAME_BUFFER_CONSOLE_H

#include <console.h>

struct kernel_args;


#define FRAME_BUFFER_CONSOLE_MODULE_NAME "console/frame_buffer/v1"

#ifdef __cplusplus
extern "C" {
#endif

status_t frame_buffer_console_init(struct kernel_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_FRAME_BUFFER_CONSOLE_H */
