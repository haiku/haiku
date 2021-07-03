/*
 * Copyright 2004-2013, Haiku, Inc. All Rights Reserved.
 * Copyright 2002, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __SCSI_PERIPH_INT_H__
#define __SCSI_PERIPH_INT_H__


#include <stddef.h>

#include <scsi_periph.h>
#include <device_manager.h>

#include "IORequest.h"
#include "wrapper.h"


enum trim_command {
	TRIM_NONE,			// TRIM operation is disabled for this device
	TRIM_UNMAP,			// UNMAP command wil be used
	TRIM_WRITESAME10,	// WRITE SAME (10) with UNMAP bit enabled will be used
	TRIM_WRITESAME16	// WRITE SAME (16) with UNMAP bit enabled will be used
};


typedef struct scsi_periph_device_info {
	struct scsi_periph_handle_info *handles;

	::scsi_device scsi_device;
	scsi_device_interface *scsi;
	periph_device_cookie periph_device;
	device_node *node;

	bool removal_requested;

	bool removable;			// true, if device is removable

	enum trim_command unmap_command;	// command to be used to discard free blocks
	uint32 max_unmap_lba_count;			// max. number of LBAs in one command
	uint32 max_unmap_descriptor_count;	// max. number of ranges in one command

	uint32 block_size;
	int32 preferred_ccb_size;
	int32 rw10_enabled;		// 10 byte r/w commands supported; access must be atomic
	int32 next_tag_action;	// queuing flag for next r/w command; access must be atomic

	struct mutex mutex;
	int std_timeout;

	scsi_periph_callbacks *callbacks;
} scsi_periph_device_info;

typedef struct scsi_periph_handle_info {
	scsi_periph_device_info *device;
	struct scsi_periph_handle_info *next, *prev;

	int pending_error;		// error to be reported on all accesses
							// (used to block access after medium change until
							//  B_GET_MEDIA_STATUS is called)
	periph_handle_cookie periph_handle;
} scsi_periph_handle_info;

extern device_manager_info *gDeviceManager;


// removable.c

void periph_media_changed(scsi_periph_device_info *device, scsi_ccb *ccb);
void periph_media_changed_public(scsi_periph_device_info *device);
status_t periph_get_media_status(scsi_periph_handle_info *handle);
err_res periph_send_start_stop(scsi_periph_device_info *device, scsi_ccb *request,
	bool start, bool with_LoEj);


// error_handling.c

err_res periph_check_error(scsi_periph_device_info *device, scsi_ccb *request);


// handle.c

status_t periph_handle_open(scsi_periph_device_info *device, periph_handle_cookie periph_handle,
	scsi_periph_handle_info **res_handle);
status_t periph_handle_close(scsi_periph_handle_info *handle);
status_t periph_handle_free(scsi_periph_handle_info *handle);


// block.c

status_t periph_check_capacity(scsi_periph_device_info *device, scsi_ccb *ccb);
status_t periph_trim_device(scsi_periph_device_info *device, scsi_ccb *request,
	scsi_block_range* ranges, uint32 rangeCount, uint64* trimmedBlocks);


// device.c

status_t periph_register_device(periph_device_cookie periph_device,
	scsi_periph_callbacks *callbacks, scsi_device scsi_device,
	scsi_device_interface *scsi, device_node *node, bool removable,
	int preferredCcbSize, scsi_periph_device *driver);
status_t periph_unregister_device(scsi_periph_device_info *driver);
char *periph_compose_device_name(device_node *device_node, const char *prefix);


// io.c

status_t periph_read_write(scsi_periph_device_info *device, scsi_ccb *request,
	uint64 offset, size_t numBlocks, physical_entry* vecs, size_t vecCount,
	bool isWrite, size_t* _bytesTransferred);
status_t periph_io(scsi_periph_device_info* device, io_operation* operation,
	size_t *_bytesTransferred);
status_t periph_ioctl(scsi_periph_handle_info *handle, int op,
	void *buf, size_t len);
void periph_sync_queue_daemon(void *arg, int iteration);
status_t vpd_page_get(scsi_periph_device_info *device, scsi_ccb* request,
	uint8 page, void* data, uint16 length);


// scsi_periph.c

status_t periph_safe_exec(scsi_periph_device_info *device, scsi_ccb *request);
status_t periph_simple_exec(scsi_periph_device_info *device, void *cdb,
	uchar cdb_len, void *data, size_t data_len, int ccb_flags);


// sync.c

err_res periph_synchronize_cache(scsi_periph_device_info *device,
	scsi_ccb *request);

#endif	/* __SCSI_PERIPH_INT_H__ */
