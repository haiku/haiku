/*
 * Copyright (c) 2002, Thomas Kurschel
 * Copyright 2004-2011 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Thomas Kurschel
 *		Clemens Zeidler, <haiku@clemens-zeidler.de>
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "driver.h"
#include "device.h"
#include "lock.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <AGP.h>
#include <KernelExport.h>
#include <OS.h>
#include <PCI.h>
#include <SupportDefs.h>


#define TRACE_DRIVER
#ifdef TRACE_DRIVER
#	define TRACE(x...) dprintf("radeon_hd: " x)
#else
#	define TRACE(x...) ;
#endif

#define MAX_CARDS 1


// ATI / AMD cards starting at the Radeon X700 have an AtomBIOS

// list of supported devices
const struct supported_device {
	uint32		pciID;
	uint8		dceMajor;	// Display block family
	uint8		dceMinor;	// Display block family
	uint16		chipsetID;
	uint32		chipsetFlags;
	const char*	deviceName;
} kSupportedDevices[] = {
	// R400 Series  (Radeon) DCE 0.0 (*very* early AtomBIOS)
	// R500 Series  (Radeon Xxxx) DCE 1.0
	// R600 series	(HD24xx - HD42xx)
	// Codename: Pele
	{0x94c7, 2, 0, RADEON_RV610, CHIP_STD, "Radeon HD 2350"},
	{0x94c1, 2, 0, RADEON_RV610, CHIP_IGP, "Radeon HD 2400"},
	{0x94c3, 2, 0, RADEON_RV610, CHIP_STD, "Radeon HD 2400"},
	{0x94cc, 2, 0, RADEON_RV610, CHIP_STD, "Radeon HD 2400"},
	{0x9586, 2, 0, RADEON_RV630, CHIP_STD, "Radeon HD 2600"},
	{0x9588, 2, 0, RADEON_RV630, CHIP_STD, "Radeon HD 2600"},
	{0x958a, 2, 0, RADEON_RV630, CHIP_STD, "Radeon HD 2600 X2"},
	//	Radeon 2700		- RV630
	{0x9400, 2, 0, RADEON_R600,  CHIP_STD, "Radeon HD 2900"},
	{0x9401, 2, 0, RADEON_R600,  CHIP_STD, "Radeon HD 2900"},
	{0x9402, 2, 0, RADEON_R600,  CHIP_STD, "Radeon HD 2900"},
	{0x9403, 2, 0, RADEON_R600,  CHIP_STD, "Radeon HD 2900 Pro"},
	{0x9405, 2, 0, RADEON_R600,  CHIP_STD, "Radeon HD 2900"},
	{0x940a, 2, 0, RADEON_R600,  CHIP_STD, "Radeon FireGL V8650"},
	{0x940b, 2, 0, RADEON_R600,  CHIP_STD, "Radeon FireGL V8600"},
	{0x940f, 2, 0, RADEON_R600,  CHIP_STD, "Radeon FireGL V7600"},
	{0x9616, 2, 0, RADEON_RV610, CHIP_IGP, "Radeon HD 3000"},
	{0x9611, 3, 0, RADEON_RV620, CHIP_IGP, "Radeon HD 3100"},
	{0x9613, 3, 0, RADEON_RV620, CHIP_IGP, "Radeon HD 3100"},
	{0x9610, 2, 0, RADEON_RV610, CHIP_IGP, "Radeon HD 3200"},
	{0x9612, 2, 0, RADEON_RV610, CHIP_IGP, "Radeon HD 3200"},
	{0x9615, 2, 0, RADEON_RV610, CHIP_IGP, "Radeon HD 3200"},
	{0x9614, 2, 0, RADEON_RV610, CHIP_IGP, "Radeon HD 3300"},
	//  Radeon 3430		- RV620
	{0x95c5, 3, 0, RADEON_RV620, CHIP_STD, "Radeon HD 3450"},
	{0x95c6, 3, 0, RADEON_RV620, CHIP_STD, "Radeon HD 3450"},
	{0x95c7, 3, 0, RADEON_RV620, CHIP_STD, "Radeon HD 3450"},
	{0x95c9, 3, 0, RADEON_RV620, CHIP_STD, "Radeon HD 3450"},
	{0x95c4, 3, 0, RADEON_RV620, CHIP_STD, "Radeon HD 3470"},
	{0x95c0, 3, 0, RADEON_RV620, CHIP_STD, "Radeon HD 3550"},
	{0x9581, 2, 0, RADEON_RV630, CHIP_STD, "Radeon HD 3600"},
	{0x9583, 2, 0, RADEON_RV630, CHIP_STD, "Radeon HD 3600"},
	{0x9598, 2, 0, RADEON_RV630, CHIP_STD, "Radeon HD 3600"},
	{0x9591, 3, 0, RADEON_RV635, CHIP_STD, "Radeon HD 3600"},
	{0x9589, 2, 0, RADEON_RV630, CHIP_STD, "Radeon HD 3610"},
	//  Radeon 3650		- RV635
	//  Radeon 3670		- RV635
	{0x9507, 2, 0, RADEON_RV670, CHIP_STD, "Radeon HD 3830"},
	{0x9505, 2, 0, RADEON_RV670, CHIP_STD, "Radeon HD 3850"},
	{0x9513, 2, 0, RADEON_RV670, CHIP_STD, "Radeon HD 3850 X2"},
	{0x9501, 2, 0, RADEON_RV670, CHIP_STD, "Radeon HD 3870"},
	{0x950F, 2, 0, RADEON_RV670, CHIP_STD, "Radeon HD 3870 X2"},
	{0x9710, 3, 0, RADEON_RV620, CHIP_IGP, "Radeon HD 4200"},
	{0x9715, 3, 0, RADEON_RV620, CHIP_IGP, "Radeon HD 4250"},
	{0x9712, 3, 0, RADEON_RV620, CHIP_IGP, "Radeon HD 4270"},
	{0x9714, 3, 0, RADEON_RV620, CHIP_IGP, "Radeon HD 4290"},

	// R700 series	(HD4330 - HD4890, HD51xx, HD5xxV)
	// Codename: Wekiva
	//	Radeon 4330		- RV710
	{0x954f, 3, 2, RADEON_RV710, CHIP_IGP, "Radeon HD 4300"},
	{0x9552, 3, 2, RADEON_RV710, CHIP_IGP, "Radeon HD 4300"},
	{0x9555, 3, 2, RADEON_RV710, CHIP_STD, "Radeon HD 4350"},
	{0x9540, 3, 2, RADEON_RV710, CHIP_STD, "Radeon HD 4550"},
	{0x9480, 3, 2, RADEON_RV730, CHIP_STD, "Radeon HD 4650"},
	{0x9498, 3, 2, RADEON_RV730, CHIP_STD, "Radeon HD 4650"},
	{0x94b4, 3, 2, RADEON_RV740, CHIP_STD, "Radeon HD 4700"},
	{0x9490, 3, 2, RADEON_RV730, CHIP_STD, "Radeon HD 4710"},
	{0x94b3, 3, 2, RADEON_RV740, CHIP_STD, "Radeon HD 4770"},
	{0x94b5, 3, 2, RADEON_RV740, CHIP_STD, "Radeon HD 4770"},
	{0x944a, 3, 1, RADEON_RV770, CHIP_MOBILE, "Radeon HD 4850"},
	{0x944e, 3, 1, RADEON_RV770, CHIP_STD, "Radeon HD 4810"},
	{0x944c, 3, 1, RADEON_RV770, CHIP_STD, "Radeon HD 4830"},
	{0x9442, 3, 1, RADEON_RV770, CHIP_STD, "Radeon HD 4850"},
	{0x9443, 3, 1, RADEON_RV770, CHIP_STD, "Radeon HD 4850 X2"},
	{0x94a1, 3, 1, RADEON_RV770, CHIP_IGP, "Radeon HD 4860"},
	{0x9440, 3, 1, RADEON_RV770, CHIP_STD, "Radeon HD 4870"},
	{0x9441, 3, 1, RADEON_RV770, CHIP_STD, "Radeon HD 4870 X2"},
	{0x9460, 3, 1, RADEON_RV770, CHIP_STD, "Radeon HD 4890"},

	// From here on AMD no longer used numeric identifiers

	// R1000 series (HD54xx - HD63xx)
	// Codename: Evergreen
	//  Cedar
	{0x68e1, 4, 0, RADEON_CEDAR, CHIP_STD, "Radeon HD 5430"},
	{0x68f9, 4, 0, RADEON_CEDAR, CHIP_STD, "Radeon HD 5450"},
	{0x68e0, 4, 0, RADEON_CEDAR, CHIP_IGP, "Radeon HD 5470"},
	//  Redwood
	{0x68da, 4, 0, RADEON_REDWOOD, CHIP_STD, "Radeon HD 5500"},
	{0x68d9, 4, 0, RADEON_REDWOOD, CHIP_STD, "Radeon HD 5570"},
	{0x68b9, 4, 0, RADEON_REDWOOD, CHIP_STD, "Radeon HD 5600"},
	{0x68c1, 4, 0, RADEON_REDWOOD, CHIP_STD, "Radeon HD 5650"},
	{0x68d8, 4, 0, RADEON_REDWOOD, CHIP_STD, "Radeon HD 5670"},
	//  Juniper
	{0x68be, 4, 0, RADEON_JUNIPER, CHIP_STD, "Radeon HD 5700"},
	{0x68b8, 4, 0, RADEON_JUNIPER, CHIP_STD, "Radeon HD 5770"},
	//  Cypress
	{0x689e, 4, 0, RADEON_CYPRESS, CHIP_STD, "Radeon HD 5800"},
	{0x6899, 4, 0, RADEON_CYPRESS, CHIP_STD, "Radeon HD 5850"},
	{0x6898, 4, 0, RADEON_CYPRESS, CHIP_STD, "Radeon HD 5870"},
	//  Hemlock
	{0x689c, 4, 0, RADEON_HEMLOCK, CHIP_STD, "Radeon HD 5900"},
	// Fusion APUS
	//  Palms
	{0x9804, 4, 1, RADEON_PALM, CHIP_APU, "Radeon HD 6250"},
	{0x9805, 4, 1, RADEON_PALM, CHIP_APU, "Radeon HD 6290"},
	{0x9802, 4, 1, RADEON_PALM, CHIP_APU, "Radeon HD 6310"},
	{0x9803, 4, 1, RADEON_PALM, CHIP_APU, "Radeon HD 6310"},

	// R2000 series (HD64xx - HD69xx)
	// Codename: Nothern Islands
	//  Caicos
	{0x6760, 5, 0, RADEON_CAICOS, CHIP_MOBILE, "Radeon HD 6470M"},
	{0x6761, 5, 0, RADEON_CAICOS, CHIP_MOBILE, "Radeon HD 6430M"},
	{0x6762, 5, 0, RADEON_CAICOS, CHIP_STD, "Radeon HD CAICOS"},
	{0x6763, 5, 0, RADEON_CAICOS, CHIP_DISCREET, "Radeon HD E6460"},
	{0x6764, 5, 0, RADEON_CAICOS, CHIP_STD, "Radeon HD CAICOS"},
	{0x6765, 5, 0, RADEON_CAICOS, CHIP_STD, "Radeon HD CAICOS"},
	{0x6766, 5, 0, RADEON_CAICOS, CHIP_STD, "Radeon HD CAICOS"},
	{0x6767, 5, 0, RADEON_CAICOS, CHIP_STD, "Radeon HD CAICOS"},
	{0x6768, 5, 0, RADEON_CAICOS, CHIP_STD, "Radeon HD CAICOS"},
	{0x6770, 5, 0, RADEON_CAICOS, CHIP_STD, "Radeon HD 6400"},
	{0x6779, 5, 0, RADEON_CAICOS, CHIP_STD, "Radeon HD 6450"},
	//  Turks
	{0x6740, 5, 0, RADEON_TURKS, CHIP_MOBILE, "Radeon HD 6700M"},
	{0x6741, 5, 0, RADEON_TURKS, CHIP_MOBILE, "Radeon HD 6600M"},
	{0x6742, 5, 0, RADEON_TURKS, CHIP_MOBILE, "Radeon HD 6625M"},
	{0x6743, 5, 0, RADEON_TURKS, CHIP_DISCREET, "Radeon HD E6760"},
	{0x6744, 5, 0, RADEON_TURKS, CHIP_MOBILE, "Radeon HD TURKS M"},
	{0x6745, 5, 0, RADEON_TURKS, CHIP_MOBILE, "Radeon HD TURKS M"},
	{0x6746, 5, 0, RADEON_TURKS, CHIP_STD, "Radeon HD TURKS"},
	{0x6747, 5, 0, RADEON_TURKS, CHIP_STD, "Radeon HD TURKS"},
	{0x6748, 5, 0, RADEON_TURKS, CHIP_STD, "Radeon HD TURKS"},
	{0x6749, 5, 0, RADEON_TURKS, CHIP_STD, "FirePro v4900"},
	{0x6759, 5, 0, RADEON_TURKS, CHIP_STD, "Radeon HD 6570"},
	//  Barts
	{0x673e, 5, 0, RADEON_BARTS, CHIP_STD, "Radeon HD 6790"},
	{0x6739, 5, 0, RADEON_BARTS, CHIP_STD, "Radeon HD 6850"},
	{0x6738, 5, 0, RADEON_BARTS, CHIP_STD, "Radeon HD 6870"},
	//  Cayman
	{0x6700, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD CAYMAN"},
	{0x6701, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD CAYMAN"},
	{0x6702, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD CAYMAN"},
	{0x6703, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD CAYMAN"},
	{0x6704, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD CAYMAN"},
	{0x6705, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD CAYMAN"},
	{0x6706, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD CAYMAN"},
	{0x6707, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD CAYMAN"},
	{0x6708, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD CAYMAN"},
	{0x6709, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD CAYMAN"},
	{0x6718, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD 6970"},
	{0x6719, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD 6950"},
	{0x671C, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD CAYMAN"},
	{0x671F, 5, 0, RADEON_CAYMAN, CHIP_STD, "Radeon HD 6900"},
	//  Antilles
	{0x671d, 5, 0, RADEON_ANTILLES, CHIP_STD, "Radeon HD 6990"}

	// R3000 series (HD74xx - HD79xx)
	// Codename: Southern Islands
	//  Lombok
	//  Thames
	//  Tahiti
	//  New Zealand
};


int32 api_version = B_CUR_DRIVER_API_VERSION;


char* gDeviceNames[MAX_CARDS + 1];
radeon_info* gDeviceInfo[MAX_CARDS];
pci_module_info* gPCI;
mutex gLock;


static status_t
get_next_radeon_hd(int32 *_cookie, pci_info &info, uint32 &type)
{
	int32 index = *_cookie;

	// find devices

	for (; gPCI->get_nth_pci_info(index, &info) == B_OK; index++) {
		// check vendor
		if (info.vendor_id != VENDOR_ID_ATI
			|| info.class_base != PCI_display
			|| info.class_sub != PCI_vga)
			continue;

		// check device
		for (uint32 i = 0; i < sizeof(kSupportedDevices)
				/ sizeof(kSupportedDevices[0]); i++) {
			if (info.device_id == kSupportedDevices[i].pciID) {
				type = i;
				*_cookie = index + 1;
				return B_OK;
			}
		}
	}

	return B_ENTRY_NOT_FOUND;
}


extern "C" const char **
publish_devices(void)
{
	TRACE("%s\n", __func__);
	return (const char **)gDeviceNames;
}


extern "C" status_t
init_hardware(void)
{
	TRACE("%s\n", __func__);

	status_t status = get_module(B_PCI_MODULE_NAME, (module_info **)&gPCI);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": ERROR: pci module unavailable\n");
		return status;
	}

	int32 cookie = 0;
	uint32 type;
	pci_info info;
	status = get_next_radeon_hd(&cookie, info, type);

	put_module(B_PCI_MODULE_NAME);
	return status;
}


extern "C" status_t
init_driver(void)
{
	TRACE("%s\n", __func__);

	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&gPCI);
	if (status != B_OK) {
		dprintf(DEVICE_NAME ": ERROR: pci module unavailable\n");
		return status;
	}

	mutex_init(&gLock, "radeon hd ksync");

	// find devices

	int32 found = 0;

	for (int32 cookie = 0; found < MAX_CARDS;) {
		pci_info* info = (pci_info*)malloc(sizeof(pci_info));
		if (info == NULL)
			break;

		uint32 type;
		status = get_next_radeon_hd(&cookie, *info, type);
		if (status < B_OK) {
			free(info);
			break;
		}

		// create device names & allocate device info structure

		char name[64];
		sprintf(name, "graphics/radeon_hd_%02x%02x%02x",
			info->bus, info->device,
			info->function);

		gDeviceNames[found] = strdup(name);
		if (gDeviceNames[found] == NULL)
			break;

		gDeviceInfo[found] = (radeon_info*)malloc(sizeof(radeon_info));
		if (gDeviceInfo[found] == NULL) {
			free(gDeviceNames[found]);
			break;
		}

		// initialize the structure for later use

		memset(gDeviceInfo[found], 0, sizeof(radeon_info));
		gDeviceInfo[found]->init_status = B_NO_INIT;
		gDeviceInfo[found]->id = found;
		gDeviceInfo[found]->pci = info;
		gDeviceInfo[found]->registers = (uint8 *)info->u.h0.base_registers[0];
		gDeviceInfo[found]->pciID = kSupportedDevices[type].pciID;
		gDeviceInfo[found]->deviceName = kSupportedDevices[type].deviceName;
		gDeviceInfo[found]->chipsetID = kSupportedDevices[type].chipsetID;
		gDeviceInfo[found]->dceMajor = kSupportedDevices[type].dceMajor;
		gDeviceInfo[found]->dceMinor = kSupportedDevices[type].dceMinor;
		gDeviceInfo[found]->chipsetFlags = kSupportedDevices[type].chipsetFlags;

		dprintf(DEVICE_NAME ": GPU(%ld) %s, revision = 0x%x\n", found,
			kSupportedDevices[type].deviceName, info->revision);

		found++;
	}

	gDeviceNames[found] = NULL;

	if (found == 0) {
		mutex_destroy(&gLock);
		put_module(B_AGP_GART_MODULE_NAME);
		put_module(B_PCI_MODULE_NAME);
		return ENODEV;
	}

	return B_OK;
}


extern "C" void
uninit_driver(void)
{
	TRACE("%s\n", __func__);

	mutex_destroy(&gLock);

	// free device related structures
	char* name;
	for (int32 index = 0; (name = gDeviceNames[index]) != NULL; index++) {
		free(gDeviceInfo[index]);
		free(name);
	}

	put_module(B_PCI_MODULE_NAME);
}


extern "C" device_hooks*
find_device(const char* name)
{
	int index;

	TRACE("%s\n", __func__);

	for (index = 0; gDeviceNames[index] != NULL; index++) {
		if (!strcmp(name, gDeviceNames[index]))
			return &gDeviceHooks;
	}

	return NULL;
}

