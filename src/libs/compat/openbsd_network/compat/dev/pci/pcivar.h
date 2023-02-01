/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_DEV_PCI_PCIVAR_H_
#define _OBSD_COMPAT_DEV_PCI_PCIVAR_H_


#include_next <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <sys/rman.h>


typedef u_int32_t pcireg_t;


struct pci_matchid {
	pci_vendor_id_t		pm_vid;
	pci_product_id_t	pm_pid;
};

typedef struct {
	int rid;
} pci_intr_handle_t;


#define pci_conf_read(pct, pcitag, reg) \
	pci_read_config(SC_DEV_FOR_PCI, reg, sizeof(pcireg_t))
#define pci_conf_write(pct, pcitag, reg, val) \
	pci_write_config(SC_DEV_FOR_PCI, reg, val, sizeof(pcireg_t))
#define pci_get_capability(pct, pcitag, capability, offset, value) \
	pci_get_capability_openbsd(SC_DEV_FOR_PCI, capability, offset, value)
#define pci_mapreg_type(pct, pcitag, reg) \
	pci_mapreg_type_openbsd(SC_DEV_FOR_PCI, reg)
#define pci_mapreg_map(pa, reg, type, flags, tagp, handlep, basep, sizep, maxsize) \
	pci_mapreg_map_openbsd(SC_DEV_FOR_PCI, reg, type, flags, tagp, handlep, basep, sizep, maxsize)
#define pci_intr_establish(pa, ih, level, func, arg, what) \
	pci_intr_establish_openbsd(SC_DEV_FOR_PCI, ih, level, func, arg, what)

#define pci_intr_string(...) NULL

static int
pci_get_capability_openbsd(device_t dev, int capability, int* offset, pcireg_t* value)
{
	int res = pci_find_cap(dev, capability, offset);
	if (res != 0)
		return 0;

	if (value)
		*value = pci_read_config(dev, *offset, sizeof(pcireg_t));
	return 1;
}

static pcireg_t
pci_mapreg_type_openbsd(device_t dev, int reg)
{
	return (_PCI_MAPREG_TYPEBITS(pci_read_config(dev, reg, sizeof(pcireg_t))));
}

static int
pci_mapreg_map_openbsd(device_t dev, int reg, pcireg_t type, int flags,
	bus_space_tag_t* tagp, bus_space_handle_t* handlep, bus_addr_t* basep,
	bus_size_t* sizep, bus_size_t maxsize)
{
	struct resource* res = bus_alloc_resource_any(dev, SYS_RES_MEMORY, &reg, RF_ACTIVE);
	if (res == NULL)
		return -1;

	*tagp = rman_get_bustag(res);
	*handlep = rman_get_bushandle(res);
	if (basep != NULL)
		*basep = rman_get_start(res);
	if (sizep != NULL)
		*sizep = rman_get_size(res);
	return 0;
}

static int
pci_intr_map_msix(device_t dev, int vec, pci_intr_handle_t* ihp)
{
	if (vec != 0)
		return -1;

	int count = 1;
	ihp->rid = 1;
	return pci_alloc_msix(dev, &count);
}

static int
pci_intr_map_msi(device_t dev, pci_intr_handle_t* ihp)
{
	int count = 1;
	ihp->rid = 1;
	return pci_alloc_msi(dev, &count);
}

static int
pci_intr_map(device_t dev, pci_intr_handle_t* ihp)
{
	// No need to map legacy interrupts.
	ihp->rid = 0;
	return 0;
}

static void*
pci_intr_establish_openbsd(device_t dev, pci_intr_handle_t ih, int level,
	int(*func)(void*), void* arg, const char* what)
{
	struct resource* irq = bus_alloc_resource_any(dev, SYS_RES_IRQ, &ih.rid,
		RF_ACTIVE | (ih.rid != 0 ? 0 : RF_SHAREABLE));
	if (irq == NULL)
		return NULL;

	int flags = INTR_TYPE_NET;
	if ((level & IPL_MPSAFE) != 0)
		flags |= INTR_MPSAFE;

	void* ihp = NULL;
	bus_setup_intr(dev, irq, flags, NULL, func, arg, &ihp);
	return ihp;
}


#endif	/* _OBSD_COMPAT_DEV_PCI_PCIVAR_H_ */
