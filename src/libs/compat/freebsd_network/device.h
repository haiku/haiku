/*
 * Copyright 2009, Colin Günther, coling@gmx.de. All Rights Reserved.
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Copyright 2004, Marcus Overhagen. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEVICE_H
#define DEVICE_H


#include <stdint.h>
#include <stdio.h>

#include <KernelExport.h>
#include <drivers/PCI.h>

#include <util/list.h>

#include <compat/sys/kernel.h>
#include <compat/net/if.h>

#include "shared.h"

#ifdef __cplusplus
extern "C" {
#endif

struct root_device_softc {
	enum {
		BUS_INVALID = 0,
		BUS_pci,
		BUS_uhub,
	} bus;

	struct pci_info	pci_info;
	bool			is_msi;
	bool			is_msix;

	struct freebsd_usb_device* usb_dev;
};

enum {
	DEVICE_OPEN			= 1 << 0,
	DEVICE_CLOSED		= 1 << 1,
	DEVICE_NON_BLOCK	= 1 << 2,
	DEVICE_DESC_ALLOCED	= 1 << 3,
	DEVICE_ATTACHED		= 1 << 4,
	DEVICE_SOFTC_SET	= 1 << 5  // Set through device_set_softc().
};


extern struct net_buffer_module_info *gBufferModule;
extern pci_module_info *gPci;


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

status_t init_hard_clock(void);
void uninit_hard_clock(void);

status_t init_callout(void);
void uninit_callout(void);

status_t init_pci();
void uninit_pci();

status_t init_usb();
void uninit_usb();

status_t get_next_usb_device(uint32* cookie, struct freebsd_usb_device* result);
status_t get_usb_device_attach_arg(struct freebsd_usb_device* device, struct usb_attach_arg* uaa);

device_t find_root_device(int);
pci_info* get_device_pci_info(device_t dev);

device_method_signature_t resolve_device_method(driver_t *driver, int id);

void driver_printf(const char *format, ...)
	__attribute__ ((format (__printf__, 1, 2)));
int driver_vprintf(const char *format, va_list vl);

void device_sprintf_name(device_t dev, const char *format, ...)
	__attribute__ ((format (__printf__, 2, 3)));

void ifq_init(struct ifqueue *, const char *);
void ifq_uninit(struct ifqueue *);

#ifdef __cplusplus
}
#endif

#endif	/* DEVICE_H */
