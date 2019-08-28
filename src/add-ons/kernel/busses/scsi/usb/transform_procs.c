/*
 * Copyright 2004-2007, Haiku, Inc. All RightsReserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Siarzhuk Zharski <imker@gmx.li>
 */

/*! SCSI commands transformations support */

#include "usb_scsi.h"

#include "device_info.h"
#include "transform_procs.h"
#include "tracing.h"
#include "scsi_commands.h"
#include "settings.h"
#include "strings.h"

#include <string.h>

#define UFI_COMMAND_LEN		12
#define ATAPI_COMMAND_LEN	12


/*! Transforms a 6-byte command to 10-byte one if required. Transformed command
	is returned in rcmd parameter. In case if no transformation was performed
	the return buffer is untouched. 

	\param cmd: SCSI command buffer to be transformed 
	\param len: length of buffer, pointed by cmd parameter
	\param rcmd: a place for buffer pointer with transformed command
	\param rlen: a place for length of transformed command
	\return: true if transformation was performed
*/
static void
transform_6_to_10(uint8	*cmd, uint8	len, uint8 **rcmd, uint8 *rlen)
{
	scsi_cmd_generic_6 *from = (scsi_cmd_generic_6 *)cmd;

	*rlen = 10;
	memset(*rcmd, 0, 10);

	switch (from->opcode) {
		case READ_6:
		case WRITE_6:
		{
			scsi_cmd_rw_10 *to = (scsi_cmd_rw_10 *)(*rcmd);

			to->opcode = (from->opcode == READ_6) ? READ_10 : WRITE_10;
			to->lba = B_HOST_TO_BENDIAN_INT32(((from->addr[0] & 0x1f) << 16)
				| (from->addr[1] << 8) | from->addr[0]);
			to->lun = (from->addr[0] & CMD_LUN) >> CMD_LUN_SHIFT;
			to->control = from->ctrl;
			if (from->len == 0) {
				/* special case! in 6-byte R/W commands	*/
				/* the length 0x00 assume transfering 0x100 blocks! */
				to->length = B_HOST_TO_BENDIAN_INT16((uint16)256);
			} else
				to->length = B_HOST_TO_BENDIAN_INT16((uint16)from->len);
		}

		case MODE_SENSE_6:
		case MODE_SELECT_6:
		{
			scsi_cmd_generic_10 *to = (scsi_cmd_generic_10 *)(*rcmd);

			if (from->opcode == MODE_SENSE_6) {
				to->opcode = MODE_SENSE_10;
				((scsi_cmd_mode_sense_10 *)to)->byte3
					= ((scsi_cmd_mode_sense_6 *)from)->byte3;
			} else
				to->opcode = MODE_SELECT_10;
			to->byte2 = from->addr[0];
			to->len[1] = from->len + 4; /*TODO: hardcoded length*/
			to->ctrl = from->ctrl;
		}

		default:
			/* no transformation needed */
			break;
	}
}


/*!	Transforms a 6-byte command to 10-byte depending on information provided
	with udi object. 

	\param udi: usb_device_info object for wich transformation is requested
	\param cmd: SCSI command buffer to be transformed 
	\param len: length of buffer, pointed by cmd parameter
	\param rcmd: a place for buffer pointer with transformed command
	\param rlen: a place for length of transformed command
	\return: true if transformation was performed
*/
static bool
transform_cmd_6_to_10(usb_device_info *udi, uint8 *cmd, uint8 len,
	uint8 **rcmd, uint8 *rlen)
{
	scsi_cmd_generic_6 *from = (scsi_cmd_generic_6 *)cmd;

	switch (from->opcode) {
		case READ_6:
		case WRITE_6:
		{
			if (!HAS_FIXES(udi->properties, FIX_FORCE_RW_TO_6)) {
				transform_6_to_10(cmd, len, rcmd, rlen);
				return true;
			}
			break;
		}

		case MODE_SENSE_6:
		case MODE_SELECT_6:
		{
			if (HAS_FIXES(udi->properties, FIX_FORCE_MS_TO_10)) {
				transform_6_to_10(cmd, len, rcmd, rlen);
				return true;
			}
			break;
		}
	}

	return false;
}


