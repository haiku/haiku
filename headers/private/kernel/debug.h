/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the Haiku License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H


#include <OS.h>

struct kernel_args;


#if DEBUG
#	define ASSERT(x) \
	if (x) {} else { panic("ASSERT FAILED (%s:%d): %s\n", __FILE__, __LINE__, #x); }
#else 
#	define ASSERT(x) 
#endif

extern int dbg_register_file[B_MAX_CPU_COUNT][14];


#ifdef __cplusplus
extern "C" {
#endif

extern status_t debug_init(struct kernel_args *args);
extern status_t	debug_init_post_vm(struct kernel_args *args);
extern status_t	debug_init_post_modules(struct kernel_args *args);
extern void debug_early_boot_message(const char *string);
extern void debug_puts(const char *s);
extern bool debug_debugger_running(void);

extern void _user_debug_output(const char *userString);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_DEBUG_H */
