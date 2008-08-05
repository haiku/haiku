/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Bruno Albuquerque, bga@bug-br.org.br
 *
 * Copyright 2004-2006 yellowTAB GMbH. This file is
 * based on work I did for ZETA while employed by
 * yellowTAB and is used under permission.
 */


#include "scsi_periph_int.h"

#include <scsi_cmds.h>

#include <string.h>


err_res
periph_synchronize_cache(scsi_periph_device_info *device, scsi_ccb *request)
{
	scsi_cmd_sync_cache* cmd = (scsi_cmd_sync_cache*)request->cdb;
	
	request->flags = SCSI_DIR_NONE;
	
	request->data = NULL;
	request->sg_list = NULL;
	request->data_length = 0;
	request->timeout = device->std_timeout;
	request->sort = -1;
	
	memset(cmd, 0, sizeof(*cmd));
	
	cmd->opcode	= SCSI_OP_SYNCHRONIZE_CACHE;
	cmd->immediately = 0;
	
	// TODO: Maybe we will actually want to set this one day...
	cmd->block_count = 0;
	
	request->cdb_length = sizeof(*cmd);
	
	device->scsi->sync_io(request);

	return periph_check_error(device, request);
}
