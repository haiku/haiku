/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 */
#ifndef __DEVICE_H
#define __DEVICE_H

#include <PCI.h>
#include "setup.h"

extern pci_module_info *gPci;

status_t ipro1000_open(const char *name, uint32 flags, void** cookie);
status_t ipro1000_read(void* cookie, off_t position, void *buf, size_t* num_bytes);
status_t ipro1000_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes);
status_t ipro1000_control(void *cookie, uint32 op, void *arg, size_t len);
status_t ipro1000_close(void* cookie);
status_t ipro1000_free(void* cookie);

struct adapter;

typedef struct device {
	int 				devId;
	pci_info *			pciInfo;
	
	struct adapter *	adapter;
	uint8				pciBus;
	uint8				pciDev;
	uint8				pciFunc;
	void *				regAddr;
	
	volatile bool		nonblocking;
	volatile bool		closed;
	
	uint8				macaddr[6];
} ipro1000_device;

#endif
