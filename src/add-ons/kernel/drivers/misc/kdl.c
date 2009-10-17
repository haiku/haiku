/*
 * Copyright (c) 2009 François Revol, <revol@free.fr>.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>

int32 			api_version = B_CUR_DRIVER_API_VERSION;
static int32	sOpenMask;

status_t
init_hardware(void)
{
	dprintf("kdl: init_hardware\n");
	return B_OK;
}


status_t
uninit_hardware(void)
{
	dprintf("kdl: uninit_hardware\n");
	return B_OK;
}


status_t
init_driver(void)
{
	dprintf("kdl: init_driver\n");
	return B_OK;
}


void
uninit_driver(void)
{
	dprintf("kdl: uninit_driver\n");
}


static status_t
driver_open(const char *name, uint32 flags, void** _cookie)
{
	dprintf("kdl: open\n");

	// TODO: check for proper credencials! (root only ?)

	if (atomic_or(&sOpenMask, 1)) {
		dprintf("kdl: open, BUSY!\n");
		return B_BUSY;
	}

	dprintf("kdl: open, success\n");
	return B_OK;
}


static status_t
driver_close(void* cookie)
{
	dprintf("kdl: close enter\n");
	dprintf("kdl: close leave\n");
	return B_OK;
}


static status_t
driver_free(void* cookie)
{
	dprintf("kdl: free\n");
	atomic_and(&sOpenMask, ~1);
	return B_OK;
}


static status_t
driver_read(void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	dprintf("kdl: read\n");
	panic("requested from kdl driver.");
	*num_bytes = 0; // nothing to read
	return B_ERROR;
}


static status_t
driver_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	dprintf("kdl: write\n");
	*num_bytes = 1;		// pretend 1 byte was written
	return B_OK;
}


static status_t
driver_control(void *cookie, uint32 op, void *arg, size_t len)
{
	dprintf("kdl: control\n");
	return B_ERROR;
}


const char**
publish_devices(void)
{
	static const char *names[] = {"misc/kdl", NULL};
	dprintf("kdl: publish_devices\n");
	return names;
}


device_hooks*
find_device(const char* name)
{
	static device_hooks hooks = {
		driver_open,
		driver_close,
		driver_free,
		driver_control,
		driver_read,
		driver_write,
	};
	dprintf("kdl: find_device\n");
	return &hooks;
}

