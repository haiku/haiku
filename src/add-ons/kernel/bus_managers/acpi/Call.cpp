/*
 * Copyright 2022, Jérôme Duval. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>

#include "ACPIPrivate.h"


#define	TRACE(x...)			//dprintf("acpi_call: " x)
#define TRACE_ALWAYS(x...)	dprintf("acpi_call: " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


typedef struct {
	device_node *node;
	acpi_root_info	*acpi;
	void	*acpi_cookie;
} acpi_call_device_info;


struct acpi_call_desc
{
	char*			path;
	acpi_objects	args;
	acpi_status	retval;
	acpi_data	result;
	acpi_size	reslen;
};


//	#pragma mark - device module API


static status_t
acpi_call_init_device(void* _node, void** _cookie)
{
	CALLED();
	device_node *node = (device_node *)_node;

	acpi_call_device_info* device = (acpi_call_device_info*)calloc(1, sizeof(acpi_call_device_info));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;
	status_t err = gDeviceManager->get_driver(node, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);
	if (err != B_OK) {
		free(device);
		return err;
	}

	*_cookie = device;
	return err;
}


static void
acpi_call_uninit_device(void* _cookie)
{
	CALLED();
	acpi_call_device_info* device = (acpi_call_device_info*)_cookie;
	free(device);
}


static status_t
acpi_call_open(void* _device, const char* path, int openMode, void** _cookie)
{
	CALLED();
	acpi_call_device_info* device = (acpi_call_device_info*)_device;

	*_cookie = device;
	return B_OK;
}


static status_t
acpi_call_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	TRACE("read(%p, %" B_PRIdOFF", %p, %lu)\n", cookie, position, buffer, *numBytes);
	return B_ERROR;
}


static status_t
acpi_call_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	TRACE("write(%p, %" B_PRIdOFF", %p, %lu)\n", cookie, position, buffer, *numBytes);
	return B_ERROR;
}


void
acpi_call_fixup_pointers(acpi_object_type *p, void *target)
{
	CALLED();
	switch (p->object_type)
	{
	case ACPI_TYPE_STRING:
		p->string.string = (char*)((uint8*)(p->string.string) - (uint8*)p + (uint8*)target);
		break;
	case ACPI_TYPE_BUFFER:
		p->buffer.buffer = (void*)((uint8*)(p->buffer.buffer) - (uint8*)p + (uint8*)target);
		break;
	}
}


static status_t
acpi_call_control(void *_device, uint32 op, void *buffer, size_t length)
{
	TRACE("control(%p, %" B_PRIu32 ", %p, %lu)\n", _device, op, buffer, length);
	acpi_call_device_info* device = (acpi_call_device_info*)_device;

	if (op == 'ACCA') {
		struct acpi_call_desc params;
		char path[1024];
		if (user_memcpy(&params, buffer, sizeof(params)) != B_OK
			|| user_memcpy(path, params.path, sizeof(path)) != B_OK) {
			return B_BAD_ADDRESS;
		}
		acpi_data result;
		result.length = ACPI_ALLOCATE_BUFFER;
		result.pointer = NULL;

		acpi_status retval = device->acpi->evaluate_method(NULL, path, &params.args, &result);
		if (retval == 0) {
			if (result.pointer != NULL) {
				if (params.result.pointer != NULL) {
					params.result.length = min_c(params.result.length, result.length);
					if (result.length >= sizeof(acpi_object_type))
						acpi_call_fixup_pointers((acpi_object_type*)(result.pointer), params.result.pointer);

					if (user_memcpy(params.result.pointer, result.pointer, params.result.length) != B_OK
						|| user_memcpy(buffer, &params, sizeof(params)) != B_OK) {
						return B_BAD_ADDRESS;
					}
				}
				free(result.pointer);
			}
		}
		return B_OK;
	}

	return B_ERROR;
}


static status_t
acpi_call_close(void *cookie)
{
	TRACE("close(%p)\n", cookie);
	return B_OK;
}


static status_t
acpi_call_free(void *cookie)
{
	TRACE("free(%p)\n", cookie);
	return B_OK;
}


//	#pragma mark -


struct device_module_info gAcpiCallDeviceModule = {
	{
		ACPI_CALL_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	acpi_call_init_device,
	acpi_call_uninit_device,
	NULL, // remove,

	acpi_call_open,
	acpi_call_close,
	acpi_call_free,
	acpi_call_read,
	acpi_call_write,
	NULL,	// io
	acpi_call_control,

	NULL,	// select
	NULL,	// deselect
};

