/* 
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_DBG_CONSOLE_H
#define KERNEL_DBG_CONSOLE_H


#include <SupportDefs.h>


struct kernel_args;

#ifdef __cplusplus
extern "C" {
#endif

char arch_dbg_con_read(void);
char arch_dbg_con_putch(char c);
void arch_dbg_con_puts(const char *s);

void arch_dbg_con_early_boot_message(const char *string);
status_t arch_dbg_con_init(struct kernel_args *args);
status_t arch_dbg_con_init_settings(struct kernel_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_DBG_CONSOLE_H */
