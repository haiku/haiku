/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_DEBUG_H
#define _KERNEL_DEBUG_H

#include <kernel.h>
#include <stage2.h>
#include <cdefs.h>

extern int dbg_register_file[2][14]; /* XXXmpetit -- must be made generic */

int  dbg_init(kernel_args *ka);
int  dbg_init2(kernel_args *ka);
char dbg_putch(char c);
void dbg_puts(const char *s);
bool dbg_set_serial_debug(bool new_val);
bool dbg_get_serial_debug(void);
int  dprintf(const char *fmt, ...) __PRINTFLIKE(1,2);
int  panic(const char *fmt, ...) __PRINTFLIKE(1,2);
void kernel_debugger(void);
int  dbg_add_command(void (*func)(int, char **), const char *cmd, const char *desc);

extern void dbg_save_registers(int *);	/* arch provided */

#endif
