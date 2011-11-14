/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the Haiku License.
 */
#ifndef CONSOLE_H
#define CONSOLE_H

#include <boot/platform/generic/text_console.h>

#ifdef __cplusplus
extern "C" {
#endif

extern status_t console_init(void);
extern int console_check_for_key(void);

#ifdef __cplusplus
}
#endif

#endif	/* CONSOLE_H */
