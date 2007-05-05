/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 *
 * Some of this code is based on previous work by Marcus Overhagen.
 */

#include "device.h"

#include <stdlib.h>
#include <Drivers.h>
#include <ether_driver.h>

#include <compat/sys/haiku-module.h>

#include <compat/sys/mbuf.h>
#include <compat/net/ethernet.h>


#define MAX_DEVICES		8


char *gDevNameList[MAX_DEVICES + 1];


static status_t
compat_open(const char *name, uint32 flags, void **cookie)
{
	return B_ERROR;
}


static status_t
compat_close(void *cookie)
{
	device_t dev = cookie;
	return B_ERROR;
}


static status_t
compat_free(void *cookie)
{
	device_t dev = cookie;
	free(dev);
	return B_ERROR;
}


static status_t
compat_read(void *cookie, off_t position, void *buf, size_t *numBytes)
{
	uint32 semFlags = B_CAN_INTERRUPT;
	device_t dev = cookie;
	status_t status;
	struct mbuf *mb;
	size_t len;

	if (dev->flags & DEVICE_CLOSED)
		return B_INTERRUPTED;

	if (dev->flags & DEVICE_NON_BLOCK)
		semFlags |= B_RELATIVE_TIMEOUT;

	do {
		status = acquire_sem_etc(dev->receive_sem, 1, semFlags, 0);
		if (dev->flags & DEVICE_CLOSED)
			return B_INTERRUPTED;

		if (status == B_WOULD_BLOCK) {
			*numBytes = 0;
			return B_OK;
		} else if (status < B_OK)
			return status;

		IF_DEQUEUE(&dev->receive_queue, mb);
	} while (mb == NULL);

	len = min_c(max_c((size_t)mb->m_len, 0), *numBytes);

	mb = m_defrag(mb, 0);
	if (mb == NULL) {
		*numBytes = 0;
		return B_NO_MEMORY;
	}

	memcpy(buf, mtod(mb, const void *), len);
	*numBytes = len;

	m_freem(mb);
	return B_OK;
}


static status_t
compat_write(void *cookie, off_t position, const void *buffer,
	size_t *numBytes)
{
	device_t dev = cookie;
	struct mbuf *mb;

	mb = m_getcl(0, MT_DATA, 0);
	if (mb == NULL)
		return ENOBUFS;

	/* if we are waiting, check after if the ifp is still valid */

	mb->m_len = min_c(*numBytes, (size_t)MCLBYTES);
	memcpy(mtod(mb, void *), buffer, mb->m_len);

	return dev->ifp->if_output(dev->ifp, mb, NULL, NULL);
}


static status_t
compat_control(void *cookie, uint32 op, void *arg, size_t len)
{
	device_t dev = cookie;

	switch (op) {
		case ETHER_INIT:
			return B_OK;

		case ETHER_GETADDR:
			memcpy(arg, IF_LLADDR(dev->ifp), ETHER_ADDR_LEN);
			return B_OK;

		case ETHER_NONBLOCK:
			if (*(int32 *)arg)
				dev->flags |= DEVICE_NON_BLOCK;
			else
				dev->flags &= ~DEVICE_NON_BLOCK;
			return B_OK;

		case ETHER_SETPROMISC:
			/* TODO */
			return B_ERROR;

		case ETHER_GETFRAMESIZE:
			*(uint32 *)arg = dev->ifp->if_mtu;
			return B_OK;

		case ETHER_ADDMULTI:
		case ETHER_REMMULTI:
			/* TODO */
			return B_ERROR;

		case ETHER_GET_LINK_STATE:
			/* TODO */
			return B_ERROR;

		case ETHER_SET_LINK_STATE_SEM:
			/* TODO */
			return B_OK;
	}

	return B_BAD_VALUE;
}


device_hooks gDeviceHooks = {
	compat_open,
	compat_close,
	compat_free,
	compat_control,
	compat_read,
	compat_write,
};


status_t
_fbsd_init_hardware(driver_t *driver)
{
	device_method_signature_t probe = NULL;
	struct device fakeDevice;
	int i;

	dprintf("%s: init_hardware(%p)\n", gDriverName, driver);

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPci) < B_OK)
		return B_ERROR;

	for (i = 0; probe == NULL && driver->methods[i].name != NULL; i++) {
		if (strcmp(driver->methods[i].name, "device_probe") == 0)
			probe = driver->methods[i].method;
	}

	if (probe == NULL) {
		dprintf("%s: driver has no device_probe method.\n", gDriverName);
		return B_ERROR;
	}

	memset(&fakeDevice, 0, sizeof(struct device));

	for (i = 0; gPci->get_nth_pci_info(i, &fakeDevice.pciInfo) == B_OK; i++) {
		if (probe(&fakeDevice) >= 0) {
			dprintf("%s, found %s at %i\n", gDriverName,
				fakeDevice.description, i);
			put_module(B_PCI_MODULE_NAME);
			return B_OK;
		}
	}

	dprintf("%s: no hardware found.\n", gDriverName);
	put_module(B_PCI_MODULE_NAME);

	return B_ERROR;
}


status_t
_fbsd_init_driver(driver_t *driver)
{
	dprintf("%s: init_driver(%p)\n", gDriverName, driver);

	return B_ERROR;
}


void
_fbsd_uninit_driver(driver_t *driver)
{
	dprintf("%s: uninit_driver(%p)\n", gDriverName, driver);
}
