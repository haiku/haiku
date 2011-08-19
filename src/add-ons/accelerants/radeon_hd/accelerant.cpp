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

#include "bios.h"
#include "display.h"
#include "gpu.h"
#include "pll.h"
#include "utility.h"

#include <Debug.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

#include <AGP.h>


#undef TRACE

#define TRACE_ACCELERANT
#ifdef TRACE_ACCELERANT
#	define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#	define TRACE(x...) ;
#endif


struct accelerant_info *gInfo;
display_info *gDisplay[MAX_DISPLAY];


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

	if (gInfo == NULL)
		return B_NO_MEMORY;

	memset(gInfo, 0, sizeof(accelerant_info));

	gInfo->mc_info = (gpu_mc_info *)malloc(sizeof(gpu_mc_info));

	for (uint32 id = 0; id < MAX_DISPLAY; id++) {
		gDisplay[id] = (display_info *)malloc(sizeof(display_info));
		if (gDisplay[id] == NULL)
			return B_NO_MEMORY;
		memset(gDisplay[id], 0, sizeof(display_info));

		gDisplay[id]->regs = (register_info *)malloc(sizeof(register_info));
		if (gDisplay[id]->regs == NULL)
			return B_NO_MEMORY;
		memset(gDisplay[id]->regs, 0, sizeof(register_info));
	}

	gInfo->is_clone = isClone;
	gInfo->device = device;

	// get basic info from driver

	radeon_get_private_data data;
	data.magic = RADEON_PRIVATE_DATA_MAGIC;

	if (ioctl(device, RADEON_GET_PRIVATE_DATA, &data,
			sizeof(radeon_get_private_data)) != 0) {
		free(gInfo->mc_info);
		free(gInfo);
		return B_ERROR;
	}

	AreaCloner sharedCloner;
	gInfo->shared_info_area = sharedCloner.Clone("radeon hd shared info",
		(void **)&gInfo->shared_info, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		data.shared_info_area);
	status_t status = sharedCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo->mc_info);
		free(gInfo);
		TRACE("%s, failed to create shared area\n", __func__);
		return status;
	}

	AreaCloner regsCloner;
	gInfo->regs_area = regsCloner.Clone("radeon hd regs",
		(void **)&gInfo->regs, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		gInfo->shared_info->registers_area);
	status = regsCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo->mc_info);
		free(gInfo);
		TRACE("%s, failed to create mmio area\n", __func__);
		return status;
	}

	gInfo->rom_area = clone_area("radeon hd AtomBIOS",
		(void **)&gInfo->rom, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		gInfo->shared_info->rom_area);

	if (gInfo->rom_area < 0) {
		TRACE("%s: Clone of AtomBIOS failed!\n", __func__);
		gInfo->shared_info->has_rom = false;
	}

	if (gInfo->rom[0] != 0x55 || gInfo->rom[1] != 0xAA)
		TRACE("%s: didn't find a VGA bios in cloned region!\n", __func__);

	sharedCloner.Keep();
	regsCloner.Keep();

	return B_OK;
}


/*! Clean up data common to both primary and cloned accelerant */
static void
uninit_common(void)
{
	if (gInfo != NULL) {
		delete_area(gInfo->regs_area);
		delete_area(gInfo->shared_info_area);
		delete_area(gInfo->rom_area);

		gInfo->regs_area = gInfo->shared_info_area = -1;

		// close the file handle ONLY if we're the clone
		if (gInfo->is_clone)
			close(gInfo->device);

		free(gInfo->mc_info);
		free(gInfo);
	}

	for (uint32 id = 0; id < MAX_DISPLAY; id++) {
		if (gDisplay[id] != NULL) {
			free(gDisplay[id]->regs);
			free(gDisplay[id]);
		}
	}
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

	radeon_init_bios(gInfo->rom);

	status = detect_displays();
	//if (status != B_OK)
	//	return status;

	debug_displays();

	status = create_mode_list();
	//if (status != B_OK) {
	//	radeon_uninit_accelerant();
	//	return status;
	//}

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


status_t
radeon_get_accelerant_device_info(accelerant_device_info *di)
{
	di->version = B_ACCELERANT_VERSION;
	strcpy(di->name, gInfo->shared_info->device_identifier);

	char chipset[32];
	sprintf(chipset, "r%X", gInfo->shared_info->device_chipset);
	strcpy(di->chipset, chipset);

	strcpy(di->serial_no, "None" );

	di->memory = gInfo->shared_info->graphics_memory_size;
	return B_OK;
}
