/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
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
#	define TRACE(x...) dprintf("intel_extreme: " x)
#else
#	define TRACE(x) ;
#endif

#define ERROR(x...) dprintf("intel_extreme: " x)
#define CALLED(x...) TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define MAX_CARDS 4


// list of supported devices
const struct supported_device {
	uint32		device_id;
	int32		type;
	const char*	name;
} kSupportedDevices[] = {
	{0x3577, INTEL_GROUP_83x, "i830GM"},
	{0x2562, INTEL_GROUP_83x, "i845G"},

	{0x2572, INTEL_GROUP_85x, "i865G"},
	{0x3582, INTEL_GROUP_85x, "i855G"},
	{0x358e, INTEL_GROUP_85x, "i855G"},

	{0x2582, INTEL_MODEL_915, "i915G"},
	{0x258a, INTEL_MODEL_915, "i915"},
	{0x2592, INTEL_MODEL_915M, "i915GM"},
	{0x2792, INTEL_MODEL_915, "i910"},
	{0x2772, INTEL_MODEL_945, "i945G"},
	{0x27a2, INTEL_MODEL_945M, "i945GM"},
	{0x27ae, INTEL_MODEL_945M, "i945GME"},
	{0x2972, INTEL_MODEL_965, "i946G"},
	{0x2982, INTEL_MODEL_965, "G35"},
	{0x2992, INTEL_MODEL_965, "i965Q"},
	{0x29a2, INTEL_MODEL_965, "i965G"},
	{0x2a02, INTEL_MODEL_965M, "i965GM"},
	{0x2a12, INTEL_MODEL_965M, "i965GME"},
	{0x29b2, INTEL_MODEL_G33, "G33G"},
	{0x29c2, INTEL_MODEL_G33, "Q35G"},
	{0x29d2, INTEL_MODEL_G33, "Q33G"},

	{0x2a42, INTEL_MODEL_GM45, "GM45"},
	{0x2e02, INTEL_MODEL_G45, "IGD"},
	{0x2e12, INTEL_MODEL_G45, "Q45"},
	{0x2e22, INTEL_MODEL_G45, "G45"},
	{0x2e32, INTEL_MODEL_G45, "G41"},
	{0x2e42, INTEL_MODEL_G45, "B43"},
	{0x2e92, INTEL_MODEL_G45, "B43"},

	{0xa001, INTEL_MODEL_PINE, "Atom D4xx"},
	{0xa002, INTEL_MODEL_PINE, "Atom D5xx"},
	{0xa011, INTEL_MODEL_PINEM, "Atom N4xx"},
	{0xa012, INTEL_MODEL_PINEM, "Atom N5xx"},

	{0x0042, INTEL_MODEL_ILKG, "IronLake Desktop"},
	{0x0046, INTEL_MODEL_ILKGM, "IronLake Mobile"},
	{0x0046, INTEL_MODEL_ILKGM, "IronLake Mobile"},
	{0x0046, INTEL_MODEL_ILKGM, "IronLake Mobile"},

	{0x0102, INTEL_MODEL_SNBG, "SandyBridge Desktop GT1"},
	{0x0112, INTEL_MODEL_SNBG, "SandyBridge Desktop GT2"},
	{0x0122, INTEL_MODEL_SNBG, "SandyBridge Desktop GT2+"},
	{0x0106, INTEL_MODEL_SNBGM, "SandyBridge Mobile GT1"},
	{0x0116, INTEL_MODEL_SNBGM, "SandyBridge Mobile GT2"},
	{0x0126, INTEL_MODEL_SNBGM, "SandyBridge Mobile GT2+"},
	{0x010a, INTEL_MODEL_SNBGS, "SandyBridge Server"},

	{0x0152, INTEL_MODEL_IVBG, "IvyBridge Desktop GT1"},
	{0x0162, INTEL_MODEL_IVBG, "IvyBridge Desktop GT2"},
	{0x0156, INTEL_MODEL_IVBGM, "IvyBridge Mobile GT1"},
	{0x0166, INTEL_MODEL_IVBGM, "IvyBridge Mobile GT2"},
	{0x0152, INTEL_MODEL_IVBGS, "IvyBridge Server"},
	{0x015a, INTEL_MODEL_IVBGS, "IvyBridge Server GT1"},
	{0x016a, INTEL_MODEL_IVBGS, "IvyBridge Server GT2"},

	{0x0412, INTEL_MODEL_HAS, "Haswell Desktop"},
	{0x0416, INTEL_MODEL_HASM, "Haswell Mobile"},
	{0x0d26, INTEL_MODEL_HASM, "Haswell Mobile"},
	{0x0a16, INTEL_MODEL_HASM, "Haswell Mobile"},

	{0x0155, INTEL_MODEL_VLV, "ValleyView Desktop"},
	{0x0f30, INTEL_MODEL_VLVM, "ValleyView Mobile"},
	{0x0f31, INTEL_MODEL_VLVM, "ValleyView Mobile"},
	{0x0f32, INTEL_MODEL_VLVM, "ValleyView Mobile"},
	{0x0f33, INTEL_MODEL_VLVM, "ValleyView Mobile"},
	{0x0157, INTEL_MODEL_VLVM, "ValleyView Mobile"},

//	{0x1616, INTEL_MODEL_BDWM, "HD Graphics 5500 (Broadwell GT2)"},

	{0x1902, INTEL_MODEL_SKY,  "Skylake GT1"},
	{0x1906, INTEL_MODEL_SKYM, "Skylake GT1"},
	{0x190a, INTEL_MODEL_SKYS, "Skylake GT1"},
	{0x190b, INTEL_MODEL_SKY,  "Skylake GT1"},
	{0x190e, INTEL_MODEL_SKYM, "Skylake GT1"},
	{0x1912, INTEL_MODEL_SKY,  "Skylake GT2"},
	{0x1916, INTEL_MODEL_SKYM, "Skylake GT2"},
	{0x191a, INTEL_MODEL_SKYS, "Skylake GT2"},
	{0x191b, INTEL_MODEL_SKY,  "Skylake GT2"},
	{0x191d, INTEL_MODEL_SKY,  "Skylake GT2"},
	{0x191e, INTEL_MODEL_SKYM, "Skylake GT2"},
	{0x1921, INTEL_MODEL_SKYM, "Skylake GT2F"},
	{0x1926, INTEL_MODEL_SKYM, "Skylake GT3"},
	{0x192a, INTEL_MODEL_SKYS, "Skylake GT3"},
	{0x192b, INTEL_MODEL_SKY,  "Skylake GT3"},
};

