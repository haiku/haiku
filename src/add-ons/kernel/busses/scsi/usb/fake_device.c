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
#include "usb_scsi.h"

#include <strings.h>
#include "device_info.h"
#include "tracing.h"

#include "fake_device.h"

/* duplication! Hope this fake file will be not required in Haiku version*/
#define INQ_VENDOR_LEN		0x08
#define INQ_PRODUCT_LEN		0x10
#define INQ_REVISION_LEN	0x04

void fake_inquiry_request(usb_device_info *udi, CCB_SCSIIO *ccbio)
{
	uint8 *data = ccbio->cam_data_ptr;
	if(ccbio->cam_ch.cam_flags & CAM_SCATTER_VALID){
		TRACE_ALWAYS("fake_inquiry: problems!!! scatter gatter ....=-(\n");
	} else {
		memset(data, 0, ccbio->cam_dxfer_len);
		/* data[0] = 0x1F;*/ /* we can play here with type of device */
		data[1] = 0x80;
		data[2] = 0x02;
		data[3] = 0x02;
		data[4] = (0 != udi) ? 5 : 31; /* udi != 0 - mean FIX_NO_INQUIRY */
		if(ccbio->cam_dxfer_len >= 0x24){
			strncpy(&data[8],  "USB SCSI", INQ_VENDOR_LEN);
			strncpy(&data[16], "Reserved", INQ_PRODUCT_LEN);
			strncpy(&data[32], "N/A",      INQ_REVISION_LEN);
		}
	}
}

void fake_test_unit_ready_request(CCB_SCSIIO *ccbio)
{
	if(ccbio->cam_sense_ptr != NULL){
		scsi_sense_data *sense_data = (scsi_sense_data *)ccbio->cam_sense_ptr;
		memset(sense_data, 0, ccbio->cam_sense_len);
		sense_data->error_code 	 = SSD_CURRENT_ERROR;
		sense_data->flags		 = SSD_KEY_NOT_READY;
		ccbio->cam_ch.cam_status = CAM_REQ_CMP_ERR | CAM_AUTOSNS_VALID;
		ccbio->cam_scsi_status	 = SCSI_STATUS_CHECK_CONDITION;
	} else {
		ccbio->cam_ch.cam_status = CAM_REQ_CMP_ERR;
		ccbio->cam_scsi_status	 = SCSI_STATUS_OK;
	}
}

/**
	\fn:fake_scsi_io
	\param ccbio: ????
	\return: ???

	xpt_scsi_io - handle XPT_SCSI_IO sim action
*/
status_t fake_scsi_io(CCB_SCSIIO *ccbio)
{
	status_t status = B_BAD_VALUE;
	uint8 *cmd;
	scsi_cmd_generic *command;
	if(ccbio->cam_ch.cam_flags & CAM_CDB_POINTER){
		cmd = ccbio->cam_cdb_io.cam_cdb_ptr;
	}else{
		cmd = ccbio->cam_cdb_io.cam_cdb_bytes;
	}
	command = (scsi_cmd_generic *)cmd;
	switch(command->opcode){
		case TEST_UNIT_READY:{
			fake_test_unit_ready_request(ccbio);
			status = B_OK;
		}break;
		case INQUIRY:{
			fake_inquiry_request(NULL, ccbio);
			ccbio->cam_ch.cam_status = CAM_REQ_CMP;
			status = B_OK;
		}break;
		default:
			ccbio->cam_ch.cam_status = CAM_REQ_INVALID;
		break;
	}
	return status;
}

