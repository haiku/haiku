/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
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
	/* XXXmpetit -- must be made generic */

#ifdef __cplusplus
extern "C" {
#endif

extern int	dbg_init(struct kernel_args *ka);
extern int	dbg_init2(struct kernel_args *ka);
extern char	dbg_putch(char c);
extern void	dbg_puts(const char *s);

extern void _user_debug_output(const char *userString);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_DEBUG_H */
