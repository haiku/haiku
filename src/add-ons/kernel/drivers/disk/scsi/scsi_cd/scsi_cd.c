/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI CD-ROM driver

	Main file.
*/

#include "scsi_cd_int.h"
#include <string.h>
#include <scsi.h>
#include <malloc.h>

extern blkdev_interface cd_interface;

scsi_periph_interface *scsi_periph;
device_manager_info *pnp;
blkman_for_driver_interface *blkman;


#define SCSI_CD_STD_TIMEOUT 10

static status_t cd_read( cd_handle_info *handle, const phys_vecs *vecs, 
	off_t pos, size_t num_blocks, uint32 block_size, size_t *bytes_transferred )
{
	return scsi_periph->read( handle->scsi_periph_handle, vecs, pos, 
		num_blocks, block_size, bytes_transferred, 10 );
}


static status_t cd_write( cd_handle_info *handle, const phys_vecs *vecs, 
	off_t pos, size_t num_blocks, uint32 block_size, size_t *bytes_transferred )
{
	return scsi_periph->write( handle->scsi_periph_handle, vecs, pos, 
		num_blocks, block_size, bytes_transferred, 10 );
}


static status_t update_capacity( cd_device_info *device )
{
	scsi_ccb *ccb;
	status_t res;
	
	SHOW_FLOW0( 3, "" );
	
	ccb = device->scsi->alloc_ccb( device->scsi_device );
	if( ccb == NULL )
		return B_NO_MEMORY;
		
	res = scsi_periph->check_capacity( device->scsi_periph_device, ccb );
	
	device->scsi->free_ccb( ccb );

	return res;	
}


static status_t get_geometry( cd_handle_info *handle, void *buf, size_t len )
{
	cd_device_info *device = handle->device;
	device_geometry *geometry = (device_geometry *)buf;
	status_t res;
	
	SHOW_FLOW0( 3, "" );

	res = update_capacity( device );

	// it seems that Be expects B_GET_GEOMETRY to always succeed unless
	// the medium has been changed; e.g. if we report B_DEV_NO_MEDIA, the
	// device is ignored by the CDPlayer and CDBurner
	if( res == B_DEV_MEDIA_CHANGED )
		return res;
		
	geometry->bytes_per_sector = device->block_size;
	geometry->sectors_per_track = 1;
	geometry->cylinder_count = device->capacity;
	geometry->head_count = 1;
	geometry->device_type = device->device_type;
	geometry->removable = device->removable;
	
	// TBD: for all but CD-ROMs, read mode sense - medium type
	// (bit 7 of block device specific parameter for Optical Memory Block Device)
	// (same for Direct-Access Block Devices)
	// (same for write-once block devices)
	// (same for optical memory block devices)
	geometry->read_only = true;
	geometry->write_once = device->device_type == scsi_dev_WORM;
	
	SHOW_FLOW( 3, "%ld, %ld, %ld, %ld, %d, %d, %d, %d",
		geometry->bytes_per_sector,
		geometry->sectors_per_track,
		geometry->cylinder_count,
		geometry->head_count,
		geometry->device_type,
		geometry->removable,
		geometry->read_only,
		geometry->write_once );
	
	SHOW_FLOW0( 3, "done" );
	
	return B_OK;
}

