/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck, kallisti5@unixzen.com
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
#	define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#	define TRACE(x...) ;
#endif


struct accelerant_info *gInfo;
struct register_info *gRegister;


class AreaCloner {
public:
								AreaCloner();
								~AreaCloner();

			area_id				Clone(const char *name, void **_address,
									uint32 spec, uint32 protection,
									area_id sourceArea);
			status_t			InitCheck()
									{return fArea < 0 ? (status_t)fArea : B_OK;}
			void				Keep();

private:
			area_id				fArea;
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
	gRegister = (register_info *)malloc(sizeof(register_info));

	if (gInfo == NULL || gRegister == NULL)
		return B_NO_MEMORY;

	memset(gInfo, 0, sizeof(accelerant_info));
	memset(gRegister, 0, sizeof(register_info));

	gInfo->is_clone = isClone;
	gInfo->device = device;

	// get basic info from driver

	radeon_get_private_data data;
	data.magic = RADEON_PRIVATE_DATA_MAGIC;

	if (ioctl(device, RADEON_GET_PRIVATE_DATA, &data,
			sizeof(radeon_get_private_data)) != 0) {
		free(gInfo);
		return B_ERROR;
	}

	AreaCloner sharedCloner;
	gInfo->shared_info_area = sharedCloner.Clone("radeon hd shared info",
		(void **)&gInfo->shared_info, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		data.shared_info_area);
	status_t status = sharedCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo);
		TRACE("%s, failed shared area%i, %i\n",
			__func__, data.shared_info_area, gInfo->shared_info_area);
		return status;
	}

	AreaCloner regsCloner;
	gInfo->regs_area = regsCloner.Clone("radeon hd regs",
		(void **)&gInfo->regs, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		gInfo->shared_info->registers_area);
	status = regsCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo);
		return status;
	}

	sharedCloner.Keep();
	regsCloner.Keep();

	return B_OK;
}


/*! Clean up data common to both primary and cloned accelerant */
static void
uninit_common(void)
{
	delete_area(gInfo->regs_area);
	delete_area(gInfo->shared_info_area);

	gInfo->regs_area = gInfo->shared_info_area = -1;

	// close the file handle ONLY if we're the clone
	if (gInfo->is_clone)
		close(gInfo->device);

	free(gInfo);
	free(gRegister);
}


/*! Populate gRegister with device dependant register locations */
static status_t
init_registers()
{
	// gInfo should always be populated before running this
	if (gInfo == NULL)
		return B_ERROR;

	uint16_t chipset = gInfo->shared_info->device_chipset;

	if (chipset >= RADEON_R800) {
		gRegister->regOffsetCRT0 = EVERGREEN_CRTC0_REGISTER_OFFSET;
		gRegister->regOffsetCRT1 = EVERGREEN_CRTC1_REGISTER_OFFSET;
		gRegister->grphEnable = EVERGREEN_GRPH_ENABLE;
		gRegister->grphControl = EVERGREEN_GRPH_CONTROL;
		gRegister->grphSwapControl = EVERGREEN_GRPH_SWAP_CONTROL;
		gRegister->grphPrimarySurfaceAddr
			= EVERGREEN_GRPH_PRIMARY_SURFACE_ADDRESS;
		gRegister->grphPitch = EVERGREEN_GRPH_PITCH;
		gRegister->grphSurfaceOffsetX = EVERGREEN_GRPH_SURFACE_OFFSET_X;
		gRegister->grphSurfaceOffsetY = EVERGREEN_GRPH_SURFACE_OFFSET_Y;
		gRegister->grphXStart = EVERGREEN_GRPH_X_START;
		gRegister->grphYStart = EVERGREEN_GRPH_Y_START;
		gRegister->grphXEnd = EVERGREEN_GRPH_X_END;
		gRegister->grphYEnd = EVERGREEN_GRPH_Y_END;
		gRegister->modeDesktopHeight = EVERGREEN_DESKTOP_HEIGHT;
		gRegister->viewportStart = EVERGREEN_VIEWPORT_START;
		gRegister->viewportSize = EVERGREEN_VIEWPORT_SIZE;
	} else if (chipset >= RADEON_R600 && chipset < RADEON_R800) {
		gRegister->regOffsetCRT0 = D1_REG_OFFSET;
		gRegister->regOffsetCRT1 = D2_REG_OFFSET;
		gRegister->grphEnable = D1GRPH_ENABLE;
		gRegister->grphControl = D1GRPH_CONTROL;
		gRegister->grphSwapControl = D1GRPH_SWAP_CNTL;
		gRegister->grphPrimarySurfaceAddr = D1GRPH_PRIMARY_SURFACE_ADDRESS;
		gRegister->grphPitch = D1GRPH_PITCH;
		gRegister->grphSurfaceOffsetX = D1GRPH_SURFACE_OFFSET_X;
		gRegister->grphSurfaceOffsetY = D1GRPH_SURFACE_OFFSET_Y;
		gRegister->grphXStart = D1GRPH_X_START;
		gRegister->grphYStart = D1GRPH_Y_START;
		gRegister->grphXEnd = D1GRPH_X_END;
		gRegister->grphYEnd = D1GRPH_Y_END;
		gRegister->modeDesktopHeight = D1MODE_DESKTOP_HEIGHT;
		gRegister->viewportStart = D1MODE_VIEWPORT_START;
		gRegister->viewportSize = D1MODE_VIEWPORT_SIZE;
	} else {
		// this really shouldn't happen unless a driver PCIID chipset is wrong
		TRACE("%s, unknown Radeon chipset: r%X\n", __func__, chipset);
		return B_ERROR;
	}

	TRACE("%s, registers for ATI chipset r%X initialized\n", __func__, chipset);

	return B_OK;
}


//	#pragma mark - public accelerant functions


/*! Init primary accelerant */
status_t
radeon_init_accelerant(int device)
{
	TRACE("%s enter\n", __func__);

	status_t status = init_common(device, false);
	if (status != B_OK)
		return status;

	status = init_registers();
	if (status != B_OK)
		return status;

	radeon_shared_info &info = *gInfo->shared_info;

	init_lock(&info.accelerant_lock, "radeon hd accelerant");
	init_lock(&info.engine_lock, "radeon hd engine");


	status = create_mode_list();
	if (status != B_OK) {
		uninit_common();
		return status;
	}

	TRACE("%s done\n", __func__);
	return B_OK;
}


/*! This function is called for both, the primary accelerant and all of
	its clones.
*/
void
radeon_uninit_accelerant(void)
{
	TRACE("%s enter\n", __func__);

	gInfo->mode_list = NULL;

	radeon_shared_info &info = *gInfo->shared_info;

	uninit_lock(&info.accelerant_lock);
	uninit_lock(&info.engine_lock);

	uninit_common();
	TRACE("%s done\n", __func__);
}

