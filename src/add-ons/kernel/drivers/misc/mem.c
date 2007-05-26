/*
 * Copyright 2007, Haiku, Inc. All rights reserved.
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
//static status_t mem_control(void*, uint32, void*, size_t);
static status_t mem_read(void*, off_t, void*, size_t*);
static status_t mem_write(void*, off_t, const void*, size_t*);

static area_id mem_map_target(off_t position, size_t len, uint32 protection, void **va);

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

/*
status_t
mem_control(void* cookie, uint32 op, void* arg, size_t length)
{
	return ENOSYS;
}
*/

status_t
mem_read(void* cookie, off_t position, void* buffer, size_t* numBytes)
{
	status_t err;
	void *va;
	area_id area;
	
	/* check perms */
	err = EPERM;
	if (getuid() != 0 && geteuid() != 0)
		goto err1;
	err = area = mem_map_target(position, *numBytes, B_READ_AREA, &va);
	if (err < 0)
		goto err1;
	memcpy(buffer, va, *numBytes);
	delete_area(area);
	return B_OK;
err1:
	*numBytes = 0;
	return err;
}


status_t
mem_write(void* cookie, off_t position, const void* buffer, size_t* numBytes)
{
	status_t err;
	void *va;
	area_id area;
	
	/* check perms */
	err = EPERM;
	if (getuid() != 0 && geteuid() != 0)
		goto err1;
	err = area = mem_map_target(position, *numBytes, B_WRITE_AREA, &va);
	if (err < 0)
		goto err1;
	memcpy(va, buffer, *numBytes);
	delete_area(area);
	return B_OK;
err1:
	*numBytes = 0;
	return err;
}

area_id
mem_map_target(off_t position, size_t len, uint32 protection, void **va)
{
	area_id area;
	void *paddr;
	void *vaddr;
	int offset;
	size_t size;
	
	/* SIZE_MAX actually but 2G should be enough anyway */
	if (len > SSIZE_MAX - B_PAGE_SIZE)
		return EINVAL;
	
	/* the first page address */
	paddr = (void *)(addr_t)(position & ~((off_t)B_PAGE_SIZE - 1));
	/* offset of target into it */
	offset = position - (off_t)(addr_t)paddr;
	/* size of the whole mapping (page rounded) */
	size = (offset + len + B_PAGE_SIZE - 1) & ~((size_t)B_PAGE_SIZE - 1);
	
	area = map_physical_memory("mem_driver_temp",
								paddr, size, B_ANY_KERNEL_ADDRESS,
								protection, &vaddr);
	
	if (area < 0)
		return area;
	
	*va = vaddr + offset;
	
	return EINVAL;
}