/*!	Transforms a TEST_UNIT_COMAND SCSI command to START_STOP_UNIT one depending
	on properties provided with udi object. 

	\param udi: usb_device_info object for wich transformation is requested
	\param cmd: SCSI command buffer to be transformed 
	\param len: length of buffer, pointed by cmd parameter
	\param rcmd: a place for buffer pointer with transformed command
	\param rlen: a place for length of transformed command
	\return: true if transformation was performed
*/
static bool
transform_cmd_test_unit_ready(usb_device_info *udi, uint8 *cmd, uint8 len,
	uint8 **rcmd, uint8	*rlen)
{
	scsi_cmd_start_stop_unit *command = (scsi_cmd_start_stop_unit *)(*rcmd);

	if (!HAS_FIXES(udi->properties, FIX_TRANS_TEST_UNIT))
		return false;

	memset(*rcmd, 0, *rlen);
	command->opcode = START_STOP_UNIT;
	command->start_loej = CMD_SSU_START;
	*rlen = 6;

	return true;
}


//	#pragma mark -


/*!	This is the "transformation procedure" for transparent SCSI (0x06) USB
	subclass. It performs all SCSI commands transformations required by this
	protocol. Additionally it tries to make some workarounds for "broken" USB
	devices. If no transformation was performed resulting command buffer
	points to original one.

	\param udi: usb_device_info object for wich transformation is requested
	\param cmd: SCSI command buffer to be transformed 
	\param len: length of buffer, pointed by cmd parameter
	\param rcmd: a place for buffer pointer with transformed command
	\param rlen: a place for length of transformed command
	\return: B_OK if transformation was successfull, B_ERROR otherwise 
*/
static status_t
scsi_transform(usb_device_info *udi, uint8 *cmd, uint8 len, uint8 **rcmd,
	uint8 *rlen)
{
	bool transformed = false;
	scsi_cmd_generic *command = (scsi_cmd_generic *)cmd;
	TRACE_SCSI_COMMAND(cmd, len);

	switch (command->opcode) {
		case READ_6:
		case WRITE_6:
		case MODE_SENSE_6:
		case MODE_SELECT_6:
			transformed = transform_cmd_6_to_10(udi, cmd, len, rcmd, rlen);
			break;

		case TEST_UNIT_READY:
			transformed = transform_cmd_test_unit_ready(udi, cmd, len, rcmd,
				rlen);
			break;

		default:
			break;	
	}

	if (!transformed) {
		/* transformation was not required */
		*rcmd = cmd;
		*rlen = len;
	} else
		TRACE_SCSI_COMMAND_HLIGHT(*rcmd, *rlen);

	return B_OK;
}


