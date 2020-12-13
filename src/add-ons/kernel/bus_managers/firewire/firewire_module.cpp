/*
 * Copyright (C) 2007 JiSheng Zhang <jszhang3@gmail.com>. All rights reserved
 * Distributed under the terms of the MIT license.
 *
 * Kernel driver for firewire
 */

#include <OS.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <PCI.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <dpc.h>

#include "fwdebug.h"
#include "queue.h"
#include "fwglue.h"
#include "firewire.h"
#include "iec13213.h"
#include "firewirereg.h"
#include "fwdma.h"
#include "fwohcireg.h"
#include "fwohcivar.h"
#include "firewire_module.h"

status_t fwohci_pci_attach(int index);
status_t fwohci_pci_detach(int index);
pci_info *pciInfo[MAX_CARDS];
fwohci_softc_t *gFwohci_softc[MAX_CARDS];
struct firewire_softc *gFirewire_softc[MAX_CARDS];
pci_module_info	*gPci;
dpc_module_info *gDpc;

struct supported_device{
	uint16 vendor_id;
	uint32 device_id;
	const char *name;
};

struct supported_device supported_devices[] = {
	{FW_VENDORID_NATSEMI, FW_DEVICE_CS4210, "National Semiconductor CS4210"},
	{FW_VENDORID_NEC, FW_DEVICE_UPD861, "NEC uPD72861"},
	{FW_VENDORID_NEC, FW_DEVICE_UPD871, "NEC uPD72871/2"},
	{FW_VENDORID_NEC, FW_DEVICE_UPD72870, "NEC uPD72870"},
	{FW_VENDORID_NEC, FW_DEVICE_UPD72873, "NEC uPD72873"},
	{FW_VENDORID_NEC, FW_DEVICE_UPD72874, "NEC uPD72874"},
	{FW_VENDORID_SIS, FW_DEVICE_7007, "SiS 7007"},
	{FW_VENDORID_TI, FW_DEVICE_TITSB22, "Texas Instruments TSB12LV22"},
	{FW_VENDORID_TI, FW_DEVICE_TITSB23, "Texas Instruments TSB12LV23"},
	{FW_VENDORID_TI, FW_DEVICE_TITSB26, "Texas Instruments TSB12LV26"},
	{FW_VENDORID_TI, FW_DEVICE_TITSB43, "Texas Instruments TSB43AA22"},
	{FW_VENDORID_TI, FW_DEVICE_TITSB43A, "Texas Instruments TSB43AB22/A"},
	{FW_VENDORID_TI, FW_DEVICE_TITSB43AB21, "Texas Instruments TSB43AB21/A/AI/A-EP"},
	{FW_VENDORID_TI, FW_DEVICE_TITSB43AB23, "Texas Instruments TSB43AB23"},
	{FW_VENDORID_TI, FW_DEVICE_TITSB82AA2, "Texas Instruments TSB82AA2"},
	{FW_VENDORID_TI, FW_DEVICE_TIPCI4450, "Texas Instruments PCI4450"},
	{FW_VENDORID_TI, FW_DEVICE_TIPCI4410A, "Texas Instruments PCI4410A"},
	{FW_VENDORID_TI, FW_DEVICE_TIPCI4451, "Texas Instruments PCI4451"},
	{FW_VENDORID_VIA, FW_DEVICE_VT6306, "VIA Fire II (VT6306)"},
	{FW_VENDORID_RICOH, FW_DEVICE_R5C551, "Ricoh R5C551"},
	{FW_VENDORID_RICOH, FW_DEVICE_R5C552, "Ricoh R5C552"},
	{FW_VENDORID_APPLE, FW_DEVICE_PANGEA, "Apple Pangea"},
	{FW_VENDORID_APPLE, FW_DEVICE_UNINORTH, "Apple UniNorth"},
	{FW_VENDORID_LUCENT, FW_DEVICE_FW322, "Lucent FW322/323"},
	{FW_VENDORID_INTEL, FW_DEVICE_82372FB, "Intel 82372FB"},
	{FW_VENDORID_ADAPTEC, FW_DEVICE_AIC5800, "Adaptec AHA-894x/AIC-5800"},
	{FW_VENDORID_SUN, FW_DEVICE_PCIO2FW, "Sun PCIO-2"},
	{FW_VENDORID_SONY, FW_DEVICE_CXD3222, "Sony i.LINK (CXD3222)"},
	{0, 0, NULL}
};


static int
find_device_name(pci_info *info)
{
	struct supported_device *device;
	for (device = supported_devices; device->name; device++) {
		if (info->vendor_id == device->vendor_id
			&& info->device_id == device->device_id >> 16) {
			dprintf("%s\n", device->name);
			return 1;
		}
	}
	return 0;
}


