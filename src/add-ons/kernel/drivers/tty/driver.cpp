/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include "tty_private.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define TTY_TRACE
#ifdef TTY_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif

#define DRIVER_NAME "tty"


int32 api_version = B_CUR_DRIVER_API_VERSION;

static char *sDeviceNames[kNumTTYs * 2 + 1];
	// reserve space for "pt/" and "tt/" entries and the terminating NULL


status_t
init_hardware(void)
{
	TRACE((DRIVER_NAME ": init_hardware()\n"));
	return B_OK;
}


status_t
init_driver(void)
{
	TRACE((DRIVER_NAME ": init_driver()\n"));

	// create driver name array
	
	char letter = 'p';
	int8 digit = 0;

	for (uint32 i = 0; i < kNumTTYs; i++) {
		// For compatibility, we have to create the same mess in /dev/pt and /dev/tt
		// as BeOS does: we publish devices p0, p1, ..., pf, r1, ..., sf.
		// It would be nice if we could drop compatibility and create something better.
		char buffer[64];

		snprintf(buffer, sizeof(buffer), "pt/%c%x", letter, digit);
		sDeviceNames[i] = strdup(buffer);

		snprintf(buffer, sizeof(buffer), "tt/%c%x", letter, digit);
		sDeviceNames[i + kNumTTYs] = strdup(buffer);

		if (++digit > 15)
			digit = 0, letter++;
	}

	sDeviceNames[kNumTTYs * 2] = NULL;

	// initialize TTY structures
	
	memset(gMasterTTYs, 0, kNumTTYs * sizeof(struct tty));
	memset(gSlaveTTYs, 0, kNumTTYs * sizeof(struct tty));

	return B_OK;
}


void
uninit_driver(void)
{
	TRACE((DRIVER_NAME ": uninit_driver()\n"));

	for (int32 i = 0; sDeviceNames[i] != NULL; i++)
		free(sDeviceNames[i]);
}


const char **
publish_devices(void)
{
	TRACE((DRIVER_NAME ": publish_devices()\n"));
	return const_cast<const char **>(sDeviceNames);
}


device_hooks *
find_device(const char *name)
{
	TRACE((DRIVER_NAME ": find_device(\"%s\")\n", name));

	for (uint32 i = 0; sDeviceNames[i] != NULL; i++)
		if (!strcmp(name, sDeviceNames[i]))
			return i < kNumTTYs ? &gMasterTTYHooks : &gSlaveTTYHooks;

	return NULL;
}

