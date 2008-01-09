/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open IDE bus manager

	Command queuing functions
*/

#include "ide_internal.h"
#include "ide_sim.h"
#include "ide_cmds.h"

#include <string.h>
#include <malloc.h>


// maximum number of errors until command queuing is disabled
#define MAX_CQ_FAILURES 3




/**	tell device to abort all queued requests
 *	(tells XPT to resubmit these requests)
 *	return: true - abort successful
 *	        false - abort failed (in this case, nothing can be done)
 */

bool
send_abort_queue(ide_device_info *device)
{
	int status;
	ide_bus_info *bus = device->bus;

	SHOW_FLOW0( 3, "" );

	device->tf.write.command = IDE_CMD_NOP;
	// = discard outstanding commands
	device->tf.write.features = IDE_CMD_NOP_NOP;

	device->tf_param_mask = ide_mask_features;

	if (!send_command(device, NULL, true, 0, ide_state_accessing))
		goto err;

	if (!wait_for_drdy(device))
		goto err;

	// device must answer "command rejected" and discard outstanding commands
	status = bus->controller->get_altstatus(bus->channel_cookie);

	if ((status & ide_status_err) == 0)
		goto err;

	if (!bus->controller->read_command_block_regs(bus->channel_cookie,
			&device->tf, ide_mask_error)) {
		// don't bother trying bus_reset as controller disappeared
		device->subsys_status = SCSI_HBA_ERR;
		return false;
	}

	if ((device->tf.read.error & ide_error_abrt) == 0)
		goto err;

	finish_all_requests(device, NULL, 0, true);
	return true;

err:
	// ouch! device didn't react - we have to reset it
	//return reset_device(device, NULL);
	return false;
}
