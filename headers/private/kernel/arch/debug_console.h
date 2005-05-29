/* 
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_ARCH_DEBUG_CONSOLE_H
#define KERNEL_ARCH_DEBUG_CONSOLE_H


#include <SupportDefs.h>


struct kernel_args;

#ifdef __cplusplus
extern "C" {
#endif

char arch_debug_blue_screen_getchar(void);
char arch_debug_serial_getchar(void);
void arch_debug_serial_putchar(char c);
void arch_debug_serial_puts(const char *s);
void arch_debug_serial_early_boot_message(const char *string);

status_t arch_debug_console_init(struct kernel_args *args);
status_t arch_debug_console_init_settings(struct kernel_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_ARCH_DEBUG_CONSOLE_H */
