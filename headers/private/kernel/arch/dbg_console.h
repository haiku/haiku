/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_ARCH_DBG_CONSOLE
#define _NEWOS_KERNEL_ARCH_DBG_CONSOLE

#include <stage2.h>

char arch_dbg_con_read(void);
char arch_dbg_con_putch(char c);
void arch_dbg_con_puts(const char *s);
int arch_dbg_con_init(kernel_args *ka);

#endif