static status_t get_toc( cd_device_info *device, scsi_toc *toc )
{
	scsi_ccb *ccb;
	status_t res;
	scsi_cmd_read_toc *cmd;
	size_t data_len;
	scsi_toc_general *short_response = (scsi_toc_general *)toc->toc_data;
	
	SHOW_FLOW0( 0, "" );
		
	ccb = device->scsi->alloc_ccb( device->scsi_device );
	
	if( ccb == NULL )
		return B_NO_MEMORY;

	// first read number of tracks only		
	ccb->flags = SCSI_DIR_IN;
	
	cmd = (scsi_cmd_read_toc *)ccb->cdb;
	
	memset( cmd, 0, sizeof( *cmd ));
	cmd->opcode = SCSI_OP_READ_TOC;
	cmd->TIME = 1;
	cmd->format = SCSI_TOC_FORMAT_TOC;
	cmd->track = 1;
	cmd->high_allocation_length = 0;
	cmd->low_allocation_length = sizeof( scsi_toc_general );
	
	ccb->cdb_len = sizeof( *cmd );

	ccb->sort = -1;
	ccb->timeout = SCSI_CD_STD_TIMEOUT;
	
	ccb->data = toc->toc_data;
	ccb->sg_list = NULL;
	ccb->data_len = sizeof( toc->toc_data );

	res = scsi_periph->safe_exec( device->scsi_periph_device, ccb );	
	if( res != B_OK )
		goto err;
		
	SHOW_FLOW( 0, "tracks: %d - %d", short_response->first, short_response->last );

	// then read all track infos
	// (little hint: number of tracks is last - first + 1; 
	//  but scsi_toc_toc has already one track, so we get 
	//  last - first extra tracks; finally, we want the lead-out as
	//  well, so we add an extra track)
	data_len = (short_response->last - short_response->first + 1) 
		* sizeof( scsi_toc_track ) + sizeof( scsi_toc_toc );
	data_len = min( data_len, sizeof( toc->toc_data ));
	
	SHOW_FLOW( 0, "data_len: %d", (int)data_len );
	
	cmd->high_allocation_length = data_len >> 8;
	cmd->low_allocation_length = data_len & 0xff;
	
	res = scsi_periph->safe_exec( device->scsi_periph_device, ccb );

err:	
	device->scsi->free_ccb( ccb );
	
	return res;
}

static status_t load_eject( cd_device_info *device, bool load )
{
	scsi_ccb *ccb;
	err_res res;
	
	SHOW_FLOW0( 0, "" );
		
	ccb = device->scsi->alloc_ccb( device->scsi_device );

	res = scsi_periph->send_start_stop( device->scsi_periph_device, 
		ccb, load, true );
		
	device->scsi->free_ccb( ccb );
	
	return res.error_code;
}

static status_t get_position( cd_device_info *device, scsi_position *position )
{
	scsi_cmd_read_subchannel cmd;
	
	SHOW_FLOW0( 3, "" );
	
	memset( &cmd, 0, sizeof( cmd ));
	cmd.opcode = SCSI_OP_READ_SUB_CHANNEL;
	cmd.TIME = 1;
	cmd.SUBQ = 1;
	cmd.parameter_list = scsi_sub_channel_parameter_list_cd_pos;
	cmd.track = 0;
	cmd.high_allocation_length = sizeof( *position ) >> 8;
	cmd.low_allocation_length = sizeof( *position ) & 0xff;
	
	return scsi_periph->simple_exec( device->scsi_periph_device,
		&cmd, sizeof( cmd ), 
		position, sizeof( *position ), SCSI_DIR_IN );
}

