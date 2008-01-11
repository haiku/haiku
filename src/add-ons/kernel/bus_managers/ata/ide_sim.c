/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open IDE bus manager

	Interface between ide bus manager and scsi bus manager.

	The IDE bus manager has a bit unusual structure as it
	consists of a single level only. In fact it is no bus manager
	in terms of the PnP structure at all but a driver that maps
	one SCSI bus onto one IDE controller.
	
	This structure does not allow us to publish IDE devices
	as they can be accessed via the SCSI bus node only. Therefore
	we do a full bus scan every time the IDE bus node is loaded.
	The drawback is that a bus rescan must be done indirectly via a
	SCSI bus scan.
*/

#include "ide_internal.h"
#include "ide_sim.h"

#include <scsi_cmds.h>
#include <safemode.h>

#include <string.h>
#include <malloc.h>
#include <stdio.h>

//#define FLOW dprintf
#define FLOW(x...)
#define TRACE dprintf
//#define TRACE(x...)

scsi_for_sim_interface *scsi;



static void
sim_scsi_io(ide_bus_info *bus, scsi_ccb *ccb)
{
	ide_device_info *device;
	ata_request *request;

	FLOW("sim_scsi_iobus %p, %d:%d\n", bus, ccb->target_id, ccb->target_lun);

	if (bus->disconnected)
		goto err_disconnected;

	// make sure, device is valid
	// I've read that there are ATAPI devices with more then one LUN,
	// but it seems that most (all?) devices ignore LUN, so we have
	// to restrict to LUN 0 to avoid mirror devices
	if (ccb->target_id >= 2)
		goto err_inv_device;

	device = bus->devices[ccb->target_id];		
	if (device == NULL)
		goto err_inv_device;

	if (ccb->target_lun > device->last_lun)
		goto err_inv_device;

	ata_request_start(&request, device, ccb);

	if (request) {
		TRACE("calling exec_io: %p, %d:%d\n", bus, ccb->target_id, ccb->target_lun);
		device->exec_io(device, request);
		return;
	}

	TRACE("Bus busy\n");
	ACQUIRE_BEN(&bus->status_report_ben);
	scsi->requeue(ccb, true);
	RELEASE_BEN(&bus->status_report_ben);
	return;


err_inv_device:

	FLOW("Invalid device %d:%d\n", ccb->target_id, ccb->target_lun);
	ccb->subsys_status = SCSI_SEL_TIMEOUT;
	ACQUIRE_BEN(&bus->status_report_ben);
	scsi->finished(ccb, 1);
	RELEASE_BEN(&bus->status_report_ben);
	return;


err_disconnected:

	TRACE("No controller anymore\n");
	ccb->subsys_status = SCSI_NO_HBA;
	ACQUIRE_BEN(&bus->status_report_ben);
	scsi->finished(ccb, 1);
	RELEASE_BEN(&bus->status_report_ben);
	return;
}


static uchar
sim_path_inquiry(ide_bus_info *bus, scsi_path_inquiry *info)
{
	char *controller_name;

	FLOW("sim_path_inquiry, bus %p\n", bus);

	if (bus->disconnected)
		return SCSI_NO_HBA;

	info->hba_inquiry = SCSI_PI_TAG_ABLE | SCSI_PI_WIDE_16;

	info->hba_misc = 0;

	memset(info->vuhba_flags, 0, sizeof(info->vuhba_flags));
	// we don't need any of the private data
	info->sim_priv = 0;

	// there is no initiator for IDE, but SCSI needs it for scanning
	info->initiator_id = 2;	
	// we only support 1 request at a time
	info->hba_queue_size = 1;

	strncpy(info->sim_vid, "Haiku", SCSI_SIM_ID);

	if (pnp->get_attr_string(bus->node, SCSI_DESCRIPTION_CONTROLLER_NAME,
			&controller_name, true) == B_OK) {
		strlcpy(info->hba_vid, controller_name, SCSI_HBA_ID);
		free(controller_name);
	} else
		strlcpy(info->hba_vid, "", SCSI_HBA_ID);

	strlcpy(info->controller_family, "IDE", SCSI_FAM_ID);
	strlcpy(info->controller_type, "IDE", SCSI_TYPE_ID);

	SHOW_FLOW0(4, "done");

	return SCSI_REQ_CMP;
}


