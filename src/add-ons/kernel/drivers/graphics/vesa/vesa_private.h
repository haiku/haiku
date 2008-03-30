/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VESA_PRIVATE_H
#define VESA_PRIVATE_H


#include <Drivers.h>
#include <Accelerant.h>
#include <PCI.h>


#define DEVICE_NAME				"vesa"
#define VESA_ACCELERANT_NAME	"vesa.accelerant"


struct vesa_get_supported_modes;

struct vesa_info {
	uint32			cookie_magic;
	int32			open_count;
	int32			id;
	pci_info		*pci;
	struct vesa_shared_info *shared_info;
	area_id			shared_area;
};

extern status_t vesa_init(vesa_info &info);
extern void vesa_uninit(vesa_info &info);

#endif	/* VESA_PRIVATE_H */
