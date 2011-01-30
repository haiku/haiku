/*
 *	SiS 7018, Trident 4D Wave DX/NX, Acer Lab M5451 Sound Driver.
 *	Copyright (c) 2002, 2008-2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _SiS7018_DRIVER_H_
#define _SiS7018_DRIVER_H_


#include <Drivers.h>
#include <PCI.h>


#define DRIVER_NAME	"sis7018"
#define MAX_DEVICES	3


const char* const kVersion = "2.0.2";

extern pci_module_info *gPCI;

extern "C" {

status_t		init_hardware();
status_t		init_driver();
void			uninit_driver();
const char**	publish_devices();
device_hooks*	find_device(const char* name);

}


#endif // _SiS7018_DRIVER_H_

