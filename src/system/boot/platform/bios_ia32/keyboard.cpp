/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "keyboard.h"
#include "bios.h"

#include <boot/platform.h>


/** Note, checking for keys doesn't seem to work in graphics
 *	mode, at least in Bochs.
 */

static uint16
check_for_key(void)
{
	bios_regs regs;
	regs.eax = 0x0100;
	call_bios(0x16, &regs);

	// the zero flag is set when there is no key stroke waiting for us
	if (regs.flags & ZERO_FLAG)
		return 0;

	// remove the key from the buffer
	regs.eax = 0;
	call_bios(0x16, &regs);

	return regs.eax & 0xffff;
}


extern "C" void
clear_key_buffer(void)
{
	while (check_for_key() != 0)
		;
}


extern "C" union key
wait_for_key(void)
{
	bios_regs regs;
	regs.eax = 0;
	call_bios(0x16, &regs);

	union key key;
	key.ax = regs.eax & 0xffff;

	return key;
}


extern "C" uint32
check_for_boot_keys(void)
{
	bios_regs regs;
	uint32 options = 0;
	uint32 keycode = 0;
	regs.eax = 0x0200;
	call_bios(0x16, &regs);
		// Read Keyboard flags. bit 0 LShift, bit 1 RShift
	if ((regs.eax & 0x03) != 0) {
		// LShift or RShift - option menu
		options |= BOOT_OPTION_MENU;
	} else {
		keycode = search_keyboard_buffer();
		if (keycode == 0x4200 || keycode == 0x8600 || keycode == 0x3920) {
			// F8 or F12 or Space - option menu
			options |= BOOT_OPTION_MENU;
		} else if (keycode == 0x011B) {
			// ESC - debug output
			options |= BOOT_OPTION_DEBUG_OUTPUT;
 		}
	}

	dprintf("options = %ld\n", options);
	return options;
}

