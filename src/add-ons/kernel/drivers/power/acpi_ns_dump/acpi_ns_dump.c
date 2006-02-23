/* ++++++++++
	ACPI namespace dump. 
	Nothing special here, just tree enumeration and type identification.
+++++ */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include <ACPI.h>

acpi_module_info *acpi;

/* ----------
	init_hardware - called once the first time the driver is loaded
----- */
status_t
init_hardware (void)
{
	return B_OK;
}


/* ----------
	init_driver - optional function - called every time the driver
	is loaded.
----- */
status_t
init_driver (void)
{
	dprintf("acpi_ns_dump: init_driver\n");
	return get_module(B_ACPI_MODULE_NAME,(module_info **)&acpi);
}


/* ----------
	uninit_driver - optional function - called every time the driver
	is unloaded
----- */
void
uninit_driver (void)
{
	dprintf("acpi_ns_dump: uninit_driver\n");
	put_module(B_ACPI_MODULE_NAME);
}

void dump_acpi_namespace(char *root, void *buf, size_t* num_bytes, int indenting) {
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
	
	while (acpi->get_next_entry(ACPI_TYPE_ANY, root, result, 255, &counter) == B_OK) {
		type = acpi->get_object_type(result);
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
				acpi->get_device_hid(result, hid);
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
		
		dump_acpi_namespace(result, buf, num_bytes, indenting + 1);
	}
}


/* ----------
	my_device_open - handle open() calls
----- */

static status_t
my_device_open (const char *name, uint32 flags, void** cookie)
{
	dprintf("\nacpi_ns_dump: device_open\n");
	*cookie = NULL;
	
	return B_OK;
}


/* ----------
	my_device_read - handle read() calls
----- */
static status_t
my_device_read (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	size_t bytes = 0;
	
	if (position == 0) { // First read
		dump_acpi_namespace("\\", buf, &bytes, 0);
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
	my_device_write - handle write() calls
----- */

static status_t
my_device_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	dprintf("acpi_ns_dump: device_write\n");
	*num_bytes = 0;				/* tell caller nothing was written */
	return B_IO_ERROR;
}


/* ----------
	my_device_control - handle ioctl calls
----- */

static status_t
my_device_control (void* cookie, uint32 op, void* arg, size_t len)
{
	dprintf("acpi_nd_dump: device_control\n");
	return B_BAD_VALUE;
}


/* ----------
	my_device_close - handle close() calls
----- */

static status_t
my_device_close (void* cookie)
{
	dprintf("acpi_ns_dump: device_close\n");
	return B_OK;
}


/* -----
	my_device_free - called after the last device is closed, and after
	all i/o is complete.
----- */
static status_t
my_device_free (void* cookie)
{
	dprintf("acpi_ns_dump: device_free\n");

	return B_OK;
}


/* -----
	null-terminated array of device names supported by this driver
----- */

static const char *my_device_name[] = {
	"power/namespace",
	NULL
};

/* -----
	function pointers for the device hooks entry points
----- */

device_hooks my_device_hooks = {
	my_device_open, 			/* -> open entry point */
	my_device_close, 			/* -> close entry point */
	my_device_free,			/* -> free cookie */
	my_device_control, 		/* -> control entry point */
	my_device_read,			/* -> read entry point */
	my_device_write			/* -> write entry point */
};

/* ----------
	publish_devices - return a null-terminated array of devices
	supported by this driver.
----- */

const char**
publish_devices()
{
	dprintf("acpi_ns_driver: publish_devices\n");
	return my_device_name;
}

/* ----------
	find_device - return ptr to device hooks structure for a
	given device name
----- */

device_hooks*
find_device(const char* name)
{
	dprintf("acpi_ns_driver: find_device(%s)\n", name);
	return &my_device_hooks;
}
