/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/*
** Modified 2001/09/05 by Rob Judd<judd@ob-wan.com>
*/
#include <kernel.h>
#include <int.h>
#include <arch/cpu.h>
#include <arch/dbg_console.h>

#include <stage2.h>

#include <string.h>

#define BOCHS_E9_HACK 0

// Select between COM1 and COM2 for debug output
#define USE_COM1 1

static const int dbg_baud_rate = 115200;

int arch_dbg_con_init(kernel_args *ka)
{
	short divisor = 115200 / dbg_baud_rate;

#if USE_COM1
	out8(0x80, 0x3fb);	/* set up to load divisor latch	*/
	out8(divisor & 0xf, 0x3f8);		/* LSB */
	out8(divisor >> 8, 0x3f9);		/* MSB */
	out8(3, 0x3fb);		/* 8N1 */
#else // COM2
	out8(0x80, 0x2fb);	/* set up to load divisor latch	*/
	out8(divisor & 0xf, 0x2f8);		/* LSB */
	out8(divisor >> 8, 0x2f9);		/* MSB */
	out8(3, 0x2fb);		/* 8N1 */
#endif

	return 0;
}

char arch_dbg_con_read(void)
{
#if USE_COM1
	while ((in8(0x3fd) & 1) == 0)
		;
	return in8(0x3f8);
#else
	while ((in8(0x2fd) & 1) == 0)
		;
	return in8(0x2f8);
#endif
}

static void _arch_dbg_con_putch(const char c)
{
#if BOCHS_E9_HACK
	out8(c, 0xe9);
#else

#if USE_COM1
	while ((in8(0x3fd) & 0x20) == 0)
		;
	out8(c, 0x3f8);
#else // COM2
	while ((in8(0x2fd) & 0x20) == 0)
		;
	out8(c, 0x2f8);
#endif

#endif
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

