/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/*
** Modified 2001/09/05 by Rob Judd<judd@ob-wan.com>
** Modified 2002/09/28 by Marcus Overhagen <marcus@overhagen.de>
*/

#include <kernel.h>
#include <int.h>
#include <arch/cpu.h>
#include <arch/dbg_console.h>

#include <boot/stage2.h>

#include <string.h>

/* define this only if not used as compile parameter
 * setting it to 1 will disable serial debug and
 * redirect it to Bochs
 */
// #define BOCHS_DEBUG_HACK 0

// Select between COM1 and COM2 for debug output
#define USE_COM1 1

static const int dbg_baud_rate = 115200;


char
arch_dbg_con_read(void)
{
#if BOCHS_DEBUG_HACK
	/* polling the keyboard, similar to code in keyboard
	 * driver, but without using an interrupt
	 */
	static const char unshifted_keymap[128] = {
		0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, '\t',
		'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
		'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
		'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		'\\', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	const char shifted_keymap[128] = {
		0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, '\t',
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};	
	static bool shift = false;
	static uint8 last = 0;
	uint8 key, ascii = 0;
	do {
		while ((key = in8(0x60)) == last)
			;
		last = key;
		if (key & 0x80) {
			if (key == (0x80 + 42) || key == (54 + 0x80))
				shift = false;
		} else {
			if (key == 42 || key == 54)
				shift = true;
			else
				ascii = shift ? shifted_keymap[key] : unshifted_keymap[key];
		}
	} while (!ascii);
	return ascii;
#endif

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


static void
_arch_dbg_con_putch(const char c)
{
#if BOCHS_DEBUG_HACK
	out8(c, 0xe9);
	return;
#endif

#if USE_COM1
	while ((in8(0x3fd) & 0x20) == 0)
		;
	out8(c, 0x3f8);
#else // COM2
	while ((in8(0x2fd) & 0x20) == 0)
		;
	out8(c, 0x2f8);
#endif
}


char
arch_dbg_con_putch(const char c)
{
	if (c == '\n') {
		_arch_dbg_con_putch('\r');
		_arch_dbg_con_putch('\n');
	} else if (c != '\r')
		_arch_dbg_con_putch(c);

	return c;
}


void
arch_dbg_con_puts(const char *s)
{
	while (*s != '\0') {
		arch_dbg_con_putch(*s);
		s++;
	}
}


void
arch_dbg_con_early_boot_message(const char *string)
{
	// this function will only be called in fatal situations
	// ToDo: also enable output via text console?!
	arch_dbg_con_init(NULL);
	arch_dbg_con_puts(string);
}


status_t
arch_dbg_con_init(kernel_args *args)
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

