/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <libroot_private.h>

#include <OS.h>
#include <image.h>

#include <string.h>


bool __gR5Compatibility = false;
addr_t __gNetworkStart;
addr_t __gNetworkEnd;
addr_t __gNetAPIStart;
addr_t __gNetAPIEnd;


static void
find_own_image()
{
	int32 cookie = 0;
	image_info info;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		if (((addr_t)info.text <= (addr_t)find_own_image
			&& (addr_t)info.text + (addr_t)info.text_size
				> (addr_t)find_own_image)) {
			// found us
			__gNetworkStart = (addr_t)min_c(info.text, info.data);
			__gNetworkEnd = min_c((addr_t)info.text + info.text_size,
				(addr_t)info.data + info.data_size);
			break;
		}
	}
}


extern "C" void
initialize_before()
{
	// determine if we have to run in BeOS compatibility mode

	// get image of executable
	image_info info;
	uint32 cookie = 0;
	if (get_next_image_info(B_CURRENT_TEAM, (int32*)&cookie, &info) != B_OK)
		return;

	if (get_image_symbol(info.id, "__gHaikuStartupCode", B_SYMBOL_TYPE_DATA,
			NULL) == B_OK) {
		// we were linked on/for Haiku
		return;
	}

	// We're using the BeOS startup code, check if BONE libraries are in
	// use, and if not, enable the BeOS R5 compatibility layer.
	// As dependencies to network libraries may be "hidden" in libraries, we
	// may have to scan not only the executable, but every loaded image.
	int enable = 0;
	uint32 crumble;
	const char *name;
	do {
		crumble = 0;
		while (__get_next_image_dependency(info.id, &crumble, &name) == B_OK) {
			if (!strcmp(name, "libbind.so")
				|| !strcmp(name, "libsocket.so")
				|| !strcmp(name, "libbnetapi.so")
				|| !strcmp(name, "libnetwork.so"))
				enable -= 2;
			else if (!strcmp(name, "libnet.so")
				|| !strcmp(name, "libnetapi.so"))
				enable++;
		}

		if (enable > 0) {
			__gR5Compatibility = true;
			find_own_image();
			debug_printf("libnetwork.so running in R5 compatibility mode.\n");
			return;
		}
	} while (enable == 0
		&& get_next_image_info(B_CURRENT_TEAM, (int32*)&cookie, &info) == B_OK);
}
