/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef CONSOLE_H
#define CONSOLE_H


#include <boot/vfs.h>
#include <boot/stdio.h>


enum console_color {
	// foreground and background colors
	BLACK,
	BLUE,
	GREEN,
	CYAN,
	RED,
	PURPLE,
	BROWN,
	GRAY,
	// foreground colors only if blinking is enabled
	DARK_GRAY,
	BRIGHT_BLUE,
	BRIGHT_GREEN,
	BRIGHT_CYAN,
	BRIGHT_RED,
	MAGENTA,
	YELLOW,
	WHITE
};


#ifdef __cplusplus
extern "C" {
#endif

extern void console_clear_screen(void);
extern int32 console_width(void);
extern int32 console_height(void);
extern void console_set_cursor(int32 x, int32 y);
extern void console_set_color(int32 foreground, int32 background);

extern status_t console_init(void);

extern void serial_disable(void);
extern void serial_enable(void);

#ifdef __cplusplus
}
#endif

#endif	/* CONSOLE_H */
