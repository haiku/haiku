/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H


#include <kernel.h>
#include <arch/smp.h>

struct kernel_args;


extern int dbg_register_file[SMP_MAX_CPUS][14];
	/* XXXmpetit -- must be made generic */

int  dbg_init(struct kernel_args *ka);
int  dbg_init2(struct kernel_args *ka);
char dbg_putch(char c);
void dbg_puts(const char *s);

#if DEBUG 
#	define ASSERT(x) \
	if (x) {} else { panic("ASSERT FAILED (%s:%d): %s\n", __FILE__, __LINE__, #x); }
#else 
#	define ASSERT(x) 
#endif

#endif	/* _KERNEL_DEBUG_H */
