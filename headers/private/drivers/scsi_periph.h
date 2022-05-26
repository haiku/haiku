/*
 * Copyright 2004-2013, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCSI_PERIPH_H
#define _SCSI_PERIPH_H


/*!	Use this module to minimize work required to write a SCSI
	peripheral driver.

	It takes care of:
	- error handling
	- medium changes (including restarting the medium)
	- detection of medium capacity
*/


#include <bus/SCSI.h>
#include <scsi_cmds.h>
#include <Drivers.h>

// cookie issued by module per device
typedef struct scsi_periph_device_info *scsi_periph_device;
// cookie issued by module per file handle
typedef struct scsi_periph_handle_info *scsi_periph_handle;

// "standardized" error code to simplify handling of scsi errors
typedef enum {
	err_act_ok,					// executed successfully
	err_act_retry,				// failed, retry 3 times
	err_act_fail,				// failed, don't retry
	err_act_many_retries,		// failed, retry multiple times (currently 10 times)
	err_act_start,				// devices requires a "start" command
	err_act_invalid_req			// request is invalid
} err_act;

// packed scsi command result
typedef struct err_res {
	status_t error_code : 32;	// Be error code
	uint32 action : 8;			// err_act code
} err_res;

#define MK_ERROR( aaction, code ) ({ \
	err_res _res = {error_code: (code), action: (aaction) };	\
	_res;					\
})

// cookie issued by driver to identify itself
//typedef struct periph_info *periph_cookie;
// cookie issued by driver per device
typedef struct periph_device_info *periph_device_cookie;
// cookie issued by driver per file handle
typedef struct periph_handle_info *periph_handle_cookie;

typedef struct IOOperation io_operation;

// callbacks to be provided by peripheral driver
typedef struct scsi_periph_callbacks {
	// *** block devices ***
	// informs of new size of medium
	// (set to NULL if not a block device)
	void (*set_capacity)(periph_device_cookie cookie, uint64 capacity,
		uint32 blockSize);

	// *** removable devices
	// called when media got changed (can be NULL if medium is not changable)
	// (you don't need to call periph->media_changed, but it's doesn't if you do)
	// ccb - request at your disposal
	void (*media_changed)(periph_device_cookie cookie, scsi_ccb *request);
} scsi_periph_callbacks;

typedef struct scsi_block_range {
	// values are in blocks
	uint64	lba;
	uint64	size;
} scsi_block_range;

// functions provided by this module
typedef struct scsi_periph_interface {
	module_info info;

	// *** init/cleanup ***
	// preferred_ccb_size - preferred command size; if zero, the shortest is used
	status_t (*register_device)(periph_device_cookie cookie,
		scsi_periph_callbacks *callbacks, scsi_device scsiDevice,
		scsi_device_interface *scsi, device_node *node, bool removable,
		int preferredCcbSize, scsi_periph_device *driver);
	status_t (*unregister_device)(scsi_periph_device driver);

	// *** basic command execution ***
	// exec command, retrying on problems
	status_t (*safe_exec)(scsi_periph_device periphCookie, scsi_ccb *request);
	// exec simple command
	status_t (*simple_exec)(scsi_periph_device device, void *cdb,
		uint8 cdbLength, void *data, size_t dataLength, int ccbFlags);

	// *** file handling ***
	// to be called when a new file is opened
	status_t (*handle_open)(scsi_periph_device device,
		periph_handle_cookie periph_handle, scsi_periph_handle *_handle);
	// to be called when a file is closed
	status_t (*handle_close)(scsi_periph_handle handle);
	// to be called when a file is freed
	status_t (*handle_free)(scsi_periph_handle handle);

	// *** default implementation for block devices ***
	status_t (*read_write)(scsi_periph_device_info *device, scsi_ccb *request,
		uint64 offset, size_t numBlocks, physical_entry* vecs, size_t vecCount,
		bool isWrite, size_t* _bytesTransferred);
	status_t (*io)(scsi_periph_device device, io_operation *operation,
		size_t *_bytesTransferred);

	// block ioctls
	status_t (*ioctl)(scsi_periph_handle handle, int op, void *buffer,
		size_t length);
	// check medium capacity (calls set_capacity callback on success)
	// request - ccb for this device; is used to talk to device
	status_t (*check_capacity)(scsi_periph_device device, scsi_ccb *request);

	// synchronizes (flush) the device cache
	err_res (*synchronize_cache)(scsi_periph_device device, scsi_ccb *request);

	status_t (*trim_device)(scsi_periph_device_info *device, scsi_ccb *request,
		scsi_block_range* ranges, uint32 rangeCount, uint64* trimmedBlocks);

	// *** removable media ***
	// to be called when a medium change is detected to block subsequent commands
	void (*media_changed)(scsi_periph_device device);
	// convert result of *request to err_res
	err_res (*check_error)(scsi_periph_device device, scsi_ccb *request);
	// send start or stop command to device
	// withLoadEject = true - include loading/ejecting,
	//   false - only do allow/deny
	err_res (*send_start_stop)(scsi_periph_device device, scsi_ccb *request,
		bool start, bool withLoadEject);
	// checks media status and waits for device to become ready
	// returns: B_OK, B_DEV_MEDIA_CHANGE_REQUESTED, B_NO_MEMORY or
	// pending error reported by handle_get_error
	status_t (*get_media_status)(scsi_periph_handle handle);

	// compose device name consisting of prefix and path/target/LUN
	// (result must be freed by caller)
	char *(*compose_device_name)(device_node *device_node, const char *prefix);
} scsi_periph_interface;

#define SCSI_PERIPH_MODULE_NAME "generic/scsi_periph/v1"

#endif	/* _SCSI_PERIPH_H */
