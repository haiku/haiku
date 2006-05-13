/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "accelerant.h"

#include "utility.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>


#define TRACE_ACCELERANT
#ifdef TRACE_ACCELERANT
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


struct accelerant_info *gInfo;


class AreaCloner {
	public:
		AreaCloner();
		~AreaCloner();

		area_id Clone(const char *name, void **_address, uint32 spec,
					uint32 protection, area_id sourceArea);
		status_t InitCheck() { return fArea < B_OK ? (status_t)fArea : B_OK; }
		void Keep();

	private:
		area_id	fArea;
};


AreaCloner::AreaCloner()
	:
	fArea(-1)
{
}


AreaCloner::~AreaCloner()
{
	if (fArea >= B_OK)
		delete_area(fArea);
}


area_id 
AreaCloner::Clone(const char *name, void **_address, uint32 spec,
	uint32 protection, area_id sourceArea)
{
	fArea = clone_area(name, _address, spec, protection, sourceArea);
	return fArea;
}


void 
AreaCloner::Keep()
{
	fArea = -1;
}


//	#pragma mark -


/** This is the common accelerant_info initializer. It is called by
 *	both, the first accelerant and all clones.
 */

static status_t
init_common(int device, bool isClone)
{
	// initialize global accelerant info structure

	gInfo = (accelerant_info *)malloc(sizeof(accelerant_info));
	if (gInfo == NULL)
		return B_NO_MEMORY;

	memset(gInfo, 0, sizeof(accelerant_info));

	gInfo->is_clone = isClone;
	gInfo->device = device;

	// get basic info from driver

	intel_get_private_data data;
	data.magic = INTEL_PRIVATE_DATA_MAGIC;

	if (ioctl(device, INTEL_GET_PRIVATE_DATA, &data, sizeof(intel_get_private_data)) != 0) {
		free(gInfo);
		return B_ERROR;
	}

	AreaCloner sharedCloner;
	gInfo->shared_info_area = sharedCloner.Clone("intel extreme shared info",
								(void **)&gInfo->shared_info, B_ANY_ADDRESS,
								B_READ_AREA | B_WRITE_AREA,
								data.shared_info_area);
	status_t status = sharedCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo);
		return status;
	}

	AreaCloner regsCloner;
	gInfo->regs_area = regsCloner.Clone("intel extreme regs",
							(void **)&gInfo->regs, B_ANY_ADDRESS,
							B_READ_AREA | B_WRITE_AREA,
							gInfo->shared_info->registers_area);
	status = regsCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo);
		return status;
	}

	AreaCloner cursorCloner;
	gInfo->cursor_area = cursorCloner.Clone("intel extreme cursor",
		(void **)&gInfo->cursor_memory, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA,
		gInfo->shared_info->cursor_area);
	if (cursorCloner.InitCheck() < B_OK) {
		// we can't do a hardware cursor then...
		gInfo->cursor_memory = NULL;
	}

	sharedCloner.Keep();
	regsCloner.Keep();
	cursorCloner.Keep();

	// The overlay registers, hardware status, and cursor memory share
	// a single area with the shared_info

	gInfo->overlay_registers = (struct overlay_registers *)((uint8 *)gInfo->shared_info
		+ ROUND_TO_PAGE_SIZE(sizeof(intel_shared_info)));
	gInfo->status = (hardware_status *)((uint8 *)gInfo->overlay_registers + B_PAGE_SIZE);
	gInfo->cursor_memory = (uint8 *)gInfo->overlay_registers + 2 * B_PAGE_SIZE;

	return B_OK;
}


/** Clean up data common to both primary and cloned accelerant */

static void
uninit_common(void)
{
	delete_area(gInfo->cursor_area);
	delete_area(gInfo->regs_area);
	delete_area(gInfo->shared_info_area);

	gInfo->regs_area = gInfo->shared_info_area = -1;

	gInfo->regs = NULL;
	gInfo->shared_info = NULL;

	// close the file handle ONLY if we're the clone
	if (gInfo->is_clone)
		close(gInfo->device);

	free(gInfo);
}


//	#pragma mark - public accelerant functions


/** Init primary accelerant */

status_t
intel_init_accelerant(int device)
{
	TRACE(("intel_init_accelerant()\n"));

	status_t status = init_common(device, false);
	if (status != B_OK) 
		return status;

	setup_ring_buffer(gInfo->shared_info->primary_ring_buffer, "intel primary ring buffer");
	setup_ring_buffer(gInfo->shared_info->secondary_ring_buffer, "intel secondary ring buffer");

	status = create_mode_list();
	if (status != B_OK) {
		uninit_common();
		return status;
	}

	return B_OK;
}


ssize_t
intel_accelerant_clone_info_size(void)
{
	TRACE(("intel_accelerant_clone_info_size()\n"));
	// clone info is device name, so return its maximum size
	return B_PATH_NAME_LENGTH;
}


void
intel_get_accelerant_clone_info(void *info)
{
	TRACE(("intel_get_accelerant_clone_info()\n"));
	ioctl(gInfo->device, INTEL_GET_DEVICE_NAME, info, B_PATH_NAME_LENGTH);
}


status_t
intel_clone_accelerant(void *info)
{
	TRACE(("intel_clone_accelerant()\n"));

	// create full device name
	char path[B_PATH_NAME_LENGTH];
	strcpy(path, "/dev/");
#ifdef __HAIKU__
	strlcat(path, (const char *)info, sizeof(path));
#else
	strcat(path, (const char *)info);
#endif

	int fd = open(path, B_READ_WRITE);
	if (fd < 0)
		return fd;

	status_t status = init_common(fd, true);
	if (status != B_OK)
		goto err1;

	// get read-only clone of supported display modes
	status = gInfo->mode_list_area = clone_area(
		"intel extreme cloned modes", (void **)&gInfo->mode_list,
		B_ANY_ADDRESS, B_READ_AREA, gInfo->shared_info->mode_list_area);
	if (status < B_OK) 
		goto err2;

	return B_OK;

err2:
	uninit_common();
err1:
	close(fd);
	return status;
}


/** This function is called for both, the primary accelerant and all of
 *	its clones.
 */

void
intel_uninit_accelerant(void)
{
	TRACE(("intel_uninit_accelerant()\n"));

	// delete accelerant instance data
	delete_area(gInfo->mode_list_area);
	gInfo->mode_list = NULL;

	uninit_common();
}


status_t
intel_get_accelerant_device_info(accelerant_device_info *info)
{
	TRACE(("intel_get_accelerant_device_info()\n"));

	info->version = B_ACCELERANT_VERSION;
	strcpy(info->name, gInfo->shared_info->device_type == INTEL_TYPE_7xx
		? "Intel Extreme Graphics 1" : "Intel Extreme Graphics 2");
	strcpy(info->chipset, gInfo->shared_info->device_identifier);
	strcpy(info->serial_no, "None");

	info->memory = gInfo->shared_info->graphics_memory_size;
	info->dac_speed = gInfo->shared_info->pll_info.max_frequency;

	return B_OK;
}


sem_id
intel_accelerant_retrace_semaphore()
{
	TRACE(("intel_accelerant_retrace_semaphore()\n"));
	return gInfo->shared_info->vblank_sem;
}

