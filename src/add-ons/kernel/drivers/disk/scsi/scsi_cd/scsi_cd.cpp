/*
 * Copyright 2004-2010, Haiku, Inc. All rights reserved.
 * Copyright 2002-2003, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


/*!	Peripheral driver to handle CD-ROM drives. To be more
	precisely, it supports CD-ROM and WORM drives (well -
	I've never _seen_ a WORM driver).

	Much work is done by scsi_periph and block_io.
*/


#include "scsi_cd.h"

#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <io_requests.h>
#include <vm/vm_page.h>

#include "IOCache.h"
#include "IOSchedulerSimple.h"


//#define TRACE_CD_DISK
#ifdef TRACE_CD_DISK
#	define TRACE(x...) dprintf("scsi_cd: " x)
#else
#	define TRACE(x...) ;
#endif


static const uint8 kCDIcon[] = {
	0x6e, 0x63, 0x69, 0x66, 0x05, 0x05, 0x00, 0x02, 0x03, 0x06, 0x05, 0xb8,
	0x12, 0xa5, 0xbe, 0x03, 0xe1, 0x3d, 0xe7, 0x84, 0xb8, 0x02, 0x10, 0x49,
	0xf7, 0x9f, 0x49, 0xed, 0xd8, 0x00, 0xf1, 0xf1, 0xf1, 0x36, 0xd9, 0xdd,
	0xf4, 0x8a, 0x99, 0x96, 0xb9, 0xb4, 0xb8, 0xbe, 0xdb, 0xff, 0xf4, 0xf4,
	0xf4, 0x04, 0xeb, 0xd0, 0x02, 0x00, 0x06, 0x02, 0x3c, 0x92, 0xc0, 0x38,
	0x8f, 0x5f, 0xb8, 0x54, 0x50, 0x3c, 0x57, 0x63, 0x48, 0xd8, 0xdf, 0x48,
	0x89, 0x5b, 0x00, 0x41, 0x37, 0xa9, 0xff, 0xb9, 0xb9, 0xb9, 0x04, 0x01,
	0x7e, 0x04, 0x02, 0x04, 0x3f, 0x2c, 0x4e, 0x2c, 0x30, 0x2c, 0x22, 0x40,
	0x22, 0x34, 0x22, 0x4c, 0x3f, 0x54, 0x30, 0x54, 0x4e, 0x54, 0x5c, 0x40,
	0x5c, 0x4c, 0x5c, 0x34, 0x02, 0x04, 0x3f, 0x3a, 0x43, 0x3a, 0x3b, 0x3a,
	0x39, 0x3e, 0x39, 0x3c, 0x39, 0x40, 0x3f, 0x42, 0x3b, 0x42, 0x43, 0x42,
	0x45, 0x3e, 0x45, 0x40, 0x45, 0x3c, 0x02, 0x04, 0x4b, 0x3e, 0x4b, 0x3a,
	0x4b, 0x42, 0x3f, 0x46, 0x47, 0x46, 0x37, 0x46, 0x33, 0x3e, 0x33, 0x42,
	0x33, 0x3a, 0x3f, 0xbb, 0xf7, 0x37, 0xbb, 0xf7, 0x47, 0xbb, 0xf7, 0x02,
	0x04, 0x40, 0x2a, 0x54, 0x2a, 0x50, 0x2c, 0x5c, 0x40, 0x5c, 0x34, 0x5c,
	0x4c, 0x40, 0x56, 0x50, 0x54, 0x54, 0x56, 0x60, 0x40, 0x60, 0x4c, 0x60,
	0x34, 0x06, 0x0a, 0x04, 0x01, 0x03, 0x00, 0x0a, 0x00, 0x02, 0x00, 0x01,
	0x18, 0x15, 0xff, 0x01, 0x17, 0x84, 0x00, 0x04, 0x0a, 0x00, 0x02, 0x00,
	0x01, 0x18, 0x00, 0x15, 0x01, 0x17, 0x86, 0x00, 0x04, 0x0a, 0x01, 0x02,
	0x00, 0x02, 0x00, 0x0a, 0x02, 0x02, 0x02, 0x01, 0x00, 0x0a, 0x03, 0x01,
	0x02, 0x10, 0x01, 0x17, 0x82, 0x00, 0x04
};


static scsi_periph_interface *sSCSIPeripheral;
static device_manager_info *sDeviceManager;


#define SCSI_CD_STD_TIMEOUT 10


static status_t
update_capacity(cd_driver_info *info)
{
	TRACE("update_capacity()\n");

	scsi_ccb *ccb = info->scsi->alloc_ccb(info->scsi_device);
	if (ccb == NULL)
		return B_NO_MEMORY;

	status_t status = sSCSIPeripheral->check_capacity(
		info->scsi_periph_device, ccb);

	info->scsi->free_ccb(ccb);

	return status;
}


