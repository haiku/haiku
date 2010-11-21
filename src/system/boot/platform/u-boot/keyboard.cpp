/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include "keyboard.h"

#include <boot/platform.h>


/** Note, checking for keys doesn't seem to work in graphics
 *	mode, at least in Bochs.
 */

static uint16
check_for_key(void)
{
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
	key.ax=0;
	return key;
}


extern "C" uint32
check_for_boot_keys(void)
{
	return 0;
}

