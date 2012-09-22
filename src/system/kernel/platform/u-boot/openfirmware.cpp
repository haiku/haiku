/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <platform/openfirmware/devices.h>
#include <platform/openfirmware/openfirmware.h>

#include <stdarg.h>


// FIXME: HACK: until we support multiple platforms at once in the kernel.

int gChosen;


status_t
of_init(int (*openFirmwareEntry)(void *))
{
	gChosen = of_finddevice("/chosen");
	if (gChosen == OF_FAILED)
		return B_ERROR;

	return B_OK;
}


int
of_call_client_function(const char *method, int numArgs, int numReturns, ...)
{
	return OF_FAILED;
}


int
of_interpret(const char *command, int numArgs, int numReturns, ...)
{
	return OF_FAILED;
}


int
of_call_method(int handle, const char *method, int numArgs, int numReturns, ...)
{
	return OF_FAILED;
}


int
of_finddevice(const char *device)
{
	return OF_FAILED;
}


status_t
of_get_next_device(int *_cookie, int root, const char *type, char *path,
	size_t pathSize)
{
	return B_ERROR;
}


/** Returns the first child of the given node
 */

int
of_child(int node)
{
	return OF_FAILED;
}


/** Returns the next sibling of the given node
 */

int
of_peer(int node)
{
	return OF_FAILED;
}


/** Returns the parent of the given node
 */

int
of_parent(int node)
{
	return OF_FAILED;
}


int
of_instance_to_path(int instance, char *pathBuffer, int bufferSize)
{
	return OF_FAILED;
}


int
of_instance_to_package(int instance)
{
	return OF_FAILED;
}


int
of_getprop(int package, const char *property, void *buffer, int bufferSize)
{
	return OF_FAILED;
}


int
of_setprop(int package, const char *property, const void *buffer, int bufferSize)
{
	return OF_FAILED;
}


int
of_getproplen(int package, const char *property)
{
	return OF_FAILED;
}


int
of_nextprop(int package, const char *previousProperty, char *nextProperty)
{
	return OF_FAILED;
}


int
of_package_to_path(int package, char *pathBuffer, int bufferSize)
{
	return OF_FAILED;
}


//	I/O functions


int
of_open(const char *nodeName)
{
	return OF_FAILED;
}


void
of_close(int handle)
{
}


int
of_read(int handle, void *buffer, int bufferSize)
{
	return OF_FAILED;
}


int
of_write(int handle, const void *buffer, int bufferSize)
{
	return OF_FAILED;
}


int
of_seek(int handle, long long pos)
{
	return OF_FAILED;
}


// memory functions


int
of_release(void *virtualAddress, int size)
{
	return OF_FAILED;
}


void *
of_claim(void *virtualAddress, int size, int align)
{
	return NULL;
}


// misc functions


/** tests if the given service is missing
 */

int
of_test(const char *service)
{
	return OF_FAILED;
}


/** Returns the millisecond counter
 */

int
of_milliseconds(void)
{
	return OF_FAILED;
}


void
of_exit(void)
{
}

