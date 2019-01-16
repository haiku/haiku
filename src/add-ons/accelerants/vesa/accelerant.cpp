/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2016-207, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "accelerant_protos.h"
#include "accelerant.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>


//#define TRACE_ACCELERANT
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


/*!	This is the common accelerant_info initializer. It is called by
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
	gInfo->current_mode = UINT16_MAX;

	// get basic info from driver

	area_id sharedArea;
	if (ioctl(device, VESA_GET_PRIVATE_DATA, &sharedArea, sizeof(area_id))
			!= 0) {
		free(gInfo);
		return B_ERROR;
	}

	AreaCloner sharedCloner;
	gInfo->shared_info_area = sharedCloner.Clone("vesa shared info",
		(void **)&gInfo->shared_info, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, sharedArea);
	status_t status = sharedCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo);
		return status;
	}

	if (gInfo->shared_info->vesa_mode_count == 0)
		gInfo->vesa_modes = NULL;
	else
		gInfo->vesa_modes = (vesa_mode *)((uint8 *)gInfo->shared_info
			+ gInfo->shared_info->vesa_mode_offset);

	sharedCloner.Keep();
	return B_OK;
}


/*!	Cleans up everything done by a successful init_common(). */
static void
uninit_common(void)
{
	delete_area(gInfo->shared_info_area);
	gInfo->shared_info_area = -1;
	gInfo->shared_info = NULL;

	// close the file handle ONLY if we're the clone
	// (this is what Be tells us ;)
	if (gInfo->is_clone)
		close(gInfo->device);

	free(gInfo);
}


//	#pragma mark - public accelerant functions


/*!	Init primary accelerant */
status_t
vesa_init_accelerant(int device)
{
	TRACE(("vesa_init_accelerant()\n"));

	status_t status = init_common(device, false);
	if (status != B_OK)
		return status;

	status = create_mode_list();
	if (status != B_OK) {
		uninit_common();
		return status;
	}

	// Initialize current mode completely from the mode list
	vesa_propose_display_mode(&gInfo->shared_info->current_mode, NULL, NULL);
	return B_OK;
}


ssize_t
vesa_accelerant_clone_info_size(void)
{
	// clone info is device name, so return its maximum size
	return B_PATH_NAME_LENGTH;
}


void
vesa_get_accelerant_clone_info(void *info)
{
	ioctl(gInfo->device, VESA_GET_DEVICE_NAME, info, B_PATH_NAME_LENGTH);
}


status_t
vesa_clone_accelerant(void *info)
{
	TRACE(("vesa_clone_accelerant()\n"));

	// create full device name
	char path[MAXPATHLEN];
	strcpy(path, "/dev/");
	strcat(path, (const char *)info);

	int fd = open(path, B_READ_WRITE);
	if (fd < 0)
		return errno;

	status_t status = init_common(fd, true);
	if (status != B_OK)
		goto err1;

	// get read-only clone of supported display modes
	status = gInfo->mode_list_area = clone_area(
		"vesa cloned modes", (void **)&gInfo->mode_list,
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


/*!	This function is called for both, the primary accelerant and all of
	its clones.
*/
void
vesa_uninit_accelerant(void)
{
	TRACE(("vesa_uninit_accelerant()\n"));

	// delete accelerant instance data
	delete_area(gInfo->mode_list_area);
	gInfo->mode_list = NULL;

	uninit_common();
}


status_t
vesa_get_accelerant_device_info(accelerant_device_info *info)
{
	info->version = B_ACCELERANT_VERSION;

	// TODO: provide some more insight here...
	if (gInfo->vesa_modes != NULL) {
		strcpy(info->name, "VESA driver");
		strcpy(info->chipset, "VESA");
	} else {
		strcpy(info->name, "Framebuffer");
		strcpy(info->chipset, "");
	}

	strcpy(info->serial_no, "None");

#if 0
	info->memory = ???
	info->dac_speed = ???
#endif

	return B_OK;
}


sem_id
vesa_accelerant_retrace_semaphore()
{
	return -1;
}

