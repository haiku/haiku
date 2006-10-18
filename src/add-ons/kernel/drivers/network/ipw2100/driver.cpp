/*
	Driver for Intel(R) PRO/Wireless 2100 devices.
	Copyright (C) 2006 Michael Lotz <mmlr@mlotz.ch>
	Released under the terms of the MIT license.
*/

#include <Drivers.h>
#include <KernelExport.h>
#include <PCI.h>
#include <stdio.h>
#include <string.h>

#include "driver.h"
#include "ipw2100.h"
#include "kernel_cpp.h"


status_t	ipw2100_open(const char *name, uint32 flags, void **cookie);
status_t	ipw2100_close(void *cookie);
status_t	ipw2100_free(void *cookie);
status_t	ipw2100_control(void *cookie, uint32 op, void *args, size_t length);
status_t	ipw2100_read(void *cookie, off_t position, void *buffer, size_t *numBytes);
status_t	ipw2100_write(void *cookie, off_t position, const void *buffer, size_t *numBytes);


int32 api_version = B_CUR_DRIVER_API_VERSION;


pci_module_info *gPCIModule;
char *gDeviceNameList[MAX_INSTANCES + 1];
pci_info *gDeviceList[MAX_INSTANCES];
int32 gOpenMask = 0;

device_hooks gDeviceHooks = {
	ipw2100_open,
	ipw2100_close,
	ipw2100_free,
	ipw2100_control,
	ipw2100_read,
	ipw2100_write
};


const char *
identify_device(const pci_info *info)
{
	if (info->vendor_id != VENDOR_ID_INTEL)
		return NULL;

	switch (info->device_id) {
		case 0x1043: return "Intel(R) PRO/Wireless 2100";
	}

	return NULL;
}


status_t
init_hardware()
{
#ifdef TRACE_IPW2100
	set_dprintf_enabled(true);
#endif

	//TRACE((DRIVER_NAME": init hardware\n"));

	pci_module_info *module;
	status_t result = get_module(B_PCI_MODULE_NAME, (module_info **)&module);
	if (result < B_OK)
		return result;

	int32 index = 0;
	result = B_ERROR;
	pci_info info;
	while (module->get_nth_pci_info(index++, &info) == B_OK) {
		const char *deviceName = identify_device(&info);
		if (deviceName) {
			TRACE((DRIVER_NAME": found device \"%s\"\n", deviceName));
			result = B_OK;
			break;
		}
	}

	put_module(B_PCI_MODULE_NAME);
	return result;
}


status_t
init_driver()
{
	TRACE((DRIVER_NAME": init driver\n"));

	for (int32 i = 0; i < MAX_INSTANCES; i++)
		gDeviceList[i] = NULL;
	for (int32 i = 0; i < MAX_INSTANCES + 1; i++)
		gDeviceNameList[i] = NULL;

	pci_info *info = new pci_info;
	if (!info)
		return B_NO_MEMORY;

	status_t result = get_module(B_PCI_MODULE_NAME, (module_info **)&gPCIModule);
	if (result < B_OK) {
		delete info;
		return result;
	}

	int32 index = 0;
	int32 count = 0;
	while (gPCIModule->get_nth_pci_info(index++, info) == B_OK
		&& count < MAX_INSTANCES) {
		const char *deviceName = identify_device(info);
		if (!deviceName)
			continue;

		char publishName[64];
		sprintf(publishName, "net/ipw2100/%ld", count);

		gDeviceList[count] = info;
		gDeviceNameList[count] = strdup(publishName);

		info = new pci_info;
		if (!info)
			goto error_out_of_memory;

		dprintf(DRIVER_NAME": will publish an \"%s\" as device %ld to /dev/%s\n", deviceName, count, publishName);
		count++;
	}

	delete info;
	return B_OK;

error_out_of_memory:
	for (int32 i = 0; i < MAX_INSTANCES; i++) {
		free(gDeviceNameList[i]);
		delete gDeviceList[i];
		gDeviceNameList[i] = NULL;
		gDeviceList[i] = NULL;
	}

	put_module(B_PCI_MODULE_NAME);
	return B_ERROR;
}


void
uninit_driver()
{
	TRACE((DRIVER_NAME": uninit driver\n"));
	for (int32 i = 0; i < MAX_INSTANCES; i++) {
		free(gDeviceNameList[i]);
		delete gDeviceList[i];
		gDeviceNameList[i] = NULL;
		gDeviceList[i] = NULL;
	}

	put_module(B_PCI_MODULE_NAME);
}


const char **
publish_devices(void)
{
	//TRACE((DRIVER_NAME": publish devices\n"));
	return (const char **)gDeviceNameList;
}


device_hooks *
find_device(const char *name)
{
	//TRACE((DRIVER_NAME": find device \"%s\"\n", name));

	for (int32 i = 0; i < MAX_INSTANCES; i++) {
		if (strcmp(gDeviceNameList[i], name) == 0)
			return &gDeviceHooks;
	}

	TRACE_ALWAYS((DRIVER_NAME": couldn't find device \"%s\"\n", name));
	return NULL;
}


//#pragma mark -


status_t
ipw2100_open(const char *name, uint32 flags, void **cookie)
{
	//TRACE((DRIVER_NAME": open device\n"));
	int32 deviceID = -1;
	for (int32 i = 0; i < MAX_INSTANCES && gDeviceNameList[i]; i++) {
		if (strcmp(gDeviceNameList[i], name) == 0) {
			deviceID = i;
			break;
		}
	}

	if (deviceID < 0)
		return B_ERROR;

	// allow only one concurrent access
	int32 mask = 1 << deviceID;
	if (atomic_or(&gOpenMask, mask) & mask)
		return B_BUSY;

	IPW2100 *device = new IPW2100(deviceID, gDeviceList[deviceID], gPCIModule);
	status_t result = device->InitCheck();
	if (device->InitCheck() < B_OK) {
		delete device;
		return result;
	}

	*cookie = (void *)device;
	return device->Open(flags);
}


status_t
ipw2100_close(void *cookie)
{
	//TRACE((DRIVER_NAME": close device\n"));
	IPW2100 *device = (IPW2100 *)cookie;
	return device->Close();
}


status_t
ipw2100_free(void *cookie)
{
	//TRACE((DRIVER_NAME": free device\n"));
	IPW2100 *device = (IPW2100 *)cookie;

	int32 mask = 1 << device->DeviceID();

	device->Free();
	delete device;

	atomic_and(&gOpenMask, ~mask);
	return B_OK;
}


status_t
ipw2100_control(void *cookie, uint32 op, void *args, size_t length)
{
	//TRACE((DRIVER_NAME": control device\n"));
	IPW2100 *device = (IPW2100 *)cookie;
	return device->Control(op, args, length);
}


status_t
ipw2100_read(void *cookie, off_t position, void *buffer, size_t *numBytes)
{
	//TRACE((DRIVER_NAME": read device\n"));
	IPW2100 *device = (IPW2100 *)cookie;
	return device->Read(position, buffer, numBytes);
}


status_t
ipw2100_write(void *cookie, off_t position, const void *buffer, size_t *numBytes)
{
	//TRACE((DRIVER_NAME": write device\n"));
	IPW2100 *device = (IPW2100 *)cookie;
	return device->Write(position, buffer, numBytes);
}
