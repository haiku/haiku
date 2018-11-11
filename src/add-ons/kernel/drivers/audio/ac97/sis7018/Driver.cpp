/*
 *	SiS 7018, Trident 4D Wave DX/NX, Acer Lab M5451 Sound Driver.
 *	Copyright (c) 2002, 2008-2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */


#include "Driver.h"

#include <kernel_cpp.h>
#include <string.h>

#include "Device.h"
#include "Settings.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;

size_t gNumCards = 0;
Device *gDevices[MAX_DEVICES] = { 0 };
char *gDeviceNames[MAX_DEVICES + 1] = { 0 };
pci_module_info *gPCI = NULL;


static Device::Info cardInfos[] = {
	{ SiS7018, "SiS 7018" },
	{ ALi5451, "ALi M5451" },
	{ TridentDX, "Trident DX" },
	{ TridentNX, "Trident NX" }
};


status_t
init_hardware()
{
	dprintf("sis7018:init_hardware:ver:%s\n", kVersion);
	status_t result = get_module(B_PCI_MODULE_NAME, (module_info **)&gPCI);
	if (result < B_OK) {
		return ENOSYS;
	}

	pci_info info = {0};
	for (long i = 0; B_OK == (*gPCI->get_nth_pci_info)(i, &info); i++) {
		for (size_t idx = 0; idx < B_COUNT_OF(cardInfos); idx++) {
			if (info.vendor_id == cardInfos[idx].VendorId() &&
				info.device_id == cardInfos[idx].DeviceId())
			{
				put_module(B_PCI_MODULE_NAME);
				return B_OK;
			}
		}
	}

	put_module(B_PCI_MODULE_NAME);
	return ENODEV;
}


status_t
init_driver()
{
	status_t status = get_module(B_PCI_MODULE_NAME, (module_info **)&gPCI);
	if (status < B_OK) {
		return ENOSYS;
	}

	load_settings();

	pci_info info = { 0 };
	for (long i = 0; B_OK == (*gPCI->get_nth_pci_info)(i, &info); i++) {
		for (size_t idx = 0; idx < B_COUNT_OF(cardInfos); idx++) {
			if (info.vendor_id == cardInfos[idx].VendorId() &&
				info.device_id == cardInfos[idx].DeviceId())
			{
				if (gNumCards == MAX_DEVICES) {
					ERROR("Skipped:%s [%#06x:%#06x]\n", cardInfos[idx].Name(),
						cardInfos[idx].VendorId(), cardInfos[idx].DeviceId());
					break;
				}

				Device* device = new(std::nothrow) Device(cardInfos[idx], info);
				if (device == 0) {
					return ENODEV;
				}

				status_t status = device->InitCheck();
				if (status < B_OK) {
					delete device;
					break;
				}

				status = device->Setup();
				if (status < B_OK) {
					delete device;
					break;
				}

				char name[32] = {0};
				sprintf(name, "audio/hmulti/%s/%ld",
								cardInfos[idx].Name(), gNumCards);
				gDeviceNames[gNumCards] = strdup(name);
				gDevices[gNumCards++] = device;

				TRACE("Found:%s [%#06x:%#06x]\n", cardInfos[idx].Name(),
						cardInfos[idx].VendorId(), cardInfos[idx].DeviceId());
			}
		}
	}

	if (gNumCards == 0) {
		put_module(B_PCI_MODULE_NAME);
		return ENODEV;
	}

	return B_OK;
}


void
uninit_driver()
{
	for (size_t i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i]) {
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
SiS7018_open(const char *name, uint32 flags, void **cookie)
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
SiS7018_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	Device *device = (Device *)cookie;
	return device->Read((uint8 *)buffer, numBytes);
}


static status_t
SiS7018_write(void *cookie, off_t position,
							const void *buffer, size_t *numBytes)
{
	Device *device = (Device *)cookie;
	return device->Write((const uint8 *)buffer, numBytes);
}


static status_t
SiS7018_control(void *cookie, uint32 op, void *buffer, size_t length)
{
	Device *device = (Device *)cookie;
	return device->Control(op, buffer, length);
}


static status_t
SiS7018_close(void *cookie)
{
	Device *device = (Device *)cookie;
	return device->Close();
}


static status_t
SiS7018_free(void *cookie)
{
	Device *device = (Device *)cookie;
	return device->Free();
}


const char **
publish_devices()
{
	for (size_t i = 0; i < MAX_DEVICES; i++) {
		if (gDevices[i] == NULL)
			continue;

		if (gDeviceNames[i])
			TRACE("%s\n", gDeviceNames[i]);
	}

	return (const char **)&gDeviceNames[0];
}


device_hooks *
find_device(const char *name)
{
	static device_hooks deviceHooks = {
		SiS7018_open,
		SiS7018_close,
		SiS7018_free,
		SiS7018_control,
		SiS7018_read,
		SiS7018_write,
		NULL,				// select
		NULL				// deselect
	};

	return &deviceHooks;
}