/*!	Iteratively correct the reported capacity by trying to read from the device
	close to its end.
*/
static uint64
test_capacity(cd_driver_info *info)
{
	static const size_t kMaxEntries = 4;
	const uint32 blockSize = info->block_size;
	const size_t kBufferSize = blockSize * 4;

	TRACE("test_capacity: read with buffer size %" B_PRIuSIZE ", block size %"
		B_PRIu32", capacity %llu\n", kBufferSize, blockSize,
		info->original_capacity);

	info->capacity = info->original_capacity;

	size_t numBlocks = B_PAGE_SIZE / blockSize;
	uint64 offset = info->original_capacity;
	if (offset <= numBlocks)
		return B_OK;

	offset -= numBlocks;

	scsi_ccb *request = info->scsi->alloc_ccb(info->scsi_device);
	if (request == NULL)
		return B_NO_MEMORY;

	// Allocate buffer

	physical_entry entries[4];
	size_t numEntries = 0;

	vm_page_reservation reservation;
	vm_page_reserve_pages(&reservation,
		(kBufferSize - 1 + B_PAGE_SIZE) / B_PAGE_SIZE, VM_PRIORITY_SYSTEM);

	for (size_t left = kBufferSize; numEntries < kMaxEntries && left > 0;
			numEntries++) {
		size_t bytes = std::min(left, (size_t)B_PAGE_SIZE);

		vm_page* page = vm_page_allocate_page(&reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_BUSY);

		entries[numEntries].address = page->physical_page_number * B_PAGE_SIZE;
		entries[numEntries].size = bytes;;

		left -= bytes;
	}

	vm_page_unreserve_pages(&reservation);

	// Read close to the end of the device to find out its real end

	// Only try 1 second before the end (= 75 blocks)
	while (offset > info->original_capacity - 75) {
		size_t bytesTransferred;
		status_t status = sSCSIPeripheral->read_write(info->scsi_periph_device,
			request, offset, numBlocks, entries, numEntries, false,
			&bytesTransferred);

		TRACE("test_capacity: read from offset %llu: %s\n", offset,
			strerror(status));

		if (status == B_OK || (request->sense[0] & 0x7f) != 0x70)
			break;

		switch (request->sense[2]) {
			case SCSIS_KEY_MEDIUM_ERROR:
			case SCSIS_KEY_ILLEGAL_REQUEST:
			case SCSIS_KEY_VOLUME_OVERFLOW:
			{
				// find out the problematic sector
				uint32 errorBlock = (request->sense[3] << 24U)
					| (request->sense[4] << 16U) | (request->sense[5] << 8U)
					| request->sense[6];
				if (errorBlock >= offset)
					info->capacity = errorBlock;
				break;
			}

			default:
				break;
		}

		if (numBlocks > offset)
			break;

		offset -= numBlocks;
	}

	info->scsi->free_ccb(request);

	for (size_t i = 0; i < numEntries; i++) {
		vm_page_set_state(vm_lookup_page(entries[i].address / B_PAGE_SIZE),
			PAGE_STATE_FREE);
	}

	if (info->capacity != info->original_capacity) {
		dprintf("scsi_cd: adjusted capacity from %" B_PRIu64 " to %" B_PRIu64
			" blocks.\n", info->original_capacity, info->capacity);
	}

	return B_OK;
}


static status_t
get_geometry(cd_handle *handle, device_geometry *geometry)
{
	cd_driver_info *info = handle->info;

	status_t status = update_capacity(info);

	// it seems that Be expects B_GET_GEOMETRY to always succeed unless
	// the medium has been changed; e.g. if we report B_DEV_NO_MEDIA, the
	// info is ignored by the CDPlayer and CDBurner
	if (status == B_DEV_MEDIA_CHANGED)
		return B_DEV_MEDIA_CHANGED;

	geometry->bytes_per_sector = info->block_size;
	geometry->sectors_per_track = 1;
	geometry->cylinder_count = info->capacity;
	geometry->head_count = 1;
	geometry->device_type = info->device_type;
	geometry->removable = info->removable;

	// TBD: for all but CD-ROMs, read mode sense - medium type
	// (bit 7 of block device specific parameter for Optical Memory Block Device)
	// (same for Direct-Access Block Devices)
	// (same for write-once block devices)
	// (same for optical memory block devices)
	geometry->read_only = true;
	geometry->write_once = info->device_type == scsi_dev_WORM;

	TRACE("scsi_disk: get_geometry(): %ld, %ld, %ld, %ld, %d, %d, %d, %d\n",
		geometry->bytes_per_sector, geometry->sectors_per_track,
		geometry->cylinder_count, geometry->head_count, geometry->device_type,
		geometry->removable, geometry->read_only, geometry->write_once);

	return B_OK;
}


