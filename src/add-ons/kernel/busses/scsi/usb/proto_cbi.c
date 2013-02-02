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
/* References:
 * USB Mass Storage Class specifications:
 * http://www.usb.org/developers/data/devclass/usbmassover_11.pdf
 * http://www.usb.org/developers/data/devclass/usbmass-ufi10.pdf
 * http://www.usb.org/developers/data/devclass/usbmass-cbi10.pdf
 */

#include <string.h>

#include "usb_scsi.h"

#include "device_info.h"

#include "proto_module.h"
#include "proto_common.h"
#include "proto_cbi.h"

#include "usb_defs.h"

#define USB_REQ_CBI_ADSC 0x00


typedef struct _usb_mass_CBI_CB{
	uint8 op;
	uint8 op2;
	uint8 padding[14];
} usb_mass_CBI_CB;

#define CDB_LEN 16
#define CDB_RESET_LEN 12

//typedef uint8 usb_mass_CDB[CDB_LEN];

typedef union _usb_mass_CBI_IDB{
	struct {
		uint8 type;
		uint8 value;
	} common;
	struct {
		uint8 asc;
		uint8 ascq;
	} ufi;
} usb_mass_CBI_IDB;

#define CBI_IDB_TYPE_CCI			0x00
#define CBI_IDB_VALUE_PASS			0x00
#define CBI_IDB_VALUE_FAIL			0x01
#define CBI_IDB_VALUE_PHASE			0x02
#define CBI_IDB_VALUE_PERSISTENT	0x03
#define CBI_IDB_VALUE_STATUS_MASK	0x03

static void trace_CDB(usb_device_info *udi, const usb_mass_CBI_CB *cb, int len);
static status_t cbi_reset(usb_device_info *udi);
static status_t cbi_initialize(usb_device_info *udi);
static void cbi_transfer(usb_device_info *udi, uint8 *cmd, uint8 cmdlen, //sg_buffer *sgb,
						iovec *sg_data, int32 sg_count,	 int32	transfer_len,
						EDirection	dir, CCB_SCSIIO *ccbio,	ud_transfer_callback cb);

/** returns size of private protocol buffer */
//int cbi_buffer_length(){ return sizeof(usb_mass_CBI_CB) + sizeof(usb_mass_CBI_IDB); }
/** casts private protocol buffer to CBI_IDB */
/*#define IDB_BUFFER(__buff)\
							 ((usb_mass_CBI_IDB*)(((uint8*)__buff) + sizeof(usb_mass_CBI_CB)))
*/
/** casts private protocol buffer to CBI_CB */
/*#define CB_BUFFER(__buff)\
							 ((usb_mass_CBI_CB*)(__buff))
*/
void trace_CDB(usb_device_info *udi, const usb_mass_CBI_CB *cb, int len)
{
	PTRACE(udi, "CB:{op:0x%02x; op2:0x%02x;}\n", cb->op, cb->op2);
	udi->trace_bytes(" padding:", cb->padding, len - 2);
}

static status_t
send_request_adsc(usb_device_info *udi,	void *cb, int length)
{
	status_t status = B_ERROR;
	size_t len = 0;
	trace_CDB(udi, cb, length);
	status = (*udi->usb_m->send_request)(udi->device,
						USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT,
						USB_REQ_CBI_ADSC, 0,
						udi->interface, length,
						cb, &len);
	return status;
}

static status_t
request_interrupt(usb_device_info *udi, usb_mass_CBI_IDB *idb)
{
	status_t status = B_ERROR;
	status = (*udi->usb_m->queue_interrupt)(udi->pipe_intr, idb,
					 sizeof(usb_mass_CBI_IDB), bulk_callback, udi);
	if(status != B_OK){
		PTRACE(udi, "request_interrupt:failed:%08x\n", status);
	} else {
		status = acquire_sem_etc(udi->trans_sem, 1, B_RELATIVE_TIMEOUT, udi->trans_timeout/*LOCK_TIMEOUT*/);
		if(status != B_OK){
			PTRACE(udi, "request_interrupt:acquire_sem_etc failed:%08x\n", status);
			(*udi->usb_m->cancel_queued_transfers)(udi->pipe_intr);
		}
	}
	udi->trace_bytes("Intr status:", (uint8*)idb, sizeof(usb_mass_CBI_IDB));
	return status;
}