/*!	This is the "transformation procedure" for RBC USB subclass (0x01). It
	performs all SCSI commands transformations required by this protocol.
	Additionally it tries to make some workarounds for "broken" USB devices.
	If no transformation was performed resulting command buffer points to
	original one.

	\param udi: usb_device_info object for wich transformation is requested
	\param cmd: SCSI command buffer to be transformed 
	\param len: length of buffer, pointed by cmd parameter
	\param rcmd: a place for buffer pointer with transformed command
	\param rlen: a place for length of transformed command
	\return: B_OK if transformation was successfull, B_ERROR otherwise 
*/
static status_t
rbc_transform(usb_device_info *udi, uint8 *cmd, uint8 len, uint8 **rcmd,
	uint8 *rlen)
{
	bool transformed = false;
	scsi_cmd_generic *command = (scsi_cmd_generic *)cmd;
	TRACE_SCSI_COMMAND(cmd, len);

	switch (command->opcode) {
		case TEST_UNIT_READY:
			transformed = transform_cmd_test_unit_ready(udi, cmd, len, rcmd, rlen);
			break;

		case READ_6:
		case WRITE_6: /* there are no such command in list of allowed - transform*/
			transformed = transform_cmd_6_to_10(udi, cmd, len, rcmd, rlen);
			break;	

		/* TODO: all following ones are not checked against specs !!!*/
		case FORMAT_UNIT:
		case INQUIRY: /*TODO: check !!! */
		case MODE_SELECT_6: /*TODO: check !!! */
		case MODE_SENSE_6: /*TODO: check !!! */
		case PERSISTENT_RESERVE_IN: /*TODO: check !!! */
		case PERSISTENT_RESERVE_OUT: /*TODO: check !!! */
		case PREVENT_ALLOW_MEDIA_REMOVAL: /*TODO: check !!! */
		case READ_10: /*TODO: check !!! */
		case READ_CAPACITY: /*TODO: check !!! */
		case RELEASE_6: /*TODO: check !!! */
		case REQUEST_SENSE: /*TODO: check !!! */
		case RESERVE_6: /*TODO: check !!! */
		case START_STOP_UNIT: /*TODO: check !!! */
		case SYNCHRONIZE_CACHE: /*TODO: check !!! */
		case VERIFY: /*TODO: check !!! */
		case WRITE_10: /*TODO: check !!! */
		case WRITE_BUFFER: /*TODO Check correctnes of such translation!*/
			*rcmd = cmd;		/* No need to copy */
			*rlen = len;	/*TODO: check !!! */
			break;

		default:
			TRACE_ALWAYS("An unsupported RBC command: %08x\n", command->opcode);
			return B_ERROR;
	}

	if (transformed) 
		TRACE_SCSI_COMMAND_HLIGHT(*rcmd, *rlen);

	return B_OK;
}


/*!	This is the "transformation procedure" for UFI USB subclass (0x04). It
	performs all SCSI commands transformations required by this protocol.
	Additionally it tries to make some workarounds for "broken" USB devices.
	If no transformation was performed resulting command buffer points to
	the original one.

	\param udi: usb_device_info object for wich transformation is requested
	\param cmd: SCSI command buffer to be transformed 
	\param len: length of buffer, pointed by cmd parameter
	\param rcmd: a place for buffer pointer with transformed command
	\param rlen: a place for length of transformed command
	\return: B_OK if transformation was successfull, B_ERROR otherwise 
*/
static status_t
ufi_transform(usb_device_info *udi, uint8 *cmd, uint8 len, uint8 **rcmd,
	uint8 *rlen)
{
	scsi_cmd_generic *command = (scsi_cmd_generic *)cmd;
	status_t status = B_OK;

	TRACE_SCSI_COMMAND(cmd, len);
	memset(*rcmd, 0, UFI_COMMAND_LEN);

	switch (command->opcode) {
		case READ_6:
		case WRITE_6:
		case MODE_SENSE_6:
		case MODE_SELECT_6:
			// TODO: not transform_cmd_*()?
			transform_6_to_10(cmd, len, rcmd, rlen);
			break;
		case TEST_UNIT_READY:
			if (transform_cmd_test_unit_ready(udi, cmd, len, rcmd, rlen))
				break; /* if TEST UNIT READY was transformed*/
		case FORMAT_UNIT:	/* TODO: mismatch */
		case INQUIRY:				
		case START_STOP_UNIT: 
		case MODE_SELECT_10: 
		case MODE_SENSE_10: 
		case PREVENT_ALLOW_MEDIA_REMOVAL: 
		case READ_10: 
		case READ_12: 
		case READ_CAPACITY: 
		case READ_FORMAT_CAPACITY: /* TODO: not in the SCSI-2 specs */
		case REQUEST_SENSE: 
		case REZERO_UNIT: 
		case SEEK_10: 
		case SEND_DIAGNOSTICS: /* TODO: parameter list len mismatch */
		case VERIFY: 
		case WRITE_10: 
		case WRITE_12: /* TODO: EBP. mismatch */
		case WRITE_AND_VERIFY: 
			memcpy(*rcmd, cmd, len);
			/*TODO what about control? ignored in UFI?*/
			break;
		default:
			TRACE_ALWAYS("An unsupported UFI command: %08x\n",
				command->opcode);
			status = B_ERROR;
			break;	
	}

	*rlen = UFI_COMMAND_LEN; /* override any value set in transform funcs !!!*/

	if (status == B_OK)
		TRACE_SCSI_COMMAND_HLIGHT(*rcmd, *rlen); 

	return status;
}