#if 0
static status_t
fw_add_child(const char *childname,
		const struct firewire_notify_hooks *hooks)
{
	status_t status;
	int i;
	TRACE("add child %s\n", childname);
	for (i = 0; gFirewire_softc[i] != NULL; i++) {
		status = firewire_add_child(gFirewire_softc[i], childname, hooks);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


static status_t
fw_remove_child(const char *childname)
{
	status_t status;
	int i;
	TRACE("remove child %s\n", childname);
	for (i = 0; gFirewire_softc[i] != NULL; i++) {
		status = firewire_remove_child(gFirewire_softc[i], childname);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}
#endif


static int
fw_get_handle(int socket, struct firewire_softc **handle)
{
	if (handle == NULL)
		return B_BAD_VALUE;
	if (socket >= 0 && socket < MAX_CARDS && gFirewire_softc[socket]) {
		*handle = gFirewire_softc[socket];
		return B_OK;
	}
	*handle = NULL;
	return ENODEV;
}


static status_t
fw_module_init(void)
{
	status_t status;
	int i, found;
	fwohci_softc_t *fwohci_sc;
	struct firewire_softc *fw_sc;

	pci_info *info = (pci_info*)malloc(sizeof(pci_info));
	if (!info)
		return B_NO_MEMORY;

	if ((status = get_module(B_PCI_MODULE_NAME,(module_info **)&gPci)) != B_OK) {
		TRACE("pci module unavailable\n");
		free(info);
		return status;
	}

	if ((status = get_module(B_DPC_MODULE_NAME,(module_info **)&gDpc)) != B_OK) {
		TRACE("pci module unavailable\n");
		free(info);
		put_module(B_PCI_MODULE_NAME);
		return status;
	}

	memset(gFwohci_softc, 0, sizeof(gFwohci_softc));

	// find devices
	for (i = 0, found = 0; (status = gPci->get_nth_pci_info(i, info)) == B_OK; i++) {
		if (find_device_name(info)
				|| ((info->class_base == PCI_serial_bus)
					&& (info->class_sub == PCI_firewire)
					&& (info->class_api == PCI_INTERFACE_OHCI))) {
			dprintf( "vendor=%x, device=%x, revision = %x\n", info->vendor_id, info->device_id, info->revision);
			pciInfo[found] = info;

			fwohci_sc = (fwohci_softc_t*)malloc(sizeof(fwohci_softc_t));
			if (!fwohci_sc) {
				free(info);
				goto err_outofmem;
			}
			memset(fwohci_sc, 0, sizeof(fwohci_softc_t));
			gFwohci_softc[found] = fwohci_sc;

			fw_sc = (firewire_softc*)malloc(sizeof(struct firewire_softc));
			if (!fw_sc) {
				free(info);
				free(fwohci_sc);
				goto err_outofmem;
			}
			memset(fw_sc, 0, sizeof(struct firewire_softc));
			gFirewire_softc[found] = fw_sc;
			if (found < MAX_CARDS - 1)
				gFirewire_softc[found + 1] = NULL;

			found++;
			info = (pci_info*)malloc(sizeof(pci_info));
			if (!info)
				goto err_outofmem;

			if (found == MAX_CARDS)
				break;

		}
	}
	TRACE("found %d cards\n", found);
	free(info);

	if ((status = initialize_timer()) != B_OK) {
		ERROR("timer init failed\n");
		goto err_timer;
	}

	for (i = 0; i < found; i++) {
		if (fwohci_pci_attach(i) != B_OK) {
			ERROR("fwohci_pci_attach failed\n");
			goto err_pci;
		}
	}
	return B_OK;

err_pci:
	terminate_timer();
err_timer:
err_outofmem:
	for (i = 0; i < found; i++) {
		free(gFirewire_softc[i]);
		free(gFwohci_softc[i]);
		free(pciInfo[i]);
	}
	put_module(B_PCI_MODULE_NAME);
	put_module(B_DPC_MODULE_NAME);
	return B_ERROR;

}

static status_t
fw_module_uninit(void)
{
	int i;

	terminate_timer();

	for (i = 0; i < MAX_CARDS && gFirewire_softc[i] != NULL; i++) {
		fwohci_pci_detach(i);
		free(gFirewire_softc[i]);
		free(gFwohci_softc[i]);
		free(pciInfo[i]);
	}

	put_module(B_PCI_MODULE_NAME);
	put_module(B_DPC_MODULE_NAME);

	return B_OK;
}


static status_t
fw_module_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			TRACE("fw_module_init\n");
			return fw_module_init();

		case B_MODULE_UNINIT:
			TRACE("fw_module_uninit\n");
			return fw_module_uninit();
	}
	return B_BAD_VALUE;
}

static struct fw_module_info gModuleInfo = {
	{
		{
			FIREWIRE_MODULE_NAME,
			0,
			fw_module_std_ops
		},
		NULL
	},
	fw_noderesolve_nodeid,
	fw_noderesolve_eui64,
	fw_asyreq,
	fw_xferwake,
	fw_xferwait,
	fw_bindlookup,
	fw_bindadd,
	fw_bindremove,
	fw_xferlist_add,
	fw_xferlist_remove,
	fw_xfer_alloc,
	fw_xfer_alloc_buf,
	fw_xfer_done,
	fw_xfer_unload,
	fw_xfer_free_buf,
	fw_xfer_free,
	fw_asy_callback_free,
	fw_open_isodma,
	fw_get_handle,
	fwdma_malloc_multiseg,
	fwdma_free_multiseg
};

module_info *modules[] = {
	(module_info *)&gModuleInfo,
	NULL
};
