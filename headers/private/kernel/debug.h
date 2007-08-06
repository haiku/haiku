/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the Haiku License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H


#include <OS.h>

#define KDEBUG 1

#if DEBUG
/* 
 * The kernel debug level. 
 * Level 1 is usual asserts, > 1 should be used for very expensive runtime checks
 */
#define KDEBUG 1
#endif

#define ASSERT_ALWAYS(x) \
	do { if (!(x)) { panic("ASSERT FAILED (%s:%d): %s\n", __FILE__, __LINE__, #x); } } while (0)

#define ASSERT_ALWAYS_PRINT(x, format...) \
	do {																	\
		if (!(x)) {															\
			dprintf(format);												\
			panic("ASSERT FAILED (%s:%d): %s\n", __FILE__, __LINE__, #x);	\
		}																	\
	} while (0)

#if KDEBUG
#	define ASSERT(x)					ASSERT_ALWAYS(x)
#	define ASSERT_PRINT(x, format...)	ASSERT_ALWAYS_PRINT(x, format)
#else 
#	define ASSERT(x)					do { } while(0)
#	define ASSERT_PRINT(x, format...)	do { } while(0)
#endif

extern int dbg_register_file[B_MAX_CPU_COUNT][14];

#ifdef __cplusplus
extern "C" {
#endif

struct kernel_args;

extern status_t debug_init(struct kernel_args *args);
extern status_t	debug_init_post_vm(struct kernel_args *args);
extern status_t	debug_init_post_modules(struct kernel_args *args);
extern void debug_early_boot_message(const char *string);
extern void debug_puts(const char *s, int32 length);
extern bool debug_debugger_running(void);
extern void debug_stop_screen_debug_output(void);

extern void _user_debug_output(const char *userString);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_DEBUG_H */