/*!	This is the "transformation procedure" for SFF8020I and SFF8070I
	USB subclassses (0x02 and 0x05). It performs all SCSI commands
	transformations required by this protocol. Additionally it tries to make
	some workarounds for "broken" USB devices. If no transformation was
	performed resulting command buffer points to the original one.

	\param udi: usb_device_info object for wich transformation is requested
	\param cmd: SCSI command buffer to be transformed 
	\param len: length of buffer, pointed by cmd parameter
	\param rcmd: a place for buffer pointer with transformed command
	\param rlen: a place for length of transformed command
	\return: B_OK if transformation was successfull, B_ERROR otherwise 
*/
static status_t
atapi_transform(usb_device_info *udi, uint8 *cmd, uint8 len, uint8 **rcmd,
	uint8 *rlen)
{
	scsi_cmd_generic *command = (scsi_cmd_generic *)cmd;
	status_t status = B_OK;

	TRACE_SCSI_COMMAND(cmd, len);
	memset(*rcmd, 0, ATAPI_COMMAND_LEN);

	switch (command->opcode) {
		case READ_6:
		case WRITE_6:
		case MODE_SENSE_6:
		case MODE_SELECT_6:
			// TODO: not transform_cmd_*()?
			transform_6_to_10(cmd, len, rcmd, rlen);
			break;
		case TEST_UNIT_READY:
			if (transform_cmd_test_unit_ready(udi, cmd, len, rcmd, rlen))
				break;
		case FORMAT_UNIT: 
		case INQUIRY:		 
		case MODE_SELECT_10: 
		case MODE_SENSE_10: 
		case PREVENT_ALLOW_MEDIA_REMOVAL: 
		case READ_10: 
		case READ_12: /* mismatch in byte 1 */
		case READ_CAPACITY: /* mismatch. no transf len defined... */
		case READ_FORMAT_CAPACITY: /* TODO: check!!! */
		case REQUEST_SENSE: 
		case SEEK_10: 
		case START_STOP_UNIT: 
		case VERIFY: /* mismatch DPO */
		case WRITE_10: /* mismatch in byte 1 */
		case WRITE_12: /* mismatch in byte 1 */
		case WRITE_AND_VERIFY: /* mismatch byte 1 */
		case PAUSE_RESUME:
		case PLAY_AUDIO:
		case PLAY_AUDIO_MSF:
		case REWIND:
		case PLAY_AUDIO_TRACK:
		/* are in FreeBSD driver but no in 8070/8020 specs ...	 
		//case REZERO_UNIT: 
		//case SEND_DIAGNOSTIC: 
		//case POSITION_TO_ELEMENT:	*/
		case GET_CONFIGURATION:
		case SYNCHRONIZE_CACHE: 
		case READ_BUFFER: 
	 	case READ_SUBCHANNEL: 
		case READ_TOC: /* some mismatch */
		case READ_HEADER:
		case READ_DISK_INFO:
		case READ_TRACK_INFO:
		case SEND_OPC:
		case READ_MASTER_CUE:
		case CLOSE_TR_SESSION:
		case READ_BUFFER_CAP:
		case SEND_CUE_SHEET:
		case BLANK:
		case EXCHANGE_MEDIUM:
		case READ_DVD_STRUCTURE:
		case SET_CD_SPEED:
		case DVD_REPORT_KEY:
		case DVD_SEND_KEY:
		//case 0xe5: /* READ_TRACK_INFO_PHILIPS *//* TODO: check!!! */
			memcpy(*rcmd, cmd, len); /* TODO: check!!! */
			break;

		default:
			TRACE_ALWAYS("An unsupported (?) ATAPI command: %08x\n",
				command->opcode);
			status = B_ERROR;
			break;	
	}

	*rlen = ATAPI_COMMAND_LEN;

	if (status == B_OK)
		TRACE_SCSI_COMMAND_HLIGHT(*rcmd, *rlen); 

	return status;
}


