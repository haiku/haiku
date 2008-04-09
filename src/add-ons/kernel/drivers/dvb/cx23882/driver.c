/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "driver.h"
#include "dvb_interface.h"

#define TRACE_DRIVER
#ifdef TRACE_DRIVER
  #define TRACE dprintf
#else
  #define TRACE(a...)
#endif

typedef struct
{
	int			vendor;
	int			device;
	int			subvendor;
	int			subdevice;
	const char *name;
} card_info;

int32 				api_version = B_CUR_DRIVER_API_VERSION;
pci_module_info *	gPci;
static char *		sDevNameList[MAX_CARDS + 1];
static pci_info *	sDevList[MAX_CARDS];
static int32		sOpenMask;

static card_info sCardTable[] =
{
	{ 0x14f1, 0x8802, 0x0070, 0x9002, "Hauppauge WinTV-NOVA-T model 928" },
	{ /* end */ }
};


typedef struct
{
	void *	cookie;
	int		dev_id;
} interface_cookie;


static const char *
identify_device(const card_info *cards, const pci_info *info)
{
	for (; cards->name; cards++) {
		if (cards->vendor >= 0 && cards->vendor != info->vendor_id)
			continue;
		if (cards->device >= 0 && cards->device != info->device_id)
			continue;
		if ((info->header_type & PCI_header_type_mask) != PCI_header_type_generic)
			continue;
		if (cards->subvendor >= 0 && cards->subvendor != info->u.h0.subsystem_vendor_id)
			continue;
		if (cards->subdevice >= 0 && cards->subdevice != info->u.h0.subsystem_id)
			continue;
		return cards->name;		
	}
	return NULL;
}


status_t
init_hardware(void)
{
	pci_info info;
	status_t res;
	int i;

	TRACE("cx23882: init_hardware()\n");

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPci) < B_OK)
		return B_ERROR;
	for (res = B_ERROR, i = 0; gPci->get_nth_pci_info(i, &info) == B_OK; i++) {
		if (identify_device(sCardTable, &info)) {
			res = B_OK;
			break;
		}
	}
	put_module(B_PCI_MODULE_NAME);
	gPci = NULL;

	return res;
}


status_t
init_driver(void)
{
	struct pci_info *item;
	int index;
	int cards;

#if defined(DEBUG) && !defined(__HAIKU__)
	set_dprintf_enabled(true);
	load_driver_symbols("cx23882");
#endif
	
	dprintf(INFO);
	
	item = (pci_info *)malloc(sizeof(pci_info));
	if (!item)
		return B_NO_MEMORY;
	
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPci) < B_OK) {
		free(item);
		return B_ERROR;
	}
	
	for (cards = 0, index = 0; gPci->get_nth_pci_info(index++, item) == B_OK; ) {
		const char *info = identify_device(sCardTable, item);
		if (info) {
			char name[64];
			sprintf(name, "dvb/cx23882/%d", cards + 1);
			dprintf("cx23882: /dev/%s is a %s\n", name, info);
			sDevList[cards] = item;
			sDevNameList[cards] = strdup(name);
			sDevNameList[cards + 1] = NULL;
			cards++;
			item = (pci_info *)malloc(sizeof(pci_info));
			if (!item)
				goto err_outofmem;
			if (cards == MAX_CARDS)
				break;
		}
	}

	free(item);

	if (!cards)
		goto err_cards;
	
	return B_OK;

err_outofmem:
	TRACE("cx23882: err_outofmem\n");
	for (index = 0; index < cards; index++) {
		free(sDevList[index]);
		free(sDevNameList[index]);
	}
err_cards:
	put_module(B_PCI_MODULE_NAME);
	return B_ERROR;
}


void
uninit_driver(void)
{
	int32 i;

	TRACE("cx23882: uninit_driver\n");
	
	for (i = 0; sDevNameList[i] != NULL; i++) {
		free(sDevList[i]);
		free(sDevNameList[i]);
	}
	
	put_module(B_PCI_MODULE_NAME);
}


static status_t
driver_open(const char *name, uint32 flags, void** _cookie)
{
	interface_cookie *cookie;
	char *deviceName;
	status_t status;
	int dev_id;
	int mask;
	
	TRACE("cx23882: driver open\n");

	for (dev_id = 0; (deviceName = sDevNameList[dev_id]) != NULL; dev_id++) {
		if (!strcmp(name, deviceName))
			break;
	}		
	if (deviceName == NULL) {
		TRACE("cx23882: invalid device name\n");
		return B_ERROR;
	}
	
	// allow only one concurrent access
	mask = 1 << dev_id;
	if (atomic_or(&sOpenMask, mask) & mask)
		return B_BUSY;
		
	cookie = (interface_cookie *)malloc(sizeof(interface_cookie));
	if (!cookie)
		return B_NO_MEMORY;
		
	cookie->dev_id = dev_id;
	status = interface_attach(&cookie->cookie, sDevList[dev_id]);
	if (status != B_OK) {
		free(cookie);
		atomic_and(&sOpenMask, ~(1 << dev_id));
		return status;
	}

	*_cookie = cookie;
	return B_OK;
}


static status_t
driver_close(void* cookie)
{
	TRACE("cx23882: driver close enter\n");
	interface_detach(((interface_cookie *)cookie)->cookie);
	TRACE("cx23882: driver close leave\n");
	return B_OK;
}


static status_t
driver_free(void* cookie)
{
	TRACE("cx23882: driver free\n");
	atomic_and(&sOpenMask, ~(1 << ((interface_cookie *)cookie)->dev_id));
	free(cookie);
	return B_OK;
}


static status_t
driver_read(void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	TRACE("cx23882: driver read\n");
	*num_bytes = 0; // required by design for read hook!
	return B_ERROR;
}


static status_t
driver_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	TRACE("cx23882: driver write\n");
	*num_bytes = 0; // not sure if required for write hook
	return B_ERROR;
}


static status_t
driver_control(void *cookie, uint32 op, void *arg, size_t len)
{
//	TRACE("cx23882: driver control\n");
	return interface_ioctl(((interface_cookie *)cookie)->cookie, op, arg, len);
}


static device_hooks
sDeviceHooks = {
	driver_open,
	driver_close,
	driver_free,
	driver_control,
	driver_read,
	driver_write,
};


const char**
publish_devices(void)
{
	return (const char**)sDevNameList;
}


device_hooks*
find_device(const char* name)
{
	return &sDeviceHooks;
}
