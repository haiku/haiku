/*
 *	SiS 190/191 NIC Driver.
 *	Copyright (c) 2009 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */


#include "Driver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lock.h>

#include "Device.h"
#include "Settings.h"


// TODO: Optimize buffers - use size 1536 instead of 2048 and dynamically determine count of descriptors.
// TODO: implement tx ring cleanup on reconnect (?)
// TODO: Tx speed is extremely low!!! Only 200 K/sek :-(


int32 api_version = B_CUR_DRIVER_API_VERSION;

size_t numCards = 0;
Device* gDevices[MAX_DEVICES] = {0};
char* gDeviceNames[MAX_DEVICES + 1] = {0};

pci_module_info* gPCIModule = NULL;


static Device::Info cardInfos[] = {
	{ SiS190,	"SiS190", "SiS 190 PCI Fast Ethernet Adapter"	 },
	{ SiS191,	"SiS191", "SiS 191 PCI Gigabit Ethernet Adapter" }
};


status_t
init_hardware()
{
	TRACE_ALWAYS("SiS19X:init_hardware()\n");
	status_t result = get_module(B_PCI_MODULE_NAME, (module_info**)&gPCIModule);
	if (result < B_OK) {
		return ENOSYS;
	}

	pci_info info = {0};
	for (long i = 0; B_OK == (*gPCIModule->get_nth_pci_info)(i, &info); i++) {
		for (size_t idx = 0; idx < _countof(cardInfos); idx++) {
			if (CARDID(info.vendor_id, info.device_id) == cardInfos[idx].Id()) {
				TRACE_ALWAYS("Found:%s %#010x\n",
						cardInfos[idx].Description(), cardInfos[idx].Id());
				put_module(B_PCI_MODULE_NAME);
				return B_OK;
			}
		}
	}

	put_module(B_PCI_MODULE_NAME);
	return ENODEV;
}


static int SiS19X_DebuggerCommand(int argc, char** argv)
{
	const char* usageInfo = "usage:" DRIVER_NAME " [index] <t|r>\n"
							" - t - dump Transmit ring;\n"
							" - r - dump Receive ring.\n"
							" - g - dump reGisters.\n";

	uint64 cardId = 0;
	int cmdIndex = 1;

	if (argc < 2) {
		kprintf(usageInfo);
		return 0;
	} else
	if (argc > 2) {
		cardId = parse_expression(argv[2]);
		cmdIndex++;
	}

	if (cardId >= numCards) {
		kprintf("%" B_PRId64 " - invalid index.\n", cardId);
		kprintf(usageInfo);
		return 0;
	}

	Device* device = gDevices[cardId];
	if (device == NULL) {
		kprintf("Invalid device pointer!!!.\n");
		return 0;
	}

	switch(*argv[cmdIndex]) {
		case 'g': device->DumpRegisters(); break;
		case 't': device->fTxDataRing.Dump(); break;
		case 'r': device->fRxDataRing.Dump(); break;
		default:
			kprintf("'%s' - invalid parameter\n", argv[cmdIndex]);
			kprintf(usageInfo);
			break;
	}	

	return 0;
}


status_t
init_driver()
{
	status_t status = get_module(B_PCI_MODULE_NAME, (module_info**)&gPCIModule);
	if (status < B_OK) {
		return ENOSYS;
	}

	load_settings();

	TRACE_ALWAYS("%s\n", kVersion);

	pci_info info = {0};
	for (long i = 0; B_OK == (*gPCIModule->get_nth_pci_info)(i, &info); i++) {
		for (size_t idx = 0; idx < _countof(cardInfos); idx++) {
			if (info.vendor_id == cardInfos[idx].VendorId()
					&& info.device_id == cardInfos[idx].DeviceId())
			{
				TRACE_ALWAYS("Found:%s %#010x\n",
						cardInfos[idx].Description(), cardInfos[idx].Id());

				if (numCards == MAX_DEVICES) {
					break;
				}

				Device* device = new Device(cardInfos[idx], info);
				if (device == 0) {
					return ENODEV;
				}

				status_t status = device->InitCheck();
				if (status < B_OK) {
					delete device;
					break;
				}

				status = device->SetupDevice();
				if (status < B_OK) {
					delete device;
					break;
				}

				char name[DEVNAME_LEN] = {0};
				sprintf(name, "net/%s/%ld", cardInfos[idx].Name(), numCards);
				gDeviceNames[numCards] = strdup(name);
				gDevices[numCards++] = device;
			}
		}
	}

	if (numCards == 0) {
		put_module(B_PCI_MODULE_NAME);
		return ENODEV;
	}

	add_debugger_command(DRIVER_NAME, SiS19X_DebuggerCommand,
								"SiS190/191 Ethernet driver info");

	return B_OK;
}


void
uninit_driver()
{
	remove_debugger_command(DRIVER_NAME, SiS19X_DebuggerCommand);

	for (size_t i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i]) {
			gDevices[i]->TeardownDevice();
			delete gDevices[i];
			gDevices[i] = NULL;
		}

		free(gDeviceNames[i]);
		gDeviceNames[i] = NULL;
	}

	put_module(B_PCI_MODULE_NAME);

	release_settings();
}


static status_t
SiS19X_open(const char* name, uint32 flags, void** cookie)
{
	status_t status = ENODEV;
	*cookie = NULL;
	for (size_t i = 0; i < MAX_DEVICES; i++) {
		if (gDeviceNames[i] && !strcmp(gDeviceNames[i], name)) {
			status = gDevices[i]->Open(flags);
			*cookie = gDevices[i];
		}
	}

	return status;
}


static status_t
SiS19X_read(void* cookie, off_t position, void* buffer, size_t* numBytes)
{
	Device* device = (Device*)cookie;
	return device->Read((uint8*)buffer, numBytes);
}


static status_t
SiS19X_write(void* cookie, off_t position,
							const void* buffer, size_t* numBytes)
{
	Device* device = (Device*)cookie;
	return device->Write((const uint8*)buffer, numBytes);
}


static status_t
SiS19X_control(void* cookie, uint32 op, void* buffer, size_t length)
{
	Device* device = (Device*) cookie;
	return device->Control(op, buffer, length);
}


static status_t
SiS19X_close(void* cookie)
{
	Device* device = (Device*)cookie;
	return device->Close();
}


static status_t
SiS19X_free(void* cookie)
{
	Device* device = (Device*)cookie;
	return device->Free();
}


const char**
publish_devices()
{
	for (size_t i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i] == NULL)
			continue;

		if (gDeviceNames[i])
			TRACE("%s\n", gDeviceNames[i]);
	}

	return (const char**)&gDeviceNames[0];
}


device_hooks*
find_device(const char* name)
{
	static device_hooks deviceHooks = {
		SiS19X_open,
		SiS19X_close,
		SiS19X_free,
		SiS19X_control,
		SiS19X_read,
		SiS19X_write,
		NULL,	// select
		NULL	// deselect
	};

	return &deviceHooks;
}