int32 api_version = B_CUR_DRIVER_API_VERSION;

char* gDeviceNames[MAX_CARDS + 1];
intel_info* gDeviceInfo[MAX_CARDS];
pci_module_info* gPCI;
pci_x86_module_info* gPCIx86Module = NULL;
agp_gart_module_info* gGART;
mutex gLock;


static status_t
get_next_intel_extreme(int32* _cookie, pci_info &info, uint32 &type)
{
	int32 index = *_cookie;

	// find devices

	for (; gPCI->get_nth_pci_info(index, &info) == B_OK; index++) {
		// check vendor
		if (info.vendor_id != VENDOR_ID_INTEL
			|| info.class_base != PCI_display
			|| info.class_sub != PCI_vga)
			continue;

		// check device
		for (uint32 i = 0; i < sizeof(kSupportedDevices)
				/ sizeof(kSupportedDevices[0]); i++) {
			if (info.device_id == kSupportedDevices[i].device_id) {
				type = i;
				*_cookie = index + 1;
				return B_OK;
			}
		}
	}

	return B_ENTRY_NOT_FOUND;
}


static enum pch_info
detect_intel_pch()
{
	pci_info info;

	// find devices
	for (int32 i = 0; gPCI->get_nth_pci_info(i, &info) == B_OK; i++) {
		// check vendor
		if (info.vendor_id != VENDOR_ID_INTEL
			|| info.class_base != PCI_bridge
			|| info.class_sub != PCI_isa) {
			continue;
		}

		// check device
		unsigned short id = info.device_id & INTEL_PCH_DEVICE_ID_MASK;
		switch(id) {
			case INTEL_PCH_IBX_DEVICE_ID:
				ERROR("%s: Found Ibex Peak PCH\n", __func__);
				return INTEL_PCH_IBX;
			case INTEL_PCH_CPT_DEVICE_ID:
				ERROR("%s: Found CougarPoint PCH\n", __func__);
				return INTEL_PCH_CPT;
			case INTEL_PCH_PPT_DEVICE_ID:
				ERROR("%s: Found PantherPoint PCH\n", __func__);
				return INTEL_PCH_CPT;
			case INTEL_PCH_LPT_DEVICE_ID:
			case INTEL_PCH_LPT_LP_DEVICE_ID:
			case INTEL_PCH_WPT_DEVICE_ID:
			case INTEL_PCH_WPT_LP_DEVICE_ID:
				// WildcatPoint is LPT compatible
				ERROR("%s: Found LynxPoint PCH\n", __func__);
				return INTEL_PCH_LPT;
			case INTEL_PCH_SPT_DEVICE_ID:
			case INTEL_PCH_SPT_LP_DEVICE_ID:
				ERROR("%s: Found SunrisePoint PCH\n", __func__);
				return INTEL_PCH_SPT;
			case INTEL_PCH_KBP_DEVICE_ID:
				ERROR("%s: Found Kaby Lake PCH\n", __func__);
				return INTEL_PCH_KBP;
			case INTEL_PCH_CNP_DEVICE_ID:
			case INTEL_PCH_CNP_LP_DEVICE_ID:
				ERROR("%s: Found Cannon Lake PCH\n", __func__);
				return INTEL_PCH_CNP;
			case INTEL_PCH_ICP_DEVICE_ID:
				ERROR("%s: Found Ice Lake PCH\n", __func__);
				return INTEL_PCH_ICP;
		}
	}

	ERROR("%s: No PCH detected.\n", __func__);
	return INTEL_PCH_NONE;
}


