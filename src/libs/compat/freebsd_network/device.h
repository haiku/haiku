/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 *
 * Some of this code is based on previous work by Marcus Overhagen.
 */
#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <stdint.h>

#include <KernelExport.h>
#include <drivers/PCI.h>

#include <compat/sys/kernel.h>
#include <compat/net/if.h>
#include <compat/net/if_var.h>

struct ifnet;

struct device {
	int				devId;
	pci_info *		pciInfo;

	int32			flags;

	struct ifqueue	receive_queue;
	sem_id			receive_sem;

	struct ifnet *	ifp;
};


enum {
	DEVICE_CLOSED		= 1 << 0,
	DEVICE_NON_BLOCK	= 1 << 1,
};


extern pci_module_info *gPci;

#endif
