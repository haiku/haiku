/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "accelerant_protos.h"
#include "accelerant.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>

#include <AutoDeleterOS.h>


#define TRACE_ACCELERANT
#ifdef TRACE_ACCELERANT
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


struct accelerant_info *gInfo;


//	#pragma mark -


/*!	This is the common accelerant_info initializer. It is called by
	both, the first accelerant and all clones.
*/
static status_t
init_common(int device, bool isClone)
{
	// initialize global accelerant info structure

	gInfo = (accelerant_info *)malloc(sizeof(accelerant_info));
	MemoryDeleter infoDeleter(gInfo);
	if (gInfo == NULL)
		return B_NO_MEMORY;

	memset(gInfo, 0, sizeof(accelerant_info));

	gInfo->is_clone = isClone;
	gInfo->device = device;
	gInfo->current_mode = UINT16_MAX;

	// get basic info from driver

	area_id sharedArea;
	if (ioctl(device, VIRTIO_GPU_GET_PRIVATE_DATA, &sharedArea, sizeof(area_id)) != 0)
		return B_ERROR;

	AreaDeleter sharedDeleter(clone_area("virtio_gpu shared info",
		(void **)&gInfo->shared_info, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, sharedArea));
	status_t status = gInfo->shared_info_area = sharedDeleter.Get();
	if (status < B_OK)
		return status;

	infoDeleter.Detach();
	sharedDeleter.Detach();
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
virtio_gpu_init_accelerant(int device)
{
	TRACE(("virtio_gpu_init_accelerant()\n"));

	status_t status = init_common(device, false);
	if (status != B_OK)
		return status;

	status = create_mode_list();
	if (status != B_OK) {
		uninit_common();
		return status;
	}

	return B_OK;
}


ssize_t
virtio_gpu_accelerant_clone_info_size(void)
{
	// clone info is device name, so return its maximum size
	return B_PATH_NAME_LENGTH;
}


void
virtio_gpu_get_accelerant_clone_info(void *info)
{
	ioctl(gInfo->device, VIRTIO_GPU_GET_DEVICE_NAME, info, B_PATH_NAME_LENGTH);
}


status_t
virtio_gpu_clone_accelerant(void *info)
{
	TRACE(("virtio_gpu_clone_accelerant()\n"));

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
		"virtio_gpu cloned modes", (void **)&gInfo->mode_list,
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
virtio_gpu_uninit_accelerant(void)
{
	TRACE(("virtio_gpu_uninit_accelerant()\n"));

	// delete accelerant instance data
	delete_area(gInfo->mode_list_area);
	gInfo->mode_list = NULL;

	uninit_common();
}


status_t
virtio_gpu_get_accelerant_device_info(accelerant_device_info *info)
{
	info->version = B_ACCELERANT_VERSION;
	strcpy(info->name, "VirtioGpu Driver");
	strcpy(info->chipset, "Virtio");
		// ToDo: provide some more insight here...
	strcpy(info->serial_no, "None");

#if 0
	info->memory = ???
	info->dac_speed = ???
#endif

	return B_OK;
}


sem_id
virtio_gpu_accelerant_retrace_semaphore()
{
	return -1;
}

