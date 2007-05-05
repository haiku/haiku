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

#include <net_stack.h>

#include <compat/sys/kernel.h>
#include <compat/net/if.h>
#include <compat/net/if_var.h>

struct ifnet;

struct device {
	pci_info		pci_info;
	char			dev_name[128];

	int32			flags;

	struct ifqueue	receive_queue;
	sem_id			receive_sem;

	struct ifnet *	ifp;

	int				unit;
	char			nameunit[64];
	const char *	description;
	void *			softc;
};


enum {
	DEVICE_OPEN			= 1 << 0,
	DEVICE_CLOSED		= 1 << 1,
	DEVICE_NON_BLOCK	= 1 << 2,
};


status_t init_mbufs(void);
void uninit_mbufs(void);

status_t init_mutexes(void);
void uninit_mutexes(void);

/* busdma_machdep.c */
void init_bounce_pages(void);
void uninit_bounce_pages(void);

extern struct net_stack_module_info *gStack;
extern pci_module_info *gPci;

extern char *gDevNameList[];
extern struct device *gDevices[];

#endif
