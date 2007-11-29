/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Copyright 2004, Marcus Overhagen. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef DEVICE_H
#define DEVICE_H


#include <stdint.h>
#include <stdio.h>

#include <KernelExport.h>
#include <drivers/PCI.h>

#include <util/list.h>

#include <net_stack.h>

#include <compat/sys/kernel.h>
#include <compat/net/if.h>
#include <compat/net/if_var.h>


#define MAX_DEVICES	8

struct ifnet;

struct device {
	struct device	*parent;
	struct device	*root;

	driver_t		*driver;
	struct list		children;

	int32			flags;

	char			device_name[128];
	int				unit;
	char			nameunit[64];
	const char		*description;
	void			*softc;
	void			*ivars;

	struct {
		int (*probe)(device_t dev);
		int (*attach)(device_t dev);
		int (*detach)(device_t dev);
		int (*suspend)(device_t dev);
		int (*resume)(device_t dev);
		void (*shutdown)(device_t dev);

		int (*miibus_readreg)(device_t, int, int);
		int (*miibus_writereg)(device_t, int, int, int);
		void (*miibus_statchg)(device_t);
		void (*miibus_linkchg)(device_t);
		void (*miibus_mediainit)(device_t);
	} methods;

	struct list_link link;
};

struct root_device_softc {
	struct pci_info	pci_info;
};

enum {
	DEVICE_OPEN			= 1 << 0,
	DEVICE_CLOSED		= 1 << 1,
	DEVICE_NON_BLOCK	= 1 << 2,
	DEVICE_DESC_ALLOCED	= 1 << 3,
	DEVICE_ATTACHED		= 1 << 4
};


extern struct net_stack_module_info *gStack;
extern pci_module_info *gPci;

extern const char *gDeviceNameList[];
extern struct ifnet *gDevices[];
extern int32 gDeviceCount;


static inline void
__unimplemented(const char *method)
{
	char msg[128];
	snprintf(msg, sizeof(msg), "fbsd compat, unimplemented: %s", method);
	panic(msg);
}


#define UNIMPLEMENTED() __unimplemented(__FUNCTION__)

status_t init_mbufs(void);
void uninit_mbufs(void);

status_t init_mutexes(void);
void uninit_mutexes(void);

status_t init_taskqueues(void);
void uninit_taskqueues(void);

device_t find_root_device(int unit);

/* busdma_machdep.c */
void init_bounce_pages(void);
void uninit_bounce_pages(void);

void driver_printf(const char *format, ...)
	__attribute__ ((format (__printf__, 1, 2)));
void driver_vprintf(const char *format, va_list vl);

void device_sprintf_name(device_t dev, const char *format, ...)
	__attribute__ ((format (__printf__, 2, 3)));

void ifq_init(struct ifqueue *ifq, const char *name);
void ifq_uninit(struct ifqueue *ifq);

#endif	/* DEVICE_H */
