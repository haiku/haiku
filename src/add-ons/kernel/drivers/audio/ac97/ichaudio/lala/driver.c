#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <OS.h>
#include <PCI.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "lala.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;

#define MAX_DEVICES 8

char *	drv_path[MAX_DEVICES + 1];
drv_t *	drv_data[MAX_DEVICES];
int		drv_count;

pci_module_info *pcimodule;


status_t
init_hardware(void)
{
	dprintf("init_hardware\n");
	return B_OK;
}


status_t
init_driver(void)
{
	struct pci_info info; 
	struct pci_info *pciinfo = &info;
	int pciindex;
	int devindex;

	dprintf("init_driver\n");
	dprintf("driver base name '%s'\n", driver_info.basename);
	
	if (get_module(B_PCI_MODULE_NAME,(module_info **)&pcimodule) < 0) {
		return B_ERROR; 
	}
	
	drv_count = 0;
	
	for (pciindex = 0; B_OK == pcimodule->get_nth_pci_info(pciindex, pciinfo); pciindex++) { 
		dprintf("Checking PCI device, vendor 0x%04x, id 0x%04x, bus 0x%02x, dev 0x%02x, func 0x%02x, rev 0x%02x, api 0x%02x, sub 0x%02x, base 0x%02x\n",
			pciinfo->vendor_id, pciinfo->device_id, pciinfo->bus, pciinfo->device, pciinfo->function,
			pciinfo->revision, pciinfo->class_api, pciinfo->class_sub, pciinfo->class_base);
			
		for (devindex = 0; driver_info.table[devindex].vendor != 0; devindex++) {
			if (driver_info.table[devindex].vendor != -1 && driver_info.table[devindex].vendor != pciinfo->vendor_id)
				continue;
			if (driver_info.table[devindex].device != -1 && driver_info.table[devindex].device != pciinfo->device_id)
				continue;
			if (driver_info.table[devindex].revision != -1 && driver_info.table[devindex].revision != pciinfo->revision)
				continue;
			if (driver_info.table[devindex].class != -1 && driver_info.table[devindex].class != pciinfo->class_base)
				continue;
			if (driver_info.table[devindex].subclass != -1 && driver_info.table[devindex].subclass != pciinfo->class_sub)
				continue;
			if (pciinfo->header_type == 0) {
				if (driver_info.table[devindex].subsystem_vendor != -1 && driver_info.table[devindex].subsystem_vendor != pciinfo->u.h0.subsystem_vendor_id)
					continue;
				if (driver_info.table[devindex].subsystem_device != -1 && driver_info.table[devindex].subsystem_device != pciinfo->u.h0.subsystem_id)
					continue;
			}

			dprintf("found device '%s'\n", driver_info.table[devindex].name);
			
			drv_path[drv_count] = (char *) malloc(strlen(driver_info.basename) + 5);
			sprintf(drv_path[drv_count], "%s/%d", driver_info.basename, drv_count + 1);
			
			drv_data[drv_count] = (drv_t *) malloc(sizeof(drv_t));
			drv_data[drv_count]->pci 		= pcimodule;
			drv_data[drv_count]->bus		= pciinfo->bus;
			drv_data[drv_count]->device		= pciinfo->device;
			drv_data[drv_count]->function	= pciinfo->function;
			drv_data[drv_count]->name		= driver_info.table[devindex].name;
			drv_data[drv_count]->flags		= driver_info.table[devindex].flags;
		
			drv_count++;
			break;
		}
		if (drv_count == MAX_DEVICES)
			break;
	}
	
	drv_path[drv_count + 1] = NULL;
	
	return B_OK;
}

void
uninit_driver(void)
{
	int i;
	dprintf("uninit_driver\n");
	
	for (i = 0; i < drv_count; i++) {
		free(drv_path[i]);
		free(drv_data[i]);
	}
}

static status_t
ich_open(const char *name, uint32 flags, void** cookie)
{
	dprintf("open\n");
	return B_OK;
}

static status_t
ich_close(void* cookie)
{
	dprintf("close\n");
	return B_OK;
}

static status_t
ich_free(void* cookie)
{
	dprintf("free\n");
	return B_OK;
}

static status_t
ich_control(void* cookie, uint32 op, void* arg, size_t len)
{
	return B_OK;
}

static status_t
ich_read(void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	*num_bytes = 0;
	return B_IO_ERROR;
}

static status_t
ich_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;
	return B_IO_ERROR;
}

device_hooks ich_hooks = {
	ich_open,
	ich_close,
	ich_free,
	ich_control,
	ich_read,
	ich_write
};

const char **
publish_devices(void)
{
	dprintf("publish_devices\n");
	return (const char **) drv_path;
}

device_hooks*
find_device(const char* name)
{
	dprintf("find_device\n");
	return &ich_hooks;
}

