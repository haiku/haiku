/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "keyboard.h"
#include "nextrom.h"

#include <boot/platform.h>


static uint32
check_for_key(void)
{
	union key k;
	k.d0 = mg->mg_try_getc();

	return k.d0;
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
	key.d0 = mg->mg_getc();

	return key;
}


extern "C" uint32
check_for_boot_keys(void)
{
	union key key;
	uint32 options = 0;

	while ((key.d0 = check_for_key()) != 0) {
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

