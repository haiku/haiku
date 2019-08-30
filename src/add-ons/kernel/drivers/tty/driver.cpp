/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <new>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lock.h>

#include "tty_private.h"


//#define TTY_TRACE
#ifdef TTY_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif

#define DRIVER_NAME "tty"

static const int kMaxCachedSemaphores = 8;

int32 api_version = B_CUR_DRIVER_API_VERSION;

char *gDeviceNames[kNumTTYs * 2 + 3];
	// reserve space for "pt/" and "tt/" entries, "ptmx", "tty", and the
	// terminating NULL

static mutex sTTYLocks[kNumTTYs];

struct mutex gGlobalTTYLock;
struct mutex gTTYCookieLock;
struct recursive_lock gTTYRequestLock;


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

	memset(gDeviceNames, 0, sizeof(gDeviceNames));

	// create the request mutex
	recursive_lock_init(&gTTYRequestLock, "tty requests");

	// create the global mutex
	mutex_init(&gGlobalTTYLock, "tty global");

	// create the cookie mutex
	mutex_init(&gTTYCookieLock, "tty cookies");

	// create driver name array and initialize basic TTY structures

	char letter = 'p';
	int8 digit = 0;

	for (uint32 i = 0; i < kNumTTYs; i++) {
		// For compatibility, we have to create the same mess in /dev/pt and
		// /dev/tt as BeOS does: we publish devices p0, p1, ..., pf, r1, ...,
		// sf. It would be nice if we could drop compatibility and create
		// something better. In fact we already don't need the master devices
		// anymore, since "/dev/ptmx" does the job. The slaves entries could
		// be published on the fly when a master is opened (e.g via
		// vfs_create_special_node()).
		char buffer[64];

		snprintf(buffer, sizeof(buffer), "pt/%c%x", letter, digit);
		gDeviceNames[i] = strdup(buffer);

		snprintf(buffer, sizeof(buffer), "tt/%c%x", letter, digit);
		gDeviceNames[i + kNumTTYs] = strdup(buffer);

		if (++digit > 15)
			digit = 0, letter++;

		mutex_init(&sTTYLocks[i], "tty lock");
		reset_tty(&gMasterTTYs[i], i, &sTTYLocks[i], true);
		reset_tty(&gSlaveTTYs[i], i, &sTTYLocks[i], false);
		reset_tty_settings(&gTTYSettings[i], i);

		if (!gDeviceNames[i] || !gDeviceNames[i + kNumTTYs]) {
			uninit_driver();
			return B_NO_MEMORY;
		}
	}

	gDeviceNames[2 * kNumTTYs] = (char *)"ptmx";
	gDeviceNames[2 * kNumTTYs + 1] = (char *)"tty";

	tty_add_debugger_commands();

	return B_OK;
}


void
uninit_driver(void)
{
	TRACE((DRIVER_NAME ": uninit_driver()\n"));

	tty_remove_debugger_commands();

	for (int32 i = 0; i < (int32)kNumTTYs * 2; i++)
		free(gDeviceNames[i]);

	for (int32 i = 0; i < (int32)kNumTTYs; i++)
		mutex_destroy(&sTTYLocks[i]);

	recursive_lock_destroy(&gTTYRequestLock);
	mutex_destroy(&gTTYCookieLock);
	mutex_destroy(&gGlobalTTYLock);
}


const char **
publish_devices(void)
{
	TRACE((DRIVER_NAME ": publish_devices()\n"));
	return const_cast<const char **>(gDeviceNames);
}


device_hooks *
find_device(const char *name)
{
	TRACE((DRIVER_NAME ": find_device(\"%s\")\n", name));

	for (uint32 i = 0; gDeviceNames[i] != NULL; i++)
		if (!strcmp(name, gDeviceNames[i])) {
			return i < kNumTTYs || i == (2 * kNumTTYs)
				? &gMasterTTYHooks : &gSlaveTTYHooks;
		}

	return NULL;
}

