/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2008, Marcus Overhagen.
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002-2003, Thomas Kurschel.
 *
 * Distributed under the terms of the MIT License.
 */

#include "ATAPrivate.h"


scsi_for_sim_interface *gSCSIModule = NULL;
device_manager_info *gDeviceManager = NULL;


static status_t
ata_sim_init_bus(device_node *node, void **cookie)
{
	ATAChannel *channel = new(std::nothrow) ATAChannel(node);
	if (channel == NULL)
		return B_NO_MEMORY;

	status_t result = channel->InitCheck();
	if (result != B_OK) {
		TRACE_ERROR("failed to set up ata channel object\n");
		return result;
	}

	*cookie = channel;
	return B_OK;
}


static void
ata_sim_uninit_bus(void *cookie)
{
	ATAChannel *channel = (ATAChannel *)cookie;
	delete channel;
}


static void
ata_sim_bus_removed(void *cookie)
{
	ATAChannel *channel = (ATAChannel *)cookie;
	if (channel->Bus() != NULL) {
		gSCSIModule->block_bus(channel->Bus());
		channel->SetBus(NULL);
	}
}


static void
ata_sim_set_scsi_bus(scsi_sim_cookie cookie, scsi_bus bus)
{
	ATAChannel *channel = (ATAChannel *)cookie;
	channel->SetBus(bus);
	channel->ScanBus();
}


static void
ata_sim_scsi_io(scsi_sim_cookie cookie, scsi_ccb *ccb)
{
	ATAChannel *channel = (ATAChannel *)cookie;
	if (channel->Bus() == NULL) {
		ccb->subsys_status = SCSI_NO_HBA;
		gSCSIModule->finished(ccb, 1);
		return;
	}

	if (channel->ExecuteIO(ccb) == B_BUSY)
		gSCSIModule->requeue(ccb, true);
}


static uchar
ata_sim_abort(scsi_sim_cookie cookie, scsi_ccb *ccb)
{
	ATAChannel *channel = (ATAChannel *)cookie;
	if (channel->Bus() == NULL)
		return SCSI_NO_HBA;

	// aborting individual commands is not possible
	return SCSI_REQ_CMP;
}


static uchar
ata_sim_reset_device(scsi_sim_cookie cookie, uchar targetId, uchar targetLun)
{
	ATAChannel *channel = (ATAChannel *)cookie;
	if (channel->Bus() == NULL)
		return SCSI_NO_HBA;

	// TODO: implement
	return SCSI_REQ_INVALID;
}


static uchar
ata_sim_term_io(scsi_sim_cookie cookie, scsi_ccb *ccb)
{
	ATAChannel *channel = (ATAChannel *)cookie;
	if (channel->Bus() == NULL)
		return SCSI_NO_HBA;

	// we don't terminate commands, ignore
	return SCSI_REQ_CMP;
}


static uchar
ata_sim_path_inquiry(scsi_sim_cookie cookie, scsi_path_inquiry *info)
{
	ATAChannel *channel = (ATAChannel *)cookie;
	if (channel->Bus() == NULL)
		return SCSI_NO_HBA;

	channel->PathInquiry(info);
	return SCSI_REQ_CMP;
}


static uchar
ata_sim_rescan_bus(scsi_sim_cookie cookie)
{
	// TODO: implement
	return SCSI_REQ_CMP;
}


static uchar
ata_sim_reset_bus(scsi_sim_cookie cookie)
{
	ATAChannel *channel = (ATAChannel *)cookie;
	if (channel->Bus() == NULL)
		return SCSI_NO_HBA;

	//channel->Reset();
	panic("asking for trouble");
	return SCSI_REQ_CMP;
}


static void
ata_sim_get_restrictions(scsi_sim_cookie cookie, uchar targetID,
	bool *isATAPI, bool *noAutoSense, uint32 *maxBlocks)
{
	ATAChannel *channel = (ATAChannel *)cookie;
	channel->GetRestrictions(targetID, isATAPI, noAutoSense, maxBlocks);
}


static status_t
ata_sim_control(scsi_sim_cookie cookie, uchar targetID, uint32 op, void *buffer,
	size_t length)
{
	ATAChannel *channel = (ATAChannel *)cookie;
	return channel->Control(targetID, op, buffer, length);
}


status_t
ata_channel_added(device_node *parent)
{
	const char *controllerName;
	if (gDeviceManager->get_attr_string(parent,
		ATA_CONTROLLER_CONTROLLER_NAME_ITEM, &controllerName, true) != B_OK) {
		TRACE_ERROR("controller name missing\n");
		return B_ERROR;
	}

	int32 channelID = gDeviceManager->create_id(ATA_CHANNEL_ID_GENERATOR);
	if (channelID < 0) {
		TRACE_ERROR("out of channel ids\n");
		return B_ERROR;
	}

	device_attr attributes[] = {
		{
			B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
				{ string: SCSI_FOR_SIM_MODULE_NAME }
		},

		{
			SCSI_DESCRIPTION_CONTROLLER_NAME, B_STRING_TYPE,
				{ string: controllerName }
		},

		// maximum number of blocks per transmission:
		// - ATAPI uses packets, i.e. normal SCSI limits apply
		//   but I'm not sure about controller restrictions
		// - ATA allows up to 256 blocks for LBA28 and 65535 for LBA48
		// to fix specific drive bugs use ATAChannel::GetRestrictions()
		{ B_DMA_MAX_TRANSFER_BLOCKS, B_UINT32_TYPE, { ui32: 0xffff } },
		{ ATA_CHANNEL_ID_ITEM, B_UINT32_TYPE, { ui32: (uint32)channelID } },
		{ NULL }
	};

	return gDeviceManager->register_node(parent, ATA_SIM_MODULE_NAME,
		attributes, NULL, NULL);
}


status_t
ata_interrupt_handler(void *cookie, uint8 status)
{
	ATAChannel *channel = (ATAChannel *)cookie;
	return channel->Interrupt(status);
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			break;
	}

	return B_ERROR;
}


scsi_sim_interface ata_sim_module = {
	{
		{
			ATA_SIM_MODULE_NAME,
			0,
			std_ops
		},

		NULL, // supported devices
		NULL, // register node
		ata_sim_init_bus,
		ata_sim_uninit_bus,
		NULL, // register child devices
		NULL, // rescan
		ata_sim_bus_removed,
		NULL, // suspend
		NULL, // resume
	},

	ata_sim_set_scsi_bus,
	ata_sim_scsi_io,
	ata_sim_abort,
	ata_sim_reset_device,
	ata_sim_term_io,
	ata_sim_path_inquiry,
	ata_sim_rescan_bus,
	ata_sim_reset_bus,
	ata_sim_get_restrictions,
	ata_sim_control
};

ata_for_controller_interface ata_for_controller_module = {
	{
		{
			ATA_FOR_CONTROLLER_MODULE_NAME,
			0,
			&std_ops
		},

		NULL, // supported devices
		ata_channel_added,
		NULL,
		NULL,
		NULL
	},

	ata_interrupt_handler
};


module_dependency module_dependencies[] = {
	{ SCSI_FOR_SIM_MODULE_NAME, (module_info **)&gSCSIModule },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager },
	{}
};

module_info *modules[] = {
	(module_info *)&ata_for_controller_module,
	(module_info *)&ata_sim_module,
	NULL
};
