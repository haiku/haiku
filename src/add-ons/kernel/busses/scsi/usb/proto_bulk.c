/**
 *
 * TODO: description
 *
 * This file is a part of USB SCSI CAM for Haiku.
 * May be used under terms of the MIT License
 *
 * Author(s):
 * 	Siarzhuk Zharski <imker@gmx.li>
 *
 *
 */
/** bulk-only protocol specific implementation */

/* References:
 * USB Mass Storage Class specifications:
 * http://www.usb.org/developers/data/devclass/usbmassover_11.pdf	[1]
 * http://www.usb.org/developers/data/devclass/usbmassbulk_10.pdf	[2]
 */
#include "usb_scsi.h"

#include "device_info.h"

#include "proto_module.h"
#include "proto_common.h"
#include "proto_bulk.h"

#include "usb_defs.h"

#include <string.h>	/* strncpy */

/*Bulk-Only protocol specifics*/
#define USB_REQ_MS_RESET			 0xff		/* Bulk-Only reset */
#define USB_REQ_MS_GET_MAX_LUN 0xfe		/* Get maximum lun */

/* Command Block Wrapper */
typedef struct _usb_mass_CBW{
	uint32 signature;
	uint32 tag;
	uint32 data_transfer_len;
	uint8	flags;
	uint8	lun;
	uint8	cdb_len;
#define CBW_CDB_LENGTH	16
	uint8	CDB[CBW_CDB_LENGTH];
} usb_mass_CBW; /*sizeof(usb_mass_CBW) must be 31*/
#define CBW_LENGTH 0x1f

/* Command Status Wrapper */
typedef struct _usb_mass_CSW{
	uint32 signature;
	uint32 tag;
	uint32 data_residue;
	uint8	status;
} usb_mass_CSW; /*sizeof(usb_mass_CSW) must be 13*/
#define CSW_LENGTH 0x0d

#define CSW_STATUS_GOOD		0x0
#define CSW_STATUS_FAILED	0x1
#define CSW_STATUS_PHASE	0x2

#define CBW_SIGNATURE 0x43425355
#define CSW_SIGNATURE 0x53425355

#define CBW_FLAGS_OUT	0x00
#define CBW_FLAGS_IN	0x80

static void 	trace_CBW(usb_device_info *udi, const usb_mass_CBW *cbw);
static void 	trace_CSW(usb_device_info *udi, const usb_mass_CSW *csw);
static status_t	bulk_only_initialize(usb_device_info *udi);
static status_t	bulk_only_reset(usb_device_info *udi);
static void		bulk_only_transfer(usb_device_info *udi, uint8 *cmd,
								uint8	cmdlen,	 //sg_buffer *sgb,
								iovec *sg_data,	 int32 sg_count,
								int32	transfer_len, EDirection	dir,
								CCB_SCSIIO *ccbio, ud_transfer_callback cb);

/*=========================== tracing helpers ==================================*/
void trace_CBW(usb_device_info *udi, const usb_mass_CBW *cbw)
{
	char buf[sizeof(uint32) + 1] = {0};
	strncpy(buf, (char *)&cbw->signature, sizeof(uint32));
	PTRACE(udi, "\nCBW:{'%s'; tag:%d; data_len:%d; flags:0x%02x; lun:%d; cdb_len:%d;}\n",
		buf, cbw->tag, cbw->data_transfer_len,
		cbw->flags,	cbw->lun, cbw->cdb_len);
	udi->trace_bytes("CDB:\n", cbw->CDB, CBW_CDB_LENGTH);
}

void trace_CSW(usb_device_info *udi, const usb_mass_CSW *csw)
{
	char buf[sizeof(uint32) + 1] = {0};
	strncpy(buf, (char *)&csw->signature, sizeof(uint32));
	PTRACE(udi, "CSW:{'%s'; tag:%d; residue:%d; status:0x%02x}\n",
		buf, csw->tag, csw->data_residue, csw->status);
}