static status_t
get_toc(cd_driver_info *info, scsi_toc *toc)
{
	scsi_ccb *ccb;
	status_t res;
	scsi_cmd_read_toc *cmd;
	size_t dataLength;
	scsi_toc_general *shortResponse = (scsi_toc_general *)toc->toc_data;

	TRACE("get_toc()\n");

	ccb = info->scsi->alloc_ccb(info->scsi_device);
	if (ccb == NULL)
		return B_NO_MEMORY;

	// first read number of tracks only
	ccb->flags = SCSI_DIR_IN;

	cmd = (scsi_cmd_read_toc *)ccb->cdb;

	memset(cmd, 0, sizeof(*cmd));
	cmd->opcode = SCSI_OP_READ_TOC;
	cmd->time = 1;
	cmd->format = SCSI_TOC_FORMAT_TOC;
	cmd->track = 1;
	cmd->allocation_length = B_HOST_TO_BENDIAN_INT16(sizeof(scsi_toc_general));

	ccb->cdb_length = sizeof(*cmd);

	ccb->sort = -1;
	ccb->timeout = SCSI_CD_STD_TIMEOUT;

	ccb->data = toc->toc_data;
	ccb->sg_list = NULL;
	ccb->data_length = sizeof(toc->toc_data);

	res = sSCSIPeripheral->safe_exec(info->scsi_periph_device, ccb);
	if (res != B_OK)
		goto err;

	// then read all track infos
	// (little hint: number of tracks is last - first + 1;
	//  but scsi_toc_toc has already one track, so we get
	//  last - first extra tracks; finally, we want the lead-out as
	//  well, so we add an extra track)
	dataLength = (shortResponse->last - shortResponse->first + 1)
		* sizeof(scsi_toc_track) + sizeof(scsi_toc_toc);
	dataLength = min_c(dataLength, sizeof(toc->toc_data));

	TRACE("  tracks: %d - %d, data length %d\n", shortResponse->first,
		shortResponse->last, (int)dataLength);

	cmd->allocation_length = B_HOST_TO_BENDIAN_INT16(dataLength);

	res = sSCSIPeripheral->safe_exec(info->scsi_periph_device, ccb);

err:
	info->scsi->free_ccb(ccb);

	return res;
}


static status_t
load_eject(cd_driver_info *info, bool load)
{
	TRACE("load_eject()\n");

	scsi_ccb *ccb = info->scsi->alloc_ccb(info->scsi_device);
	if (ccb == NULL)
		return B_NO_MEMORY;

	err_res result = sSCSIPeripheral->send_start_stop(
		info->scsi_periph_device, ccb, load, true);

	info->scsi->free_ccb(ccb);

	return result.error_code;
}


static status_t
get_position(cd_driver_info *info, scsi_position *position)
{
	scsi_cmd_read_subchannel cmd;

	TRACE("get_position()\n");

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SCSI_OP_READ_SUB_CHANNEL;
	cmd.time = 1;
	cmd.subq = 1;
	cmd.parameter_list = scsi_sub_channel_parameter_list_cd_pos;
	cmd.track = 0;
	cmd.allocation_length = B_HOST_TO_BENDIAN_INT16(sizeof(scsi_position));

	return sSCSIPeripheral->simple_exec(info->scsi_periph_device,
		&cmd, sizeof(cmd), position, sizeof(*position), SCSI_DIR_IN);
}


static status_t
get_set_volume(cd_driver_info *info, scsi_volume *volume, bool set)
{
	scsi_cmd_mode_sense_6 cmd;
	scsi_mode_param_header_6 header;
	size_t len;
	void *buffer;
	scsi_modepage_audio	*page;
	status_t res;

	TRACE("get_set_volume()\n");

	// determine size of block descriptor
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SCSI_OP_MODE_SENSE_6;
	cmd.page_code = SCSI_MODEPAGE_AUDIO;
	cmd.page_control = SCSI_MODE_SENSE_PC_CURRENT;
	cmd.allocation_length = sizeof(header);

	memset(&header, -2, sizeof(header));

	res = sSCSIPeripheral->simple_exec(info->scsi_periph_device, &cmd,
		sizeof(cmd), &header, sizeof(header), SCSI_DIR_IN);
	if (res != B_OK)
		return res;

	TRACE("  block_desc_len=%d", header.block_desc_length);
#if 0
	// ToDo: why this??
	return B_ERROR;
#endif

	// retrieve param header, block descriptor and actual codepage
	len = sizeof(header) + header.block_desc_length
		+ sizeof(scsi_modepage_audio);

	buffer = malloc(len);
	if (buffer == NULL)
		return B_NO_MEMORY;

	memset(buffer, -1, sizeof(buffer));

	cmd.allocation_length = len;

	res = sSCSIPeripheral->simple_exec(info->scsi_periph_device, &cmd,
		sizeof(cmd), buffer, len, SCSI_DIR_IN);
	if (res != B_OK) {
		free(buffer);
		return res;
	}

	TRACE("  mode_data_len=%d, block_desc_len=%d",
		((scsi_mode_param_header_6 *)buffer)->mode_data_length,
		((scsi_mode_param_header_6 *)buffer)->block_desc_length);

	// find control page and retrieve values
	page = (scsi_modepage_audio *)((char *)buffer + sizeof(header)
		+ header.block_desc_length);

	TRACE("  page=%p, codepage=%d", page, page->header.page_code);

	if (!set) {
		volume->port0_channel = page->ports[0].channel;
		volume->port0_volume  = page->ports[0].volume;
		volume->port1_channel = page->ports[1].channel;
		volume->port1_volume  = page->ports[1].volume;
		volume->port2_channel = page->ports[2].channel;
		volume->port2_volume  = page->ports[2].volume;
		volume->port3_channel = page->ports[3].channel;
		volume->port3_volume  = page->ports[3].volume;

#if 0
		SHOW_FLOW(3, "1: %d - %d", volume->port0_channel, volume->port0_volume);
		SHOW_FLOW(3, "2: %d - %d", volume->port1_channel, volume->port1_volume);
		SHOW_FLOW(3, "3: %d - %d", volume->port2_channel, volume->port2_volume);
		SHOW_FLOW(3, "4: %d - %d", volume->port3_channel, volume->port3_volume);
#endif
		res = B_OK;
	} else {
		scsi_cmd_mode_select_6 cmd;

		if (volume->flags & 0x01)
			page->ports[0].channel = volume->port0_channel;
		if (volume->flags & 0x02)
			page->ports[0].volume = volume->port0_volume;
		if (volume->flags & 0x04)
			page->ports[1].channel = volume->port1_channel;
		if (volume->flags & 0x08)
			page->ports[1].volume = volume->port1_volume;
		if (volume->flags & 0x10)
			page->ports[2].channel = volume->port2_channel;
		if (volume->flags & 0x20)
			page->ports[2].volume = volume->port2_volume;
		if (volume->flags & 0x40)
			page->ports[3].channel = volume->port3_channel;
		if (volume->flags & 0x80)
			page->ports[3].volume = volume->port3_volume;

		memset(&cmd, 0, sizeof(cmd));
		cmd.opcode = SCSI_OP_MODE_SELECT_6;
		cmd.pf = 1;
		cmd.param_list_length = sizeof(header) + header.block_desc_length
			+ sizeof(*page);

		res = sSCSIPeripheral->simple_exec(info->scsi_periph_device,
			&cmd, sizeof(cmd), buffer, len, SCSI_DIR_OUT);
	}

	free(buffer);
	return res;
}