static void
scan_bus(ide_bus_info *bus)
{
	uint32 deviceSignature[2];
	bool devicePresent[2];
	ide_device_info *device;
	status_t status;
	bool isAtapi;
	int i;

	TRACE("ATA: scan_bus: bus %p\n", bus);

	if (bus->disconnected)
		return;

	// XXX fix me
	IDE_LOCK(bus);
	ASSERT(bus->state == ata_state_idle);
	bus->state = ata_state_busy;
	IDE_UNLOCK(bus);

	for (i = 0; i < bus->max_devices; ++i) {
		if (bus->devices[i])
			destroy_device(bus->devices[i]);
	}

	status = ata_reset_bus(bus, &devicePresent[0], &deviceSignature[0], &devicePresent[1], &deviceSignature[1]);

	for (i = 0; i < bus->max_devices; ++i) {
		if (!devicePresent[i])
			continue;

		dprintf("ATA: scan_bus: bus %p, creating device %d, signature is 0x%08lx\n",
			bus, i, deviceSignature[i]);

		isAtapi = deviceSignature[i] == 0xeb140101;
		device = create_device(bus, i /* isDevice1 */);

		if (scan_device(device, isAtapi) != B_OK) {
			dprintf("ATA: scan_bus: bus %p, scanning failed, destroying device %d\n", bus, i);
			destroy_device(device);
			continue;
		}
		if (configure_device(device, isAtapi) != B_OK) {
			dprintf("ATA: scan_bus: bus %p, configure failed, destroying device %d\n", bus, i);
			destroy_device(device);
		}
	}

	// XXX fix me
	IDE_LOCK(bus);
	ASSERT(bus->state == ata_state_busy);
	bus->state = ata_state_idle;
	IDE_UNLOCK(bus);

	TRACE("ATA: scan_bus: bus %p finished\n", bus);
}

static uchar
sim_rescan_bus(ide_bus_info *bus)
{
	TRACE("ATA: sim_rescan_bus - not implemented\n");
	return SCSI_REQ_CMP;
}


static uchar
sim_abort(ide_bus_info *bus, scsi_ccb *ccb_to_abort)
{
	// we cannot abort specific commands, so just ignore
	if (bus->disconnected)
		return SCSI_NO_HBA;

	return SCSI_REQ_CMP;
}


static uchar
sim_term_io(ide_bus_info *bus, scsi_ccb *ccb_to_abort)
{
	// we cannot terminate commands, so just ignore
	if (bus->disconnected)
		return SCSI_NO_HBA;

	return SCSI_REQ_CMP;
}


static uchar
sim_reset_bus(ide_bus_info *bus)
{
	// no, we don't do that
	if (bus->disconnected)
		return SCSI_NO_HBA;

	TRACE("ATA: sim_reset_bus - not implemented\n");
	return SCSI_REQ_INVALID;
}


static uchar
sim_reset_device(ide_bus_info *bus, uchar target_id, uchar target_lun)
{
	// xxx to do
	if (bus->disconnected)
		return SCSI_NO_HBA;

	return SCSI_REQ_INVALID;
}


static status_t
ide_sim_init_bus(device_node_handle node, void *user_cookie, void **cookie)
{
	device_node_handle parent;
	ide_bus_info *bus;
	bool dmaDisabled = false;
	status_t status;

	FLOW("ide_sim_init_bus, node %p\n", node);

	// first prepare the info structure	
	bus = (ide_bus_info *)malloc(sizeof(*bus));
	if (bus == NULL)
		return B_NO_MEMORY;

	memset(bus, 0, sizeof(*bus));
	bus->node = node;
	bus->lock = 0;
	bus->disconnected = false;

	{
		int32 channel_id = -1;

		pnp->get_attr_uint32(node, IDE_CHANNEL_ID_ITEM, (uint32 *)&channel_id, true);

		sprintf(bus->name, "ide_bus %d", (int)channel_id);
	}

	bus->scsi_cookie = user_cookie;
	bus->timer.bus = bus;

	if ((status = scsi->alloc_dpc(&bus->irq_dpc)) < B_OK)
		goto err1;

	bus->state = ata_state_idle;
	bus->active_device = NULL;

	bus->devices[0] = NULL;
	bus->devices[1] = NULL;

	status = INIT_BEN(&bus->status_report_ben, "ide_status_report");
	if (status < B_OK) 
		goto err4;

	{
		// check if safemode settings disable DMA
		void *settings = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
		if (settings != NULL) {
			dmaDisabled = get_driver_boolean_parameter(settings, B_SAFEMODE_DISABLE_IDE_DMA,
				false, false);
			unload_driver_settings(settings);
		}
	}

	// read restrictions of controller

	if (pnp->get_attr_uint8(node, IDE_CONTROLLER_MAX_DEVICES_ITEM,
			&bus->max_devices, true) != B_OK) {
		// per default, 2 devices are supported per node
		bus->max_devices = 2;
	}

	bus->max_devices = min(bus->max_devices, 2);

	if (dmaDisabled
		|| pnp->get_attr_uint8(node, IDE_CONTROLLER_CAN_DMA_ITEM, &bus->can_DMA, true) != B_OK) {
		// per default, no dma support
		bus->can_DMA = false;
	}

	SHOW_FLOW(2, "can_dma: %d", bus->can_DMA);

	parent = pnp->get_parent(node);

	status = pnp->init_driver(parent, bus, (driver_module_info **)&bus->controller, 
		(void **)&bus->channel_cookie);

	pnp->put_device_node(parent);
	if (status != B_OK)
		goto err5;

	*cookie = bus;

	// detect devices
	scan_bus(bus);
	return B_OK;

err5:
	DELETE_BEN(&bus->status_report_ben);
err4:
	scsi->free_dpc(bus->irq_dpc);
err1:
	free(bus);

	return status;
}


