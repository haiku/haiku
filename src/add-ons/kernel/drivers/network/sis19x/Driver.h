/*
 *	SiS 190/191 NIC Driver.
 *	Copyright (c) 2009 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _SiS19X_DRIVER_H_
#define _SiS19X_DRIVER_H_


#include <Drivers.h>
#include <PCI.h>

#define DRIVER_NAME	"sis19x"
#define MAX_DEVICES	3
#define DEVNAME_LEN	32

#define CARDID(vendor_id, device_id)\
	(((uint32)(vendor_id) << 16) | (device_id))

#define VENDORID(card_id)	(((card_id) >> 16) & 0xffff)
#define DEVICEID(card_id)	 ((card_id) & 0xffff)


const char* const kVersion = "ver.1.0.0";

// ids for supported hardware
const uint32 SiS190 = CARDID(0x1039, 0x0190);
const uint32 SiS191 = CARDID(0x1039, 0x0191);

extern pci_module_info* gPCIModule;


extern "C" {

status_t		init_hardware();
status_t		init_driver();
void			uninit_driver();
const char**	publish_devices();
device_hooks*	find_device(const char* name);

}

#endif //_SiS19X_DRIVER_H_