/*!	Play audio cd; time is in MSF */
static status_t
play_msf(cd_driver_info *info, const scsi_play_position *position)
{
	scsi_cmd_play_msf cmd;

	TRACE("play_msf(): %d:%d:%d-%d:%d:%d\n", position->start_m,
		position->start_s, position->start_f, position->end_m, position->end_s,
		position->end_f);

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = SCSI_OP_PLAY_MSF;
	cmd.start_minute = position->start_m;
	cmd.start_second = position->start_s;
	cmd.start_frame = position->start_f;
	cmd.end_minute = position->end_m;
	cmd.end_second = position->end_s;
	cmd.end_frame = position->end_f;

	return sSCSIPeripheral->simple_exec(info->scsi_periph_device,
		&cmd, sizeof(cmd), NULL, 0, SCSI_DIR_NONE);
}


/*! Play audio cd; time is in track/index */
static status_t
play_track_index(cd_driver_info *info, const scsi_play_track *buf)
{
	scsi_toc generic_toc;
	scsi_toc_toc *toc;
	status_t res;
	int start_track, end_track;
	scsi_play_position position;

	TRACE("play_track_index(): %d-%d\n", buf->start_track, buf->end_track);

	// the corresponding command PLAY AUDIO TRACK/INDEX	is deprecated,
	// so we have to simulate it by converting track to time via TOC
	res = get_toc(info, &generic_toc);
	if (res != B_OK)
		return res;

	toc = (scsi_toc_toc *)&generic_toc.toc_data[0];

	start_track = buf->start_track;
	end_track = buf->end_track;

	if (start_track > toc->last_track)
		return B_BAD_INDEX;

	if (end_track > toc->last_track)
		end_track = toc->last_track + 1;

	if (end_track < toc->last_track + 1)
		++end_track;

	start_track -= toc->first_track;
	end_track -= toc->first_track;

	if (start_track < 0 || end_track < 0)
		return B_BAD_INDEX;

	position.start_m = toc->tracks[start_track].start.time.minute;
	position.start_s = toc->tracks[start_track].start.time.second;
	position.start_f = toc->tracks[start_track].start.time.frame;

	position.end_m = toc->tracks[end_track].start.time.minute;
	position.end_s = toc->tracks[end_track].start.time.second;
	position.end_f = toc->tracks[end_track].start.time.frame;

	return play_msf(info, &position);
}


static status_t
stop_audio(cd_driver_info *info)
{
	scsi_cmd_stop_play cmd;

	TRACE("stop_audio()\n");

	memset( &cmd, 0, sizeof( cmd ));
	cmd.opcode = SCSI_OP_STOP_PLAY;

	return sSCSIPeripheral->simple_exec(info->scsi_periph_device,
		&cmd, sizeof(cmd), NULL, 0, SCSI_DIR_NONE);
}


