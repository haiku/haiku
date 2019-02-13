/*
 * Copyright 2006-2013, Jérôme Duval. All rights reserved.
 * Copyright 2011-2012, Fredrik Holmqvis. All rights reserved.
 * Copyright 2008, Stefano Ceccherini. All rights reserved.
 * Copyright 2006, Bryan Varner. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Drivers.h>

#include <util/kernel_cpp.h>
#include <util/ring_buffer.h>

#include "ACPIPrivate.h"


class RingBuffer {
public:
	RingBuffer(size_t size = 1024);
	~RingBuffer();
	size_t Read(void *buffer, ssize_t length);
	size_t Write(const void *buffer, ssize_t length);
	size_t WritableAmount() const;
	size_t ReadableAmount() const;

	bool Lock();
	void Unlock();
	void DestroyLock();
private:
	ring_buffer *fBuffer;
	sem_id fLock;
};


typedef struct acpi_ns_device_info {
	device_node *node;
	acpi_root_info	*acpi;
	void	*acpi_cookie;
	thread_id thread;
	sem_id read_sem;
	RingBuffer *buffer;
} acpi_ns_device_info;



// called with the buffer lock held
static bool
make_space(acpi_ns_device_info *device, size_t space)
{
	size_t available = device->buffer->WritableAmount();
	if (space <= available)
		return true;
	bool released = false;
	do {
		device->buffer->Unlock();

		if (!released) {
			if (release_sem_etc(device->read_sem, 1, B_RELEASE_IF_WAITING_ONLY) == B_OK)
				released = true;
		}
		snooze(10000);

		if (!device->buffer->Lock())
			return false;

	} while (device->buffer->WritableAmount() < space);

	return true;
}



static void
dump_acpi_namespace(acpi_ns_device_info *device, char *root, int indenting)
{
	char result[255];
	char output[320];
	char tabs[255] = "";
	char hid[16] = "";
	int i;
	size_t written = 0;
	for (i = 0; i < indenting; i++)
		strlcat(tabs, "|    ", sizeof(tabs));

	strlcat(tabs, "|--- ", sizeof(tabs));

	int depth = sizeof(char) * 5 * indenting + sizeof(char); // index into result where the device name will be.

	void *counter = NULL;
	while (device->acpi->get_next_entry(ACPI_TYPE_ANY, root, result, 255, &counter) == B_OK) {
		uint32 type = device->acpi->get_object_type(result);
		snprintf(output, sizeof(output), "%s%s", tabs, result + depth);
		switch(type) {
			case ACPI_TYPE_INTEGER:
				strncat(output, "     INTEGER", sizeof(output));
				break;
			case ACPI_TYPE_STRING:
				strncat(output, "     STRING", sizeof(output));
				break;
			case ACPI_TYPE_BUFFER:
				strncat(output, "     BUFFER", sizeof(output));
				break;
			case ACPI_TYPE_PACKAGE:
				strncat(output, "     PACKAGE", sizeof(output));
				break;
			case ACPI_TYPE_FIELD_UNIT:
				strncat(output, "     FIELD UNIT", sizeof(output));
				break;
			case ACPI_TYPE_DEVICE:
				hid[0] = 0; /* zero-terminate string; get_device_hid can (and will) fail! */
				device->acpi->get_device_hid(result, hid, sizeof(hid));
				strncat(output, "     DEVICE (", sizeof(output));
				strncat(output, hid, sizeof(output));
				strncat(output, ")", sizeof(output));
				break;
			case ACPI_TYPE_EVENT:
				strncat(output, "     EVENT", sizeof(output));
				break;
			case ACPI_TYPE_METHOD:
				strncat(output, "     METHOD", sizeof(output));
				break;
			case ACPI_TYPE_MUTEX:
				strncat(output, "     MUTEX", sizeof(output));
				break;
			case ACPI_TYPE_REGION:
				strncat(output, "     REGION", sizeof(output));
				break;
			case ACPI_TYPE_POWER:
				strncat(output, "     POWER", sizeof(output));
				break;
			case ACPI_TYPE_PROCESSOR:
				strncat(output, "     PROCESSOR", sizeof(output));
				break;
			case ACPI_TYPE_THERMAL:
				strncat(output, "     THERMAL", sizeof(output));
				break;
			case ACPI_TYPE_BUFFER_FIELD:
				strncat(output, "     BUFFER_FIELD", sizeof(output));
				break;
			case ACPI_TYPE_ANY:
			default:
				break;
		}
		written = 0;
		RingBuffer &ringBuffer = *device->buffer;
		size_t toWrite = strlen(output);

		if (toWrite == 0)
			break;

		toWrite = strlcat(output, "\n", sizeof(output));

		if (!ringBuffer.Lock())
			break;

		if (ringBuffer.WritableAmount() < toWrite &&
			!make_space(device, toWrite))
			break;

		written = ringBuffer.Write(output, toWrite);
		ringBuffer.Unlock();
		dump_acpi_namespace(device, result, indenting + 1);
	}
}


