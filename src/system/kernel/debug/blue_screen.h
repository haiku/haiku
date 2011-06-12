/*
 * Copyright 2005-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLUE_SCREEN_H
#define BLUE_SCREEN_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t blue_screen_init(void);
status_t blue_screen_enter(bool debugOutput);

bool blue_screen_paging_enabled(void);
void blue_screen_set_paging(bool enabled);

void blue_screen_clear_screen(void);
int blue_screen_try_getchar(void);
char blue_screen_getchar(void);
void blue_screen_putchar(char c);
void blue_screen_puts(const char *text);

#ifdef __cplusplus
}
#endif

#endif	/* BLUE_SCREEN_H */