static status_t
pause_resume(cd_driver_info *info, bool resume)
{
	scsi_cmd_pause_resume cmd;

	TRACE("pause_resume()\n");

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SCSI_OP_PAUSE_RESUME;
	cmd.resume = resume;

	return sSCSIPeripheral->simple_exec(info->scsi_periph_device,
		&cmd, sizeof(cmd), NULL, 0, SCSI_DIR_NONE);
}


static status_t
scan(cd_driver_info *info, const scsi_scan *buf)
{
	scsi_cmd_scan cmd;
	scsi_position curPos;
	scsi_cd_current_position *cdPos;

	TRACE("scan(direction =% d)\n", buf->direction);

	status_t res = get_position(info, &curPos);
	if (res != B_OK)
		return res;

	cdPos = (scsi_cd_current_position *)((char *)&curPos
		+ sizeof(scsi_subchannel_data_header));

	if (buf->direction == 0) {
		scsi_play_position playPos;

		// to stop scan, we issue play command with "open end"
		playPos.start_m = cdPos->absolute_address.time.minute;
		playPos.start_s = cdPos->absolute_address.time.second;
		playPos.start_f = cdPos->absolute_address.time.frame;
		playPos.end_m = 99;
		playPos.end_s = 59;
		playPos.end_f = 24;

		return play_msf(info, &playPos);
	}

	memset(&cmd, 0, sizeof(cmd));

	cmd.opcode = SCSI_OP_SCAN;
	cmd.direct = buf->direction < 0;
	cmd.start.time = cdPos->absolute_address.time;
	cmd.type = scsi_scan_msf;

	/*
	tmp = (uint8 *)&cmd;
	dprintf("%d %d %d %d %d %d %d %d %d %d %d %d\n",
		tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5],
		tmp[6], tmp[7], tmp[8], tmp[9], tmp[10], tmp[11]);
	*/

	return sSCSIPeripheral->simple_exec(info->scsi_periph_device,
		&cmd, sizeof(cmd), NULL, 0, SCSI_DIR_NONE);
}


static status_t
read_cd(cd_driver_info *info, const scsi_read_cd *readCD)
{
	scsi_cmd_read_cd *cmd;
	uint32 lba, length;
	scsi_ccb *ccb;
	status_t res;

	// we use safe_exec instead of simple_exec as we want to set
	// the sorting order manually (only makes much sense if you grab
	// multiple tracks at once, but we are prepared)
	ccb = info->scsi->alloc_ccb(info->scsi_device);

	if (ccb == NULL)
		return B_NO_MEMORY;

	cmd = (scsi_cmd_read_cd *)ccb->cdb;
	memset(cmd, 0, sizeof(*cmd));
	cmd->opcode = SCSI_OP_READ_CD;
	cmd->sector_type = 1;

	// skip first two seconds, they are lead-in
	lba = (readCD->start_m * 60 + readCD->start_s) * 75 + readCD->start_f
		- 2 * 75;
	length = (readCD->length_m * 60 + readCD->length_s) * 75 + readCD->length_f;

	cmd->lba = B_HOST_TO_BENDIAN_INT32(lba);
	cmd->high_length = (length >> 16) & 0xff;
	cmd->mid_length = (length >> 8) & 0xff;
	cmd->low_length = length & 0xff;

	cmd->error_field = scsi_read_cd_error_none;
	cmd->edc_ecc = 0;
	cmd->user_data = 1;
	cmd->header_code = scsi_read_cd_header_none;
	cmd->sync = 0;
	cmd->sub_channel_selection = scsi_read_cd_sub_channel_none;

	ccb->cdb_length = sizeof(*cmd);

	ccb->flags = SCSI_DIR_IN | SCSI_DIS_DISCONNECT;
	ccb->sort = lba;
	// are 10 seconds enough for timeout?
	ccb->timeout = 10;

	// TODO: we pass a user buffer here!
	ccb->data = (uint8 *)readCD->buffer;
	ccb->sg_list = NULL;
	ccb->data_length = readCD->buffer_length;

	res = sSCSIPeripheral->safe_exec(info->scsi_periph_device, ccb);

	info->scsi->free_ccb(ccb);

	return res;
}


static int
log2(uint32 x)
{
	int y;

	for (y = 31; y >= 0; --y) {
		if (x == (1UL << y))
			break;
	}

	return y;
}


static status_t
do_io(void* cookie, IOOperation* operation)
{
	cd_driver_info* info = (cd_driver_info*)cookie;

	// TODO: this can go away as soon as we pushed the IOOperation to the upper
	// layers - we can then set scsi_periph::io() as callback for the scheduler
	size_t bytesTransferred;
	status_t status = sSCSIPeripheral->io(info->scsi_periph_device, operation,
		&bytesTransferred);

	info->io_scheduler->OperationCompleted(operation, status, bytesTransferred);
	return status;
}


//	#pragma mark - device module API


static status_t
cd_init_device(void* _info, void** _cookie)
{
	cd_driver_info* info = (cd_driver_info*)_info;

	update_capacity(info);
		// Get initial capacity, but ignore the result; we do not care
		// whether or not a media is present

	*_cookie = info;
	return B_OK;
}


