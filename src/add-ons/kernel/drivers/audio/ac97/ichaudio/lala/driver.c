#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <OS.h>
#include <PCI.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "lala.h"

#define MAX_DEVICES 8

int32 			api_version = B_CUR_DRIVER_API_VERSION;

sem_id			drv_sem;
char *			drv_path[MAX_DEVICES + 1];
audio_drv_t *	drv_data[MAX_DEVICES];
int				drv_open_count[MAX_DEVICES];
void *			drv_cookie[MAX_DEVICES];
int				drv_count;

pci_module_info *pcimodule;

extern driver_info_t driver_info;

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
	
	if (get_module(B_PCI_MODULE_NAME, (module_info **) &pcimodule) < 0) {
		return B_ERROR; 
	}
	
	drv_sem = create_sem(1, "drv_sem");
	
	drv_count = 0;
	
	for (pciindex = 0; B_OK == pcimodule->get_nth_pci_info(pciindex, pciinfo); pciindex++) { 
//		dprintf("Checking PCI device, vendor 0x%04x, id 0x%04x, bus 0x%02x, dev 0x%02x, func 0x%02x, rev 0x%02x, api 0x%02x, sub 0x%02x, base 0x%02x\n",
//			pciinfo->vendor_id, pciinfo->device_id, pciinfo->bus, pciinfo->device, pciinfo->function,
//			pciinfo->revision, pciinfo->class_api, pciinfo->class_sub, pciinfo->class_base);
			
		for (devindex = 0; driver_info.pci_id_table[devindex].vendor != 0; devindex++) {
			if (driver_info.pci_id_table[devindex].vendor != -1 && driver_info.pci_id_table[devindex].vendor != pciinfo->vendor_id)
				continue;
			if (driver_info.pci_id_table[devindex].device != -1 && driver_info.pci_id_table[devindex].device != pciinfo->device_id)
				continue;
			if (driver_info.pci_id_table[devindex].revision != -1 && driver_info.pci_id_table[devindex].revision != pciinfo->revision)
				continue;
			if (driver_info.pci_id_table[devindex].class != -1 && driver_info.pci_id_table[devindex].class != pciinfo->class_base)
				continue;
			if (driver_info.pci_id_table[devindex].subclass != -1 && driver_info.pci_id_table[devindex].subclass != pciinfo->class_sub)
				continue;
			if (pciinfo->header_type == 0) {
				if (driver_info.pci_id_table[devindex].subsystem_vendor != -1 && driver_info.pci_id_table[devindex].subsystem_vendor != pciinfo->u.h0.subsystem_vendor_id)
					continue;
				if (driver_info.pci_id_table[devindex].subsystem_device != -1 && driver_info.pci_id_table[devindex].subsystem_device != pciinfo->u.h0.subsystem_id)
					continue;
			}

			dprintf("found device '%s'\n", driver_info.pci_id_table[devindex].name);
			
			drv_path[drv_count] = (char *) malloc(strlen(driver_info.basename) + 5);
			sprintf(drv_path[drv_count], "%s/%d", driver_info.basename, drv_count + 1);
			
			drv_data[drv_count] = (audio_drv_t *) malloc(sizeof(audio_drv_t));
			drv_data[drv_count]->pci 		= pcimodule;
			drv_data[drv_count]->bus		= pciinfo->bus;
			drv_data[drv_count]->device		= pciinfo->device;
			drv_data[drv_count]->function	= pciinfo->function;
			drv_data[drv_count]->name		= driver_info.pci_id_table[devindex].name;
			drv_data[drv_count]->param		= driver_info.pci_id_table[devindex].param;
			drv_open_count[drv_count]		= 0;

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
	delete_sem(drv_sem);
}

static status_t
driver_open(const char *name, uint32 flags, void** cookie)
{
	int index;
	status_t res;
	
	acquire_sem(drv_sem);
	
	for (index = 0; index < drv_count; index++) {
		if (0 == strcmp(drv_path[index], name))
			break;
	}
	if (index == drv_count) { // name not found
		release_sem(drv_sem);
		return B_ERROR;
	}
	*cookie = (void *) index;
	
	if (drv_open_count[index] == 0) {
		res = driver_info.attach(drv_data[index], &drv_cookie[index]);
		drv_open_count[index] = (res == B_OK) ? 1 : 0;
	} else {
		res = B_OK;
		drv_open_count[index]++;
	}

	release_sem(drv_sem);

	return res;
}

static status_t
driver_close(void* cookie)
{
	dprintf("close\n");
	return B_OK;
}

static status_t
driver_free(void* cookie)
{
	int index;
	status_t res;
	
	dprintf("free\n");
	
	index = (int) cookie;

	acquire_sem(drv_sem);

	drv_open_count[index]--;

	if (drv_open_count[index] == 0)
		res = driver_info.detach(drv_data[index], drv_cookie[index]);
	else
		res = B_OK;
	
	release_sem(drv_sem);

	return res;
}

static status_t
driver_control(void* cookie, uint32 op, void* arg, size_t len)
{
	return B_OK;
}

static status_t
driver_read(void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	*num_bytes = 0;
	return B_IO_ERROR;
}

static status_t
driver_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;
	return B_IO_ERROR;
}

device_hooks driver_hooks = {
	driver_open,
	driver_close,
	driver_free,
	driver_control,
	driver_read,
	driver_write
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
	return &driver_hooks;
}

