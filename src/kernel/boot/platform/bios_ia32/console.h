/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef CONSOLE_H
#define CONSOLE_H


#include <boot/vfs.h>
#include <boot/stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern status_t console_init(void);

#ifdef __cplusplus
}
#endif

#endif	/* CONSOLE_H */
