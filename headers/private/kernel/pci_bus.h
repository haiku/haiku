/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _PCI_BUS_H
#define _PCI_BUS_H

#include <stage2.h>

struct pci_cfg {
	uint16 vendor_id;
	uint16 device_id;
	
	uint16 command;
	uint16 status;
	
	uint8 revision_id;
	uint8 interface;
	uint8 sub_class;
	uint8 base_class;
	
	uint8 cache_line_size;
	uint8 latency_timer;
	uint8 header_type;
	uint8 bist;
	
	uint8 bus;
	uint8 unit;
	uint8 func;
	uint8 irq;

	uint32 base[6];
	uint32 size[6];
};

typedef enum {
	PCI_GET_CFG = 10099,
	PCI_DUMP_CFG
} pci_ioctl_cmd;

int pci_bus_init(kernel_args *ka);

#endif