static status_t get_set_volume( cd_device_info *device, scsi_volume *volume, bool set )
{
	scsi_cmd_mode_sense_6 cmd;
	scsi_mode_param_header_6 header;
	size_t len;
	void *buffer;
	scsi_modepage_audio	*page;
	status_t res;
	
	SHOW_FLOW0( 3, "" );
	
	// determine size of block descriptor
	memset( &cmd, 0, sizeof( cmd ));
	cmd.opcode = SCSI_OP_MODE_SENSE_6;
	cmd.page_code = SCSI_MODEPAGE_AUDIO;
	cmd.PC = SCSI_MODE_SENSE_PC_CURRENT;
	cmd.allocation_length = sizeof( header );
	
	memset( &header, -2, sizeof( header ));
	
	res = scsi_periph->simple_exec( device->scsi_periph_device, &cmd, sizeof( cmd ),
		&header, sizeof( header ), SCSI_DIR_IN );
	if( res != B_OK )
		return res;
		
	SHOW_FLOW( 0, "block_desc_len=%d", header.block_desc_len );
	return B_ERROR;
	
	// retrieve param header, block descriptor and actual codepage
	len = sizeof( header ) + header.block_desc_len + 
		sizeof( scsi_modepage_audio );
		
	buffer = malloc( len );
	if( buffer == NULL )
		return B_NO_MEMORY;
		
	memset( buffer, -1, sizeof( buffer ));
		
	cmd.allocation_length = len;
	
	res = scsi_periph->simple_exec( device->scsi_periph_device, &cmd, sizeof( cmd ),
		buffer, len, SCSI_DIR_IN );
	if( res != B_OK ) {
		free( buffer );
		return res;
	}
	
	SHOW_FLOW( 3, "mode_data_len=%d, block_desc_len=%d", 
		((scsi_mode_param_header_6 *)buffer)->mode_data_len,
		((scsi_mode_param_header_6 *)buffer)->block_desc_len );

	// find control page and retrieve values
	page = (scsi_modepage_audio *)((char *)buffer + sizeof( header ) + header.block_desc_len);

	SHOW_FLOW( 0, "page=%p, codepage=%d", page, page->header.page_code );

	if( !set ) {
		volume->port0_channel = page->ports[0].channel;
		volume->port0_volume  = page->ports[0].volume;
		volume->port1_channel = page->ports[1].channel;
		volume->port1_volume  = page->ports[1].volume;
		volume->port2_channel = page->ports[2].channel;
		volume->port2_volume  = page->ports[2].volume;
		volume->port3_channel = page->ports[3].channel;
		volume->port3_volume  = page->ports[3].volume;
		
		SHOW_FLOW( 3, "1: %d - %d", volume->port0_channel, volume->port0_volume );
		SHOW_FLOW( 3, "2: %d - %d", volume->port1_channel, volume->port1_volume );
		SHOW_FLOW( 3, "3: %d - %d", volume->port2_channel, volume->port2_volume );
		SHOW_FLOW( 3, "4: %d - %d", volume->port3_channel, volume->port3_volume );
		
		res = B_OK;
		
	} else {
		scsi_cmd_mode_select_6 cmd;
		
		if( volume->flags & 1 )
			page->ports[0].channel = volume->port0_channel;
		if( volume->flags & 2 )
			page->ports[0].volume = volume->port0_volume;
		if( volume->flags & 4 )
			page->ports[1].channel = volume->port1_channel;
		if( volume->flags & 8 )
			page->ports[1].volume = volume->port1_volume;
		if( volume->flags & 0x10 )
			page->ports[2].channel = volume->port2_channel;
		if( volume->flags & 0x20 )
			page->ports[2].volume = volume->port2_volume;
		if( volume->flags & 0x40 )
			page->ports[3].channel = volume->port3_channel;
		if( volume->flags & 0x80 )
			page->ports[3].volume = volume->port3_volume;
		
		memset( &cmd, 0, sizeof( cmd ));
		cmd.opcode = SCSI_OP_MODE_SELECT_6;
		cmd.PF = 1;
		cmd.param_list_length = sizeof( header ) + header.block_desc_len 
			+ sizeof( *page );
		
		res = scsi_periph->simple_exec( device->scsi_periph_device, 
			&cmd, sizeof( cmd ),
			buffer, len, SCSI_DIR_OUT );
	}

	free( buffer );
	return res;
}


// play audio cd; time is in MSF
static status_t play_msf( cd_device_info *device, scsi_play_position *buf )
{
	scsi_cmd_play_msf cmd;
	
	SHOW_FLOW( 0, "%d:%d:%d-%d:%d:%d", 
		buf->start_m, buf->start_s, buf->start_f,
		buf->end_m, buf->end_s, buf->end_f  );
	
	memset( &cmd, 0, sizeof( cmd ));
	
	cmd.opcode = SCSI_OP_PLAY_MSF;
	cmd.start_minute = buf->start_m;
	cmd.start_second = buf->start_s;
	cmd.start_frame = buf->start_f;
	cmd.end_minute = buf->end_m;
	cmd.end_second = buf->end_s;
	cmd.end_frame = buf->end_f;
	
	return scsi_periph->simple_exec( device->scsi_periph_device,
		&cmd, sizeof( cmd ),
		NULL, 0, 0 );
}