static void
cd_uninit_device(void* _cookie)
{
	cd_driver_info* info = (cd_driver_info*)_cookie;

	delete info->io_scheduler;
	delete info->dma_resource;
}


static status_t
cd_open(void* _info, const char* path, int openMode, void** _cookie)
{
	cd_driver_info* info = (cd_driver_info*)_info;

	cd_handle* handle = (cd_handle*)malloc(sizeof(cd_handle));
	if (handle == NULL)
		return B_NO_MEMORY;

	handle->info = info;

	status_t status = sSCSIPeripheral->handle_open(info->scsi_periph_device,
		(periph_handle_cookie)handle, &handle->scsi_periph_handle);
	if (status < B_OK) {
		free(handle);
		return status;
	}

	*_cookie = handle;
	return B_OK;
}


static status_t
cd_close(void* cookie)
{
	cd_handle* handle = (cd_handle*)cookie;
	TRACE("close()\n");

	sSCSIPeripheral->handle_close(handle->scsi_periph_handle);
	return B_OK;
}


static status_t
cd_free(void* cookie)
{
	cd_handle* handle = (cd_handle*)cookie;
	TRACE("free()\n");

	sSCSIPeripheral->handle_free(handle->scsi_periph_handle);
	free(handle);
	return B_OK;
}


static status_t
cd_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	cd_handle* handle = (cd_handle*)cookie;
	size_t length = *_length;

	if (handle->info->capacity == 0)
		return B_DEV_NO_MEDIA;

	IORequest request;
	status_t status = request.Init(pos, (addr_t)buffer, length, false, 0);
	if (status != B_OK)
		return status;

	status = handle->info->io_scheduler->ScheduleRequest(&request);
	if (status != B_OK)
		return status;

	status = request.Wait(0, 0);
	if (status == B_OK)
		*_length = length;
	else
		dprintf("cd_read(): request.Wait() returned: %s\n", strerror(status));

	return status;
}


static status_t
cd_write(void* cookie, off_t pos, const void* buffer, size_t* _length)
{
	cd_handle* handle = (cd_handle*)cookie;
	size_t length = *_length;

	if (handle->info->capacity == 0)
		return B_DEV_NO_MEDIA;

	IORequest request;
	status_t status = request.Init(pos, (addr_t)buffer, length, true, 0);
	if (status != B_OK)
		return status;

	status = handle->info->io_scheduler->ScheduleRequest(&request);
	if (status != B_OK)
		return status;

	status = request.Wait(0, 0);
	if (status == B_OK)
		*_length = length;
	else
		dprintf("cd_write(): request.Wait() returned: %s\n", strerror(status));

	return status;
}


static status_t
cd_io(void* cookie, io_request* request)
{
	cd_handle* handle = (cd_handle*)cookie;

	if (handle->info->capacity == 0) {
		notify_io_request(request, B_DEV_NO_MEDIA);
		return B_DEV_NO_MEDIA;
	}

	return handle->info->io_scheduler->ScheduleRequest(request);
}


static status_t
cd_ioctl(void* cookie, uint32 op, void* buffer, size_t length)
{
	cd_handle* handle = (cd_handle*)cookie;
	cd_driver_info *info = handle->info;

	TRACE("ioctl(op = %lu)\n", op);

	switch (op) {
		case B_GET_DEVICE_SIZE:
		{
			status_t status = update_capacity(info);
			if (status != B_OK)
				return status;

			size_t size = info->capacity * info->block_size;
			return user_memcpy(buffer, &size, sizeof(size_t));
		}

		case B_GET_GEOMETRY:
		{
			if (buffer == NULL /*|| length != sizeof(device_geometry)*/)
				return B_BAD_VALUE;

		 	device_geometry geometry;
			status_t status = get_geometry(handle, &geometry);
			if (status != B_OK)
				return status;

			return user_memcpy(buffer, &geometry, sizeof(device_geometry));
		}

		case B_GET_ICON_NAME:
			return user_strlcpy((char*)buffer, "devices/drive-optical",
				B_FILE_NAME_LENGTH);

		case B_GET_VECTOR_ICON:
		{
			device_icon iconData;
			if (length != sizeof(device_icon))
				return B_BAD_VALUE;
			if (user_memcpy(&iconData, buffer, sizeof(device_icon)) != B_OK)
				return B_BAD_ADDRESS;

			if (iconData.icon_size >= (int32)sizeof(kCDIcon)) {
				if (user_memcpy(iconData.icon_data, kCDIcon,
						sizeof(kCDIcon)) != B_OK)
					return B_BAD_ADDRESS;
			}

			iconData.icon_size = sizeof(kCDIcon);
			return user_memcpy(buffer, &iconData, sizeof(device_icon));
		}

		case B_SCSI_GET_TOC:
			// TODO: we pass a user buffer here!
			return get_toc(info, (scsi_toc *)buffer);

		case B_EJECT_DEVICE:
		case B_SCSI_EJECT:
			return load_eject(info, false);

		case B_LOAD_MEDIA:
			return load_eject(info, true);

		case B_SCSI_GET_POSITION:
		{
			if (buffer == NULL)
				return B_BAD_VALUE;

			scsi_position position;
			status_t status = get_position(info, &position);
			if (status != B_OK)
				return status;

			return user_memcpy(buffer, &position, sizeof(scsi_position));
		}

		case B_SCSI_GET_VOLUME:
			// TODO: we pass a user buffer here!
			return get_set_volume(info, (scsi_volume *)buffer, false);
		case B_SCSI_SET_VOLUME:
			// TODO: we pass a user buffer here!
			return get_set_volume(info, (scsi_volume *)buffer, true);

		case B_SCSI_PLAY_TRACK:
		{
			scsi_play_track track;
			if (user_memcpy(&track, buffer, sizeof(scsi_play_track)) != B_OK)
				return B_BAD_ADDRESS;

			return play_track_index(info, &track);
		}
		case B_SCSI_PLAY_POSITION:
		{
			scsi_play_position position;
			if (user_memcpy(&position, buffer, sizeof(scsi_play_position))
					!= B_OK)
				return B_BAD_ADDRESS;

			return play_msf(info, &position);
		}

		case B_SCSI_STOP_AUDIO:
			return stop_audio(info);
		case B_SCSI_PAUSE_AUDIO:
			return pause_resume(info, false);
		case B_SCSI_RESUME_AUDIO:
			return pause_resume(info, true);

		case B_SCSI_SCAN:
		{
			scsi_scan scanBuffer;
			if (user_memcpy(&scanBuffer, buffer, sizeof(scsi_scan)) != B_OK)
				return B_BAD_ADDRESS;

			return scan(info, &scanBuffer);
		}
		case B_SCSI_READ_CD:
			// TODO: we pass a user buffer here!
			return read_cd(info, (scsi_read_cd *)buffer);

		default:
			return sSCSIPeripheral->ioctl(handle->scsi_periph_handle, op,
				buffer, length);
	}
}


