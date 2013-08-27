/* Yarrow Random Number Generator (True Randomness Achieved in Software) *
 * Copyright (c) 1998-2000 by Yarrow Charnot, Identikey <mailto://ycharnot@identikey.com>
 * All Lefts, Rights, Ups, Downs, Forwards, Backwards, Pasts and Futures Reserved *
 */

/* Made into a BeOS /dev/random and /dev/urandom by Daniel Berlin */
/* Adapted for Haiku by David Reid, Axel Dörfler */


#include <stdio.h>

#include <Drivers.h>
#include <util/AutoLock.h>

#include "yarrow_rng.h"


#define TRACE_DRIVER
#ifdef TRACE_DRIVER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

int32 api_version = B_CUR_DRIVER_API_VERSION;

#define	DRIVER_NAME "random"

static const char *sRandomNames[] = {
	"random",
	"urandom",
	NULL
};

static status_t random_open(const char *name, uint32 flags, void **cookie);
static status_t random_read(void *cookie, off_t position, void *_buffer, size_t *_numBytes);
static status_t random_write(void *cookie, off_t position, const void *buffer, size_t *_numBytes);
static status_t random_control(void *cookie, uint32 op, void *arg, size_t length);
static status_t random_close(void *cookie);
static status_t random_free(void *cookie);
static status_t random_select(void *cookie, uint8 event, uint32 ref, selectsync *sync);
static status_t random_deselect(void *cookie, uint8 event, selectsync *sync);


static device_hooks sRandomHooks = {
	random_open,
	random_close,
	random_free,
	random_control,
	random_read,
	random_write,
	random_select,
	random_deselect,
	NULL,
	NULL
};


static mutex sRandomLock;


//	#pragma mark -
//	device driver


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

	mutex_init(&sRandomLock, "/dev/random lock");

	return gRandomModuleInfo->init();
}


void
uninit_driver(void)
{
	TRACE((DRIVER_NAME ": uninit_driver()\n"));
	gRandomModuleInfo->uninit();
	mutex_destroy(&sRandomLock);
}


const char **
publish_devices(void)
{
	TRACE((DRIVER_NAME ": publish_devices()\n"));
	return sRandomNames;
}


device_hooks *
find_device(const char* name)
{
	TRACE((DRIVER_NAME ": find_device(\"%s\")\n", name));

	for (int i = 0; sRandomNames[i] != NULL; i++)
		if (strcmp(name, sRandomNames[i]) == 0)
			return &sRandomHooks;

	return NULL;
}


//	#pragma mark -
//	device functions


static status_t
random_open(const char *name, uint32 flags, void **cookie)
{
	TRACE((DRIVER_NAME ": open(\"%s\")\n", name));
	return B_OK;
}


static status_t
random_read(void *cookie, off_t position, void *_buffer, size_t *_numBytes)
{
	TRACE((DRIVER_NAME ": read(%Ld,, %ld)\n", position, *_numBytes));

	MutexLocker locker(&sRandomLock);
	return gRandomModuleInfo->read(_buffer, _numBytes);
}


static status_t
random_write(void *cookie, off_t position, const void *buffer, size_t *_numBytes)
{
	TRACE((DRIVER_NAME ": write(%Ld,, %ld)\n", position, *_numBytes));
	MutexLocker locker(&sRandomLock);
	return gRandomModuleInfo->write(buffer, _numBytes);
}


static status_t
random_control(void *cookie, uint32 op, void *arg, size_t length)
{
	TRACE((DRIVER_NAME ": ioctl(%ld)\n", op));
	return B_ERROR;
}


static status_t
random_close(void *cookie)
{
	TRACE((DRIVER_NAME ": close()\n"));
	return B_OK;
}


static status_t
random_free(void *cookie)
{
	TRACE((DRIVER_NAME ": free()\n"));
	return B_OK;
}


static status_t
random_select(void *cookie, uint8 event, uint32 ref, selectsync *sync)
{
	TRACE((DRIVER_NAME ": select()\n"));

	if (event == B_SELECT_READ) {
		/* tell there is already data to read */
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
		notify_select_event(sync, ref);
#else
		notify_select_event(sync, event);
#endif
	} else if (event == B_SELECT_WRITE) {
		/* we're now writable */
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
		notify_select_event(sync, ref);
#else
		notify_select_event(sync, event);
#endif
	}
	return B_OK;
}


static status_t
random_deselect(void *cookie, uint8 event, selectsync *sync)
{
	TRACE((DRIVER_NAME ": deselect()\n"));
	return B_OK;
}
