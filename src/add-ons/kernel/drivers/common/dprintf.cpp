/*
 * Copyright 2005-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author(s):
 *   Ingo Weinhold <bonefish@users.sourceforge.net>
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <debug.h>

#include <Drivers.h>
#include <KernelExport.h>

#include <string.h>
#include <termios.h>


#define DEVICE_NAME "dprintf"

int32 api_version = B_CUR_DRIVER_API_VERSION;


static status_t
dprintf_open(const char *name, uint32 flags, void **cookie)
{
	*cookie = NULL;
	return B_OK;
}


static status_t
dprintf_close(void *cookie)
{
	return B_OK;
}


static status_t
dprintf_freecookie(void *cookie)
{
	return B_OK;
}


static status_t
dprintf_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	if (op == TCGETA) {
		// let isatty() think we are a terminal
		// (this lets libroot use unbuffered I/O)
		return B_OK;
	}

	return EPERM;
}


static status_t
dprintf_read(void *cookie, off_t pos, void *buffer, size_t *length)
{
	*length = 0;
	return B_OK;
}


static status_t
dprintf_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	const char *str = (const char*)buffer;

	int bytesLeft = *_length;
	while (bytesLeft > 0) {
		int chunkSize = strnlen(str, bytesLeft);
		if (chunkSize == 0) {
			// null bytes -- skip
			str++;
			bytesLeft--;
			continue;
		}

		if (chunkSize == bytesLeft) {
			// no null-byte in the remainder of the buffer
			// we need to copy to a local buffer and null-terminate
			while (bytesLeft > 0) {
				chunkSize = bytesLeft;

				char localBuffer[512];
				if (bytesLeft > (int)sizeof(localBuffer) - 1)
					chunkSize = (int)sizeof(localBuffer) - 1;
				memcpy(localBuffer, str, chunkSize);
				localBuffer[chunkSize] = '\0';

				debug_puts(localBuffer, chunkSize);

				str += chunkSize;
				bytesLeft -= chunkSize;
			}
		} else {
			// null-terminated chunk -- just write it
			debug_puts(str, chunkSize);

			str += chunkSize + 1;
			bytesLeft -= chunkSize + 1;
		}
	}

	return B_OK;
}


//	#pragma mark -


status_t
init_hardware(void)
{
	return B_OK;
}


const char **
publish_devices(void)
{
	static const char *devices[] = {
		DEVICE_NAME, 
		NULL
	};

	return devices;
}


device_hooks *
find_device(const char *name)
{
	static device_hooks hooks = {
		&dprintf_open,
		&dprintf_close,
		&dprintf_freecookie,
		&dprintf_ioctl,
		&dprintf_read,
		&dprintf_write,
		/* Leave select/deselect/readv/writev undefined. The kernel will
		 * use its own default implementation. The basic hooks above this
		 * line MUST be defined, however. */
		NULL,
		NULL,
		NULL,
		NULL
	};

	if (!strcmp(name, DEVICE_NAME))
		return &hooks;

	return NULL;
}


status_t
init_driver(void)
{
	return B_OK;
}


void
uninit_driver(void)
{
}