//	#pragma mark - scsi_periph callbacks


static void
cd_set_capacity(cd_driver_info* info, uint64 capacity, uint32 blockSize)
{
	TRACE("cd_set_capacity(info = %p, capacity = %Ld, blockSize = %ld)\n",
		info, capacity, blockSize);

	// get log2, if possible
	uint32 blockShift = log2(blockSize);

	if ((1UL << blockShift) != blockSize)
		blockShift = 0;

	if (info->block_size != blockSize) {
		if (capacity == 0) {
			// there is obviously no medium in the drive, don't try to update
			// the DMA resource
			return;
		}

		if (info->block_size != 0) {
			dprintf("old %" B_PRId32 ", new %" B_PRId32 "\n", info->block_size,
				blockSize);
			panic("updating DMAResource not yet implemented...");
		}

		// TODO: we need to replace the DMAResource in our IOScheduler
		status_t status = info->dma_resource->Init(info->node, blockSize, 1024,
			32);
		if (status != B_OK)
			panic("initializing DMAResource failed: %s", strerror(status));

		// Allocate the I/O scheduler. If there seems to be sufficient memory
		// we use an IOCache, since that adds caching at the lowest I/O layer
		// and thus dramatically reduces I/O operations and seeks. The
		// disadvantage is that it increases free memory (physical pages)
		// fragmentation, which makes large contiguous allocations more likely
		// to fail.
		size_t freeMemory = vm_page_num_free_pages();
		if (freeMemory > 180 * 1024 * 1024 / B_PAGE_SIZE) {
			info->io_scheduler = new(std::nothrow) IOCache(info->dma_resource,
				1024 * 1024);
		} else {
			dprintf("scsi_cd: Using IOSchedulerSimple instead of IOCache to "
				"avoid memory allocation issues.\n");
			info->io_scheduler = new(std::nothrow) IOSchedulerSimple(
				info->dma_resource);
		}

		if (info->io_scheduler == NULL)
			panic("allocating IOScheduler failed.");

		// TODO: use whole device name here
		status = info->io_scheduler->Init("scsi");
		if (status != B_OK)
			panic("initializing IOScheduler failed: %s", strerror(status));

		info->io_scheduler->SetCallback(do_io, info);
		info->block_size = blockSize;
	}

	if (info->original_capacity != capacity && info->io_scheduler != NULL) {
		info->original_capacity = capacity;

		// For CDs, it's obviously relatively normal that they report a larger
		// capacity than it can actually address. Therefore we'll manually
		// correct the value here.
		test_capacity(info);

		info->io_scheduler->SetDeviceCapacity(info->capacity * blockSize);
	}
}


static void
cd_media_changed(cd_driver_info* info, scsi_ccb* request)
{
	// do a capacity check
	// TODO: is this a good idea (e.g. if this is an empty CD)?
	info->original_capacity = 0;
	info->capacity = 0;
	sSCSIPeripheral->check_capacity(info->scsi_periph_device, request);

	if (info->io_scheduler != NULL)
		info->io_scheduler->MediaChanged();
}


scsi_periph_callbacks callbacks = {
	(void (*)(periph_device_cookie, uint64, uint32))cd_set_capacity,
	(void (*)(periph_device_cookie, scsi_ccb *))cd_media_changed
};


