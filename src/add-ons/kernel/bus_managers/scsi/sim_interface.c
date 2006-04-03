/*
 * Copyright 2004-2006, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Controllers use this interface to interact with bus manager.
*/


#include "scsi_internal.h"
#include "queuing.h"

#include <stdlib.h>
#include <string.h>


/**	new scsi controller added
 *	in return, we register a new scsi bus node and let its fixed 
 *	consumer (the SCSI device layer) automatically scan it for devices
 */

static status_t
scsi_controller_added(device_node_handle parent)
{
	char *controller_name;
	int path_id;

	SHOW_FLOW0(4, "");

	if (pnp->get_attr_string(parent, SCSI_DESCRIPTION_CONTROLLER_NAME, 
		&controller_name, false) != B_OK) {
		dprintf("scsi: ignored controller - controller name missing\n");
		return B_ERROR;
	}

	path_id = pnp->create_id(SCSI_PATHID_GENERATOR);
	if (path_id < 0) {
		dprintf("scsi: Cannot register SCSI controller %s - out of path IDs\n",
			controller_name);
		free(controller_name);
		return B_ERROR;
	}

	free(controller_name);

	{
		device_attr attrs[] = {
			// general information
			{ B_DRIVER_MODULE, B_STRING_TYPE, { string: SCSI_BUS_MODULE_NAME }},

			// we are a bus
			{ PNP_BUS_IS_BUS, B_UINT8_TYPE, { ui8: 1 }},

			// remember who we are 
			// (could use the controller name, but probably some software would choke)
			{ SCSI_BUS_PATH_ID_ITEM, B_UINT8_TYPE, { ui8: path_id }},

			// tell PnP manager to clean up ID
			{ PNP_MANAGER_ID_GENERATOR, B_STRING_TYPE, { string: SCSI_PATHID_GENERATOR }},
			{ PNP_MANAGER_AUTO_ID, B_UINT32_TYPE, { ui32: path_id }},

			// tell internal bus raw driver to register bus' device in devfs
			{ B_DRIVER_FIXED_CHILD, B_STRING_TYPE, { string: SCSI_BUS_RAW_MODULE_NAME }},
			{ NULL, 0 }
		};

		device_node_handle node;
// TODO: this seems to cause a crash in some configurations, and needs to be investigated!
//		see bug #389 and #393.
#if 0
		uint32 channel;

		if (pnp->get_attr_uint32(parent, "ide/channel_id", &channel, true) == B_OK) {
			// this is actually an IDE device, we don't need to publish
			// a bus device for those
			attrs[5].name = NULL;
		}
#endif

		return pnp->register_device(parent, attrs, NULL, &node);
	}
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


scsi_for_sim_interface scsi_for_sim_module =
{
	{
		{
			SCSI_FOR_SIM_MODULE_NAME,
			0,
			std_ops
		},

		NULL,	// supported devices
		scsi_controller_added,
		NULL,
		NULL,	
		NULL
	},

	scsi_requeue_request,
	scsi_resubmit_request,
	scsi_request_finished,

	scsi_alloc_dpc,
	scsi_free_dpc,
	scsi_schedule_dpc,

	scsi_block_bus,
	scsi_unblock_bus,
	scsi_block_device,
	scsi_unblock_device,

	scsi_cont_send_bus,
	scsi_cont_send_device
};
