/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ahci_defs.h"

#include <KernelExport.h>

#define TRACE(a...) dprintf("\33[34mahci:\33[30m " a)
#define FLOW(a...)	dprintf("ahci: " a)


pci_device_module_info *gPCI;
scsi_for_sim_interface *gSCSI;


//	#pragma mark - SIM module interface


//! execute request
static void
ahci_scsi_io(scsi_sim_cookie cookie, scsi_ccb *request)
{
	TRACE("ahci_scsi_io()\n");

	request->subsys_status = SCSI_NO_HBA;
	gSCSI->finished(request, 1);
}


//! abort request
static uchar
ahci_abort_io(scsi_sim_cookie cookie, scsi_ccb *request)
{
	TRACE("ahci_abort_io()\n");

	return SCSI_REQ_CMP;
}


static uchar
ahci_reset_device(scsi_sim_cookie cookie, uchar targetID, uchar targetLUN)
{
	TRACE("ahci_reset_device()\n");

	return SCSI_REQ_CMP;
}


//! terminate request
static uchar
ahci_terminate_io(scsi_sim_cookie cookie, scsi_ccb *request)
{
	TRACE("ahci_terminate_io()\n");

	return SCSI_NO_HBA;
}


//! get information about bus
static uchar
ahci_path_inquiry(scsi_sim_cookie cookie, scsi_path_inquiry *info)
{
	TRACE("ahci_path_inquiry()\n");

	return SCSI_NO_HBA;
}


//! this is called immediately before the SCSI bus manager scans the bus
static uchar
ahci_scan_bus(scsi_sim_cookie cookie)
{
	TRACE("ahci_scan_bus()\n");

	return SCSI_NO_HBA;
}


static uchar
ahci_reset_bus(scsi_sim_cookie cookie)
{
	TRACE("ahci_reset_bus()\n");

	return SCSI_NO_HBA;
}


/*!	Get restrictions of one device 
	(used for non-SCSI transport protocols and bug fixes)
*/
static void
ahci_get_restrictions(scsi_sim_cookie cookie, uchar targetID, bool *isATAPI,
	bool *noAutoSense, uint32 *maxBlocks)
{
	TRACE("ahci_get_restrictions()\n");

	*isATAPI = false;
	*noAutoSense = false;
	*maxBlocks = 255;
}


static status_t
ahci_ioctl(scsi_sim_cookie cookie, uint8 targetID, uint32 op, void *buffer,
	size_t length)
{
	TRACE("ahci_ioctl()\n");
	return B_BAD_VALUE;
}


//	#pragma mark -


static status_t
ahci_sim_init_bus(device_node_handle node, void *user_cookie, void **_cookie)
{
	TRACE("ahci_sim_init_bus\n");
	return B_OK;
}


static status_t
ahci_sim_uninit_bus(void *cookie)
{
	TRACE("ahci_sim_uninit_bus\n");
	return B_OK;
}


static void
ahci_sim_bus_removed(device_node_handle node, void *cookie)
{
	TRACE("ahci_sim_bus_removed\n");
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


scsi_sim_interface gAHCISimInterface = {
	{
		{
			AHCI_SIM_MODULE_NAME,
			0,
			std_ops
		},
		NULL,	// supported devices				
		NULL,	// register node
		ahci_sim_init_bus,
		ahci_sim_uninit_bus,
		ahci_sim_bus_removed,
		NULL,	// device cleanup
		NULL,	// get supported paths
	},
	ahci_scsi_io,
	ahci_abort_io,
	ahci_reset_device,
	ahci_terminate_io,
	ahci_path_inquiry,
	ahci_scan_bus,
	ahci_reset_bus,
	ahci_get_restrictions,
	ahci_ioctl
};

