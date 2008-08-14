/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open IDE bus manager

	Manager of IDE controllers

	Whenever a new IDE channel is reported, a new SIM is
	registered at the SCSI bus manager.
*/

#include "ide_internal.h"
#include "ide_sim.h"

#include <string.h>
#include <malloc.h>


/** called when an IDE channel was registered by a controller driver */

static status_t
ide_channel_added(device_node *parent)
{
	const char *controller_name = NULL;
	uint32 channel_id;

	SHOW_FLOW0(2, "");

	if (pnp->get_attr_string(parent, IDE_CONTROLLER_CONTROLLER_NAME_ITEM,
			&controller_name, true) != B_OK) {
		dprintf("ide: ignored controller - controller name missing\n");
		goto err;
	}

	channel_id = pnp->create_id(IDE_CHANNEL_ID_GENERATOR);

	if (channel_id < 0) {
		SHOW_ERROR(0, "Cannot register IDE controller %s - out of IDs", controller_name);
		goto err;
	}

	{
		device_attr attrs[] =
		{
			{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE, { string: SCSI_FOR_SIM_MODULE_NAME }},

			{ SCSI_DESCRIPTION_CONTROLLER_NAME, B_STRING_TYPE,
				{ string: controller_name }},
			// maximum number of blocks per transmission:
			// - ATAPI uses packets, i.e. normal SCSI limits apply
			//   but I'm not sure about controller restrictions
			// - ATA allows up to 256 blocks
			// - some broken disk's firmware (read: IBM DTTA drives)
			//   don't like 256 blocks in command queuing mode
			// -> use 255 blocks as a least common nominator
			//    (this is still 127.5K for HDs and 510K for CDs,
			//     which should be sufficient)
			// Note: to fix specific drive bugs, use ide_sim_get_restrictions()
			// in ide_sim.c!
			{ B_DMA_MAX_TRANSFER_BLOCKS, B_UINT32_TYPE, { ui32: 255 }},
			{ IDE_CHANNEL_ID_ITEM, B_UINT32_TYPE, { ui32: channel_id }},
//			{ PNP_MANAGER_ID_GENERATOR, B_STRING_TYPE, { string: IDE_CHANNEL_ID_GENERATOR }},
//			{ PNP_MANAGER_AUTO_ID, B_UINT32_TYPE, { ui32: channel_id }},

			{ NULL }
		};

		return pnp->register_node(parent, IDE_SIM_MODULE_NAME, attrs, NULL,
			NULL);
	}

err:
	return B_NO_MEMORY;
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


ide_for_controller_interface ide_for_controller_module = {
	{
		{
			IDE_FOR_CONTROLLER_MODULE_NAME,
			0,
			&std_ops
		},

		NULL,	// supported devices
		ide_channel_added,
		NULL,
		NULL,
		NULL
	},

	ide_irq_handler
};
