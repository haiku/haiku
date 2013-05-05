/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "keyboard.h"

#include <boot/platform.h>


static uint16
check_for_key(void)
{
#warning IMPLEMENT check_for_key
	return 0;
}


extern "C" void
clear_key_buffer(void)
{
#warning IMPLEMENT clear_key_buffer
	while (check_for_key() != 0)
		;
}


extern "C" union key
wait_for_key(void)
{
#warning IMPLEMENT wait_for_key
	union key key;
	return key;
}


extern "C" uint32
check_for_boot_keys(void)
{
#warning IMPLEMENT check_for_boot_keys
	return 0;
}
