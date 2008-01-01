/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "keyboard.h"
#include "bios.h"

#include <boot/platform.h>


static uint16
check_for_key(void)
{
//XXX: non blocking in ?
#if 0
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
#endif
	return 0;
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
	union key key;
	key.d0 = Bconin(2);

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

