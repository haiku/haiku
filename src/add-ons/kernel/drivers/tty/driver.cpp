/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <new>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lock.h>

#include "SemaphorePool.h"
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

static char *sDeviceNames[kNumTTYs * 2 + 1];
	// reserve space for "pt/" and "tt/" entries and the terminating NULL

struct mutex gGlobalTTYLock;
struct mutex gTTYCookieLock;
struct recursive_lock gTTYRequestLock;
static char sSemaphorePoolBuffer[sizeof(SemaphorePool)];
SemaphorePool *gSemaphorePool = 0;


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

	memset(sDeviceNames, 0, sizeof(sDeviceNames));

	// create the global mutex
	status_t error = mutex_init(&gGlobalTTYLock, "tty global");
	if (error != B_OK)
		return error;

	// create the cookie mutex
	error = mutex_init(&gTTYCookieLock, "tty cookies");
	if (error != B_OK) {
		mutex_destroy(&gGlobalTTYLock);
		return error;
	}

	// create the request mutex
	error = recursive_lock_init(&gTTYRequestLock, "tty requests");
	if (error != B_OK) {
		mutex_destroy(&gTTYCookieLock);
		mutex_destroy(&gGlobalTTYLock);
		return error;
	}

	// create the semaphore pool
	gSemaphorePool
		= new(sSemaphorePoolBuffer) SemaphorePool(kMaxCachedSemaphores);
	error = gSemaphorePool->Init();
	if (error != B_OK) {
		uninit_driver();
		return error;
	}

	// create driver name array and initialize basic TTY structures

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

		reset_tty(&gMasterTTYs[i], i);
		reset_tty(&gSlaveTTYs[i], i);

		if (!sDeviceNames[i] || !sDeviceNames[i + kNumTTYs]) {
			uninit_driver();
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


void
uninit_driver(void)
{
	TRACE((DRIVER_NAME ": uninit_driver()\n"));

	for (int32 i = 0; i < (int32)kNumTTYs * 2; i++)
		free(sDeviceNames[i]);

	if (gSemaphorePool) {
		gSemaphorePool->~SemaphorePool();
		gSemaphorePool = NULL;
	}

	recursive_lock_destroy(&gTTYRequestLock);
	mutex_destroy(&gTTYCookieLock);
	mutex_destroy(&gGlobalTTYLock);
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

