/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI bus manager

	Interface for SIMs

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
scsi_controller_added(pnp_node_handle parent)
{
	int path_id;
	char *str, *controller_name;
	
	SHOW_FLOW0( 4, "" );
	
	if( pnp->get_attr_string( parent, PNP_DRIVER_TYPE, &str, false ) != B_OK )
		return B_ERROR;
		
	if (strcmp(str, SCSI_SIM_TYPE_NAME) != 0) {
		free(str);
		return B_ERROR;
	}
	
	free( str );

	if( pnp->get_attr_string( parent, SCSI_DESCRIPTION_CONTROLLER_NAME, 
		&controller_name, false ) != B_OK )
	{
		pnp->get_attr_string( parent, PNP_DRIVER_DRIVER, &str, false );
		SHOW_ERROR( 0, "Ignored controller managed by %s - controller name missing",
			str );
		return B_ERROR;
	}
	
	path_id = pnp->create_id( SCSI_PATHID_GENERATOR );

	if( path_id < 0 ) {
		SHOW_ERROR( 0, "Cannot register SCSI controller %s - out of path IDs",
			controller_name );
		free( controller_name );
		return B_ERROR;
	}
	
	free(controller_name);
	
	{
		pnp_node_attr attrs[] =
		{
			// general information
			{ PNP_DRIVER_DRIVER, B_STRING_TYPE, { string: SCSI_BUS_MODULE_NAME }},
			{ PNP_DRIVER_TYPE, B_STRING_TYPE, { string: SCSI_BUS_TYPE_NAME }},

			// we are a bus
			{ PNP_BUS_IS_BUS, B_UINT8_TYPE, { ui8: 1 }},
			// search for peripheral drivers after bus is fully scanned
			{ PNP_BUS_DEFER_PROBE, B_UINT8_TYPE, {ui8: 1 }},

			// remember who we are 
			// (could use the controller name, but probably some software would choke)
			{ SCSI_BUS_PATH_ID_ITEM, B_UINT8_TYPE, { ui8: path_id }},

			// tell PnP manager to clean up ID
			{ PNP_MANAGER_ID_GENERATOR, B_STRING_TYPE, { string: SCSI_PATHID_GENERATOR }},
			{ PNP_MANAGER_AUTO_ID, B_UINT32_TYPE, { ui32: path_id }},

			// tell internal bus raw driver to register bus' device in devfs
			{ PNP_DRIVER_FIXED_CONSUMER, B_STRING_TYPE, { string: SCSI_BUS_RAW_MODULE_NAME }},
			{ NULL, 0 }
		};

		pnp_node_handle node;
	
		return pnp->register_device( parent, attrs, NULL, &node );
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

		NULL,
		NULL,	
		scsi_controller_added,
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
