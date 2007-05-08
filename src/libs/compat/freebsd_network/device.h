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

	sem_id			link_state_sem;

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
	DEVICE_DESC_ALLOCED	= 1 << 3,
};


#define UNIMPLEMENTED() \
	panic("fbsd compat, unimplemented: " __FUNCTION__)

status_t init_mbufs(void);
void uninit_mbufs(void);

status_t init_mutexes(void);
void uninit_mutexes(void);

status_t init_compat_layer(void);

status_t init_taskqueues(void);
void uninit_taskqueues(void);

/* busdma_machdep.c */
void init_bounce_pages(void);
void uninit_bounce_pages(void);

void driver_printf(const char *format, ...) __attribute__ ((format (__printf__, 1, 2)));
void driver_vprintf(const char *format, va_list vl);

void ifq_init(struct ifqueue *ifq, const char *name);
void ifq_uninit(struct ifqueue *ifq);

extern struct net_stack_module_info *gStack;
extern pci_module_info *gPci;

extern char *gDevNameList[];
extern struct device *gDevices[];

#endif
