/*
 * Copyright 2004-2008, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Controllers use this interface to interact with bus manager.
*/


#include "scsi_internal.h"
#include "queuing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**	new scsi controller added
 *	in return, we register a new scsi bus node and let its fixed
 *	consumer (the SCSI device layer) automatically scan it for devices
 */

static status_t
scsi_controller_added(device_node *parent)
{
	const char *controller_name;
	int32 pathID;

	SHOW_FLOW0(4, "");

	if (pnp->get_attr_string(parent, SCSI_DESCRIPTION_CONTROLLER_NAME,
			&controller_name, false) != B_OK) {
		dprintf("scsi: ignored controller - controller name missing\n");
		return B_ERROR;
	}

	pathID = pnp->create_id(SCSI_PATHID_GENERATOR);
	if (pathID < 0) {
		dprintf("scsi: Cannot register SCSI controller %s - out of path IDs\n",
			controller_name);
		return B_ERROR;
	}

	{
		device_attr attrs[] = {
			{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
				{ string: "SCSI Controller" }},

			// remember who we are
			// (could use the controller name, but probably some software would choke)
			// TODO create_id() generates a 32 bit ranged integer but we need only 8 bits
			{ SCSI_BUS_PATH_ID_ITEM, B_UINT8_TYPE, { ui8: (uint8)pathID }},

			// tell PnP manager to clean up ID
//			{ PNP_MANAGER_ID_GENERATOR, B_STRING_TYPE, { string: SCSI_PATHID_GENERATOR }},
//			{ PNP_MANAGER_AUTO_ID, B_UINT32_TYPE, { ui32: path_id }},
			{}
		};

		return pnp->register_node(parent, SCSI_BUS_MODULE_NAME, attrs, NULL,
			NULL);
	}
}


static status_t
scsi_controller_init(device_node *node, void **_cookie)
{
	*_cookie = node;
	return B_OK;
}


static status_t
scsi_controller_register_raw_device(void *_cookie)
{
	device_node *node = (device_node *)_cookie;
	uint32 channel;
	uint8 pathID;
	char *name;

#if 1
// TODO: this seems to cause a crash in some configurations, and needs to be investigated!
//		see bug #389 and #393.
// TODO: check if the above is still true
	if (pnp->get_attr_uint32(node, "ide/channel_id", &channel, true) == B_OK) {
		// this is actually an IDE device, we don't need to publish
		// a bus device for those
		return B_OK;
	}
#endif
	pnp->get_attr_uint8(node, SCSI_BUS_PATH_ID_ITEM, &pathID, false);

	// put that on heap to not overflow the limited kernel stack
	name = (char*)malloc(PATH_MAX + 1);
	if (name == NULL)
		return B_NO_MEMORY;

	snprintf(name, PATH_MAX + 1, "bus/scsi/%d/bus_raw", pathID);

	return pnp->publish_device(node, name, SCSI_BUS_RAW_MODULE_NAME);
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
		scsi_controller_init,
		NULL,	// uninit
		scsi_controller_register_raw_device,
		NULL,	// rescan
		NULL,	// removed
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