// play audio cd; time is in track/index
static status_t play_track_index( cd_device_info *device, scsi_play_track *buf )
{
	scsi_toc generic_toc;
	scsi_toc_toc *toc;
	status_t res;
	int start_track, end_track;
	scsi_play_position position;
	
	SHOW_FLOW( 0, "%d-%d", buf->start_track, buf->end_track );

	// the corresponding command PLAY AUDIO TRACK/INDEX	is deprecated,
	// so we have to simulate it by converting track to time via TOC
	res = get_toc( device, &generic_toc );
	if( res != B_OK )
		return res;
	
	toc = (scsi_toc_toc *)&generic_toc.toc_data[0];
	
	start_track = buf->start_track;
	end_track = buf->end_track;
	
	if( start_track > toc->last_track )
		return B_BAD_INDEX;

	if( end_track > toc->last_track )
		end_track = toc->last_track + 1;
		
	if( end_track < toc->last_track + 1)
		++end_track;
		
	start_track -= toc->first_track;
	end_track -= toc->first_track;
	
	if( start_track < 0 || end_track < 0 )
		return B_BAD_INDEX;

	position.start_m = toc->tracks[start_track].start.time.minute;
	position.start_s = toc->tracks[start_track].start.time.second;
	position.start_f = toc->tracks[start_track].start.time.frame;
	
	position.end_m = toc->tracks[end_track].start.time.minute;
	position.end_s = toc->tracks[end_track].start.time.second;
	position.end_f = toc->tracks[end_track].start.time.frame;
	
	return play_msf( device, &position );
}

static status_t stop_audio( cd_device_info *device )
{
	scsi_cmd_stop_play cmd;
	
	SHOW_FLOW0( 3, "" );
	
	memset( &cmd, 0, sizeof( cmd ));
	cmd.opcode = SCSI_OP_STOP_PLAY;
	
	return scsi_periph->simple_exec( device->scsi_periph_device,
		&cmd, sizeof( cmd ), NULL, 0, 0 );
}

static status_t pause_resume( cd_device_info *device, bool resume )
{
	scsi_cmd_pause_resume cmd;
	
	SHOW_FLOW0( 3, "" );
	
	memset( &cmd, 0, sizeof( cmd ));
	cmd.opcode = SCSI_OP_PAUSE_RESUME;
	cmd.resume = resume;
	
	return scsi_periph->simple_exec( device->scsi_periph_device,
		&cmd, sizeof( cmd ), NULL, 0, 0 );
}

static status_t scan( cd_device_info *device, scsi_scan *buf )
{
	scsi_cmd_scan cmd;
	scsi_position cur_pos;
	scsi_cd_current_position *cd_pos;
	status_t res;
	/*uint8 *tmp;*/
	
	SHOW_FLOW( 3, "direction=%d", buf->direction );
	
	res = get_position( device, &cur_pos );
	if( res != B_OK )
		return res;
		
	cd_pos = (scsi_cd_current_position *)((char *)&cur_pos 
		+ sizeof( scsi_subchannel_data_header ));
	
	if( buf->direction == 0 ) {
		scsi_play_position play_pos;
		
		// to stop scan, we issue play command with "open end"
		play_pos.start_m = cd_pos->absolute_address.time.minute;
		play_pos.start_s = cd_pos->absolute_address.time.second;
		play_pos.start_f = cd_pos->absolute_address.time.frame;
		play_pos.end_m = 99;
		play_pos.end_s = 59;
		play_pos.end_f = 24;
		
		return play_msf( device, &play_pos );
	}
	
	memset( &cmd, 0, sizeof( cmd ));
	
	cmd.opcode = SCSI_OP_SCAN;
	cmd.Direct = buf->direction < 0;
	cmd.start.time = cd_pos->absolute_address.time;
	cmd.type = scsi_scan_msf;
	
	/*
	tmp = (uint8 *)&cmd;
	dprintf( "%d %d %d %d %d %d %d %d %d %d %d %d\n",
		tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5], 
		tmp[6], tmp[7], tmp[8], tmp[9], tmp[10], tmp[11] );
	*/

	return scsi_periph->simple_exec( device->scsi_periph_device,
		&cmd, sizeof( cmd ), NULL, 0, 0 );
}


