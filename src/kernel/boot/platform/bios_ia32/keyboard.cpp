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
	union key key;
	uint32 options = 0;

	while ((key.ax = check_for_key()) != 0) {
		switch (key.code.ascii) {
			case ' ':
				options |= BOOT_OPTION_MENU;
				break;
			case 0x1b:	// escape
				options |= BOOT_OPTION_DEBUG_OUTPUT;
				break;
			case 0:
				// evaluate BIOS scan codes
				// ...
				break;
		}
	}

	dprintf("options = %ld\n", options);
	return options;
}

