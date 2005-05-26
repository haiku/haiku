/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_CONSOLE_H
#define _KERNEL_CONSOLE_H


#include <module.h>
#include <stdio.h>

struct kernel_args;


typedef struct {
	module_info	info;

	status_t (*get_size)(int32 *_width, int32 *_height);
	void (*move_cursor)(int32 x, int32 y);
	void (*put_glyph)(int32 x, int32 y, uint8 glyph, uint8 attr);
	void (*fill_glyph)(int32 x, int32 y, int32 width, int32 height, uint8 glyph, uint8 attr);
	void (*blit)(int32 srcx, int32 srcy, int32 width, int32 height, int32 destx, int32 desty);
	void (*clear)(uint8 attr);
} console_module_info;


#ifdef __cplusplus
extern "C" {
#endif

int con_init(struct kernel_args *args);
void kprintf(const char *fmt, ...) __PRINTFLIKE(1,2);
void kprintf_xy(int x, int y, const char *fmt, ...) __PRINTFLIKE(3,4);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_CONSOLE_H */