extern "C" const char**
publish_devices(void)
{
	CALLED();
	return (const char**)gDeviceNames;
}


extern "C" status_t
init_hardware(void)
{
	CALLED();

	status_t status = get_module(B_PCI_MODULE_NAME,(module_info**)&gPCI);
	if (status != B_OK) {
		ERROR("pci module unavailable\n");
		return status;
	}

	int32 cookie = 0;
	uint32 type;
	pci_info info;
	status = get_next_intel_extreme(&cookie, info, type);

	put_module(B_PCI_MODULE_NAME);
	return status;
}


extern "C" status_t
init_driver(void)
{
	CALLED();

	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&gPCI);
	if (status != B_OK) {
		ERROR("pci module unavailable\n");
		return status;
	}

	status = get_module(B_AGP_GART_MODULE_NAME, (module_info**)&gGART);
	if (status != B_OK) {
		ERROR("AGP GART module unavailable\n");
		put_module(B_PCI_MODULE_NAME);
		return status;
	}

	mutex_init(&gLock, "intel extreme ksync");

	// Try to get the PCI x86 module as well so we can enable possible MSIs.
	if (get_module(B_PCI_X86_MODULE_NAME,
			(module_info **)&gPCIx86Module) != B_OK) {
		ERROR("failed to get pci x86 module\n");
		gPCIx86Module = NULL;
	}

	// find the PCH device (if any)
	enum pch_info pchInfo = detect_intel_pch();

	// find devices

	int32 found = 0;

	for (int32 cookie = 0; found < MAX_CARDS;) {
		pci_info* info = (pci_info*)malloc(sizeof(pci_info));
		if (info == NULL)
			break;

		uint32 type;
		status = get_next_intel_extreme(&cookie, *info, type);
		if (status < B_OK) {
			free(info);
			break;
		}

		// create device names & allocate device info structure

		char name[64];
		sprintf(name, "graphics/intel_extreme_%02x%02x%02x",
			 info->bus, info->device,
			 info->function);

		gDeviceNames[found] = strdup(name);
		if (gDeviceNames[found] == NULL)
			break;

		gDeviceInfo[found] = (intel_info*)malloc(sizeof(intel_info));
		if (gDeviceInfo[found] == NULL) {
			free(gDeviceNames[found]);
			break;
		}

		// initialize the structure for later use

		memset(gDeviceInfo[found], 0, sizeof(intel_info));
		gDeviceInfo[found]->init_status = B_NO_INIT;
		gDeviceInfo[found]->id = found;
		gDeviceInfo[found]->pci = info;
		gDeviceInfo[found]->registers = info->u.h0.base_registers[0];
		gDeviceInfo[found]->device_identifier = kSupportedDevices[type].name;
		gDeviceInfo[found]->device_type = kSupportedDevices[type].type;
		gDeviceInfo[found]->pch_info = pchInfo;

		dprintf(DEVICE_NAME ": (%" B_PRId32 ") %s, revision = 0x%x\n", found,
			kSupportedDevices[type].name, info->revision);

		found++;
	}

	gDeviceNames[found] = NULL;

	if (found == 0) {
		mutex_destroy(&gLock);
		put_module(B_AGP_GART_MODULE_NAME);
		put_module(B_PCI_MODULE_NAME);
		if (gPCIx86Module != NULL) {
			gPCIx86Module = NULL;
			put_module(B_PCI_X86_MODULE_NAME);
		}
		return ENODEV;
	}

	return B_OK;
}


extern "C" void
uninit_driver(void)
{
	CALLED();

	mutex_destroy(&gLock);

	// free device related structures
	char* name;
	for (int32 index = 0; (name = gDeviceNames[index]) != NULL; index++) {
		free(gDeviceInfo[index]);
		free(name);
	}

	put_module(B_AGP_GART_MODULE_NAME);
	put_module(B_PCI_MODULE_NAME);
	if (gPCIx86Module != NULL) {
		gPCIx86Module = NULL;
		put_module(B_PCI_X86_MODULE_NAME);
	}
}


extern "C" device_hooks*
find_device(const char* name)
{
	CALLED();

	int index;
	for (index = 0; gDeviceNames[index] != NULL; index++) {
		if (!strcmp(name, gDeviceNames[index]))
			return &gDeviceHooks;
	}

	return NULL;
}
