/**
 *
 * TODO: description
 * 
 * This file is a part of USB SCSI CAM for Haiku OS.
 * May be used under terms of the MIT License
 *
 * Author(s):
 * 	Siarzhuk Zharski <imker@gmx.li>
 * 	
 * 	
 */
#include <string.h>

#include "usb_scsi.h"
 
#include "device_info.h" 
#include "proto_common.h" 
#include "tracing.h" 
#include "usb_defs.h" 
#include "scsi_commands.h" 

/**
	\fn:bulk_callback
	\param cookie:???
	\param status:???
	\param data:???
	\param actual_len:???
	\return:???
	
	???
*/
void bulk_callback(void	*cookie, status_t status, void* data, uint32 actual_len)
{
	TRACE_BULK_CALLBACK(status, actual_len);
	if(cookie){
		usb_device_info *udi = (usb_device_info *)cookie;
		udi->status = status;
		udi->data = data;
		udi->actual_len = actual_len;
		if(udi->status != B_CANCELED)
			release_sem(udi->trans_sem);
	}		
}

/**
	\fn:exec_io
	\param udi: ???
	\return:???
	
	???
*/
status_t process_data_io(usb_device_info *udi, //sg_buffer *sgb,
				 iovec *sg_data, int32 sg_count, EDirection dir)
{
	status_t status = B_OK;
	usb_pipe pipe = (dir == eDirIn) ? udi->pipe_in : udi->pipe_out;
//	TRACE_DATA_IO_SG(data_sg, sglist_count);
	status = (*udi->usb_m->queue_bulk_v)(pipe, sg_data, sg_count, bulk_callback, udi);
	if(status == B_OK){
		status = acquire_sem_etc(udi->trans_sem, 1, B_RELATIVE_TIMEOUT, udi->trans_timeout/*LOCK_TIMEOUT*/);
		if(status == B_OK){
			status = udi->status;
			if(udi->status == B_DEV_STALLED){
				status_t st=(*udi->usb_m->clear_feature)(pipe, USB_FEATURE_ENDPOINT_HALT);
				TRACE_ALWAYS("clear_on_STALL:%08x\n",st);
			}		 
		}else{
			TRACE_ALWAYS("process_data_io:acquire_sem failed:%08x\n", status); 
			(*udi->usb_m->cancel_queued_transfers)(pipe);
		} 
	} else {
		TRACE_ALWAYS("process_data_io:queue_bulk_v failed:%08x\n", status);
	}
	TRACE_DATA_IO("process_data_io:processed:%d;status:%08x\n", udi->actual_len, status);	
	return status;
}

void transfer_callback(struct _usb_device_info *udi, CCB_SCSIIO *ccbio,
				int32 residue, status_t status)
{
	ccbio->cam_resid = residue;
	switch(status){
	case B_OK:
		ccbio->cam_ch.cam_status = CAM_REQ_CMP;
		break;
	case B_CMD_FAILED:
	case B_CMD_UNKNOWN:{
			size_t sense_data_len = (0 != ccbio->cam_sense_len) ?
					ccbio->cam_sense_len : SSD_MIN_SIZE;
			uchar *sense_data_ptr = (NULL != ccbio->cam_sense_ptr) ?
					ccbio->cam_sense_ptr : (uchar*)&udi->autosense_data;
			uint8 lun = ((ccbio->cam_ch.cam_target_lun) << CMD_LUN_SHIFT) & CMD_LUN;
			scsi_cmd_generic_6 cmd = { REQUEST_SENSE, {lun, 0}, sense_data_len, 0};
			/* transform command as required by protocol */
			uint8 *rcmd		= udi->scsi_command_buf;
			uint8 rcmdlen	= sizeof(udi->scsi_command_buf);
			iovec sense_sg = { sense_data_ptr, sense_data_len };
			TRACE("transfer_callback:requesting sense information "
									"due status:%08x\n", status);
			memset(&udi->autosense_data, 0, SSD_FULL_SIZE); /* just to be sure */
			if(B_OK != (*udi->transform_m->transform)(udi, (uint8 *)&cmd, sizeof(cmd), &rcmd, &rcmdlen)){
				TRACE_ALWAYS("transfer_callback: REQUEST SENSE command transform failed\n");
				ccbio->cam_ch.cam_status = CAM_IDE; //?????????
				break;
			}
		/* transfer command to device. SCSI status will be handled in callback */
			(*udi->protocol_m->transfer)(udi, rcmd, rcmdlen, &sense_sg, 1, sense_data_len,
									 eDirIn, ccbio, sense_callback);
		}
		break;
	case B_CMD_WIRE_FAILED:
		ccbio->cam_ch.cam_status = CAM_REQ_CMP_ERR;
		break;
	default:
		TRACE_ALWAYS("transfer_callback:unknown status:%08x\n", status);
		ccbio->cam_ch.cam_status = CAM_IDE;
		break;
	}
}

void sense_callback(struct _usb_device_info *udi, CCB_SCSIIO *ccbio,
				 int32 residue, status_t status)
{
	ccbio->cam_sense_resid = residue;
	switch(status){
	case B_CMD_UNKNOWN:	
	case B_CMD_FAILED:
	case B_OK:{
			bool b_own_data = (ccbio->cam_sense_ptr == NULL);
			scsi_sense_data *sense_data = b_own_data ?
				&udi->autosense_data : (scsi_sense_data *)ccbio->cam_sense_ptr;
			int data_len = (ccbio->cam_sense_len != 0) ? ccbio->cam_sense_len : SSD_MIN_SIZE;
			TRACE_SENSE_DATA((uint8*)sense_data, data_len);
			if((sense_data->flags & SSD_KEY) == SSD_KEY_NO_SENSE){
				/* no problems. normal case for CB handling */
				TRACE("sense_callback: key OK\n");
				ccbio->cam_ch.cam_status = CAM_REQ_CMP;
			} else {
				if(!b_own_data){ /* we have used CCBIO provided buffer for sense data */
					TRACE("sense_callback:sense info OK????:%08x \n", sense_data);
					ccbio->cam_ch.cam_status = CAM_REQ_CMP_ERR | CAM_AUTOSNS_VALID;
					ccbio->cam_scsi_status = SCSI_STATUS_CHECK_CONDITION;
				} else {
					/*TODO: ?????????????????????????????????????? */
					ccbio->cam_ch.cam_status = CAM_REQ_CMP;
	 //			 ccbio->cam_ch.cam_status = CAM_REQ_CMP_ERR /*| CAM_AUTOSNS_VALID*/;
	 //			 ccbio->cam_scsi_status = SCSI_STATUS_CHECK_CONDITION;
					TRACE("sense_callback: sense still not handled...\n");
				}	
			}
		}
		break;
	default:
		TRACE_ALWAYS("sense_callback:unknown status:%08x\n", status);
	case B_CMD_WIRE_FAILED:
		ccbio->cam_ch.cam_status = CAM_AUTOSENSE_FAIL;
		break;
	}
}