static int32
acpi_namespace_dump(void *arg)
{
	acpi_ns_device_info *device = (acpi_ns_device_info*)(arg);
	dump_acpi_namespace(device, NULL, 0);

	delete_sem(device->read_sem);
	device->read_sem = -1;

	return 0;
}

extern "C" {
/* ----------
	acpi_namespace_open - handle open() calls
----- */

static status_t
acpi_namespace_open(void *_cookie, const char* path, int flags, void** cookie)
{
	acpi_ns_device_info *device = (acpi_ns_device_info *)_cookie;

	dprintf("\nacpi_ns_dump: device_open\n");

	*cookie = device;

	RingBuffer *ringBuffer = new RingBuffer(1024);
	if (ringBuffer == NULL)
		return B_NO_MEMORY;

	device->read_sem = create_sem(0, "read_sem");
	if (device->read_sem < B_OK) {
		delete ringBuffer;
		return device->read_sem;
	}

	device->thread = spawn_kernel_thread(acpi_namespace_dump, "acpi dumper",
		 B_NORMAL_PRIORITY, device);
	if (device->thread < 0) {
		delete ringBuffer;
		delete_sem(device->read_sem);
		return device->thread;
	}

	device->buffer = ringBuffer;

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
	RingBuffer &ringBuffer = *device->buffer;

	if (!ringBuffer.Lock()) {
		*num_bytes = 0;
		return B_ERROR;
	}

	if (ringBuffer.ReadableAmount() == 0) {
		ringBuffer.Unlock();
		status_t status = acquire_sem_etc(device->read_sem, 1, B_CAN_INTERRUPT, 0);
		if (status != B_OK && status != B_BAD_SEM_ID) {
			*num_bytes = 0;
			return status;
		}
		if (!ringBuffer.Lock()) {
			*num_bytes = 0;
			return B_ERROR;
		}
	}

	*num_bytes = ringBuffer.Read(buf, *num_bytes);
	ringBuffer.Unlock();

	return B_OK;
}


/* ----------
	acpi_namespace_write - handle write() calls
----- */

static status_t
acpi_namespace_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
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
	return B_DEV_INVALID_IOCTL;
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
	status_t status;
	acpi_ns_device_info *device = (acpi_ns_device_info *)cookie;
	dprintf("acpi_ns_dump: device_free\n");

	if (device->read_sem >= 0)
		delete_sem(device->read_sem);

	device->buffer->DestroyLock();
	wait_for_thread(device->thread, &status);
	delete device->buffer;

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


RingBuffer::RingBuffer(size_t size)
{
	fBuffer = create_ring_buffer(size);
	fLock = create_sem(1, "ring buffer lock");
}


RingBuffer::~RingBuffer()
{
	delete_ring_buffer(fBuffer);
}


size_t
RingBuffer::Read(void *buffer, ssize_t size)
{
	return ring_buffer_read(fBuffer, (uint8*)buffer, size);
}


size_t
RingBuffer::Write(const void *buffer, ssize_t size)
{
	return ring_buffer_write(fBuffer, (uint8*)buffer, size);
}


size_t
RingBuffer::ReadableAmount() const
{
	return ring_buffer_readable(fBuffer);
}


size_t
RingBuffer::WritableAmount() const
{
	return ring_buffer_writable(fBuffer);
}


bool
RingBuffer::Lock()
{
	//status_t status = acquire_sem_etc(fLock, 1, B_CAN_INTERRUPT, 0);
	status_t status = acquire_sem(fLock);
	return status == B_OK;
}


void
RingBuffer::Unlock()
{
	release_sem(fLock);
}


void
RingBuffer::DestroyLock()
{
	delete_sem(fLock);
}

