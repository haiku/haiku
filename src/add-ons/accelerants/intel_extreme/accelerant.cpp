/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "accelerant.h"

#include "utility.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

#include <AGP.h>


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

			area_id			Clone(const char *name, void **_address,
								uint32 spec, uint32 protection,
								area_id sourceArea);
			status_t		InitCheck()
								{ return fArea < 0 ? (status_t)fArea : B_OK; }
			void			Keep();

private:
			area_id			fArea;
};


AreaCloner::AreaCloner()
	:
	fArea(-1)
{
}


AreaCloner::~AreaCloner()
{
	if (fArea >= 0)
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


/*! This is the common accelerant_info initializer. It is called by
	both, the first accelerant and all clones.
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

	if (ioctl(device, INTEL_GET_PRIVATE_DATA, &data,
			sizeof(intel_get_private_data)) != 0) {
		free(gInfo);
		return B_ERROR;
	}

	AreaCloner sharedCloner;
	gInfo->shared_info_area = sharedCloner.Clone("intel extreme shared info",
		(void **)&gInfo->shared_info, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		data.shared_info_area);
	status_t status = sharedCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo);
		return status;
	}

	AreaCloner regsCloner;
	gInfo->regs_area = regsCloner.Clone("intel extreme regs",
		(void **)&gInfo->registers, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		gInfo->shared_info->registers_area);
	status = regsCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo);
		return status;
	}

	sharedCloner.Keep();
	regsCloner.Keep();

	// The overlay registers, hardware status, and cursor memory share
	// a single area with the shared_info

	gInfo->overlay_registers = (struct overlay_registers *)
		(gInfo->shared_info->graphics_memory
		+ gInfo->shared_info->overlay_offset);

	if (gInfo->shared_info->device_type.InGroup(INTEL_TYPE_96x)) {
		// allocate some extra memory for the 3D context
		if (intel_allocate_memory(INTEL_i965_3D_CONTEXT_SIZE,
				B_APERTURE_NON_RESERVED, gInfo->context_base) == B_OK) {
			gInfo->context_offset = gInfo->context_base
				- (addr_t)gInfo->shared_info->graphics_memory;
		}
	}

	return B_OK;
}


/*! Clean up data common to both primary and cloned accelerant */
static void
uninit_common(void)
{
	intel_free_memory(gInfo->context_base);

	delete_area(gInfo->regs_area);
	delete_area(gInfo->shared_info_area);

	gInfo->regs_area = gInfo->shared_info_area = -1;

	// close the file handle ONLY if we're the clone
	if (gInfo->is_clone)
		close(gInfo->device);

	free(gInfo);
}


//	#pragma mark - public accelerant functions


/*! Init primary accelerant */
status_t
intel_init_accelerant(int device)
{
	TRACE(("intel_init_accelerant()\n"));

	status_t status = init_common(device, false);
	if (status != B_OK)
		return status;

	intel_shared_info &info = *gInfo->shared_info;

	init_lock(&info.accelerant_lock, "intel extreme accelerant");
	init_lock(&info.engine_lock, "intel extreme engine");

	setup_ring_buffer(info.primary_ring_buffer, "intel primary ring buffer");

	// determine head depending on what's already enabled from the BIOS
	// TODO: it would be nicer to retrieve this data via DDC - else the
	//	display is gone for good if the BIOS decides to only show the
	//	picture on the connected analog monitor!
	gInfo->head_mode = 0;
	if (read32(INTEL_DISPLAY_B_PIPE_CONTROL) & DISPLAY_PIPE_ENABLED)
		gInfo->head_mode |= HEAD_MODE_B_DIGITAL;
	if (read32(INTEL_DISPLAY_A_PIPE_CONTROL) & DISPLAY_PIPE_ENABLED)
		gInfo->head_mode |= HEAD_MODE_A_ANALOG;

	uint32 lvds = read32(INTEL_DISPLAY_LVDS_PORT);

	// If we have an enabled display pipe we save the passed information and
	// assume it is the valid panel size..
	// Later we query for proper EDID info if it exists, or figure something
	// else out. (Default modes, etc.)
	bool isSNB = gInfo->shared_info->device_type.InGroup(INTEL_TYPE_SNB);
	if ((isSNB && (lvds & PCH_LVDS_DETECTED) != 0)
		|| (!isSNB && (lvds & DISPLAY_PIPE_ENABLED) != 0)) {
		save_lvds_mode();
		gInfo->head_mode |= HEAD_MODE_LVDS_PANEL;
	}

	TRACE(("head detected: %#x\n", gInfo->head_mode));
	TRACE(("adpa: %08lx, dova: %08lx, dovb: %08lx, lvds: %08lx\n",
		read32(INTEL_DISPLAY_A_ANALOG_PORT),
		read32(INTEL_DISPLAY_A_DIGITAL_PORT),
		read32(INTEL_DISPLAY_B_DIGITAL_PORT), read32(INTEL_DISPLAY_LVDS_PORT)));

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
		return errno;

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


/*! This function is called for both, the primary accelerant and all of
	its clones.
*/
void
intel_uninit_accelerant(void)
{
	TRACE(("intel_uninit_accelerant()\n"));

	// delete accelerant instance data
	delete_area(gInfo->mode_list_area);
	gInfo->mode_list = NULL;

	intel_shared_info &info = *gInfo->shared_info;

	uninit_lock(&info.accelerant_lock);
	uninit_lock(&info.engine_lock);

	uninit_ring_buffer(info.primary_ring_buffer);

	uninit_common();
}


status_t
intel_get_accelerant_device_info(accelerant_device_info *info)
{
	TRACE(("intel_get_accelerant_device_info()\n"));

	info->version = B_ACCELERANT_VERSION;
	strcpy(info->name, gInfo->shared_info->device_type.InFamily(INTEL_TYPE_7xx)
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

