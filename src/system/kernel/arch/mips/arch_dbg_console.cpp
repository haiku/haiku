/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/kernel.h>
#include <boot/stage2.h>

#include <kernel/arch/dbg_console.h>

int arch_dbg_con_init(kernel_args *ka)
{
	return 0;
}

char arch_dbg_con_read()
{
	return 0;
}

/* Flush all FIFO'd bytes out of the serial port buffer */
static void arch_dbg_con_flush()
{
}

static void _arch_dbg_con_putch(const char c)
{
}

char arch_dbg_con_putch(const char c)
{
	if (c == '\n') {
		_arch_dbg_con_putch('\r');
		_arch_dbg_con_putch('\n');
	} else if (c != '\r')
		_arch_dbg_con_putch(c);

	return c;
}

void arch_dbg_con_puts(const char *s)
{
	while(*s != '\0') {
		arch_dbg_con_putch(*s);
		s++;
	}
}

