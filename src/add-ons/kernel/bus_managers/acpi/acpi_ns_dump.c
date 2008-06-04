/* ++++++++++
	ACPI namespace dump. 
	Nothing special here, just tree enumeration and type identification.
+++++ */

#include <Drivers.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "acpi_priv.h"


typedef struct acpi_ns_device_info {
	device_node *node;
	acpi_root_info	*acpi;
	void	*acpi_cookie;
} acpi_ns_device_info;


static void 
dump_acpi_namespace(acpi_ns_device_info *device, char *root, void *buf, size_t* num_bytes, int indenting) {
	char result[255];
	char output[255];
	char tabs[255];
	char hid[9];
	int i, depth;
	uint32 type;
	void *counter = NULL;
	
	hid[8] = '\0';
	tabs[0] = '\0';

	for (i = 0; i < indenting; i++) {
		sprintf(tabs, "%s|    ", tabs);
	}
	sprintf(tabs, "%s|--- ", tabs);
	depth = sizeof(char) * 5 * indenting + sizeof(char); // index into result where the device name will be.
	
	dprintf("acpi_ns_dump: recursing from %s\n", root);
	while (device->acpi->get_next_entry(ACPI_TYPE_ANY, root, result, 255, &counter) == B_OK) {
		type = device->acpi->get_object_type(result);
		sprintf(output, "%s%s", tabs, result + depth);
		switch(type) {
			case ACPI_TYPE_ANY: 
				break;
			case ACPI_TYPE_INTEGER:
				sprintf(output, "%s     INTEGER", output);
				break;
			case ACPI_TYPE_STRING:
				sprintf(output, "%s     STRING", output);
				break;
			case ACPI_TYPE_BUFFER:
				sprintf(output, "%s     BUFFER", output);
				break;
			case ACPI_TYPE_PACKAGE:
				sprintf(output, "%s     PACKAGE", output);
				break;
			case ACPI_TYPE_FIELD_UNIT:
				sprintf(output, "%s     FIELD UNIT", output);
				break;
			case ACPI_TYPE_DEVICE:
				device->acpi->get_device_hid(result, hid);
				sprintf(output, "%s     DEVICE (%s)", output, hid);
				break;
			case ACPI_TYPE_EVENT:
				sprintf(output, "%s     EVENT", output);
				break;
			case ACPI_TYPE_METHOD:
				sprintf(output, "%s     METHOD", output);
				break;
			case ACPI_TYPE_MUTEX:
				sprintf(output, "%s     MUTEX", output);
				break;
			case ACPI_TYPE_REGION:
				sprintf(output, "%s     REGION", output);
				break;
			case ACPI_TYPE_POWER:
				sprintf(output, "%s     POWER", output);
				break;
			case ACPI_TYPE_PROCESSOR:
				sprintf(output, "%s     PROCESSOR", output);
				break;
			case ACPI_TYPE_THERMAL:
				sprintf(output, "%s     THERMAL", output);
				break;
			case ACPI_TYPE_BUFFER_FIELD:
				sprintf(output, "%s     BUFFER_FIELD", output);
				break;
		}
		sprintf(output, "%s\n", output);
		
		sprintf((buf + *num_bytes), "%s", output);
		*num_bytes += strlen(output);
		
		dump_acpi_namespace(device, result, buf, num_bytes, indenting + 1);
	}
}


/* ----------
	acpi_namespace_open - handle open() calls
----- */

static status_t
acpi_namespace_open(void *_cookie, const char* path, int flags, void** cookie)
{
	acpi_ns_device_info *device = (acpi_ns_device_info *)_cookie;
	dprintf("\nacpi_ns_dump: device_open\n");
	*cookie = device;
	
	return B_OK;
}


/* ----------
	acpi_namespace_read - handle read() calls
----- */
static status_t
acpi_namespace_read(void *_cookie, off_t position, void *buf, size_t* num_bytes)
{
	acpi_ns_device_info *device = (acpi_ns_device_info *)_cookie;
	size_t bytes = 0;
	
	if (position == 0) { // First read
		dump_acpi_namespace(device, "\\", buf, &bytes, 0);
		if (bytes <= *num_bytes) {
			*num_bytes = bytes;
			dprintf("acpi_ns_dump: read %lu bytes\n", *num_bytes);
		} else {
			*num_bytes = 0;
			return B_IO_ERROR;
		}
	} else {
		*num_bytes = 0;
	}
	
	return B_OK;
}


/* ----------
	acpi_namespace_write - handle write() calls
----- */

static status_t
acpi_namespace_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	dprintf("acpi_ns_dump: device_write\n");
	*num_bytes = 0;				/* tell caller nothing was written */
	return B_IO_ERROR;
}


/* ----------
	acpi_namespace_control - handle ioctl calls
----- */

static status_t
acpi_namespace_control(void* cookie, uint32 op, void* arg, size_t len)
{
	dprintf("acpi_ns_dump: device_control\n");
	return B_BAD_VALUE;
}


/* ----------
	acpi_namespace_close - handle close() calls
----- */

static status_t
acpi_namespace_close(void* cookie)
{
	dprintf("acpi_ns_dump: device_close\n");
	return B_OK;
}


/* -----
	acpi_namespace_free - called after the last device is closed, and after
	all i/o is complete.
----- */
static status_t
acpi_namespace_free(void* cookie)
{
	dprintf("acpi_ns_dump: device_free\n");

	return B_OK;
}


//	#pragma mark - device module API


static status_t
acpi_namespace_init_device(void *_cookie, void **cookie)
{
	device_node *node = (device_node *)_cookie;
	status_t err;
	
	acpi_ns_device_info *device = (acpi_ns_device_info *)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;
	err = gDeviceManager->get_driver(node, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);
	if (err != B_OK) {
		free(device);
		return err;
	}
	
	*cookie = device;
	return B_OK;
}


static void
acpi_namespace_uninit_device(void *_cookie)
{
	acpi_ns_device_info *device = (acpi_ns_device_info *)_cookie;
	free(device);
}


struct device_module_info acpi_ns_dump_module = {
	{
		ACPI_NS_DUMP_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	acpi_namespace_init_device,
	acpi_namespace_uninit_device,
	NULL,
		
	acpi_namespace_open,
	acpi_namespace_close,
	acpi_namespace_free,
	acpi_namespace_read,
	acpi_namespace_write,
	NULL,
	acpi_namespace_control,

	NULL,
	NULL
};
