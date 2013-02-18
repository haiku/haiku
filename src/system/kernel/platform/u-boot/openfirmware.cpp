/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <platform/openfirmware/devices.h>
#include <platform/openfirmware/openfirmware.h>

#include <stdarg.h>
#include <KernelExport.h>

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};

#define TRACE_OF
#ifdef TRACE_OF
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// FIXME: HACK: until we support multiple platforms at once in the kernel.

extern void *gFDT;
int gChosen;


static int fdt2of(int err)
{
	if (err >= 0)
		return err;
	switch (err) {
		case -FDT_ERR_NOTFOUND:
		case -FDT_ERR_EXISTS:
		case -FDT_ERR_NOSPACE:
		case -FDT_ERR_BADOFFSET:
		case -FDT_ERR_BADPATH:
		case -FDT_ERR_BADPHANDLE:
		case -FDT_ERR_BADSTATE:
		case -FDT_ERR_TRUNCATED:
		case -FDT_ERR_BADMAGIC:
		case -FDT_ERR_BADVERSION:
		case -FDT_ERR_BADSTRUCTURE:
		case -FDT_ERR_BADLAYOUT:
		case -FDT_ERR_INTERNAL:
		default:
			return OF_FAILED;
	}
}


status_t
of_init(int (*openFirmwareEntry)(void *))
{
	if (gFDT == NULL)
		return OF_FAILED;

	gChosen = fdt2of(fdt_path_offset(gFDT, "/chosen"));

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
	const char *name = device;
	int node;

	if (device == NULL)
		return OF_FAILED;

	if (device[0] != '/' && fdt_get_alias(gFDT, device))
		name = fdt_get_alias(gFDT, device);

	node = fdt_path_offset(gFDT, name);
	TRACE(("of_finddevice('%s') name='%s' node=%d ret=%d\n", device, name, node,
		fdt2of(node)));
	return fdt2of(node);
}


status_t
of_get_next_device(int *_cookie, int root, const char *type, char *path,
	size_t pathSize)
{
	int next;
	int err;

	int startoffset = *_cookie ? *_cookie : -1;

	// iterate by property value
	next = fdt_node_offset_by_prop_value(gFDT, startoffset, "device_type",
		type, strlen(type) + 1);
	TRACE(("of_get_next_device(c:%d, %d, '%s', %p, %zd) soffset=%d next=%d\n",
		*_cookie, root, type, path, pathSize, startoffset, fdt2of(next)));
	if (next < 0)
		return B_ENTRY_NOT_FOUND;

	// make sure root is the parent
	int parent = next;
	while (root && parent) {
		parent = fdt_parent_offset(gFDT, parent);
		if (parent == root)
			break;
		if (parent <= 0)
			return B_ENTRY_NOT_FOUND;
	}

	*_cookie = next;

	err = fdt_get_path(gFDT, next, path, pathSize);
	if (err < 0)
		return B_ERROR;

	return B_OK;
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
	int oldSize = bufferSize;
	const void *p = fdt_getprop(gFDT, package, property, &bufferSize);

	TRACE(("of_getprop(%d, '%s', , %d) =%p sz=%d ret=%d\n", package, property,
		oldSize, p, bufferSize, fdt2of(bufferSize)));

	if (p == NULL)
		return fdt2of(bufferSize);

	memcpy(buffer, p, bufferSize);
	return bufferSize;
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


/** given the package provided, get the number of cells
+   in the reg property
+ */

int32
of_address_cells(int package) {
	uint32 address_cells;
	if (of_getprop(package, "#address-cells",
		&address_cells, sizeof(address_cells)) == OF_FAILED)
		return OF_FAILED;

	return address_cells;
}


int32
of_size_cells(int package) {
	uint32 size_cells;
	if (of_getprop(package, "#size-cells",
		&size_cells, sizeof(size_cells)) == OF_FAILED)
		return OF_FAILED;

	return size_cells;
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

