/*
 * Copyright 2007-2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Drivers.h>
#include <KernelExport.h>

#include <string.h>
#include <unistd.h>
#include <sys/types.h>


#define DRIVER_NAME "mem"
#define DEVICE_NAME "misc/mem"

#define DEVMNT "/dev/"

/* also publish /dev/mem */
#define PUBLISH_DEV_MEM

static status_t mem_open(const char*, uint32, void**);
static status_t mem_close(void*);
static status_t mem_free(void*);
static status_t mem_read(void*, off_t, void*, size_t*);
static status_t mem_write(void*, off_t, const void*, size_t*);

static area_id mem_map_target(off_t position, size_t length, uint32 protection,
	void **virtualAddress);

static const char* mem_name[] = {
	DEVICE_NAME,
#ifdef PUBLISH_DEV_MEM
	DRIVER_NAME,
#endif
	NULL
};


device_hooks mem_hooks = {
	mem_open,
	mem_close,
	mem_free,
	NULL, /*mem_control,*/
	mem_read,
	mem_write,
};

int32 api_version = B_CUR_DRIVER_API_VERSION;


status_t
init_hardware(void)
{
	return B_OK;
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


const char**
publish_devices(void)
{
	return mem_name;
}


device_hooks*
find_device(const char* name)
{
	return &mem_hooks;
}


status_t
mem_open(const char* name, uint32 flags, void** cookie)
{
	// not really needed.
	*cookie = NULL;
	return B_OK;
}


status_t
mem_close(void* cookie)
{
	return B_OK;
}


status_t
mem_free(void* cookie)
{
	return B_OK;
}


status_t
mem_read(void* cookie, off_t position, void* buffer, size_t* numBytes)
{
	void *virtualAddress;
	area_id area;
	status_t status = B_OK;

	/* check permissions */
	if (getuid() != 0 && geteuid() != 0) {
		*numBytes = 0;
		return EPERM;
	}

	area = mem_map_target(position, *numBytes, B_READ_AREA, &virtualAddress);
	if (area < 0) {
		*numBytes = 0;
		return area;
	}

	if (user_memcpy(buffer, virtualAddress, *numBytes) != B_OK)
		status = B_BAD_ADDRESS;

	delete_area(area);
	return status;
}


status_t
mem_write(void* cookie, off_t position, const void* buffer, size_t* numBytes)
{
	void *virtualAddress;
	area_id area;
	status_t status = B_OK;

	/* check permissions */
	if (getuid() != 0 && geteuid() != 0) {
		*numBytes = 0;
		return EPERM;
	}

	area = mem_map_target(position, *numBytes, B_WRITE_AREA, &virtualAddress);
	if (area < 0) {
		*numBytes = 0;
		return area;
	}

	if (user_memcpy(virtualAddress, buffer, *numBytes) != B_OK)
		status = B_BAD_ADDRESS;

	delete_area(area);
	return status;
}


area_id
mem_map_target(off_t position, size_t length, uint32 protection,
	void **virtualAddress)
{
	area_id area;
	phys_addr_t physicalAddress;
	size_t offset;
	size_t size;

	/* SIZE_MAX actually but 2G should be enough anyway */
	if (length > SSIZE_MAX - B_PAGE_SIZE)
		return EINVAL;

	/* the first page address */
	physicalAddress = (phys_addr_t)position & ~((off_t)B_PAGE_SIZE - 1);

	/* offset of target into it */
	offset = position - (off_t)physicalAddress;

	/* size of the whole mapping (page rounded) */
	size = (offset + length + B_PAGE_SIZE - 1) & ~((size_t)B_PAGE_SIZE - 1);
	area = map_physical_memory("mem_driver_temp", physicalAddress, size,
		B_ANY_KERNEL_ADDRESS, protection, virtualAddress);
	if (area < 0)
		return area;

	*virtualAddress = (void*)((addr_t)(*virtualAddress) + offset);
	return area;
}
