/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <libroot_private.h>

#include <OS.h>
#include <image.h>


bool __gR5Compatibility = false;


extern "C" void
initialize_before()
{
	// determine if we have to run in R5 compatibility mode

	// get image of executable
	image_info info;
	uint32 cookie = 0;
	if (get_next_image_info(B_CURRENT_TEAM, (int32 *)&cookie, &info) != B_OK)
		return;

	if (get_image_symbol(info.id, "__gHaikuStartupCode", B_SYMBOL_TYPE_DATA,
			NULL) == B_OK) {
		// we were linked on/for Haiku
		return;
	}

	// We're using the BeOS startup code, check if BONE libraries are in
	// use, and if not, enable the R5 compatibility layer.

	const char *name;
	cookie = 0;
	int enable = 0;

	while (__get_next_image_dependency(info.id, &cookie, &name) == B_OK) {
		if (!strcmp(name, "libbind.so")
			|| !strcmp(name, "libsocket.so")
			|| !strcmp(name, "libnetwork.so"))
			enable -= 2;
		else if (!strcmp(name, "libnet.so"))
			enable++;
	}

	if (enable > 0) {
		__gR5Compatibility = true;
		debug_printf("libnetwork.so running in R5 compatibility mode.\n");
	}
}
