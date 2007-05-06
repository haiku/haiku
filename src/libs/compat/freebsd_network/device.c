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

#include <compat/sys/bus.h>
#include <compat/sys/mbuf.h>
#include <compat/net/ethernet.h>


#define MAX_DEVICES		8


device_t gDevices[MAX_DEVICES];
char *gDevNameList[MAX_DEVICES + 1];


static device_probe_t  *sDeviceProbe;
static device_attach_t *sDeviceAttach;
static device_detach_t *sDeviceDetach;


static device_t
allocate_device(driver_t *driver)
{
	char semName[64];

	device_t dev = (device_t)malloc(sizeof(struct device));
	if (dev == NULL)
		return NULL;

	memset(dev, 0, sizeof(struct device));

	snprintf(semName, sizeof(semName), "%s rcv", gDriverName);

	dev->softc = malloc(driver->softc_size);
	if (dev->softc == NULL) {
		free(dev);
		return NULL;
	}

	dev->receive_sem = create_sem(0, semName);
	if (dev->receive_sem < 0) {
		free(dev->softc);
		free(dev);
		return NULL;
	}

	ifq_init(&dev->receive_queue, semName);

	return dev;
}


static void
free_device(device_t dev)
{
	delete_sem(dev->receive_sem);
	ifq_uninit(&dev->receive_queue);
	free(dev->softc);
	free(dev);
}


static device_method_signature_t
_resolve_method(driver_t *driver, const char *name)
{
	device_method_signature_t method = NULL;
	int i;

	for (i = 0; method == NULL && driver->methods[i].name != NULL; i++) {
		if (strcmp(driver->methods[i].name, name) == 0)
			method = driver->methods[i].method;
	}

	return method;
}


static status_t
compat_open(const char *name, uint32 flags, void **cookie)
{
	status_t status;
	device_t dev;
	int i;

	driver_printf("compat_open(%s, 0x%lx)\n", name, flags);

	for (i = 0; gDevNameList[i] != NULL; i++) {
		if (strcmp(gDevNameList[i], name) == 0)
			break;
	}

	if (gDevNameList[i] == NULL)
		return B_ERROR;

	dev = gDevices[i];

	if (atomic_or(&dev->flags, DEVICE_OPEN) & DEVICE_OPEN)
		return B_BUSY;

	status = sDeviceAttach(dev);
	if (status != 0)
		dev->flags = 0;

	driver_printf(" ... status = 0x%ld\n", status);

	*cookie = dev;

	return status;
}


static status_t
compat_close(void *cookie)
{
	device_t dev = cookie;

	device_printf(dev, "compat_close()\n");

	return B_ERROR;
}


static status_t
compat_free(void *cookie)
{
	device_t dev = cookie;

	device_printf(dev, "compat_free()\n");

	free_device(dev);
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

	driver_printf("compat_read(%p, %lld, %p, [%lu])\n", cookie, position, buf,
		*numBytes);

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

	driver_printf("compat_write(%p, %lld, %p, [%lu])\n", cookie, position,
		buffer, *numBytes);

	mb = m_getcl(0, MT_DATA, 0);
	if (mb == NULL)
		return ENOBUFS;

	/* if we waited, check after if the ifp is still valid */

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
	struct device fakeDevice;
	device_probe_t *probe;
	int i;

	dprintf("%s: init_hardware(%p)\n", gDriverName, driver);

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPci) < B_OK)
		return B_ERROR;

	probe = (device_probe_t *)_resolve_method(driver, "device_probe");
	if (probe == NULL) {
		dprintf("%s: driver has no device_probe method.\n", gDriverName);
		return B_ERROR;
	}

	memset(&fakeDevice, 0, sizeof(struct device));

	for (i = 0; gPci->get_nth_pci_info(i, &fakeDevice.pci_info) == B_OK; i++) {
		if (probe(&fakeDevice) >= 0) {
			dprintf("%s, found %s at %d\n", gDriverName,
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
	int i, ncards = 0;
	status_t status;
	device_t dev;

	dprintf("%s: init_driver(%p)\n", gDriverName, driver);

	sDeviceProbe  = (device_probe_t  *)_resolve_method(driver, "device_probe");
	sDeviceAttach = (device_attach_t *)_resolve_method(driver, "device_attach");
	sDeviceDetach = (device_detach_t *)_resolve_method(driver, "device_detach");

	dev = allocate_device(driver);
	if (dev == NULL)
		return B_NO_MEMORY;

	init_compat_layer();

	status = init_mutexes();
	if (status < B_OK) {
		free_device(dev);
		return status;
	}

	status = init_mbufs();
	if (status < B_OK) {
		uninit_mutexes();
		free_device(dev);
		return status;
	}

	init_bounce_pages();

	for (i = 0; dev != NULL
			&& gPci->get_nth_pci_info(i, &dev->pci_info) == B_OK; i++) {
		if (sDeviceProbe(dev) >= 0) {
			snprintf(dev->dev_name, sizeof(dev->dev_name), "net/%s/%i",
				gDriverName, ncards);
			dprintf("%s, adding %s @%d -> /dev/%s\n", gDriverName, dev->description,
				i, dev->dev_name);

			gDevices[ncards] = dev;
			gDevNameList[ncards] = dev->dev_name;

			ncards++;
			if (ncards < MAX_DEVICES)
				dev = allocate_device(driver);
			else
				dev = NULL;
		}
	}

	if (dev != NULL)
		free_device(dev);

	dprintf("%s, ... %d cards.\n", gDriverName, ncards);

	gDevNameList[ncards + 1] = NULL;

	return B_OK;
}


void
_fbsd_uninit_driver(driver_t *driver)
{
	int i;

	dprintf("%s: uninit_driver(%p)\n", gDriverName, driver);

	for (i = 0; gDevNameList[i] != NULL; i++) {
		free_device(gDevices[i]);
	}

	uninit_bounce_pages();
	uninit_mbufs();
	uninit_mutexes();
}
