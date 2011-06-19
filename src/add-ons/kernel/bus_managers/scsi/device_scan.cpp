/*
 * Copyright 2004-2007, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*!
	Device scanner.

	Scans SCSI busses for devices. Scanning is initiated by
	a SCSI device node probe (see device_mgr.c)
*/


#include "scsi_internal.h"

#include <string.h>
#include <stdlib.h>

#include <algorithm>


/*! send TUR
	result: true, if device answered
		false, if there is no device
*/
static bool
scsi_scan_send_tur(scsi_ccb *worker_req)
{
	scsi_cmd_tur *cmd = (scsi_cmd_tur *)worker_req->cdb;

	SHOW_FLOW0( 3, "" );

	memset( cmd, 0, sizeof( *cmd ));
	cmd->opcode = SCSI_OP_TEST_UNIT_READY;

	worker_req->sg_list = NULL;
	worker_req->data = NULL;
	worker_req->data_length = 0;
	worker_req->cdb_length = sizeof(*cmd);
	worker_req->timeout = 0;
	worker_req->sort = -1;
	worker_req->flags = SCSI_DIR_NONE;

	scsi_sync_io( worker_req );

	SHOW_FLOW( 3, "status=%x", worker_req->subsys_status );

	// as this command was only for syncing, we ignore almost all errors
	switch (worker_req->subsys_status) {
		case SCSI_SEL_TIMEOUT:
			// there seems to be no device around
			return false;

		default:
			return true;
	}
}


/*!	get inquiry data
	returns true on success
*/
static bool
scsi_scan_get_inquiry(scsi_ccb *worker_req, scsi_res_inquiry *new_inquiry_data)
{
	scsi_cmd_inquiry *cmd = (scsi_cmd_inquiry *)worker_req->cdb;
	scsi_device_info *device = worker_req->device;

	SHOW_FLOW0(3, "");

	// in case not whole structure gets transferred, we set remaining data to zero
	memset(new_inquiry_data, 0, sizeof(*new_inquiry_data));

	cmd->opcode = SCSI_OP_INQUIRY;
	cmd->lun = device->target_lun;
	cmd->evpd = 0;
	cmd->page_code = 0;
	cmd->allocation_length = sizeof(*new_inquiry_data);

	worker_req->sg_list = NULL;
	worker_req->data = (uchar *)new_inquiry_data;
	worker_req->data_length = sizeof(*new_inquiry_data);
	worker_req->cdb_length = 6;
	worker_req->timeout = SCSI_STD_TIMEOUT;
	worker_req->sort = -1;
	worker_req->flags = SCSI_DIR_IN;

	scsi_sync_io(worker_req);

	switch (worker_req->subsys_status) {
		case SCSI_REQ_CMP: {
			char vendor[9], product[17], rev[5];

			SHOW_FLOW0(3, "send successfully");

			// we could check transmission length here, but as we reset
			// missing bytes before, we get kind of valid data anyway (hopefully)

			strlcpy(vendor, new_inquiry_data->vendor_ident, sizeof(vendor));
			strlcpy(product, new_inquiry_data->product_ident, sizeof(product));
			strlcpy(rev, new_inquiry_data->product_rev, sizeof(rev));

			SHOW_INFO(3, "device type: %d, qualifier: %d, removable: %d, ANSI version: %d, response data format: %d\n"
				"vendor: %s, product: %s, rev: %s",
				new_inquiry_data->device_type, new_inquiry_data->device_qualifier,
				new_inquiry_data->removable_medium, new_inquiry_data->ansi_version,
				new_inquiry_data->response_data_format,
				vendor, product, rev);

			SHOW_INFO(3, "additional_length: %d", new_inquiry_data->additional_length + 4);

			// time to show standards the device conforms to;
			// unfortunately, ATAPI CD-ROM drives tend to tell that they have
			// only minimal info (36 bytes), but still they return (valid!) 96 bytes -
			// bad luck
			if (std::min((int)cmd->allocation_length,
						new_inquiry_data->additional_length + 4)
					>= (int)offsetof(scsi_res_inquiry, _res74)) {
				int i, previousStandard = -1;

				for (i = 0; i < 8; ++i) {
					int standard = B_BENDIAN_TO_HOST_INT16(
						new_inquiry_data->version_descriptor[i]);

					// omit standards reported twice
					if (standard != previousStandard && standard != 0)
						SHOW_INFO(3, "standard: %04x", standard);

					previousStandard = standard;
				}
			}

			//snooze( 1000000 );

	/*		{
				unsigned int i;

				for( i = 0; i < worker_req->data_length - worker_req->data_resid; ++i ) {
					dprintf( "%2x ", *((char *)new_inquiry_data + i) );
				}

				dprintf( "\n" );
			}*/

			return true;
		}

		default:
			return false;
	}
}