static status_t
parse_status(usb_device_info *udi, usb_mass_CBI_IDB *idb)
{
	status_t command_status = B_OK;
	if(CMDSET(udi->properties) == CMDSET_UFI){
		if(idb->ufi.asc == 0 && idb->ufi.ascq == 0){
			command_status = B_OK;
		}else{
			command_status = B_CMD_FAILED;
		}
	} else {
		if(idb->common.type == CBI_IDB_TYPE_CCI){
			switch(idb->common.value & CBI_IDB_VALUE_STATUS_MASK){
			case CBI_IDB_VALUE_PASS:
				command_status = B_OK;
				break;
			case CBI_IDB_VALUE_FAIL:
			case CBI_IDB_VALUE_PERSISTENT:
				command_status = B_CMD_FAILED;
				break;
			case CBI_IDB_VALUE_PHASE:
			default:
				command_status = B_CMD_WIRE_FAILED;
				break;
			}
		} /* else ?? */
	}
	return command_status;
}

/*================= "standard" protocol procedures ==============================*/

/**
	\fn:
	\param udi: ???
	\return:??

	??
*/
status_t
cbi_reset(usb_device_info *udi)
{
	status_t status = B_ERROR;
	//usb_mass_CBI_CB *cb = CB_BUFFER(udi->proto_buf);
	//usb_mass_CDB cdb = {0x1d, 0x04, 0x00};
	//memset(cdb + 2, 0xff, CDB_RESET_LEN - 2);
	usb_mass_CBI_CB cb = {
		.op	= 0x1D,
		.op2 = 0x04,
	};
	memset(cb.padding , 0xff, sizeof(cb.padding));
	status = send_request_adsc(udi, &cb, CDB_RESET_LEN);
	if(status != B_OK)
		PTRACE_ALWAYS(udi, "command_block_reset: reset request failed: %08x\n", status);

	if(B_OK != (status = (*udi->usb_m->clear_feature)(udi->pipe_in,
										 USB_FEATURE_ENDPOINT_HALT)))
		PTRACE_ALWAYS(udi, "command_block_reset: clear_feature on pipe_in failed: %08x\n", status);

	if(B_OK != (status = (*udi->usb_m->clear_feature)(udi->pipe_out,
										 USB_FEATURE_ENDPOINT_HALT)))
		PTRACE_ALWAYS(udi, "command_block_reset: clear_feature on pipe_out failed: %08x\n", status);
	PTRACE(udi, "command_block_reset:%08x\n", status);

	return status;
}

/**
	\fn:
*/
status_t
cbi_initialize(usb_device_info *udi)
{
	status_t status = B_OK;
	udi->max_lun = 0;
	return status;
}

/**
	\fn:bulk_only_transfer
	\param udi: ???
	\param cmd: ???
	\param cmdlen: ???
	\return:???

	???
*/
void
cbi_transfer(usb_device_info *udi, uint8 *cmd, uint8	cmdlen,
											 //sg_buffer *sgb,
			iovec *sg_data, int32 sg_count, int32	transfer_len,
			EDirection	dir,CCB_SCSIIO *ccbio, ud_transfer_callback cb)
{
	status_t status = B_OK;
	status_t command_status = B_OK;
	int32 residue = transfer_len;
	usb_mass_CBI_IDB idb = {{0}};
	do{
		status = send_request_adsc(udi, cmd, cmdlen);
		if(status != B_OK){
			PTRACE(udi, "cbi_transfer:send command block failed: %08x\n", status);
			if(status == B_DEV_STALLED){
				command_status = B_CMD_FAILED;
			} else {
				command_status = B_CMD_WIRE_FAILED;
				(*udi->protocol_m->reset)(udi);
			}
			break;
		}
		if(transfer_len != 0){
			status = process_data_io(udi, sg_data, sg_count, dir);
			if(status != B_OK){
				command_status = B_CMD_WIRE_FAILED;
				break;
			}
		}

		if(PROTO(udi->properties) == PROTO_CBI){
			status = request_interrupt(udi, &idb);
			PTRACE(udi, "cbi_transfer:request interrupt: %08x(%08x)\n", status, udi->status);
			if(status != B_OK){
				command_status = B_CMD_WIRE_FAILED;
				break;
			}
			if(udi->status == B_DEV_STALLED){
			}
			command_status = parse_status(udi, &idb);
		} else { /* PROP_CB ???*/
			command_status = B_CMD_UNKNOWN;
		}
		residue = 0; /* DEBUG!!!!!!!!!*/
	}while(false);
	cb(udi, ccbio, residue, command_status);
}

protocol_module_info cbi_protocol_m = {
	{0, 0, 0}, /* this is not a real kernel module - just interface */
	cbi_initialize,
	cbi_reset,
	cbi_transfer,
};

