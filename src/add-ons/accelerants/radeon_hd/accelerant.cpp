/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "accelerant.h"

#include <AGP.h>
#include <Debug.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <AutoDeleterOS.h>

#include "accelerant_protos.h"

#include "bios.h"
#include "connector.h"
#include "display.h"
#include "displayport.h"
#include "gpu.h"
#include "pll.h"
#include "utility.h"


#undef TRACE

#define TRACE_ACCELERANT
#ifdef TRACE_ACCELERANT
#	define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#	define TRACE(x...) ;
#endif


struct accelerant_info* gInfo;
display_info* gDisplay[MAX_DISPLAY];
connector_info* gConnector[ATOM_MAX_SUPPORTED_DEVICE];
gpio_info* gGPIOInfo[MAX_GPIO_PINS];


//	#pragma mark -


/*! This is the common accelerant_info initializer. It is called by
	both, the first accelerant and all clones.
*/
static status_t
init_common(int device, bool isClone)
{
	// initialize global accelerant info structure

	gInfo = (accelerant_info*)malloc(sizeof(accelerant_info));
	MemoryDeleter infoDeleter(gInfo);

	if (gInfo == NULL)
		return B_NO_MEMORY;

	memset(gInfo, 0, sizeof(accelerant_info));

	// malloc memory for active display information
	for (uint32 id = 0; id < MAX_DISPLAY; id++) {
		gDisplay[id] = (display_info*)malloc(sizeof(display_info));
		if (gDisplay[id] == NULL)
			return B_NO_MEMORY;
		memset(gDisplay[id], 0, sizeof(display_info));

		gDisplay[id]->regs = (register_info*)malloc(sizeof(register_info));
		if (gDisplay[id]->regs == NULL)
			return B_NO_MEMORY;
		memset(gDisplay[id]->regs, 0, sizeof(register_info));
	}

	// malloc for possible physical card connectors
	for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++) {
		gConnector[id] = (connector_info*)malloc(sizeof(connector_info));

		if (gConnector[id] == NULL)
			return B_NO_MEMORY;
		memset(gConnector[id], 0, sizeof(connector_info));

		gConnector[id]->encoder.pll.id = ATOM_PPLL_INVALID;
	}

	// malloc for card gpio pin information
	for (uint32 id = 0; id < MAX_GPIO_PINS; id++) {
		gGPIOInfo[id] = (gpio_info*)malloc(sizeof(gpio_info));

		if (gGPIOInfo[id] == NULL)
			return B_NO_MEMORY;
		memset(gGPIOInfo[id], 0, sizeof(gpio_info));
	}

	gInfo->is_clone = isClone;
	gInfo->device = device;

	gInfo->dpms_mode = B_DPMS_ON;
		// initial state

	// get basic info from driver

	radeon_get_private_data data;
	data.magic = RADEON_PRIVATE_DATA_MAGIC;

	if (ioctl(device, RADEON_GET_PRIVATE_DATA, &data,
			sizeof(radeon_get_private_data)) != 0) {
		free(gInfo);
		return B_ERROR;
	}

	AreaDeleter sharedDeleter(clone_area("radeon hd shared info",
		(void**)&gInfo->shared_info, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		data.shared_info_area));
	status_t status = gInfo->shared_info_area = sharedDeleter.Get();
	if (status < B_OK) {
		TRACE("%s, failed to create shared area\n", __func__);
		return status;
	}

	AreaDeleter regsDeleter(clone_area("radeon hd regs",
		(void**)&gInfo->regs, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		gInfo->shared_info->registers_area));
	status = gInfo->regs_area = regsDeleter.Get();
	if (status < B_OK) {
		TRACE("%s, failed to create mmio area\n", __func__);
		return status;
	}

	gInfo->rom_area = clone_area("radeon hd AtomBIOS",
		(void**)&gInfo->rom, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		gInfo->shared_info->rom_area);

	if (gInfo->rom_area < 0) {
		TRACE("%s: Clone of AtomBIOS failed!\n", __func__);
		gInfo->shared_info->has_rom = false;
	}

	if (gInfo->rom[0] != 0x55 || gInfo->rom[1] != 0xAA)
		TRACE("%s: didn't find a VGA bios in cloned region!\n", __func__);

	infoDeleter.Detach();
	sharedDeleter.Detach();
	regsDeleter.Detach();

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

		free(gInfo);
	}

	for (uint32 id = 0; id < MAX_DISPLAY; id++) {
		if (gDisplay[id] != NULL) {
			free(gDisplay[id]->regs);
			free(gDisplay[id]);
		}
	}

	for (uint32 id = 0; id < ATOM_MAX_SUPPORTED_DEVICE; id++)
		free(gConnector[id]);

	for (uint32 id = 0; id < MAX_GPIO_PINS; id++)
		free(gGPIOInfo[id]);
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

	// probe firmware information
	radeon_gpu_probe();

	// apply GPU quirks
	radeon_gpu_quirks();

	// find GPIO pins from AtomBIOS
	gpio_populate();

	// find physical card connectors from AtomBIOS
	status = connector_probe();

	if (status != B_OK) {
		TRACE("%s: falling back to legacy connector probe.\n", __func__);
		status = connector_probe_legacy();
	}

	if (status != B_OK) {
		TRACE("%s: couldn't detect supported connectors!\n", __func__);
		return status;
	}

	// print found connectors
	debug_connectors();

	// setup encoders on each connector if needed
	encoder_init();

	// program external pll clock
	pll_external_init();

	// setup link on any DisplayPort connectors
	dp_setup_connectors();

	// detect attached displays
	status = detect_displays();
	//if (status != B_OK)
	//	return status;

	// print found displays
	debug_displays();

	// create initial list of video modes
	status = create_mode_list();
	//if (status != B_OK) {
	//	radeon_uninit_accelerant();
	//	return status;
	//}

	radeon_gpu_mc_setup();

	// Set up data crunching + irq rings
	radeon_gpu_ring_setup();

	radeon_gpu_ring_boot(RADEON_QUEUE_TYPE_GFX_INDEX);

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
radeon_get_accelerant_device_info(accelerant_device_info* di)
{
	radeon_shared_info &info = *gInfo->shared_info;

	di->version = B_ACCELERANT_VERSION;
	strcpy(di->name, info.deviceName);

	char chipset[32];
	sprintf(chipset, "%s", gInfo->shared_info->chipsetName);
	strcpy(di->chipset, chipset);

	// add flags onto chipset name
	if ((info.chipsetFlags & CHIP_IGP) != 0)
		strcat(di->chipset, " IGP");
	if ((info.chipsetFlags & CHIP_MOBILE) != 0)
		strcat(di->chipset, " Mobile");
	if ((info.chipsetFlags & CHIP_APU) != 0)
		strcat(di->chipset, " APU");

	strcpy(di->serial_no, "None" );

	di->memory = gInfo->shared_info->graphics_memory_size;
	return B_OK;
}