/**
	\fn:get_max_luns
	\param udi: device for wich max LUN info is requested
	\return:always B_OK - if info was not retrieved max LUN is defaulted to 0

	tries to retrieve the maximal Logical Unit Number supported by
	this device. If device doesn't support GET_MAX_LUN request - single LUN is
	assumed. ([2] 3.2)
*/
static status_t
get_max_luns(usb_device_info *udi)
{
	status_t status = B_OK;
	udi->max_lun = 0;
	if(!HAS_FIXES(udi->properties, FIX_NO_GETMAXLUN)){
		size_t len = 0;
		if(B_OK != (status = (*udi->usb_m->send_request)(udi->device,
							 USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_CLASS,
							 USB_REQ_MS_GET_MAX_LUN, 0x0, udi->interface,
							 0x1, &udi->max_lun, &len)))
		{
			if(status == B_DEV_STALLED){
				PTRACE_ALWAYS(udi, "get_max_luns[%d]:not supported. "
									"Assuming single LUN available.\n", udi->dev_num);
			} else {
				PTRACE(udi, "get_max_luns[%d]:failed(%08x)."
							"Assuming single LUN available.\n", udi->dev_num, status);
			}
			udi->max_lun = 0;
			status = B_OK;
		} /* else - all is OK - max luns info readed */
	}
	return status;
}
/**
	\fn:queue_bulk
	\param udi: device for which que_bulk request is performed
	\param buffer: data buffer, used in bulk i/o operation
	\param len: length of data buffer
	\param b_in: is "true" if input (device->host) data transfer, "false" otherwise
	\return: status of operation.

	performs queue_bulk USB request for corresponding pipe and handle timeout of this
	operation.
*/
static status_t
queue_bulk(usb_device_info *udi, void* buffer, size_t len, bool b_in)
{
	status_t status = B_OK;
	usb_pipe pipe = b_in ? udi->pipe_in : udi->pipe_out;
	status = (*udi->usb_m->queue_bulk)(pipe, buffer, len, bulk_callback, udi);
	if(status != B_OK){
		PTRACE_ALWAYS(udi, "queue_bulk:failed:%08x\n", status);
	} else {
		status = acquire_sem_etc(udi->trans_sem, 1, B_RELATIVE_TIMEOUT, udi->trans_timeout/*LOCK_TIMEOUT*/);
		if(status != B_OK){
			PTRACE_ALWAYS(udi, "queue_bulk:acquire_sem_etc failed:%08x\n", status);
			(*udi->usb_m->cancel_queued_transfers)(pipe);
		}
	}
	return status;
}
/**
	\fn:check_CSW
	\param udi:corresponding device info
	\param csw: CSW to be checked for validity and meaningfullness
	\param transfer_len: data transferred during operation, which is checked for status
	\return: "true" if CSW valid and meanigfull, "false" otherwise

	checks CSW for validity and meaningfullness as required by USB mass strorge
	BulkOnly specification ([2] 6.3)
*/
static bool
check_CSW(usb_device_info *udi,	usb_mass_CSW* csw, int transfer_len)
{
	bool is_valid = false;
	do{
		/* check for CSW validity */
		if(udi->actual_len != CSW_LENGTH){
			PTRACE_ALWAYS(udi, "check_CSW:wrong length %d instead of %d\n",
							 udi->actual_len, CSW_LENGTH);
			break;/* failed */
		}
		if(csw->signature != CSW_SIGNATURE){
			PTRACE_ALWAYS(udi, "check_CSW:wrong signature %08x instead of %08x\n",
							 csw->signature, CSW_SIGNATURE);
			break;/* failed */
		}
		if(csw->tag != udi->tag - 1){
			PTRACE_ALWAYS(udi, "check_CSW:tag mismatch received:%d, awaited:%d\n",
							 csw->tag, udi->tag-1);
			break;/* failed */
		}
		/* check for CSW meaningfullness */
		if(CSW_STATUS_PHASE == csw->status){
			PTRACE_ALWAYS(udi, "check_CSW:not meaningfull: phase error\n");
			break;/* failed */
		}
		if(transfer_len < csw->data_residue){
			PTRACE_ALWAYS(udi, "check_CSW:not meaningfull: "
							 "residue:%d is greater than transfer length:%d\n",
									csw->data_residue, transfer_len);
			break;/* failed */
		}
		is_valid = true;
	}while(false);
	return is_valid;
}
/**
	\fn:read_status
	\param udi: corresponding device
	\param csw: buffer for CSW data
	\param transfer_len: data transferred during operation, which is checked for status
	\return: success status code

	reads CSW from device as proposed in ([2] 5.3.3; Figure 2.).
*/
static status_t
read_status(usb_device_info *udi, usb_mass_CSW* csw, int transfer_len)
{
	status_t status = B_ERROR;
	int try = 0;
	do{
		status = queue_bulk(udi, csw, CSW_LENGTH, true);
		if(try == 0){
			if(B_OK != status || B_OK != udi->status){
				status = (*udi->usb_m->clear_feature)(udi->pipe_in, USB_FEATURE_ENDPOINT_HALT);
				if(status != 0){
					PTRACE_ALWAYS(udi, "read_status:failed 1st try, "
						"status:%08x; usb_status:%08x\n", status, udi->status);
					(*udi->protocol_m->reset)(udi);
					break;
				}
				continue; /* go to second try*/
			}
			/* CSW was readed without errors */
		} else { /* second try */
			if(B_OK != status || B_OK != udi->status){
				PTRACE_ALWAYS(udi, "read_status:failed 2nd try status:%08x; usb_status:%08x\n",
								status, udi->status);
				(*udi->protocol_m->reset)(udi);
				status = (B_OK == status) ? udi->status : status;
				break;
			}
		}
		if(!check_CSW(udi, csw, transfer_len)){
			(*udi->protocol_m->reset)(udi);
			status = B_ERROR;
			break;
		}
		trace_CSW(udi, csw);
		break; /* CSW was read successfully */
	}while(try++ < 2);
	return status;
}