static status_t
ide_sim_uninit_bus(ide_bus_info *bus)
{
	device_node_handle parent;

	FLOW("ide_sim_uninit_bus: bus %p\n", bus);

	parent = pnp->get_parent(bus->node);
	pnp->uninit_driver(parent);
	pnp->put_device_node(parent);

	DELETE_BEN(&bus->status_report_ben);	
	scsi->free_dpc(bus->irq_dpc);

	free(bus);

	return B_OK;
}


static void
ide_sim_bus_removed(device_node_handle node, ide_bus_info *bus)
{	
	FLOW("ide_sim_bus_removed\n");

	if (bus == NULL)
		// driver not loaded - no manual intervention needed
		return;

	// XPT must not issue further commands
	scsi->block_bus(bus->scsi_cookie);
	// make sure, we refuse all new commands
	bus->disconnected = true;

	// abort all running commands with SCSI_NO_HBA
	// XXX
}


static void
ide_sim_get_restrictions(ide_bus_info *bus, uchar target_id,
	bool *is_atapi, bool *no_autosense, uint32 *max_blocks)
{
	ide_device_info *device = bus->devices[target_id];

	FLOW("ide_sim_get_restrictions\n");

	// we declare even ATA devices as ATAPI so we have to emulate fewer
	// commands
	*is_atapi = true;

	// we emulate autosense for ATA devices
	*no_autosense = false;

	if (device != NULL && device->is_atapi) {
		// we don't support native autosense for ATAPI devices
		*no_autosense = true;
	}

	*max_blocks = 255;

	if (device->is_atapi) {
		if (strncmp(device->infoblock.model_number, "IOMEGA  ZIP 100       ATAPI", 
		 		strlen("IOMEGA  ZIP 100       ATAPI")) == 0
		 	|| strncmp( device->infoblock.model_number, "IOMEGA  Clik!", 
		 		strlen( "IOMEGA  Clik!")) == 0) {
			SHOW_ERROR0(2, "Found buggy ZIP/Clik! drive - restricting transmission size");
			*max_blocks = 64;
		}
	}
}


static status_t
ide_sim_ioctl(ide_bus_info *bus, uint8 targetID, uint32 op, void *buffer, size_t length)
{
	ide_device_info *device = bus->devices[targetID];

	// We currently only support IDE_GET_INFO_BLOCK
	switch (op) {
		case IDE_GET_INFO_BLOCK:
			// we already have the info block, just copy it
			memcpy(buffer, &device->infoblock,
				min(sizeof(device->infoblock), length));
			return B_OK;

		case IDE_GET_STATUS:
		{
			// TODO: have our own structure and fill it with some useful stuff
			ide_status status;
			if (device->DMA_enabled)
				status.dma_status = 1;
			else if (device->DMA_supported) {
				if (device->DMA_failures > 0)
					status.dma_status = 6;
				else if (device->bus->can_DMA)
					status.dma_status = 2;
				else
					status.dma_status = 4;
			} else
				status.dma_status = 2;

			status.pio_mode = 0;
			status.dma_mode = 0;

			memcpy(buffer, &status, min(sizeof(status), length));
			return B_OK;
		}
	}

	return B_BAD_VALUE;
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


module_dependency module_dependencies[] = {
	{ SCSI_FOR_SIM_MODULE_NAME, (module_info **)&scsi },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{}
};


scsi_sim_interface ide_sim_module = {
	{
		{
			IDE_SIM_MODULE_NAME,
			0,
			std_ops,
		},

		NULL,	// supported devices				
		NULL,	// register node
		(status_t (*)(device_node_handle, void *, void **))ide_sim_init_bus,
		(status_t (*)(void *)	) 					ide_sim_uninit_bus,
		(void (*)(device_node_handle, void *))		ide_sim_bus_removed
	},

	(void (*)(scsi_sim_cookie, scsi_ccb *))			sim_scsi_io,
	(uchar (*)(scsi_sim_cookie, scsi_ccb *))		sim_abort,
	(uchar (*)(scsi_sim_cookie, uchar, uchar)) 		sim_reset_device,
	(uchar (*)(scsi_sim_cookie, scsi_ccb *))		sim_term_io,

	(uchar (*)(scsi_sim_cookie, scsi_path_inquiry *))sim_path_inquiry,
	(uchar (*)(scsi_sim_cookie))					sim_rescan_bus,
	(uchar (*)(scsi_sim_cookie))					sim_reset_bus,
	
	(void (*)(scsi_sim_cookie, uchar, 
		bool*, bool *, uint32 *))					ide_sim_get_restrictions,

	(status_t (*)(scsi_sim_cookie, uint8, uint32, void *, size_t))ide_sim_ioctl,
};