//	#pragma mark - driver module API


static float
cd_supports_device(device_node* parent)
{
	const char* bus;
	uint8 deviceType;

	// make sure parent is really the SCSI bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "scsi"))
		return 0.0;

	// check whether it's really a CD-ROM or WORM
	if (sDeviceManager->get_attr_uint8(parent, SCSI_DEVICE_TYPE_ITEM,
			&deviceType, true) != B_OK
		|| (deviceType != scsi_dev_CDROM && deviceType != scsi_dev_WORM))
		return 0.0;

	return 0.6;
}


/*!	Called whenever a new device was added to system;
	if we really support it, we create a new node that gets
	server by the block_io module
*/
static status_t
cd_register_device(device_node* node)
{
	const scsi_res_inquiry* deviceInquiry = NULL;
	size_t inquiryLength;
	uint32 maxBlocks;

	// get inquiry data
	if (sDeviceManager->get_attr_raw(node, SCSI_DEVICE_INQUIRY_ITEM,
			(const void**)&deviceInquiry, &inquiryLength, true) != B_OK
		|| inquiryLength < sizeof(deviceInquiry))
		return B_ERROR;

	// get block limit of underlying hardware to lower it (if necessary)
	if (sDeviceManager->get_attr_uint32(node, B_DMA_MAX_TRANSFER_BLOCKS,
			&maxBlocks, true) != B_OK)
		maxBlocks = INT_MAX;

	// using 10 byte commands, at most 0xffff blocks can be transmitted at once
	// (sadly, we cannot update this value later on if only 6 byte commands
	//  are supported, but the block_io module can live with that)
	maxBlocks = min_c(maxBlocks, 0xffff);

	// ready to register
	device_attr attrs[] = {
		{"removable", B_UINT8_TYPE, {ui8: deviceInquiry->removable_medium}},
		{B_DMA_MAX_TRANSFER_BLOCKS, B_UINT32_TYPE, {ui32: maxBlocks}},
		{ NULL }
	};

	return sDeviceManager->register_node(node, SCSI_CD_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
cd_init_driver(device_node* node, void** _cookie)
{
	TRACE("cd_init_driver\n");

	uint8 removable;
	status_t status = sDeviceManager->get_attr_uint8(node, "removable",
		&removable, false);
	if (status != B_OK)
		return status;

	cd_driver_info* info = (cd_driver_info*)malloc(sizeof(cd_driver_info));
	if (info == NULL)
		return B_NO_MEMORY;

	memset(info, 0, sizeof(cd_driver_info));

	info->dma_resource = new(std::nothrow) DMAResource;
	if (info->dma_resource == NULL) {
		free(info);
		return B_NO_MEMORY;
	}

	info->node = node;
	info->removable = removable;

	// set capacity to zero, so it get checked on first opened handle
	info->original_capacity = 0;
	info->capacity = 0;
	info->block_size = 0;

	sDeviceManager->get_attr_uint8(node, SCSI_DEVICE_TYPE_ITEM,
		&info->device_type, true);

	device_node *parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info**)&info->scsi,
		(void**)&info->scsi_device);
	sDeviceManager->put_node(parent);

	status = sSCSIPeripheral->register_device((periph_device_cookie)info,
		&callbacks, info->scsi_device, info->scsi, info->node,
		info->removable, 10, &info->scsi_periph_device);
	if (status != B_OK) {
		free(info);
		return status;
	}

	*_cookie = info;
	return B_OK;
}


static void
cd_uninit_driver(void* _cookie)
{
	cd_driver_info* info = (cd_driver_info*)_cookie;

	sSCSIPeripheral->unregister_device(info->scsi_periph_device);
	free(info);
}


static status_t
cd_register_child_devices(void* _cookie)
{
	cd_driver_info* info = (cd_driver_info*)_cookie;

	char* name = sSCSIPeripheral->compose_device_name(info->node, "disk/scsi");
	if (name == NULL)
		return B_ERROR;

	status_t status = sDeviceManager->publish_device(info->node, name,
		SCSI_CD_DEVICE_MODULE_NAME);

	free(name);
	return status;
}


module_dependency module_dependencies[] = {
	{SCSI_PERIPH_MODULE_NAME, (module_info**)&sSCSIPeripheral},
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager},
	{}
};

struct device_module_info sSCSICDDevice = {
	{
		SCSI_CD_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	cd_init_device,
	cd_uninit_device,
	NULL,	// remove,

	cd_open,
	cd_close,
	cd_free,
	cd_read,
	cd_write,
	cd_io,
	cd_ioctl,

	NULL,	// select
	NULL,	// deselect
};

struct driver_module_info sSCSICDDriver = {
	{
		SCSI_CD_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	cd_supports_device,
	cd_register_device,
	cd_init_driver,
	cd_uninit_driver,
	cd_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};

module_info* modules[] = {
	(module_info*)&sSCSICDDriver,
	(module_info*)&sSCSICDDevice,
	NULL
};
