/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI Disk Driver

	Everything doing the real input/output stuff.
*/

#include "scsi_dsk_int.h"

#define DAS_STD_TIMEOUT 10


// we don't want to inline this function - it's just not worth it
static int das_read_write( das_handle_info *handle, const phys_vecs *vecs, 
	off_t pos, size_t num_blocks, uint32 block_size, size_t *bytes_transferred, 
	bool write );


status_t das_read( das_handle_info *handle, const phys_vecs *vecs, 
	off_t pos, size_t num_blocks, uint32 block_size, size_t *bytes_transferred )
{
	return das_read_write( handle, vecs, pos, 
		num_blocks, block_size, bytes_transferred, false );
}


status_t das_write( das_handle_info *handle, const phys_vecs *vecs, 
	off_t pos, size_t num_blocks, uint32 block_size, size_t *bytes_transferred )
{
	return das_read_write( handle, vecs, pos, 
		num_blocks, block_size, bytes_transferred, true );
}


// universal read/write function
static int das_read_write( das_handle_info *handle, const phys_vecs *vecs, 
	off_t pos64, size_t num_blocks, uint32 block_size, size_t *bytes_transferred, 
	bool write )
{
	das_device_info *device = handle->device;
	scsi_ccb *request;
	err_res res;
	int retries = 0;
	int err;
	uint32 pos = pos64;

	// don't test rw10_enabled restrictions - this flag may get changed	
	request = device->scsi->alloc_ccb( device->scsi_device );
	
	if( request == NULL )
		return B_NO_MEMORY;

	do {
		size_t num_bytes;
		bool is_rw10;
		
		request->flags = write ? SCSI_DIR_OUT : SCSI_DIR_IN;

		// make sure we avoid 10 byte commands if they aren't supported
		if( !device->rw10_enabled ) {
			// restricting transfer is OK - the block manager will
			// take care of transferring the rest
			if( num_blocks > 0x100 )
				num_blocks = 0x100;
			
			// no way to break the 21 bit address limit	
			if( pos64 > 0x200000 ) {
				err = B_BAD_VALUE;
				goto abort;
			}
						
			// don't allow transfer cross the 24 bit address limit
			// (I'm not sure whether this is allowed, but this way we
			// are sure to not ask for trouble)
			num_blocks = min( num_blocks, 0x100000 - pos );
		}
			
		num_bytes = num_blocks * block_size;
		
		request->data = NULL;
		request->sg_list = vecs->vec;
		request->data_len = num_bytes;
		request->sglist_cnt = vecs->num;
		request->sort = pos;
		request->timeout = DAS_STD_TIMEOUT;
		// see whether daemon instructed us to post an ordered command;
		// reset flag after read
		request->flags = atomic_and( &device->next_tag_action, 0 );
			
		SHOW_FLOW( 3, "ordered: %s", 
			(request->flags & SCSI_ORDERED_QTAG) == 0 ? "yes" : "no" );
		
		// use 6 byte commands whenever possible
		if( pos + num_blocks < 0x200000 && num_blocks <= 0x100 ) {
			scsi_cmd_rw_6 *cmd = (scsi_cmd_rw_6 *)request->cdb;
			
			is_rw10 = false;
			
			memset( cmd, 0, sizeof( *cmd ));
			cmd->opcode = write ? SCSI_OP_WRITE_6 : SCSI_OP_READ_6;
			cmd->high_LBA = (pos >> 16) & 0x1f;
			cmd->mid_LBA = (pos >> 8) & 0xff;
			cmd->low_LBA = pos & 0xff;
			cmd->length = num_blocks;
			
			request->cdb_len = sizeof( *cmd );
		} else {
			scsi_cmd_rw_10 *cmd = (scsi_cmd_rw_10 *)request->cdb;
			
			is_rw10 = true;
			
			memset( cmd, 0, sizeof( *cmd ));
			cmd->opcode = write ? SCSI_OP_WRITE_10 : SCSI_OP_READ_10;
			cmd->RelAdr = 0;
			cmd->FUA = 0;
			cmd->DPO = 0;
			
			cmd->top_LBA = (pos >> 24) & 0xff;
			cmd->high_LBA = (pos >> 16) & 0xff;
			cmd->mid_LBA = (pos >> 8) & 0xff;
			cmd->low_LBA = pos & 0xff;
			
			cmd->high_length = (num_blocks >> 8) & 0xff;
			cmd->low_length = num_blocks & 0xff;
			
			request->cdb_len = sizeof( *cmd );
		}

		// last chance to detect errors that occured during concurrent accesses
		err = handle->pending_error;
		
		if( err ) 
			goto abort;
			
		device->scsi->scsi_io( request );
		
		acquire_sem( request->completion_sem );

		// ask generic peripheral layer what to do now	
		res = scsi_periph->check_error( device->scsi_periph_device, request );
		
		switch( res.action ) {
		case err_act_ok:
			*bytes_transferred = num_bytes - request->data_resid;
			break;
			
		case err_act_start:
			res = scsi_periph->send_start_stop( 
				device->scsi_periph_device, request, 1, device->removable );
			if( res.action == err_act_ok )
				res.action = err_act_retry;
			break;
			
		case err_act_invalid_req:
			// if this was a 10 byte command, the device probably doesn't 
			// support them, so disable them and retry
			if( is_rw10 ) {
				atomic_and( &device->rw10_enabled, 0 );
				res.action = err_act_retry;
			} else
				res.action = err_act_fail;
			break;
		}
		
	} while(
		(res.action == err_act_retry && retries++ < 3) ||
		(res.action == err_act_many_retries && retries++ < 30 ));
	
	device->scsi->free_ccb( request );
	
	// peripheral layer only created "read" error, so we have to
	// map them to "write" errors if this was a write request
	if( res.error_code == B_DEV_READ_ERROR && write )
		return B_DEV_WRITE_ERROR;
	else
		return res.error_code;	
		
abort:
	device->scsi->free_ccb( request );
		
	return err;
}



// kernel daemon
// once in a minute, it sets a flag so that the next command is executed
// ordered; this way, we avoid starvation of SCSI commands inside the
// SCSI queuing system - the ordered command waits for all previous
// commands and thus no command can starve longer then a minute
void das_sync_queue_daemon( void *arg, int iteration )
{
	das_device_info *device = (das_device_info *)arg;
	
	atomic_or( &device->next_tag_action, SCSI_ORDERED_QTAG );
}

void das_handle_set_error( das_handle_info *handle, status_t error_code )
{
	handle->pending_error = error_code;
}


status_t das_handle_get_error( das_handle_info *handle )
{
	return handle->pending_error;
}
