#ifndef _CB_ENABLER_H
#define _CB_ENABLER_H

#define __KERNEL__

#include <pcmcia/config.h>
#include <pcmcia/k_compat.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

#undef __KERNEL__

#include <PCI.h>

#define CB_ENABLER_MODULE_NAME	"bus_managers/cb_enabler/v0"

typedef struct cb_device_descriptor cb_device_descriptor;
typedef struct cb_notify_hooks cb_notify_hooks;
typedef struct cb_enabler_module_info cb_enabler_module_info;

struct cb_device_descriptor
{
	uint16 vendor_id;  /* 0xffff is don't-care */
	uint16 device_id;  /* 0xffff is don't-care */
	uint8 class_base;  /* 0xff is don't-care */
	uint8 class_sub;   /* 0xff is don't-care */
	uint8 class_api;   /* 0xff is don't-care */
};

struct cb_notify_hooks
{
	status_t (*device_added)(pci_info *pci, void **cookie);
	void (*device_removed)(void *cookie);
};

struct cb_enabler_module_info {
    bus_manager_info	binfo;
	
	void (*register_driver)(const char *name, 
							const cb_device_descriptor *descriptors, 
							size_t count);
	void (*install_notify)(const char *name, 
						   const cb_notify_hooks *hooks);
	void (*uninstall_notify)(const char *name, 
							  const cb_notify_hooks *hooks);
};

#endif
