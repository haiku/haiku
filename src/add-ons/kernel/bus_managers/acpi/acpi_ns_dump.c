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

#include <util/ring_buffer.h>

typedef struct acpi_ns_device_info {
	device_node *node;
	acpi_root_info	*acpi;
	void	*acpi_cookie;
	thread_id thread;
	struct ring_buffer *buffer;
	sem_id write_sem;
	sem_id sync_sem;
} acpi_ns_device_info;



static void 
dump_acpi_namespace(acpi_ns_device_info *device, char *root, int indenting) 
{
	char result[255];
	char output[255];
	char tabs[255];
	char hid[9];
	int i, depth;
	uint32 type;
	void *counter = NULL;
	size_t written = 0;
	hid[8] = '\0';
	tabs[0] = '\0';
	for (i = 0; i < indenting; i++) {
		sprintf(tabs, "%s|    ", tabs);
	}
	snprintf(tabs, sizeof(tabs), "%s|--- ", tabs);
	depth = sizeof(char) * 5 * indenting + sizeof(char); // index into result where the device name will be.
	
	//dprintf("acpi_ns_dump: recursing from %s, depth %d\n", root, depth);
	while (device->acpi->get_next_entry(ACPI_TYPE_ANY, root, result, 255, &counter) == B_OK) {
		type = device->acpi->get_object_type(result);
		snprintf(output, sizeof(output), "%s%s", tabs, result + depth);
		switch(type) {
			case ACPI_TYPE_ANY:
			default: 
				break;
			case ACPI_TYPE_INTEGER:
				snprintf(output, sizeof(output), "%s     INTEGER", output);
				break;
			case ACPI_TYPE_STRING:
				snprintf(output, sizeof(output), "%s     STRING", output);
				break;
			case ACPI_TYPE_BUFFER:
				snprintf(output, sizeof(output), "%s     BUFFER", output);
				break;
			case ACPI_TYPE_PACKAGE:
				snprintf(output, sizeof(output), "%s     PACKAGE", output);
				break;
			case ACPI_TYPE_FIELD_UNIT:
				snprintf(output, sizeof(output), "%s     FIELD UNIT", output);
				break;
			case ACPI_TYPE_DEVICE:
				device->acpi->get_device_hid(result, hid);
				snprintf(output, sizeof(output), "%s     DEVICE (%s)", output, hid);
				break;
			case ACPI_TYPE_EVENT:
				snprintf(output, sizeof(output), "%s     EVENT", output);
				break;
			case ACPI_TYPE_METHOD:
				snprintf(output, sizeof(output), "%s     METHOD", output);
				break;
			case ACPI_TYPE_MUTEX:
				snprintf(output, sizeof(output), "%s     MUTEX", output);
				break;
			case ACPI_TYPE_REGION:
				snprintf(output, sizeof(output), "%s     REGION", output);
				break;
			case ACPI_TYPE_POWER:
				snprintf(output, sizeof(output), "%s     POWER", output);
				break;
			case ACPI_TYPE_PROCESSOR:
				snprintf(output, sizeof(output), "%s     PROCESSOR", output);
				break;
			case ACPI_TYPE_THERMAL:
				snprintf(output, sizeof(output), "%s     THERMAL", output);
				break;
			case ACPI_TYPE_BUFFER_FIELD:
				snprintf(output, sizeof(output), "%s     BUFFER_FIELD", output);
				break;
		}
		strcat(output, "\n");
		written = 0;
		if (acquire_sem(device->sync_sem) == B_OK) {
			//dprintf("writing %ld bytes to the buffer.\n", strlen(output));
			written = ring_buffer_write(device->buffer, output, strlen(output));
			//dprintf("written %ld bytes\n", written);
			release_sem(device->sync_sem);
		}

		if (written > 0)
			release_sem_etc(device->write_sem, 1, 0);

		dump_acpi_namespace(device, result, indenting + 1);
	}
}


static int32
acpi_namespace_dump(void *arg)
{
	acpi_ns_device_info *device = (acpi_ns_device_info*)(arg);
        dump_acpi_namespace(device, "\\", 0);
	release_sem(device->write_sem);
	return 0;
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

	device->buffer = create_ring_buffer(2048);

	device->write_sem = create_sem(0, "sem");
	if (device->write_sem < 0)
		return device->write_sem;

	device->sync_sem = create_sem(1, "sync sem");
	if (device->sync_sem < 0) {
		delete_sem(device->write_sem);
		return device->sync_sem;
	}

	device->thread = spawn_kernel_thread(acpi_namespace_dump, "acpi dumper",
		 B_NORMAL_PRIORITY, device);
	if (device->thread < 0) {
		delete_sem(device->write_sem);
		delete_sem(device->sync_sem);
		return device->thread;
	}
	resume_thread(device->thread);
	return B_OK;
}


/* ----------
	acpi_namespace_read - handle read() calls
----- */
static status_t
acpi_namespace_read(void *_cookie, off_t position, void *buf, size_t* num_bytes)
{
	acpi_ns_device_info *device = (acpi_ns_device_info *)_cookie;
	size_t bytesRead = -1; 
	size_t bytesToRead = 0;
	status_t status;
        dprintf("acpi_namespace_read(cookie: %p, position: %lld, buffer: %p, size: %ld)\n",
                        _cookie, position, buf, *num_bytes);

	status = acquire_sem_etc(device->write_sem, 1, 0, 0);
	if (status == B_OK) {
		if (acquire_sem(device->sync_sem) == B_OK) {
        		bytesToRead = ring_buffer_readable(device->buffer);
                        bytesRead = ring_buffer_read(device->buffer, buf, bytesToRead);
                	release_sem(device->sync_sem);
		}
        }

	if (bytesRead < 0) {
		*num_bytes = 0;
		return bytesRead;
	}

	*num_bytes = bytesRead;
	
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
	status_t status;
	acpi_ns_device_info *device = (acpi_ns_device_info *)cookie;
	
	dprintf("acpi_ns_dump: device_close\n");

	delete_sem(device->write_sem);
	delete_sem(device->sync_sem);
	wait_for_thread(device->thread, &status);

	delete_ring_buffer(device->buffer);
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
