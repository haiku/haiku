/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONSOLE_H
#define CONSOLE_H


#include <boot/vfs.h>
#include <boot/stdio.h>

#include "Handle.h"


#ifdef __cplusplus
extern "C" {
#endif

extern status_t console_init(void);

extern status_t set_cursor_pos(FILE *, int x, int y);
extern status_t set_foreground_color(FILE *, int c);
extern status_t set_background_color(FILE *, int c);

#ifdef __cplusplus
}
#endif

#endif	/* CONSOLE_H */
