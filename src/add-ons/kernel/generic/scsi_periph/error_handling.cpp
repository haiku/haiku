/*
 * Copyright 2011, Haiku, Inc. All RightsReserved.
 * Copyright 2002-03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


//!	Error handling


#include "scsi_periph_int.h"


/*! Decode sense data and generate error code. */
static
err_res check_sense(scsi_periph_device_info *device, scsi_ccb *request)
{
	scsi_sense *sense = (scsi_sense *)request->sense;

	if ((request->subsys_status & SCSI_AUTOSNS_VALID) == 0) {
		SHOW_ERROR0(2, "No auto-sense (but there should be)");

		// shouldn't happen (cam_status should be CAM_AUTOSENSE_FAIL
		// as we asked for autosense)
		return MK_ERROR(err_act_fail, B_ERROR);
	}

	if (SCSI_MAX_SENSE_SIZE - request->sense_resid
			< (int)offsetof(scsi_sense, add_sense_length) + 1) {
		SHOW_ERROR(2, "sense too short (%d bytes)", SCSI_MAX_SENSE_SIZE - request->sense_resid);

		// that's a bit too short
		return MK_ERROR(err_act_fail, B_ERROR);
	}

	switch (sense->error_code) {
		case SCSIS_DEFERRED_ERROR:
			// we are doomed - some previous request turned out to have failed
			// we neither know which one nor can we resubmit it		
			SHOW_ERROR0(2, "encountered DEFERRED ERROR - bye, bye");
			return MK_ERROR(err_act_ok, B_OK);

		case SCSIS_CURR_ERROR:
			// we start with very specific and finish very general error infos
			switch ((sense->asc << 8) | sense->ascq) {
				case SCSIS_ASC_AUDIO_PLAYING:
					SHOW_INFO0(2, "busy because playing audio");

					// we need something like "busy"
					return MK_ERROR(err_act_fail, B_DEV_NOT_READY);

				case SCSIS_ASC_LUN_NEED_INIT:
					SHOW_INFO0(2, "LUN needs init");

					// reported by some devices that are idle and spun down
					// sending START UNIT should awake them
					return MK_ERROR(err_act_start, B_NO_INIT);

				case SCSIS_ASC_LUN_NEED_MANUAL_HELP:
					SHOW_ERROR0(2, "LUN needs manual help");

					return MK_ERROR(err_act_fail, B_DEV_NOT_READY);

				case SCSIS_ASC_LUN_FORMATTING:
					SHOW_INFO0(2, "LUN is formatting");

					// we could wait, but as formatting normally takes quite long,
					// we give up without any further retries
					return MK_ERROR(err_act_fail, B_DEV_NOT_READY);

				case SCSIS_ASC_MEDIUM_CHANGED:
					SHOW_FLOW0(3, "Medium changed");
					periph_media_changed(device, request);
					return MK_ERROR(err_act_fail, B_DEV_MEDIA_CHANGED);

				case SCSIS_ASC_WRITE_ERR_AUTOREALLOC:
					SHOW_ERROR0(2, "Recovered write error - block got reallocated automatically");
					return MK_ERROR(err_act_ok, B_OK);

				case SCSIS_ASC_ID_RECOV:
					SHOW_ERROR0(2, "Recovered ID with ECC");
					return MK_ERROR(err_act_ok, B_OK);

				case SCSIS_ASC_REMOVAL_REQUESTED:
					SHOW_INFO0(2, "Removal requested");
					mutex_lock(&device->mutex);
					device->removal_requested = true;
					mutex_unlock(&device->mutex);

					return MK_ERROR(err_act_retry, B_DEV_MEDIA_CHANGE_REQUESTED);

				case SCSIS_ASC_LUN_BECOMING_READY:
					SHOW_INFO0(2, "Becoming ready");
					// wait a bit - the device needs some time
					snooze(100000);
					return MK_ERROR(err_act_many_retries, B_DEV_NOT_READY);

				case SCSIS_ASC_WAS_RESET:
					SHOW_INFO0(2, "Unit was reset");
					// TBD: need a better error code here
					// as some earlier command led to the reset, we are innocent
					return MK_ERROR(err_act_retry, B_DEV_NOT_READY);
			}

			switch (sense->asc) {
				case SCSIS_ASC_DATA_RECOV_NO_ERR_CORR >> 8:
				case SCSIS_ASC_DATA_RECOV_WITH_CORR >> 8:
					// these are the groups of recovered data with or without correction
					// we should print at least a warning here
					SHOW_ERROR(0, "Recovered data, asc=0x%2x, ascq=0x%2x", 
						sense->asc, sense->ascq);
					return MK_ERROR(err_act_ok, B_OK);

				case SCSIS_ASC_WRITE_PROTECTED >> 8:
					SHOW_ERROR0( 2, "Write protected" );

					// isn't there any proper "write protected" error code?
					return MK_ERROR(err_act_fail, B_READ_ONLY_DEVICE);
					
				case SCSIS_ASC_NO_MEDIUM >> 8:
					SHOW_FLOW0(2, "No medium");
					return MK_ERROR(err_act_fail, B_DEV_NO_MEDIA);
			}

			// we issue this info very late, so we don't clutter syslog with
			// messages about changed or missing media
			SHOW_ERROR(3, "0x%04x", (sense->asc << 8) | sense->ascq);

			switch (sense->sense_key) {
				case SCSIS_KEY_NO_SENSE:
					SHOW_ERROR0(2, "No sense");

					// we thought there was an error, huh?
					return MK_ERROR(err_act_ok, B_OK);
	
				case SCSIS_KEY_RECOVERED_ERROR:
					SHOW_ERROR0(2, "Recovered error");

					// we should probably tell about that; perhaps tomorrow
					return MK_ERROR(err_act_ok, B_OK);

				case SCSIS_KEY_NOT_READY:
					return MK_ERROR(err_act_retry, B_DEV_NOT_READY);

				case SCSIS_KEY_MEDIUM_ERROR:
					SHOW_ERROR0(2, "Medium error");
					return MK_ERROR( err_act_retry, B_DEV_RECALIBRATE_ERROR);

				case SCSIS_KEY_HARDWARE_ERROR:
					SHOW_ERROR0(2, "Hardware error");
					return MK_ERROR(err_act_retry, B_DEV_SEEK_ERROR);

				case SCSIS_KEY_ILLEGAL_REQUEST:
					SHOW_ERROR0(2, "Illegal request");
					return MK_ERROR(err_act_invalid_req, B_ERROR);

				case SCSIS_KEY_UNIT_ATTENTION:
					SHOW_ERROR0(2, "Unit attention");
					return MK_ERROR( err_act_retry, B_DEV_NOT_READY);

				case SCSIS_KEY_DATA_PROTECT:
					SHOW_ERROR0(2, "Data protect");

					// we could set "permission denied", but that's probably
					// irritating to the user
					return MK_ERROR(err_act_fail, B_NOT_ALLOWED);

				case SCSIS_KEY_BLANK_CHECK:
					SHOW_ERROR0(2, "Is blank");

					return MK_ERROR(err_act_fail, B_DEV_UNREADABLE);

				case SCSIS_KEY_VENDOR_SPECIFIC:
					return MK_ERROR(err_act_fail, B_ERROR);

				case SCSIS_KEY_COPY_ABORTED:
					// we don't use copy, so this is really wrong
					return MK_ERROR(err_act_fail, B_ERROR);

				case SCSIS_KEY_ABORTED_COMMAND:
					// proper error code?
					return MK_ERROR(err_act_retry, B_ERROR);

				case SCSIS_KEY_EQUAL:
				case SCSIS_KEY_MISCOMPARE:
					// we don't search, so this is really wrong
					return MK_ERROR(err_act_fail, B_ERROR);

				case SCSIS_KEY_VOLUME_OVERFLOW:
					// not the best return code, but this error doesn't apply
					// to devices we currently support
					return MK_ERROR(err_act_fail, B_DEV_SEEK_ERROR);

				case SCSIS_KEY_RESERVED:
				default:
					return MK_ERROR(err_act_fail, B_ERROR);
			}

		default:
			// shouldn't happen - there are only 2 error codes defined
			SHOW_ERROR(2, "Invalid sense type (0x%x)", sense->error_code);
			return MK_ERROR(err_act_fail, B_ERROR);
	}
}