static status_t read_cd( cd_device_info *device, scsi_read_cd *read_cd )
{
	scsi_cmd_read_cd *cmd;
	uint32 lba, length;		
	scsi_ccb *ccb;
	status_t res;

	// we use safe_exec instead of simple_exec as we want to set
	// the sorting order manually (only makes much sense if you grab
	// multiple tracks at once, but we are prepared)
	ccb = device->scsi->alloc_ccb( device->scsi_device );
	
	if( ccb == NULL )
		return B_NO_MEMORY;
		
	cmd = (scsi_cmd_read_cd *)ccb->cdb;		
	memset( cmd, 0, sizeof( *cmd ));
	cmd->opcode = SCSI_OP_READ_CD;
	cmd->sector_type = 1;

	// skip first two seconds, they are lead-in
	lba = (read_cd->start_m * 60 + read_cd->start_s) * 75 + read_cd->start_f
		- 2 * 75;

	cmd->lba.top = (lba >> 24) & 0xff;
	cmd->lba.high = (lba >> 16) & 0xff;
	cmd->lba.mid = (lba >> 8) & 0xff;
	cmd->lba.low = lba & 0xff;

	length = (read_cd->length_m * 60 + read_cd->length_s) * 75 + read_cd->length_f;
	
	cmd->high_length = (length >> 16) & 0xff;
	cmd->mid_length = (length >> 8) & 0xff;
	cmd->low_length = length & 0xff;

	cmd->error_field = scsi_read_cd_error_none;
	cmd->EDC_ECC = 0;
	cmd->user_data = 1;
	cmd->header_code = scsi_read_cd_header_none;
	cmd->SYNC = 0;
	cmd->sub_channel_selection = scsi_read_cd_sub_channel_none;

	ccb->cdb_len = sizeof( *cmd );

	ccb->flags = SCSI_DIR_IN | SCSI_DIS_DISCONNECT;
	ccb->sort = lba;
	// are 10 seconds enough for timeout?
	ccb->timeout = 10;
	
	ccb->data = read_cd->buffer;
	ccb->sg_list = NULL;
	ccb->data_len = read_cd->buffer_length;

	res = scsi_periph->safe_exec( device->scsi_periph_device, ccb );

	device->scsi->free_ccb( ccb );
	
	return res;
}