/*!	This is the "transformation procedure" for QIC157 USB subclass (0x03). It
	performs all SCSI commands transformations required by this protocol.
	Additionally it tries to make some workarounds for "broken" USB devices.
	If no transformation was performed the resulting command buffer points to
	the original one.

	\param udi: usb_device_info object for wich transformation is requested
	\param cmd: SCSI command buffer to be transformed 
	\param len: length of buffer, pointed by cmd parameter
	\param rcmd: a place for buffer pointer with transformed command
	\param rlen: a place for length of transformed command
	\return: B_OK if transformation was successfull, B_ERROR otherwise 
*/
static status_t
qic157_transform(usb_device_info *udi, uint8 *cmd, uint8 len, uint8 **rcmd,
	uint8 *rlen)
{
	scsi_cmd_generic *command = (scsi_cmd_generic *)cmd;
	status_t status = B_OK;

	TRACE_SCSI_COMMAND(cmd, len);
	*rlen = ATAPI_COMMAND_LEN;
	memset(*rcmd, 0, *rlen);

	switch (command->opcode) {
		case READ_6:
		case WRITE_6:
		case MODE_SENSE_6:
		case MODE_SELECT_6:
			// TODO: not transform_cmd_*()?
			transform_6_to_10(cmd, len, rcmd, rlen);
			break;
		case TEST_UNIT_READY:
			if (transform_cmd_test_unit_ready(udi, cmd, len, rcmd, rlen))
				break; // if TEST UNIT READY was transformed
		case ERASE: /*TODO: check !!! */
		case INQUIRY: /*TODO: check !!! */
		case LOAD_UNLOAD: /*TODO: check !!! */
		case LOCATE: /*TODO: check !!! */
		case LOG_SELECT: /*TODO: check !!! */
		case LOG_SENSE: /*TODO: check !!! */
		case READ_POSITION: /*TODO: check !!! */
		case REQUEST_SENSE: /*TODO: check !!! */
		case REWIND: /*TODO: check !!! */
		case SPACE: /*TODO: check !!! */
		case WRITE_FILEMARK: /*TODO: check !!! */
			*rcmd = cmd; /*TODO: check !!! */ 
			*rlen = len; 
			break;
		default:
			TRACE_ALWAYS("An unsupported QIC-157 command: %08x\n",
				command->opcode);
			status = B_ERROR;
			break;	
	}

	if (status == B_OK)
		TRACE_SCSI_COMMAND_HLIGHT(*rcmd, *rlen); 

	return status;
}


transform_module_info scsi_transform_m = {
	{0, 0, 0}, /* this is not a real kernel module - just interface */
	scsi_transform,
};

transform_module_info rbc_transform_m = {
	{0, 0, 0}, /* this is not a real kernel module - just interface */
	rbc_transform,
};

transform_module_info ufi_transform_m = {
	{0, 0, 0}, /* this is not a real kernel module - just interface */
	ufi_transform,
};

transform_module_info atapi_transform_m = {
	{0, 0, 0}, /* this is not a real kernel module - just interface */
	atapi_transform,
};

transform_module_info qic157_transform_m = {
	{0, 0, 0}, /* this is not a real kernel module - just interface */
	qic157_transform,
};
