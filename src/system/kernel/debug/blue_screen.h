/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLUE_SCREEN_H
#define BLUE_SCREEN_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t blue_screen_init(void);
void blue_screen_enter(bool debugOutput);

char blue_screen_getchar(void);
void blue_screen_putchar(char c);
void blue_screen_puts(const char *text);

#ifdef __cplusplus
}
#endif

#endif	/* BLUE_SCREEN_H */