static status_t cd_ioctl( cd_handle_info *handle, int op, void *buf, size_t len )
{
	cd_device_info *device = handle->device;
	status_t res;
	
	switch( op ) {
	case B_GET_DEVICE_SIZE:
		res = update_capacity( device );
		if( res == B_OK )
			(size_t *)buf = device->capacity * device->block_size;
		break;
		
	case B_GET_GEOMETRY:
		res = get_geometry( handle, buf, len );
		break;
		
	case B_GET_ICON:
		res = scsi_periph->get_icon( icon_type_cd, (device_icon *)buf );
		break;
		
	case B_SCSI_GET_TOC:
		res = get_toc( device, (scsi_toc *)buf );
		break;
		
	case B_EJECT_DEVICE:
	case B_SCSI_EJECT:
		res = load_eject( device, false );
		break;
		
	case B_LOAD_MEDIA:
		res = load_eject( device, true );
		break;
		
	case B_SCSI_GET_POSITION:
		res = get_position( device, buf );
		break;
	
	case B_SCSI_GET_VOLUME:
		res = get_set_volume( device, buf, false );
		break;
		
	case B_SCSI_SET_VOLUME:
		res = get_set_volume( device, buf, true );
		break;
	
	case B_SCSI_PLAY_TRACK:
		res = play_track_index( device, buf );
		break;
		
	case B_SCSI_PLAY_POSITION:
		res = play_msf( device, buf );
		break;
		
	case B_SCSI_STOP_AUDIO:
		res = stop_audio( device );
		break;
		
	case B_SCSI_PAUSE_AUDIO:
		res = pause_resume( device, false );
		break;
		
	case B_SCSI_RESUME_AUDIO:
		res = pause_resume( device, true );
		break;
		
	case B_SCSI_SCAN:
		res = scan( device, buf );
		break;
		
	case B_SCSI_READ_CD:
		res = read_cd( device, buf );
		break;
		
	default:
		res = scsi_periph->ioctl( handle->scsi_periph_handle, op, buf, len );
		break;
	}
	
	SHOW_FLOW( 3, "%x: %s", op, strerror( res ));
	
	return res;
}


static int log2( uint32 x )
{
	int y;
	
	for( y = 31; y >= 0; --y )
		if( x == ((uint32)1 << y) )
			break;
			
	return y;
}


static void cd_set_capacity( cd_device_info *device, uint64 capacity,
	uint32 block_size )
{
	uint32 ld_block_size;
	
	SHOW_FLOW( 3, "device=%p, capacity=%Ld, block_size=%ld", 
		device, capacity, block_size );

	// get log2, if possible
	ld_block_size = log2( block_size );

	if( (1UL << ld_block_size) != block_size )
		ld_block_size = 0;
		
	device->capacity = capacity;
	device->block_size = block_size;

	blkman->set_media_params( device->blkman_device, block_size, 
		ld_block_size, capacity );	
}

static void cd_media_changed( cd_device_info *device, scsi_ccb *request )
{
	// do a capacity check
	// TBD: is this a good idea (e.g. if this is an empty CD)?
	scsi_periph->check_capacity( device->scsi_periph_device, request );
}


scsi_periph_callbacks callbacks = {
	(void (*)( periph_device_cookie, 
		uint64, uint32 ))							cd_set_capacity,
	(void (*)( periph_device_cookie, scsi_ccb *))	cd_media_changed
};


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


module_dependency module_dependencies[] = {
	{ SCSI_PERIPH_MODULE_NAME, (module_info **)&scsi_periph },
	{ BLKMAN_FOR_DRIVER_MODULE_NAME, (module_info **)&blkman },
	{ DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{}
};

blkdev_interface scsi_cd_module = {
	{
		{
			SCSI_CD_MODULE_NAME,
			0,			
			std_ops
		},
		
		cd_init_device,
		(status_t (*) (void *))cd_uninit_device,
		cd_device_added,
		NULL
	},
	
	(status_t (*)( blkdev_device_cookie, blkdev_handle_cookie * )) &cd_open,
	(status_t (*)( blkdev_handle_cookie ))						&cd_close,
	(status_t (*)( blkdev_handle_cookie ))						&cd_free,
	
	(status_t (*)( blkdev_handle_cookie, const phys_vecs *, 
		off_t, size_t, uint32, size_t * ))						&cd_read,
	(status_t (*)( blkdev_handle_cookie, const phys_vecs *,
		off_t, size_t, uint32, size_t * ))						&cd_write,

	(status_t (*)( blkdev_handle_cookie, int, void *, size_t ))	&cd_ioctl,
};

#if !_BUILDING_kernel && !BOOT
_EXPORT 
module_info *modules[] =
{
	(module_info *)&scsi_cd_module,
	NULL
};
#endif
