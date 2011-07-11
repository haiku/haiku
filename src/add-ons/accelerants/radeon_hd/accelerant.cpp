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
#include "pll.h"

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
crt_info *gCRT[MAX_CRT];


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

	for (uint32 id = 0; id < MAX_CRT; id++) {
		gCRT[id] = (crt_info *)malloc(sizeof(crt_info));
		if (gCRT[id] == NULL)
			return B_NO_MEMORY;
	}

	memset(gInfo, 0, sizeof(accelerant_info));
	memset(gRegister, 0, sizeof(register_info));

	for (uint32 id = 0; id < MAX_CRT; id++)
		memset(gCRT[id], 0, sizeof(crt_info));

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

	// Define Radeon PLL default ranges
	gInfo->shared_info->pll_info.reference_frequency
		= RHD_PLL_REFERENCE_DEFAULT;
	gInfo->shared_info->pll_info.min_frequency = RHD_PLL_MIN_DEFAULT;
	gInfo->shared_info->pll_info.max_frequency = RHD_PLL_MAX_DEFAULT;

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

	for (uint32 id = 0; id < MAX_CRT; id++)
		free(gCRT[id]);
}


/*! Populate gRegister with device dependant register locations */
status_t
init_registers(uint8 crtid)
{
	radeon_shared_info &info = *gInfo->shared_info;

	if (info.device_chipset >= RADEON_R800) {
		uint32 offset = 0;

		// AMD Eyefinity on Evergreen GPUs
		if (crtid == 1) {
			offset = EVERGREEN_CRTC1_REGISTER_OFFSET;
			gRegister->vgaControl = D2VGA_CONTROL;
		} else if (crtid == 2) {
			offset = EVERGREEN_CRTC2_REGISTER_OFFSET;
			gRegister->vgaControl = EVERGREEN_D3VGA_CONTROL;
		} else if (crtid == 3) {
			offset = EVERGREEN_CRTC3_REGISTER_OFFSET;
			gRegister->vgaControl = EVERGREEN_D4VGA_CONTROL;
		} else if (crtid == 4) {
			offset = EVERGREEN_CRTC4_REGISTER_OFFSET;
			gRegister->vgaControl = EVERGREEN_D5VGA_CONTROL;
		} else if (crtid == 5) {
			offset = EVERGREEN_CRTC5_REGISTER_OFFSET;
			gRegister->vgaControl = EVERGREEN_D6VGA_CONTROL;
		} else {
			offset = EVERGREEN_CRTC0_REGISTER_OFFSET;
			gRegister->vgaControl = D1VGA_CONTROL;
		}

		// Evergreen+ is crtoffset + register
		gRegister->grphEnable = offset + EVERGREEN_GRPH_ENABLE;
		gRegister->grphControl = offset + EVERGREEN_GRPH_CONTROL;
		gRegister->grphSwapControl = offset + EVERGREEN_GRPH_SWAP_CONTROL;
		gRegister->grphPrimarySurfaceAddr
			= offset + EVERGREEN_GRPH_PRIMARY_SURFACE_ADDRESS;
		gRegister->grphSecondarySurfaceAddr
			= offset + EVERGREEN_GRPH_SECONDARY_SURFACE_ADDRESS;

		gRegister->grphPrimarySurfaceAddrHigh
			= offset + EVERGREEN_GRPH_PRIMARY_SURFACE_ADDRESS_HIGH;
		gRegister->grphSecondarySurfaceAddrHigh
			= offset + EVERGREEN_GRPH_SECONDARY_SURFACE_ADDRESS_HIGH;

		gRegister->grphPitch = offset + EVERGREEN_GRPH_PITCH;
		gRegister->grphSurfaceOffsetX
			= offset + EVERGREEN_GRPH_SURFACE_OFFSET_X;
		gRegister->grphSurfaceOffsetY
			= offset + EVERGREEN_GRPH_SURFACE_OFFSET_Y;
		gRegister->grphXStart = offset + EVERGREEN_GRPH_X_START;
		gRegister->grphYStart = offset + EVERGREEN_GRPH_Y_START;
		gRegister->grphXEnd = offset + EVERGREEN_GRPH_X_END;
		gRegister->grphYEnd = offset + EVERGREEN_GRPH_Y_END;
		gRegister->crtControl = offset + EVERGREEN_CRTC_CONTROL;
		gRegister->modeDesktopHeight = offset + EVERGREEN_DESKTOP_HEIGHT;
		gRegister->modeDataFormat = offset + EVERGREEN_DATA_FORMAT;
		gRegister->viewportStart = offset + EVERGREEN_VIEWPORT_START;
		gRegister->viewportSize = offset + EVERGREEN_VIEWPORT_SIZE;

	} else if (info.device_chipset >= RADEON_R600
		&& info.device_chipset < RADEON_R800) {

		// r600 - r700 are D1 or D2 based on primary / secondary crt
		gRegister->vgaControl
			= crtid == 1 ? D2VGA_CONTROL : D1VGA_CONTROL;
		gRegister->grphEnable
			= crtid == 1 ? D2GRPH_ENABLE : D1GRPH_ENABLE;
		gRegister->grphControl
			= crtid == 1 ? D2GRPH_CONTROL : D1GRPH_CONTROL;
		gRegister->grphSwapControl
			= crtid == 1 ? D2GRPH_SWAP_CNTL : D1GRPH_SWAP_CNTL;
		gRegister->grphPrimarySurfaceAddr
			= crtid == 1 ? D2GRPH_PRIMARY_SURFACE_ADDRESS
				: D1GRPH_PRIMARY_SURFACE_ADDRESS;
		gRegister->grphSecondarySurfaceAddr
			= crtid == 1 ? D2GRPH_SECONDARY_SURFACE_ADDRESS
				: D1GRPH_SECONDARY_SURFACE_ADDRESS;

		// Surface Address high only used on r770+
		gRegister->grphPrimarySurfaceAddrHigh
			= crtid == 1 ? R700_D2GRPH_PRIMARY_SURFACE_ADDRESS_HIGH
				: R700_D1GRPH_PRIMARY_SURFACE_ADDRESS_HIGH;
		gRegister->grphSecondarySurfaceAddrHigh
			= crtid == 1 ? R700_D2GRPH_SECONDARY_SURFACE_ADDRESS_HIGH
				: R700_D1GRPH_SECONDARY_SURFACE_ADDRESS_HIGH;

		gRegister->grphPitch
			= crtid == 1 ? D2GRPH_PITCH : D1GRPH_PITCH;
		gRegister->grphSurfaceOffsetX
			= crtid == 1 ? D2GRPH_SURFACE_OFFSET_X : D1GRPH_SURFACE_OFFSET_X;
		gRegister->grphSurfaceOffsetY
			= crtid == 1 ? D2GRPH_SURFACE_OFFSET_Y : D1GRPH_SURFACE_OFFSET_Y;
		gRegister->grphXStart
			= crtid == 1 ? D2GRPH_X_START : D1GRPH_X_START;
		gRegister->grphYStart
			= crtid == 1 ? D2GRPH_Y_START : D1GRPH_Y_START;
		gRegister->grphXEnd
			= crtid == 1 ? D2GRPH_X_END : D1GRPH_X_END;
		gRegister->grphYEnd
			= crtid == 1 ? D2GRPH_Y_END : D1GRPH_Y_END;
		gRegister->crtControl
			= crtid == 1 ? D2CRTC_CONTROL : D1CRTC_CONTROL;
		gRegister->modeDesktopHeight
			= crtid == 1 ? D2MODE_DESKTOP_HEIGHT : D1MODE_DESKTOP_HEIGHT;
		gRegister->modeDataFormat
			= crtid == 1 ? D2MODE_DATA_FORMAT : D1MODE_DATA_FORMAT;
		gRegister->viewportStart
			= crtid == 1 ? D2MODE_VIEWPORT_START : D1MODE_VIEWPORT_START;
		gRegister->viewportSize
			= crtid == 1 ? D2MODE_VIEWPORT_SIZE : D1MODE_VIEWPORT_SIZE;
	} else {
		// this really shouldn't happen unless a driver PCIID chipset is wrong
		TRACE("%s, unknown Radeon chipset: r%X\n", __func__,
			info.device_chipset);
		return B_ERROR;
	}

	// Populate common registers
	// TODO : Wait.. this doesn't work with Eyefinity > crt 1.
	gRegister->crtid = crtid;

	gRegister->modeCenter
		= crtid == 1 ? D2MODE_CENTER : D1MODE_CENTER;
	gRegister->grphUpdate
		= crtid == 1 ? D2GRPH_UPDATE : D1GRPH_UPDATE;
	gRegister->crtHPolarity
		= crtid == 1 ? D2CRTC_H_SYNC_A_CNTL : D1CRTC_H_SYNC_A_CNTL;
	gRegister->crtVPolarity
		= crtid == 1 ? D2CRTC_V_SYNC_A_CNTL : D1CRTC_V_SYNC_A_CNTL;
	gRegister->crtHTotal
		= crtid == 1 ? D2CRTC_H_TOTAL : D1CRTC_H_TOTAL;
	gRegister->crtVTotal
		= crtid == 1 ? D2CRTC_V_TOTAL : D1CRTC_V_TOTAL;
	gRegister->crtHSync
		= crtid == 1 ? D2CRTC_H_SYNC_A : D1CRTC_H_SYNC_A;
	gRegister->crtVSync
		= crtid == 1 ? D2CRTC_V_SYNC_A : D1CRTC_V_SYNC_A;
	gRegister->crtHBlank
		= crtid == 1 ? D2CRTC_H_BLANK_START_END : D1CRTC_H_BLANK_START_END;
	gRegister->crtVBlank
		= crtid == 1 ? D2CRTC_V_BLANK_START_END : D1CRTC_V_BLANK_START_END;
	gRegister->crtInterlace
		= crtid == 1 ? D2CRTC_INTERLACE_CONTROL : D1CRTC_INTERLACE_CONTROL;
	gRegister->crtCountControl
		= crtid == 1 ? D2CRTC_COUNT_CONTROL : D1CRTC_COUNT_CONTROL;
	gRegister->sclUpdate
		= crtid == 1 ? D2SCL_UPDATE : D1SCL_UPDATE;
	gRegister->sclEnable
		= crtid == 1 ? D2SCL_ENABLE : D1SCL_ENABLE;
	gRegister->sclTapControl
		= crtid == 1 ? D2SCL_TAP_CONTROL : D1SCL_TAP_CONTROL;

	TRACE("%s, registers for ATI chipset r%X crt #%d loaded\n", __func__,
		info.device_chipset, crtid);

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

	radeon_shared_info &info = *gInfo->shared_info;

	init_lock(&info.accelerant_lock, "radeon hd accelerant");
	init_lock(&info.engine_lock, "radeon hd engine");

	status = init_registers(0);
		// Initilize registers for crt0 to begin

	if (status != B_OK)
		return status;

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