status_t
scsi_scan_lun(scsi_bus_info *bus, uchar target_id, uchar target_lun)
{
	scsi_ccb *worker_req;
	scsi_res_inquiry new_inquiry_data;
	status_t res;
	scsi_device_info *device;
	bool found;

	//snooze(1000000);

	SHOW_FLOW(3, "%d:%d:%d", bus->path_id, target_id, target_lun);

	res = scsi_force_get_device(bus, target_id, target_lun, &device);
	if (res != B_OK)
		goto err;

	//SHOW_FLOW(3, "temp_device: %d", (int)temp_device);

	worker_req = scsi_alloc_ccb(device);
	if (worker_req == NULL) {
		// there is no out-of-mem code
		res = B_NO_MEMORY;
		goto err2;
	}

	SHOW_FLOW0(3, "2");

	worker_req->flags = SCSI_DIR_IN;

	// to give controller a chance to transfer speed negotiation, we
	// send a TUR first; unfortunatily, some devices don't like TURing
	// invalid luns apart from lun 0...
	if (device->target_lun == 0) {
		if (!scsi_scan_send_tur(worker_req)) {
			// TBD: need better error code like "device not found"
			res = B_NAME_NOT_FOUND;
			goto err3;
		}
	}

	// get inquiry data to be used as identification
	// and to check whether there is a device at all
	found = scsi_scan_get_inquiry(worker_req, &new_inquiry_data)
		&& new_inquiry_data.device_qualifier == scsi_periph_qual_connected;

	// get rid of temporary device - as soon as the device is
	// registered, it can be loaded, and we don't want two data
	// structures for one device (the temporary and the official one)
	scsi_free_ccb(worker_req);
	scsi_put_forced_device(device);

	if (!found) {
		// TBD: better error code, s.a.
		return B_NAME_NOT_FOUND;
	}

	// !danger!
	// if a new device is detected on the same connection, all connections
	// to the old device are disabled;
	// scenario: you plug in a device, scan the bus, replace the device and then
	// open it; in this case, the connection seems to be to the old device, but really
	// is to the new one; if you scan the bus now, the opened connection is disabled
	// - bad luck -
	// solution 1: scan device during each scsi_init_device
	// disadvantage: it takes time and we had to submit commands during the load
	//   sequence, which could lead to deadlocks
	// solution 2: device drivers must scan devices before first use
	// disadvantage: it takes time and driver must perform a task that
	//   the bus_manager should really take care of
	scsi_register_device(bus, target_id, target_lun, &new_inquiry_data);

	return B_OK;

err3:
	scsi_free_ccb(worker_req);
err2:
	scsi_put_forced_device(device);
err:
	return res;
}


status_t
scsi_scan_bus(scsi_bus_info *bus)
{
	int initiator_id, target_id;
	scsi_path_inquiry inquiry;
	uchar res;

	SHOW_FLOW0( 3, "" );

	// get ID of initiator (i.e. controller)
	res = scsi_inquiry_path(bus, &inquiry);
	if (res != SCSI_REQ_CMP)
		return B_ERROR;

	initiator_id = inquiry.initiator_id;

	SHOW_FLOW(3, "initiator_id=%d", initiator_id);

	// tell SIM to rescan bus (needed at least by IDE translator)
	// as this function is optional for SIM, we ignore its result
	bus->interface->scan_bus(bus->sim_cookie);

	for (target_id = 0; target_id < (int)bus->max_target_count; ++target_id) {
		int lun;

		SHOW_FLOW(3, "target: %d", target_id);

		if (target_id == initiator_id)
			continue;

		// TODO: there are a lot of devices out there that go mad if you probe
		// anything but LUN 0, so we should probably add a black-list
		// or something
		for (lun = 0; lun <= MAX_LUN_ID; ++lun) {
			status_t res;

			SHOW_FLOW(3, "lun: %d", lun);

			res = scsi_scan_lun(bus, target_id, lun);

			// if there is no device at lun 0, there's probably no device at all
			if (lun == 0 && res != SCSI_REQ_CMP)
				break;
		}
	}

	SHOW_FLOW0(3, "done");
	return B_OK;
}
