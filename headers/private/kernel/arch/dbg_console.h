/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_DBG_CONSOLE_H
#define KERNEL_DBG_CONSOLE_H

struct kernel_args;

char arch_dbg_con_read(void);
char arch_dbg_con_putch(char c);
void arch_dbg_con_puts(const char *s);
int arch_dbg_con_init(struct kernel_args *ka);

#endif	/* KERNEL_DBG_CONSOLE_H */