/*!	Check scsi status, using sense if available. */
static err_res
check_scsi_status(scsi_periph_device_info *device, scsi_ccb *request)
{
	SHOW_FLOW(3, "%d", request->device_status & SCSI_STATUS_MASK);

	switch (request->device_status & SCSI_STATUS_MASK) {
		case SCSI_STATUS_GOOD:
			// shouldn't happen (cam_status should be CAM_REQ_CMP)
			return MK_ERROR(err_act_ok, B_OK);

		case SCSI_STATUS_CHECK_CONDITION:
			return check_sense(device, request);

		case SCSI_STATUS_QUEUE_FULL:
			// SIM should have automatically requeued request, fall through
		case SCSI_STATUS_BUSY:
			// take deep breath and try again
			snooze(1000000);
			return MK_ERROR(err_act_retry, B_DEV_TIMEOUT);

		case SCSI_STATUS_COMMAND_TERMINATED:
			return MK_ERROR(err_act_retry, B_INTERRUPTED);

		default:
			return MK_ERROR(err_act_retry, B_ERROR);
	}
}


/*!	Check result of request
 *	1. check SCSI subsystem problems
 *	2. if request hit device, check SCSI status
 *	3. if request got executed, check sense
 */
err_res
periph_check_error(scsi_periph_device_info *device, scsi_ccb *request)
{
	SHOW_FLOW(4, "%d", request->subsys_status & SCSI_SUBSYS_STATUS_MASK);

	switch (request->subsys_status & SCSI_SUBSYS_STATUS_MASK) {
		// everything is ok
		case SCSI_REQ_CMP:
			return MK_ERROR(err_act_ok, B_OK);

		// no device
		case SCSI_LUN_INVALID:
		case SCSI_TID_INVALID:
		case SCSI_PATH_INVALID:
		case SCSI_DEV_NOT_THERE:
		case SCSI_NO_HBA:
			SHOW_ERROR0(2, "No device");
			return MK_ERROR(err_act_fail, B_DEV_BAD_DRIVE_NUM);

		// device temporary unavailable
		case SCSI_SEL_TIMEOUT:
		case SCSI_BUSY:
		case SCSI_SCSI_BUSY:
		case SCSI_HBA_ERR:
		case SCSI_MSG_REJECT_REC:
		case SCSI_NO_NEXUS:
		case SCSI_FUNC_NOTAVAIL:
		case SCSI_RESRC_UNAVAIL:
			// take a deep breath and hope device becomes ready
			snooze(1000000);
			return MK_ERROR(err_act_retry, B_DEV_TIMEOUT);

		// data transmission went wrong
		case SCSI_DATA_RUN_ERR:
		case SCSI_UNCOR_PARITY:
			SHOW_ERROR0(2, "Data transmission failed");
			// retry immediately
			return MK_ERROR(err_act_retry, B_DEV_READ_ERROR);

		// request broken
		case SCSI_REQ_INVALID:
			SHOW_ERROR0(2, "Invalid request");
			return MK_ERROR(err_act_fail, B_ERROR);

		// request aborted
		case SCSI_REQ_ABORTED:
		case SCSI_SCSI_BUS_RESET:
		case SCSI_REQ_TERMIO:
		case SCSI_UNEXP_BUSFREE:
		case SCSI_BDR_SENT:
		case SCSI_CMD_TIMEOUT:
		case SCSI_IID_INVALID:
		case SCSI_UNACKED_EVENT:
		case SCSI_IDE:
		case SCSI_SEQUENCE_FAIL:
			// take a small breath and retry
			snooze(100000);
			return MK_ERROR(err_act_retry, B_DEV_TIMEOUT);

		// device error
		case SCSI_REQ_CMP_ERR:
			return check_scsi_status(device, request);

		// device error, but we don't know what happened
		case SCSI_AUTOSENSE_FAIL:
			SHOW_ERROR0(2, "Auto-sense failed, don't know what really happened");
			return MK_ERROR(err_act_fail, B_ERROR);

		// should not happen, give up
		case SCSI_BUS_RESET_DENIED:
		case SCSI_PROVIDE_FAIL:
		case SCSI_UA_TERMIO:
		case SCSI_CDB_RECVD:
		case SCSI_LUN_ALLREADY_ENAB:
			// supposed to fall through
		default:
			return MK_ERROR(err_act_fail, B_ERROR);
	}
}
