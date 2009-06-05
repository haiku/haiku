/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>

#include "accelerant_protos.h"
#include "accelerant.h"


uint32
vesa_dpms_capabilities(void)
{
	return gInfo->shared_info->dpms_capabilities;
}


uint32
vesa_dpms_mode(void)
{
	uint32 mode;
	if (ioctl(gInfo->device, VESA_GET_DPMS_MODE, &mode, sizeof(mode)) != 0)
		return B_DPMS_ON;

	return mode;
}


status_t
vesa_set_dpms_mode(uint32 mode)
{
	if (ioctl(gInfo->device, VESA_SET_DPMS_MODE, &mode, sizeof(mode)) != 0)
		return errno;

	return B_OK;
}

