/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de
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

int arch_debug_blue_screen_try_getchar(void);
char arch_debug_blue_screen_getchar(void);
int arch_debug_serial_try_getchar(void);
char arch_debug_serial_getchar(void);
void arch_debug_serial_putchar(char c);
void arch_debug_serial_puts(const char *s);
void arch_debug_serial_early_boot_message(const char *string);

void arch_debug_remove_interrupt_handler(uint32 line);
void arch_debug_install_interrupt_handlers(void);

status_t arch_debug_console_init(struct kernel_args *args);
status_t arch_debug_console_init_settings(struct kernel_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_ARCH_DEBUG_CONSOLE_H */