/*================= "standard" protocol procedures ==============================*/

/**
	\fn:bulk_only_initialize
	\param udi: device on wich we should perform initialization
	\return:error code if initialization failed or B_OK if it passed

	initialize procedure for bulk only protocol devices.
*/
status_t
bulk_only_initialize(usb_device_info *udi)
{
	status_t status = B_OK;
	status = get_max_luns(udi);
	return status;
}
/**
	\fn:bulk_only_reset
	\param udi: device on wich we should perform reset
	\return:error code if reset failed or B_OK if it passed

	reset procedure for bulk only protocol devices. Tries to send
	BulkOnlyReset USB request and clear USB_FEATURE_ENDPOINT_HALT features on
	input and output pipes. ([2] 3.1)
*/
status_t
bulk_only_reset(usb_device_info *udi)
{
	status_t status = B_ERROR;
	status = (*udi->usb_m->send_request)(udi->device,
						USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT,
						USB_REQ_MS_RESET, 0,
						udi->interface, 0, 0, 0);
	if(status != B_OK){
		PTRACE_ALWAYS(udi, "bulk_only_reset: reset request failed: %08x\n", status);
	}
	if(B_OK != (status = (*udi->usb_m->clear_feature)(udi->pipe_in,
										 USB_FEATURE_ENDPOINT_HALT)))
	{
		PTRACE_ALWAYS(udi, "bulk_only_reset: clear_feature on pipe_in failed: %08x\n", status);
	}
	if(B_OK != (status = (*udi->usb_m->clear_feature)(udi->pipe_out,
										 USB_FEATURE_ENDPOINT_HALT)))
	{
		PTRACE_ALWAYS(udi, "bulk_only_reset: clear_feature on pipe_out failed: %08x\n", status);
	}
	PTRACE(udi, "bulk_only_reset:%08x\n", status);
	return status;
}
/**
	\fn:bulk_only_transfer
	\param udi: corresponding device
	\param cmd: SCSI command to be performed on USB device
	\param cmdlen: length of SCSI command
	\param data_sg: io vectors array with data to transfer
	\param sglist_count: count of entries in io vector array
	\param transfer_len: overall length of data to be transferred
	\param dir: direction of data transfer
	\param ccbio: CCB_SCSIIO struct for original SCSI command
	\param cb: callback to handle of final stage of command performing (autosense \
						 request etc.)

	transfer procedure for bulk-only protocol. Performs	SCSI command on USB device
	[2]
*/
void
bulk_only_transfer(usb_device_info *udi, uint8 *cmd, uint8	cmdlen,	 //sg_buffer *sgb,
				iovec *sg_data,int32 sg_count, int32 transfer_len,
				EDirection dir,	CCB_SCSIIO *ccbio, ud_transfer_callback cb)
{
	status_t status			= B_OK;
	status_t command_status = B_OK;
	int32 residue			= transfer_len;
	usb_mass_CSW csw = {0};
	/* initialize and fill in Command Block Wrapper */
	usb_mass_CBW cbw = {
		.signature	 = CBW_SIGNATURE,
		.tag		 = atomic_add(&udi->tag, 1),
		.data_transfer_len = transfer_len,
		.flags		 = (dir == eDirIn) ? CBW_FLAGS_IN : CBW_FLAGS_OUT,
		.lun		 = ccbio->cam_ch.cam_target_lun & 0xf,
		.cdb_len	 = cmdlen,
	};
	memcpy(cbw.CDB, cmd, cbw.cdb_len);
	do{
		trace_CBW(udi, &cbw);
		/* send CBW to device */
		status = queue_bulk(udi, &cbw, CBW_LENGTH, false);
		if(status != B_OK || udi->status != B_OK){
			PTRACE_ALWAYS(udi, "bulk_only_transfer: queue_bulk failed:"
							"status:%08x usb status:%08x\n", status, udi->status);
			(*udi->protocol_m->reset)(udi);
			command_status = B_CMD_WIRE_FAILED;
			break;
		}
		/* perform data transfer if required */
		if(transfer_len != 0x0){
			status = process_data_io(udi, sg_data, sg_count, dir);
			if(status != B_OK && status != B_DEV_STALLED){
				command_status = B_CMD_WIRE_FAILED;
				break;
			}
		}
		/* get status of command */
		status = read_status(udi, &csw, transfer_len);
		if(B_OK != status){
			command_status = B_CMD_WIRE_FAILED;
			break;
		}
		residue = csw.data_residue;
		if(csw.status == CSW_STATUS_FAILED){
			command_status = B_CMD_FAILED;
		}else{
			command_status = B_OK;
		}
	}while(false);
	/* finalize transfer */
	cb(udi, ccbio, residue, command_status);
}

protocol_module_info bulk_only_protocol_m = {
	{0, 0, 0}, /* this is not a real kernel module - just interface */
	bulk_only_initialize,
	bulk_only_reset,
	bulk_only_transfer,
};

