/*
 * Copyright 2008-2023, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Augustin Cavalier <waddlesplash>
 */


#include "usb_disk.h"

#include <ByteOrder.h>
#include <StackOrHeapArray.h>
#include <Drivers.h>
#include <bus/USB.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel.h>
#include <fs/devfs.h>
#include <syscall_restart.h>
#include <util/AutoLock.h>

#include "IORequest.h"

#include "scsi_sense.h"
#include "usb_disk_scsi.h"
#include "icons.h"


#define MAX_IO_BLOCKS					(128)

#define USB_DISK_DEVICE_MODULE_NAME		"drivers/disk/usb_disk/device_v1"
#define USB_DISK_DRIVER_MODULE_NAME		"drivers/disk/usb_disk/driver_v1"
#define USB_DISK_DEVICE_ID_GENERATOR	"usb_disk/device_id"

#define DRIVER_NAME			"usb_disk"
#define DEVICE_NAME_BASE	"disk/usb/"
#define DEVICE_NAME			DEVICE_NAME_BASE "%" B_PRIu32 "/%d/raw"


//#define TRACE_USB_DISK
#ifdef TRACE_USB_DISK
#define TRACE(x...)			dprintf(DRIVER_NAME ": " x)
#define TRACE_ALWAYS(x...)	dprintf(DRIVER_NAME ": " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)
#else
#define TRACE(x...)			/* nothing */
#define CALLED()
#define TRACE_ALWAYS(x...)	dprintf(DRIVER_NAME ": " x)
#endif


device_manager_info *gDeviceManager;
static usb_module_info *gUSBModule = NULL;

struct {
	const char *vendor;
	const char *product;
	device_icon *icon;
	const char *name;
} kIconMatches[] = {
	// matches for Hama USB 2.0 Card Reader 35 in 1
	// vendor: "Transcend Information, Inc."
	// product: "63-in-1 Multi-Card Reader/Writer" ver. 0100
	// which report things like "Generic " "USB  CF Reader  "
//	{ NULL, " CF Reader", &kCFIconData, "devices/drive-removable-media-flash" },
	{ NULL, " SD Reader", &kSDIconData, "devices/drive-removable-media-flash" },
	{ NULL, " MS Reader", &kMSIconData, "devices/drive-removable-media-flash" },
//	{ NULL, " SM Reader", &kSMIconData, "devices/drive-removable-media-flash" },
	// match for my Kazam mobile phone
	// stupid thing says "MEDIATEK" " FLASH DISK     " even for internal memory
	{ "MEDIATEK", NULL, &kMobileIconData,
		"devices/drive-removable-media-flash" },
	{ NULL, NULL, NULL, NULL }
};


//
//#pragma mark - Forward Declarations
//


static void	usb_disk_callback(void *cookie, status_t status, void *data,
				size_t actualLength);

uint8		usb_disk_get_max_lun(disk_device *device);
void		usb_disk_reset_recovery(disk_device *device);
status_t	usb_disk_receive_csw(disk_device *device,
				usb_massbulk_command_status_wrapper *status);

status_t	usb_disk_send_diagnostic(device_lun *lun);
status_t	usb_disk_request_sense(device_lun *lun, err_act *action);
status_t	usb_disk_mode_sense(device_lun *lun);
status_t	usb_disk_test_unit_ready(device_lun *lun, err_act *action = NULL);
status_t	usb_disk_inquiry(device_lun *lun);
status_t	usb_disk_reset_capacity(device_lun *lun);
status_t	usb_disk_update_capacity(device_lun *lun);
status_t	usb_disk_synchronize(device_lun *lun, bool force);


// #pragma mark - disk_device helper functions


disk_device_s::disk_device_s()
	:
	notify(-1),
	interruptLock(-1)
{
	recursive_lock_init(&io_lock, "usb_disk i/o lock");
	mutex_init(&lock, "usb_disk device lock");
}


disk_device_s::~disk_device_s()
{
	recursive_lock_destroy(&io_lock);
	mutex_destroy(&lock);

	if (notify >= 0)
		delete_sem(notify);
	if (interruptLock >= 0)
		delete_sem(interruptLock);
}


static DMAResource*
get_dma_resource(disk_device *device, uint32 blockSize)
{
	for (int32 i = 0; i < device->dma_resources.Count(); i++) {
		DMAResource* r = device->dma_resources[i];
		if (r->BlockSize() == blockSize)
			return r;
	}
	return NULL;
}


void
usb_disk_free_device_and_luns(disk_device *device)
{
	ASSERT_LOCKED_MUTEX(&device->lock);

	for (int32 i = 0; i < device->dma_resources.Count(); i++)
		delete device->dma_resources[i];
	for (uint8 i = 0; i < device->lun_count; i++)
		free(device->luns[i]);
	free(device->luns);
	delete device;
}


//
//#pragma mark - Bulk-only Mass Storage Functions
//


static status_t
usb_disk_mass_storage_reset(disk_device *device)
{
	return gUSBModule->send_request(device->device, USB_REQTYPE_INTERFACE_OUT
		| USB_REQTYPE_CLASS, USB_MASSBULK_REQUEST_MASS_STORAGE_RESET, 0x0000,
		device->interface, 0, NULL, NULL);
}


uint8
usb_disk_get_max_lun(disk_device *device)
{
	ASSERT_LOCKED_RECURSIVE(&device->io_lock);

	uint8 result = 0;
	size_t actualLength = 0;

	// devices that do not support multiple LUNs may stall this request
	if (gUSBModule->send_request(device->device, USB_REQTYPE_INTERFACE_IN
		| USB_REQTYPE_CLASS, USB_MASSBULK_REQUEST_GET_MAX_LUN, 0x0000,
		device->interface, 1, &result, &actualLength) != B_OK
			|| actualLength != 1) {
		return 0;
	}

	if (result > MAX_LOGICAL_UNIT_NUMBER) {
		// invalid max lun
		return 0;
	}

	return result;
}


static void
usb_disk_clear_halt(usb_pipe pipe)
{
	gUSBModule->cancel_queued_transfers(pipe);
	gUSBModule->clear_feature(pipe, USB_FEATURE_ENDPOINT_HALT);
}


void
usb_disk_reset_recovery(disk_device *device, err_act *_action)
{
	TRACE("reset recovery\n");
	ASSERT_LOCKED_RECURSIVE(&device->io_lock);

	usb_disk_mass_storage_reset(device);
	usb_disk_clear_halt(device->bulk_in);
	usb_disk_clear_halt(device->bulk_out);
	if (device->is_ufi)
		usb_disk_clear_halt(device->interrupt);

	if (_action != NULL)
		*_action = err_act_retry;
}


struct transfer_data {
	union {
		physical_entry* phys_vecs;
		iovec* vecs;
	};
	uint32 vec_count = 0;
	bool physical = false;
};


static status_t
usb_disk_transfer_data(disk_device *device, bool directionIn, const transfer_data& data)
{
	status_t result;
	if (data.physical) {
		result = gUSBModule->queue_bulk_v_physical(
			directionIn ? device->bulk_in : device->bulk_out,
			data.phys_vecs, data.vec_count, usb_disk_callback, device);
	} else {
		result = gUSBModule->queue_bulk_v(
			directionIn ? device->bulk_in : device->bulk_out,
			data.vecs, data.vec_count, usb_disk_callback, device);
	}
	if (result != B_OK) {
		TRACE_ALWAYS("failed to queue data transfer: %s\n", strerror(result));
		return result;
	}

	mutex_unlock(&device->lock);
	do {
		result = acquire_sem_etc(device->notify, 1, B_RELATIVE_TIMEOUT,
			10 * 1000 * 1000);
		if (result == B_TIMED_OUT) {
			// Cancel the transfer and collect the sem that should now be
			// released through the callback on cancel. Handling of device
			// reset is done in usb_disk_operation() when it detects that
			// the transfer failed.
			gUSBModule->cancel_queued_transfers(directionIn ? device->bulk_in
				: device->bulk_out);
			acquire_sem_etc(device->notify, 1, B_RELATIVE_TIMEOUT, 0);
		}
	} while (result == B_INTERRUPTED);
	mutex_lock(&device->lock);

	if (result != B_OK) {
		TRACE_ALWAYS("acquire_sem failed while waiting for data transfer: %s\n",
			strerror(result));
		return result;
	}

	return B_OK;
}


static status_t
usb_disk_transfer_data(disk_device *device, bool directionIn,
	void* buffer, size_t dataLength)
{
	iovec vec;
	vec.iov_base = buffer;
	vec.iov_len = dataLength;

	struct transfer_data data;
	data.vecs = &vec;
	data.vec_count = 1;

	return usb_disk_transfer_data(device, directionIn, data);
}


static void
callback_interrupt(void* cookie, int32 status, void* data, size_t length)
{
	disk_device* device = (disk_device*)cookie;
	// We release the lock even if the interrupt is invalid. This way there
	// is at least a chance for the driver to terminate properly.
	release_sem(device->interruptLock);

	if (length != 2) {
		TRACE_ALWAYS("interrupt of length %" B_PRIuSIZE "! (expected 2)\n",
			length);
		// In this case we do not reschedule the interrupt. This means the
		// driver will be locked. The interrupt should perhaps be scheduled
		// when starting a transfer instead. But getting there means something
		// is really broken, so...
		return;
	}

	// Reschedule the interrupt for next time
	gUSBModule->queue_interrupt(device->interrupt, device->interruptBuffer, 2,
		callback_interrupt, cookie);
}


static status_t
receive_csw_interrupt(disk_device *device,
	interrupt_status_wrapper *status)
{
	TRACE("Waiting for result...\n");

	gUSBModule->queue_interrupt(device->interrupt,
			device->interruptBuffer, 2, callback_interrupt, device);

	acquire_sem(device->interruptLock);

	status->status = device->interruptBuffer[0];
	status->misc = device->interruptBuffer[1];

	return B_OK;
}


static status_t
receive_csw_bulk(disk_device *device,
	usb_massbulk_command_status_wrapper *status)
{
	status_t result = usb_disk_transfer_data(device, true, status,
		sizeof(usb_massbulk_command_status_wrapper));
	if (result != B_OK)
		return result;

	if (device->status != B_OK
			|| device->actual_length
			!= sizeof(usb_massbulk_command_status_wrapper)) {
		// receiving the command status wrapper failed
		return B_ERROR;
	}

	return B_OK;
}


status_t
usb_disk_operation_interrupt(device_lun *lun, uint8* operation,
	const transfer_data& data, size_t *dataLength,
	bool directionIn, err_act *_action)
{
	TRACE("operation: lun: %u; op: 0x%x; data: %p; dlen: %p (%lu); in: %c\n",
		lun->logical_unit_number, operation[0], data.vecs, dataLength,
		dataLength ? *dataLength : 0, directionIn ? 'y' : 'n');
	ASSERT_LOCKED_RECURSIVE(&lun->device->io_lock);

	disk_device* device = lun->device;

	// Step 1 : send the SCSI operation as a class specific request
	size_t actualLength = 12;
	status_t result = gUSBModule->send_request(device->device,
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT, 0 /*request*/,
		0/*value*/, device->interface/*index*/, 12, operation, &actualLength);

	if (result != B_OK || actualLength != 12) {
		TRACE("Command stage: wrote %ld bytes (error: %s)\n",
			actualLength, strerror(result));

		// There was an error, we have to do a request sense to reset the device
		if (operation[0] != SCSI_REQUEST_SENSE_6) {
			usb_disk_request_sense(lun, _action);
		}
		return result;
	}

	// Step 2 : data phase : send or receive data
	size_t transferedData = 0;
	if (data.vec_count != 0) {
		// we have data to transfer in a data stage
		result = usb_disk_transfer_data(device, directionIn, data);
		if (result != B_OK) {
			TRACE("Error %s in data phase\n", strerror(result));
			return result;
		}

		transferedData = device->actual_length;
		if (device->status != B_OK || transferedData != *dataLength) {
			// sending or receiving of the data failed
			if (device->status == B_DEV_STALLED) {
				TRACE("stall while transfering data\n");
				usb_disk_clear_halt(directionIn ? device->bulk_in : device->bulk_out);
			} else {
				TRACE_ALWAYS("sending or receiving of the data failed\n");
				usb_disk_reset_recovery(device, _action);
				return B_IO_ERROR;
			}
		}
	}

	// step 3 : wait for the device to send the interrupt ACK
	if (operation[0] != SCSI_REQUEST_SENSE_6) {
		interrupt_status_wrapper status;
		result =  receive_csw_interrupt(device, &status);
		if (result != B_OK) {
			// in case of a stall or error clear the stall and try again
			TRACE("Error receiving interrupt: %s. Retrying...\n",
				strerror(result));
			usb_disk_clear_halt(device->bulk_in);
			result = receive_csw_interrupt(device, &status);
		}

		if (result != B_OK) {
			TRACE_ALWAYS("receiving the command status interrupt failed\n");
			usb_disk_reset_recovery(device, _action);
			return result;
		}

		// wait for the device to finish the operation.
		result = usb_disk_request_sense(lun, _action);
	}
	return result;
}


status_t
usb_disk_operation_bulk(device_lun *lun, uint8 *operation, size_t operationLength,
	const transfer_data& data, size_t *dataLength,
	bool directionIn, err_act *_action)
{
	TRACE("operation: lun: %u; op: %u; data: %p; dlen: %p (%lu); in: %c\n",
		lun->logical_unit_number, operation[0],
		data.vecs, dataLength, dataLength ? *dataLength : 0,
		directionIn ? 'y' : 'n');
	ASSERT_LOCKED_RECURSIVE(&lun->device->io_lock);

	disk_device *device = lun->device;
	usb_massbulk_command_block_wrapper command;
	command.signature = USB_MASSBULK_CBW_SIGNATURE;
	command.tag = device->current_tag++;
	command.data_transfer_length = (dataLength != NULL ? *dataLength : 0);
	command.flags = (directionIn ? USB_MASSBULK_CBW_DATA_INPUT
		: USB_MASSBULK_CBW_DATA_OUTPUT);
	command.lun = lun->logical_unit_number;
	command.command_block_length
		= device->is_atapi ? ATAPI_COMMAND_LENGTH : operationLength;
	memset(command.command_block, 0, sizeof(command.command_block));
	memcpy(command.command_block, operation, operationLength);

	status_t result = usb_disk_transfer_data(device, false, &command,
		sizeof(usb_massbulk_command_block_wrapper));
	if (result != B_OK)
		return result;

	if (device->status != B_OK ||
		device->actual_length != sizeof(usb_massbulk_command_block_wrapper)) {
		// sending the command block wrapper failed
		TRACE_ALWAYS("sending the command block wrapper failed: %s\n",
			strerror(device->status));
		usb_disk_reset_recovery(device, _action);
		return B_IO_ERROR;
	}

	size_t transferedData = 0;
	if (data.vec_count != 0) {
		// we have data to transfer in a data stage
		result = usb_disk_transfer_data(device, directionIn, data);
		if (result != B_OK)
			return result;

		transferedData = device->actual_length;
		if (device->status != B_OK || transferedData != *dataLength) {
			// sending or receiving of the data failed
			if (device->status == B_DEV_STALLED) {
				TRACE("stall while transfering data\n");
				usb_disk_clear_halt(directionIn ? device->bulk_in : device->bulk_out);
			} else {
				TRACE_ALWAYS("sending or receiving of the data failed: %s\n",
					strerror(device->status));
				usb_disk_reset_recovery(device, _action);
				return B_IO_ERROR;
			}
		}
	}

	usb_massbulk_command_status_wrapper status;
	result =  receive_csw_bulk(device, &status);
	if (result != B_OK) {
		// in case of a stall or error clear the stall and try again
		usb_disk_clear_halt(device->bulk_in);
		result = receive_csw_bulk(device, &status);
	}

	if (result != B_OK) {
		TRACE_ALWAYS("receiving the command status wrapper failed: %s\n",
			strerror(result));
		usb_disk_reset_recovery(device, _action);
		return result;
	}

	if (status.signature != USB_MASSBULK_CSW_SIGNATURE
		|| status.tag != command.tag) {
		// the command status wrapper is not valid
		TRACE_ALWAYS("command status wrapper is not valid: %#" B_PRIx32 "\n",
			status.signature);
		usb_disk_reset_recovery(device, _action);
		return B_ERROR;
	}

	switch (status.status) {
		case USB_MASSBULK_CSW_STATUS_COMMAND_PASSED:
		case USB_MASSBULK_CSW_STATUS_COMMAND_FAILED:
		{
			// The residue from "status.data_residue" is not maintained
			// correctly by some devices, so calculate it instead.
			uint32 residue = command.data_transfer_length - transferedData;

			if (dataLength != NULL) {
				*dataLength -= residue;
				if (transferedData < *dataLength) {
					TRACE_ALWAYS("less data transfered than indicated: %"
						B_PRIuSIZE " vs. %" B_PRIuSIZE "\n", transferedData,
						*dataLength);
					*dataLength = transferedData;
				}
			}

			if (status.status == USB_MASSBULK_CSW_STATUS_COMMAND_PASSED) {
				// the operation is complete and has succeeded
				return B_OK;
			} else {
				if (operation[0] == SCSI_REQUEST_SENSE_6)
					return B_ERROR;

				// the operation is complete but has failed at the SCSI level
				if (operation[0] != SCSI_TEST_UNIT_READY_6) {
					TRACE_ALWAYS("operation %#" B_PRIx8
						" failed at the SCSI level\n", operation[0]);
				}

				result = usb_disk_request_sense(lun, _action);
				return result == B_OK ? B_ERROR : result;
			}
		}

		case USB_MASSBULK_CSW_STATUS_PHASE_ERROR:
		{
			// a protocol or device error occured
			TRACE_ALWAYS("phase error in operation %#" B_PRIx8 "\n",
				operation[0]);
			usb_disk_reset_recovery(device, _action);
			return B_ERROR;
		}

		default:
		{
			// command status wrapper is not meaningful
			TRACE_ALWAYS("command status wrapper has invalid status\n");
			usb_disk_reset_recovery(device, _action);
			return B_ERROR;
		}
	}
}


static status_t
usb_disk_operation(device_lun *lun, uint8* operation, size_t opLength,
	const transfer_data& data, size_t *dataLength,
	bool directionIn, err_act *_action = NULL)
{
	if (lun->device->is_ufi) {
		return usb_disk_operation_interrupt(lun, operation,
			data, dataLength, directionIn, _action);
	} else {
		return usb_disk_operation_bulk(lun, operation, opLength,
			data, dataLength, directionIn, _action);
	}
}


static status_t
usb_disk_operation(device_lun *lun, uint8* operation, size_t opLength,
	void *buffer, size_t *dataLength,
	bool directionIn, err_act *_action = NULL)
{
	iovec vec;
	vec.iov_base = buffer;

	struct transfer_data data;
	data.vecs = &vec;

	if (dataLength != NULL && *dataLength != 0) {
		vec.iov_len = *dataLength;
		data.vec_count = 1;
	} else {
		vec.iov_len = 0;
		data.vec_count = 0;
	}

	return usb_disk_operation(lun, operation, opLength,
		data, dataLength, directionIn, _action);
}


//
//#pragma mark - Helper/Convenience Functions
//


status_t
usb_disk_send_diagnostic(device_lun *lun)
{
	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

	commandBlock[0] = SCSI_SEND_DIAGNOSTIC;
	commandBlock[1] = (lun->logical_unit_number << 5) | 4;

	status_t result = usb_disk_operation(lun, commandBlock, 6, NULL, NULL, false);

	int retry = 100;
	err_act action = err_act_ok;
	while (result == B_DEV_NO_MEDIA && retry > 0) {
		snooze(10000);
		result = usb_disk_request_sense(lun, &action);
		retry--;
	}

	if (result != B_OK)
		TRACE("Send Diagnostic failed: %s\n", strerror(result));
	return result;
}


status_t
usb_disk_request_sense(device_lun *lun, err_act *_action)
{
	size_t dataLength = sizeof(scsi_request_sense_6_parameter);
	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

	commandBlock[0] = SCSI_REQUEST_SENSE_6;
	commandBlock[1] = lun->logical_unit_number << 5;
	commandBlock[2] = 0; // page code
	commandBlock[4] = dataLength;

	scsi_request_sense_6_parameter parameter;
	status_t result = B_ERROR;
	for (uint32 tries = 0; tries < 3; tries++) {
		result = usb_disk_operation(lun, commandBlock, 6, &parameter,
			&dataLength, true);
		if (result != B_TIMED_OUT)
			break;
		snooze(100000);
	}
	if (result != B_OK) {
		TRACE_ALWAYS("getting request sense data failed: %s\n",
			strerror(result));
		return result;
	}

	const char *label = NULL;
	err_act action = err_act_fail;
	status_t status = B_ERROR;
	scsi_get_sense_asc_info((parameter.additional_sense_code << 8)
		| parameter.additional_sense_code_qualifier, &label, &action,
		&status);

	if (parameter.sense_key > SCSI_SENSE_KEY_NOT_READY
		&& parameter.sense_key != SCSI_SENSE_KEY_UNIT_ATTENTION) {
		TRACE_ALWAYS("request_sense: key: 0x%02x; asc: 0x%02x; ascq: "
			"0x%02x; %s\n", parameter.sense_key,
			parameter.additional_sense_code,
			parameter.additional_sense_code_qualifier,
			label ? label : "(unknown)");
	}

	if ((parameter.additional_sense_code == 0
			&& parameter.additional_sense_code_qualifier == 0)
		|| label == NULL) {
		scsi_get_sense_key_info(parameter.sense_key, &label, &action, &status);
	}

	if (status == B_DEV_MEDIA_CHANGED) {
		lun->media_changed = true;
		lun->media_present = true;
	} else if (parameter.sense_key == SCSI_SENSE_KEY_UNIT_ATTENTION
		&& status != B_DEV_NO_MEDIA) {
		lun->media_present = true;
	} else if (status == B_DEV_NOT_READY) {
		lun->media_present = false;
		usb_disk_reset_capacity(lun);
	}

	if (_action != NULL)
		*_action = action;

	return status;
}


status_t
usb_disk_mode_sense(device_lun *lun)
{
	size_t dataLength = sizeof(scsi_mode_sense_6_parameter);

	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

	commandBlock[0] = SCSI_MODE_SENSE_6;
	commandBlock[1] = SCSI_MODE_PAGE_DEVICE_CONFIGURATION;
	commandBlock[2] = 0; // Current values
	commandBlock[3] = dataLength >> 8;
	commandBlock[4] = dataLength;

	scsi_mode_sense_6_parameter parameter;
	status_t result = usb_disk_operation(lun, commandBlock, 6,
		&parameter, &dataLength, true);
	if (result != B_OK) {
		TRACE_ALWAYS("getting mode sense data failed: %s\n", strerror(result));
		return result;
	}

	lun->write_protected
		= (parameter.device_specific & SCSI_DEVICE_SPECIFIC_WRITE_PROTECT)
			!= 0;
	TRACE_ALWAYS("write protected: %s\n", lun->write_protected ? "yes" : "no");
	return B_OK;
}


status_t
usb_disk_test_unit_ready(device_lun *lun, err_act *_action)
{
	// if unsupported we assume the unit is fixed and therefore always ok
	if (lun->device->is_ufi || !lun->device->tur_supported)
		return B_OK;

	status_t result = B_OK;
	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

	if (lun->device->is_atapi) {
		commandBlock[0] = SCSI_START_STOP_UNIT_6;
		commandBlock[1] = lun->logical_unit_number << 5;
		commandBlock[2] = 0;
		commandBlock[3] = 0;
		commandBlock[4] = 1;

		result = usb_disk_operation(lun, commandBlock, 6, NULL, NULL, false,
			_action);
	} else {
		commandBlock[0] = SCSI_TEST_UNIT_READY_6;
		commandBlock[1] = lun->logical_unit_number << 5;
		commandBlock[2] = 0;
		commandBlock[3] = 0;
		commandBlock[4] = 0;
		result = usb_disk_operation(lun, commandBlock, 6, NULL, NULL, true,
			_action);
	}

	if (result == B_DEV_INVALID_IOCTL) {
		lun->device->tur_supported = false;
		return B_OK;
	}

	return result;
}


status_t
usb_disk_inquiry(device_lun *lun)
{
	size_t dataLength = sizeof(scsi_inquiry_6_parameter);

	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

	commandBlock[0] = SCSI_INQUIRY_6;
	commandBlock[1] = lun->logical_unit_number << 5;
	commandBlock[2] = 0; // page code
	commandBlock[4] = dataLength;

	scsi_inquiry_6_parameter parameter;
	status_t result = B_ERROR;
	err_act action = err_act_ok;
	for (uint32 tries = 0; tries < 3; tries++) {
		result = usb_disk_operation(lun, commandBlock, 6, &parameter,
			&dataLength, true, &action);
		if (result == B_OK || (action != err_act_retry
				&& action != err_act_many_retries)) {
			break;
		}
	}
	if (result != B_OK) {
		TRACE_ALWAYS("getting inquiry data failed: %s\n", strerror(result));
		lun->device_type = B_DISK;
		lun->removable = true;
		return result;
	}

	TRACE("peripherial_device_type  0x%02x\n",
		parameter.peripherial_device_type);
	TRACE("peripherial_qualifier    0x%02x\n",
		parameter.peripherial_qualifier);
	TRACE("removable_medium         %s\n",
		parameter.removable_medium ? "yes" : "no");
	TRACE("version                  0x%02x\n", parameter.version);
	TRACE("response_data_format     0x%02x\n", parameter.response_data_format);
	TRACE_ALWAYS("vendor_identification    \"%.8s\"\n",
		parameter.vendor_identification);
	TRACE_ALWAYS("product_identification   \"%.16s\"\n",
		parameter.product_identification);
	TRACE_ALWAYS("product_revision_level   \"%.4s\"\n",
		parameter.product_revision_level);

	memcpy(lun->vendor_name, parameter.vendor_identification,
		MIN(sizeof(lun->vendor_name), sizeof(parameter.vendor_identification)));
	memcpy(lun->product_name, parameter.product_identification,
		MIN(sizeof(lun->product_name),
			sizeof(parameter.product_identification)));
	memcpy(lun->product_revision, parameter.product_revision_level,
		MIN(sizeof(lun->product_revision),
			sizeof(parameter.product_revision_level)));

	lun->device_type = parameter.peripherial_device_type; /* 1:1 mapping */
	lun->removable = (parameter.removable_medium == 1);
	return B_OK;
}


status_t
usb_disk_reset_capacity(device_lun *lun)
{
	lun->block_size = 512;
	lun->block_count = 0;
	return B_OK;
}


static status_t
usb_disk_update_capacity_16(device_lun *lun)
{
	size_t dataLength = sizeof(scsi_read_capacity_16_parameter);
	scsi_read_capacity_16_parameter parameter;
	status_t result = B_ERROR;
	err_act action = err_act_ok;

	uint8 commandBlock[16];
	memset(commandBlock, 0, sizeof(commandBlock));

	commandBlock[0] = SCSI_SERVICE_ACTION_IN;
	commandBlock[1] = SCSI_SAI_READ_CAPACITY_16;
	commandBlock[10] = dataLength >> 24;
	commandBlock[11] = dataLength >> 16;
	commandBlock[12] = dataLength >> 8;
	commandBlock[13] = dataLength;

	// Retry reading the capacity up to three times. The first try might only
	// yield a unit attention telling us that the device or media status
	// changed, which is more or less expected if it is the first operation
	// on the device or the device only clears the unit atention for capacity
	// reads.
	for (int32 i = 0; i < 5; i++) {
		result = usb_disk_operation(lun, commandBlock, 16, &parameter,
			&dataLength, true, &action);

		if (result == B_OK || (action != err_act_retry
				&& action != err_act_many_retries)) {
			break;
		}
	}

	if (result != B_OK) {
		TRACE_ALWAYS("failed to update capacity: %s\n", strerror(result));
		lun->media_present = false;
		lun->media_changed = false;
		usb_disk_reset_capacity(lun);
		return result;
	}

	lun->media_present = true;
	lun->media_changed = false;
	lun->block_size = B_BENDIAN_TO_HOST_INT32(parameter.logical_block_length);
	lun->physical_block_size = lun->block_size;
	lun->block_count =
		B_BENDIAN_TO_HOST_INT64(parameter.last_logical_block_address) + 1;
	return B_OK;
}


status_t
usb_disk_update_capacity(device_lun *lun)
{
	size_t dataLength = sizeof(scsi_read_capacity_10_parameter);
	scsi_read_capacity_10_parameter parameter;
	status_t result = B_ERROR;
	err_act action = err_act_ok;

	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

	commandBlock[0] = SCSI_READ_CAPACITY_10;
	commandBlock[1] = lun->logical_unit_number << 5;

	// Retry reading the capacity up to three times. The first try might only
	// yield a unit attention telling us that the device or media status
	// changed, which is more or less expected if it is the first operation
	// on the device or the device only clears the unit atention for capacity
	// reads.
	for (int32 i = 0; i < 5; i++) {
		result = usb_disk_operation(lun, commandBlock, 10, &parameter,
			&dataLength, true, &action);

		if (result == B_OK || (action != err_act_retry
				&& action != err_act_many_retries)) {
			break;
		}

		// In some cases, it's best to wait a little for the device to settle
		// before retrying.
		if (lun->device->is_ufi && (result == B_DEV_NO_MEDIA
				|| result == B_TIMED_OUT || result == B_DEV_STALLED))
			snooze(10000);
	}

	if (result != B_OK) {
		TRACE_ALWAYS("failed to update capacity: %s\n", strerror(result));
		lun->media_present = false;
		lun->media_changed = false;
		usb_disk_reset_capacity(lun);
		return result;
	}

	lun->media_present = true;
	lun->media_changed = false;
	lun->block_size = B_BENDIAN_TO_HOST_INT32(parameter.logical_block_length);
	lun->physical_block_size = lun->block_size;
	lun->block_count =
		B_BENDIAN_TO_HOST_INT32(parameter.last_logical_block_address) + 1;
	if (lun->block_count == 0) {
		// try SCSI_READ_CAPACITY_16
		result = usb_disk_update_capacity_16(lun);
		if (result != B_OK)
			return result;
	}

	// ensure we have a DMAResource for this block_size
	if (get_dma_resource(lun->device, lun->block_size) == NULL) {
		dma_restrictions restrictions = {};
		restrictions.max_transfer_size = (lun->block_size * MAX_IO_BLOCKS);

		DMAResource* dmaResource = new DMAResource;
		result = dmaResource->Init(restrictions, lun->block_size, 1, 1);
		if (result != B_OK)
			return result;

		lun->device->dma_resources.Add(dmaResource);
	}

	return B_OK;
}


status_t
usb_disk_synchronize(device_lun *lun, bool force)
{
	if (lun->device->is_ufi) {
		// UFI use interrupt because it runs all commands immediately, and
		// tells us when its done. There is no cache involved in that case,
		// so nothing to synchronize.
		return B_UNSUPPORTED;
	}

	if (lun->device->sync_support == 0) {
		// this device reported an illegal request when syncing or repeatedly
		// returned an other error, it apparently does not support syncing...
		return B_UNSUPPORTED;
	}

	if (!lun->should_sync && !force)
		return B_OK;

	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

	commandBlock[0] = SCSI_SYNCHRONIZE_CACHE_10;
	commandBlock[1] = lun->logical_unit_number << 5;

	status_t result = usb_disk_operation(lun, commandBlock, 10,
		NULL, NULL, false);

	if (result == B_OK) {
		lun->device->sync_support = SYNC_SUPPORT_RELOAD;
		lun->should_sync = false;
		return B_OK;
	}

	if (result == B_DEV_INVALID_IOCTL)
		lun->device->sync_support = 0;
	else
		lun->device->sync_support--;

	return result;
}


//
//#pragma mark - Device Attach/Detach Notifications and Callback
//


static void
usb_disk_callback(void *cookie, status_t status, void *data,
	size_t actualLength)
{
	//TRACE("callback()\n");
	disk_device *device = (disk_device *)cookie;
	device->status = status;
	device->actual_length = actualLength;
	release_sem(device->notify);
}


static status_t
usb_disk_attach(device_node *node, usb_device newDevice, void **cookie)
{
	TRACE("device_added(0x%08" B_PRIx32 ")\n", newDevice);
	disk_device *device = new(std::nothrow) disk_device;
	recursive_lock_lock(&device->io_lock);
	mutex_lock(&device->lock);

	device->node = node;
	device->device = newDevice;
	device->removed = false;
	device->open_count = 0;
	device->interface = 0xff;
	device->current_tag = 0;
	device->sync_support = SYNC_SUPPORT_RELOAD;
	device->tur_supported = true;
	device->is_atapi = false;
	device->is_ufi = false;
	device->luns = NULL;

	// scan through the interfaces to find our bulk-only data interface
	const usb_configuration_info *configuration
		= gUSBModule->get_configuration(newDevice);
	if (configuration == NULL) {
		delete device;
		return B_ERROR;
	}

	for (size_t i = 0; i < configuration->interface_count; i++) {
		usb_interface_info *interface = configuration->interface[i].active;
		if (interface == NULL)
			continue;

		if (interface->descr->interface_class == USB_MASS_STORAGE_DEVICE_CLASS
			&& (((interface->descr->interface_subclass == 0x06 /* SCSI */
					|| interface->descr->interface_subclass == 0x02 /* ATAPI */
					|| interface->descr->interface_subclass == 0x05 /* ATAPI */)
				&& interface->descr->interface_protocol == 0x50 /* bulk-only */)
			|| (interface->descr->interface_subclass == 0x04 /* UFI */
				&& interface->descr->interface_protocol == 0x00))) {

			bool hasIn = false;
			bool hasOut = false;
			bool hasInt = false;
			for (size_t j = 0; j < interface->endpoint_count; j++) {
				usb_endpoint_info *endpoint = &interface->endpoint[j];
				if (endpoint == NULL)
					continue;

				if (!hasIn && (endpoint->descr->endpoint_address
					& USB_ENDPOINT_ADDR_DIR_IN) != 0
					&& endpoint->descr->attributes == USB_ENDPOINT_ATTR_BULK) {
					device->bulk_in = endpoint->handle;
					hasIn = true;
				} else if (!hasOut && (endpoint->descr->endpoint_address
					& USB_ENDPOINT_ADDR_DIR_IN) == 0
					&& endpoint->descr->attributes == USB_ENDPOINT_ATTR_BULK) {
					device->bulk_out = endpoint->handle;
					hasOut = true;
				} else if (!hasInt && (endpoint->descr->endpoint_address
					& USB_ENDPOINT_ADDR_DIR_IN)
					&& endpoint->descr->attributes
					== USB_ENDPOINT_ATTR_INTERRUPT) {
					device->interrupt = endpoint->handle;
					hasInt = true;
				}

				if (hasIn && hasOut && hasInt)
					break;
			}

			if (!(hasIn && hasOut)) {
				// Missing one of the required endpoints, try next interface
				continue;
			}

			device->interface = interface->descr->interface_number;
			device->is_atapi = interface->descr->interface_subclass != 0x06
				&& interface->descr->interface_subclass != 0x04;
			device->is_ufi = interface->descr->interface_subclass == 0x04;

			if (device->is_ufi && !hasInt) {
				// UFI without interrupt endpoint is not possible.
				continue;
			}
			break;
		}
	}

	if (device->interface == 0xff) {
		TRACE_ALWAYS("no valid bulk-only or CBI interface found\n");
		delete device;
		return B_ERROR;
	}

	device->notify = create_sem(0, "usb_disk callback notify");
	if (device->notify < B_OK) {
		status_t result = device->notify;
		delete device;
		return result;
	}

	if (device->is_ufi) {
		device->interruptLock = create_sem(0, "usb_disk interrupt lock");
		if (device->interruptLock < B_OK) {
			status_t result = device->interruptLock;
			delete device;
			return result;
		}
	}

	device->lun_count = usb_disk_get_max_lun(device) + 1;
	device->luns = (device_lun **)malloc(device->lun_count
		* sizeof(device_lun *));
	for (uint8 i = 0; i < device->lun_count; i++)
		device->luns[i] = NULL;

	status_t result = B_OK;

	TRACE_ALWAYS("device reports a lun count of %d\n", device->lun_count);
	for (uint8 i = 0; i < device->lun_count; i++) {
		// create the individual luns present on this device
		device_lun *lun = (device_lun *)malloc(sizeof(device_lun));
		if (lun == NULL) {
			result = B_NO_MEMORY;
			break;
		}

		device->luns[i] = lun;
		lun->device = device;
		lun->logical_unit_number = i;
		lun->should_sync = false;
		lun->media_present = true;
		lun->media_changed = true;

		memset(lun->vendor_name, 0, sizeof(lun->vendor_name));
		memset(lun->product_name, 0, sizeof(lun->product_name));
		memset(lun->product_revision, 0, sizeof(lun->product_revision));

		usb_disk_reset_capacity(lun);

		// initialize this lun
		result = usb_disk_inquiry(lun);

		if (device->is_ufi) {
			// Reset the device
			// If we don't do it all the other commands except inquiry and send
			// diagnostics will be stalled.
			result = usb_disk_send_diagnostic(lun);
		}

		err_act action = err_act_ok;
		for (uint32 tries = 0; tries < 8; tries++) {
			TRACE("usb lun %" B_PRIu8 " inquiry attempt %" B_PRIu32 " begin\n",
				i, tries);
			status_t ready = usb_disk_test_unit_ready(lun, &action);
			if (ready == B_OK || ready == B_DEV_NO_MEDIA
				|| ready == B_DEV_MEDIA_CHANGED) {
				if (lun->device_type == B_CD)
					lun->write_protected = true;
				// TODO: check for write protection; disabled since some
				// devices lock up when getting the mode sense
				else if (/*usb_disk_mode_sense(lun) != B_OK*/true)
					lun->write_protected = false;

				TRACE("usb lun %" B_PRIu8 " ready. write protected = %c%s\n", i,
					lun->write_protected ? 'y' : 'n',
					ready == B_DEV_NO_MEDIA ? " (no media inserted)" : "");

				break;
			}
			TRACE("usb lun %" B_PRIu8 " inquiry attempt %" B_PRIu32 " failed\n",
				i, tries);
			if (action != err_act_retry && action != err_act_many_retries)
				break;
			bigtime_t snoozeTime = 1000000 * tries;
			TRACE("snoozing %" B_PRIu64 " microseconds for usb lun\n",
				snoozeTime);
			snooze(snoozeTime);
		}

		if (result != B_OK)
			break;
	}

	if (result != B_OK) {
		TRACE_ALWAYS("failed to initialize logical units: %s\n",
			strerror(result));
		usb_disk_free_device_and_luns(device);
		return result;
	}

	mutex_unlock(&device->lock);
	recursive_lock_unlock(&device->io_lock);

	TRACE("new device: 0x%p\n", device);
	*cookie = (void *)device;
	return B_OK;
}


static void
usb_disk_device_removed(void *cookie)
{
	TRACE("device_removed(0x%p)\n", cookie);
	disk_device *device = (disk_device *)cookie;
	mutex_lock(&device->lock);

	for (uint8 i = 0; i < device->lun_count; i++)
		gDeviceManager->unpublish_device(device->node, device->luns[i]->name);

	device->removed = true;
	gUSBModule->cancel_queued_transfers(device->bulk_in);
	gUSBModule->cancel_queued_transfers(device->bulk_out);
	if (device->open_count == 0)
		usb_disk_free_device_and_luns(device);
	else
		mutex_unlock(&device->lock);
}


static bool
usb_disk_needs_bounce(device_lun *lun, io_request *request)
{
	if (!request->Buffer()->IsVirtual())
		return true;
	if ((request->Offset() % lun->block_size) != 0)
		return true;
	if ((request->Length() % lun->block_size) != 0)
		return true;
	if (request->Length() > (lun->block_size * MAX_IO_BLOCKS))
		return true;
	return false;
}


static status_t
usb_disk_block_read(device_lun *lun, uint64 blockPosition, size_t blockCount,
	struct transfer_data data, size_t *length)
{
	uint8 commandBlock[16];
	memset(commandBlock, 0, sizeof(commandBlock));
	if (lun->device->is_ufi) {
		commandBlock[0] = SCSI_READ_12;
		commandBlock[1] = lun->logical_unit_number << 5;
		commandBlock[2] = blockPosition >> 24;
		commandBlock[3] = blockPosition >> 16;
		commandBlock[4] = blockPosition >> 8;
		commandBlock[5] = blockPosition;
		commandBlock[6] = blockCount >> 24;
		commandBlock[7] = blockCount >> 16;
		commandBlock[8] = blockCount >> 8;
		commandBlock[9] = blockCount;

		status_t result = B_OK;
		for (int tries = 0; tries < 5; tries++) {
			result = usb_disk_operation(lun, commandBlock, 12, data,
				length, true);
			if (result == B_OK)
				break;
			else
				snooze(10000);
		}
		return result;
	} else if (blockPosition + blockCount < 0x100000000LL && blockCount <= 0x10000) {
		commandBlock[0] = SCSI_READ_10;
		commandBlock[1] = 0;
		commandBlock[2] = blockPosition >> 24;
		commandBlock[3] = blockPosition >> 16;
		commandBlock[4] = blockPosition >> 8;
		commandBlock[5] = blockPosition;
		commandBlock[7] = blockCount >> 8;
		commandBlock[8] = blockCount;
		status_t result = usb_disk_operation(lun, commandBlock, 10,
			data, length, true);
		return result;
	} else {
		commandBlock[0] = SCSI_READ_16;
		commandBlock[1] = 0;
		commandBlock[2] = blockPosition >> 56;
		commandBlock[3] = blockPosition >> 48;
		commandBlock[4] = blockPosition >> 40;
		commandBlock[5] = blockPosition >> 32;
		commandBlock[6] = blockPosition >> 24;
		commandBlock[7] = blockPosition >> 16;
		commandBlock[8] = blockPosition >> 8;
		commandBlock[9] = blockPosition;
		commandBlock[10] = blockCount >> 24;
		commandBlock[11] = blockCount >> 16;
		commandBlock[12] = blockCount >> 8;
		commandBlock[13] = blockCount;
		status_t result = usb_disk_operation(lun, commandBlock, 16,
			data, length, true);
		return result;
	}
}


static status_t
usb_disk_block_write(device_lun *lun, uint64 blockPosition, size_t blockCount,
	struct transfer_data data, size_t *length)
{
	uint8 commandBlock[16];
	memset(commandBlock, 0, sizeof(commandBlock));

	if (lun->device->is_ufi) {
		commandBlock[0] = SCSI_WRITE_12;
		commandBlock[1] = lun->logical_unit_number << 5;
		commandBlock[2] = blockPosition >> 24;
		commandBlock[3] = blockPosition >> 16;
		commandBlock[4] = blockPosition >> 8;
		commandBlock[5] = blockPosition;
		commandBlock[6] = blockCount >> 24;
		commandBlock[7] = blockCount >> 16;
		commandBlock[8] = blockCount >> 8;
		commandBlock[9] = blockCount;

		status_t result;
		result = usb_disk_operation(lun, commandBlock, 12,
			data, length, false);

		int retry = 10;
		err_act action = err_act_ok;
		while (result == B_DEV_NO_MEDIA && retry > 0) {
			snooze(10000);
			result = usb_disk_request_sense(lun, &action);
			retry--;
		}

		if (result == B_OK)
			lun->should_sync = true;
		return result;
	} else if (blockPosition + blockCount < 0x100000000LL && blockCount <= 0x10000) {
		commandBlock[0] = SCSI_WRITE_10;
		commandBlock[2] = blockPosition >> 24;
		commandBlock[3] = blockPosition >> 16;
		commandBlock[4] = blockPosition >> 8;
		commandBlock[5] = blockPosition;
		commandBlock[7] = blockCount >> 8;
		commandBlock[8] = blockCount;
		status_t result = usb_disk_operation(lun, commandBlock, 10,
			data, length, false);
		if (result == B_OK)
			lun->should_sync = true;
		return result;
	} else {
		commandBlock[0] = SCSI_WRITE_16;
		commandBlock[1] = 0;
		commandBlock[2] = blockPosition >> 56;
		commandBlock[3] = blockPosition >> 48;
		commandBlock[4] = blockPosition >> 40;
		commandBlock[5] = blockPosition >> 32;
		commandBlock[6] = blockPosition >> 24;
		commandBlock[7] = blockPosition >> 16;
		commandBlock[8] = blockPosition >> 8;
		commandBlock[9] = blockPosition;
		commandBlock[10] = blockCount >> 24;
		commandBlock[11] = blockCount >> 16;
		commandBlock[12] = blockCount >> 8;
		commandBlock[13] = blockCount;
		status_t result = usb_disk_operation(lun, commandBlock, 16,
			data, length, false);
		if (result == B_OK)
			lun->should_sync = true;
		return result;
	}
}


//
//#pragma mark - Driver Hooks
//


static status_t
usb_disk_init_device(void* _info, void** _cookie)
{
	CALLED();
	*_cookie = _info;
	return B_OK;
}


static void
usb_disk_uninit_device(void* _cookie)
{
	// Nothing to do.
}


static status_t
usb_disk_open(void *deviceCookie, const char *path, int flags, void **_cookie)
{
	TRACE("open(%s)\n", path);
	if (strncmp(path, DEVICE_NAME_BASE, strlen(DEVICE_NAME_BASE)) != 0)
		return B_NAME_NOT_FOUND;

	int32 lastPart = 0;
	size_t nameLength = strlen(path);
	for (int32 i = nameLength - 1; i >= 0; i--) {
		if (path[i] == '/') {
			lastPart = i;
			break;
		}
	}

	char rawName[nameLength + 4];
	strncpy(rawName, path, lastPart + 1);
	rawName[lastPart + 1] = 0;
	strcat(rawName, "raw");

	disk_device *device = (disk_device *)deviceCookie;
	MutexLocker locker(device->lock);
	for (uint8 i = 0; i < device->lun_count; i++) {
		device_lun *lun = device->luns[i];
		if (strncmp(rawName, lun->name, 32) == 0) {
			// found the matching device/lun
			if (device->removed)
				return B_ERROR;

			device->open_count++;
			*_cookie = lun;
			return B_OK;
		}
	}

	return B_NAME_NOT_FOUND;
}


static status_t
usb_disk_close(void *cookie)
{
	TRACE("close()\n");
	device_lun *lun = (device_lun *)cookie;
	disk_device *device = lun->device;

	RecursiveLocker ioLocker(device->io_lock);
	MutexLocker deviceLocker(device->lock);

	if (!device->removed)
		usb_disk_synchronize(lun, false);

	return B_OK;
}


static status_t
usb_disk_free(void *cookie)
{
	TRACE("free()\n");

	device_lun *lun = (device_lun *)cookie;
	disk_device *device = lun->device;
	mutex_lock(&device->lock);

	device->open_count--;
	if (device->open_count == 0 && device->removed) {
		// we can simply free the device here as it has been removed from
		// the device list in the device removed notification hook
		usb_disk_free_device_and_luns(device);
	} else {
		mutex_unlock(&device->lock);
	}

	return B_OK;
}


static inline void
normalize_name(char *name, size_t nameLength)
{
	bool wasSpace = false;
	size_t insertIndex = 0;
	for (size_t i = 0; i < nameLength; i++) {
		bool isSpace = name[i] == ' ';
		if (isSpace && wasSpace)
			continue;

		name[insertIndex++] = name[i];
		wasSpace = isSpace;
	}

	if (insertIndex > 0 && name[insertIndex - 1] == ' ')
		insertIndex--;

	name[insertIndex] = 0;
}


static status_t
acquire_io_lock(disk_device *device, MutexLocker& locker, RecursiveLocker& ioLocker)
{
	locker.Unlock();
	ioLocker.SetTo(device->io_lock, false, true);
	locker.Lock();

	if (!locker.IsLocked() || !ioLocker.IsLocked())
		return B_ERROR;

	if (device->removed)
		return B_DEV_NOT_READY;

	return B_OK;
}


static status_t
handle_media_change(device_lun *lun, MutexLocker& locker)
{
	RecursiveLocker ioLocker;
	status_t result = acquire_io_lock(lun->device, locker, ioLocker);
	if (result != B_OK)
		return result;

	// It may have been handled while we were waiting for locks.
	if (lun->media_changed) {
		result = usb_disk_update_capacity(lun);
		if (result != B_OK)
			return result;
	}

	return B_OK;
}


static status_t
usb_disk_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	device_lun *lun = (device_lun *)cookie;
	disk_device *device = lun->device;
	MutexLocker locker(&device->lock);
	if (device->removed)
		return B_DEV_NOT_READY;
	RecursiveLocker ioLocker;

	switch (op) {
		case B_GET_DEVICE_SIZE:
		{
			if (lun->media_changed) {
				status_t result = handle_media_change(lun, locker);
				if (result != B_OK)
					return result;
			}

			size_t size = lun->block_size * lun->block_count;
			return user_memcpy(buffer, &size, sizeof(size));
		}

		case B_GET_MEDIA_STATUS:
		{
			status_t result = acquire_io_lock(lun->device, locker, ioLocker);
			if (result != B_OK)
				return result;

			err_act action = err_act_ok;
			status_t ready;
			for (uint32 tries = 0; tries < 3; tries++) {
				ready = usb_disk_test_unit_ready(lun, &action);
				if (ready == B_OK || ready == B_DEV_NO_MEDIA
					|| (action != err_act_retry
						&& action != err_act_many_retries)) {
					if (IS_USER_ADDRESS(buffer)) {
						if (user_memcpy(buffer, &ready, sizeof(status_t)) != B_OK)
							return B_BAD_ADDRESS;
					} else if (is_called_via_syscall()) {
						return B_BAD_ADDRESS;
					} else
						*(status_t *)buffer = ready;
					break;
				}
				snooze(500000);
			}
			TRACE("B_GET_MEDIA_STATUS: 0x%08" B_PRIx32 "\n", ready);
			return B_OK;
		}

		case B_GET_GEOMETRY:
		{
			if (buffer == NULL || length > sizeof(device_geometry))
				return B_BAD_VALUE;
			if (lun->media_changed) {
				status_t result = handle_media_change(lun, locker);
				if (result != B_OK)
					return result;
			}

			device_geometry geometry;
			devfs_compute_geometry_size(&geometry, lun->block_count,
				lun->block_size);
			geometry.bytes_per_physical_sector = lun->physical_block_size;

			geometry.device_type = lun->device_type;
			geometry.removable = lun->removable;
			geometry.read_only = lun->write_protected;
			geometry.write_once = lun->device_type == B_WORM;
			TRACE("B_GET_GEOMETRY: %" B_PRId32 " sectors at %" B_PRId32
				" bytes per sector\n", geometry.cylinder_count,
				geometry.bytes_per_sector);
			return user_memcpy(buffer, &geometry, length);
		}

		case B_FLUSH_DRIVE_CACHE:
		{
			TRACE("B_FLUSH_DRIVE_CACHE\n");

			status_t result = acquire_io_lock(lun->device, locker, ioLocker);
			if (result != B_OK)
				return result;

			return usb_disk_synchronize(lun, true);
		}

		case B_EJECT_DEVICE:
		{
			status_t result = acquire_io_lock(lun->device, locker, ioLocker);
			if (result != B_OK)
				return result;

			uint8 commandBlock[12];
			memset(commandBlock, 0, sizeof(commandBlock));

			commandBlock[0] = SCSI_START_STOP_UNIT_6;
			commandBlock[1] = lun->logical_unit_number << 5;
			commandBlock[4] = 2;

			return usb_disk_operation(lun, commandBlock, 6, NULL, NULL,
				false);
		}

		case B_LOAD_MEDIA:
		{
			status_t result = acquire_io_lock(lun->device, locker, ioLocker);
			if (result != B_OK)
				return result;

			uint8 commandBlock[12];
			memset(commandBlock, 0, sizeof(commandBlock));

			commandBlock[0] = SCSI_START_STOP_UNIT_6;
			commandBlock[1] = lun->logical_unit_number << 5;
			commandBlock[4] = 3;

			return usb_disk_operation(lun, commandBlock, 6, NULL, NULL,
				false);
		}

		case B_GET_ICON:
			// We don't support this legacy ioctl anymore, but the two other
			// icon ioctls below instead.
			break;

		case B_GET_ICON_NAME:
		{
			const char *iconName = "devices/drive-removable-media-usb";
			char vendor[sizeof(lun->vendor_name)+1];
			char product[sizeof(lun->product_name)+1];

			if (device->is_ufi) {
				iconName = "devices/drive-floppy-usb";
			}

			switch (lun->device_type) {
				case B_CD:
				case B_OPTICAL:
					iconName = "devices/drive-optical";
					break;
				case B_TAPE:	// TODO
				default:
					snprintf(vendor, sizeof(vendor), "%.8s",
						lun->vendor_name);
					snprintf(product, sizeof(product), "%.16s",
						lun->product_name);
					for (int i = 0; kIconMatches[i].icon; i++) {
						if (kIconMatches[i].vendor != NULL
							&& strstr(vendor, kIconMatches[i].vendor) == NULL)
							continue;
						if (kIconMatches[i].product != NULL
							&& strstr(product, kIconMatches[i].product) == NULL)
							continue;
						iconName = kIconMatches[i].name;
					}
					break;
			}
			return user_strlcpy((char *)buffer, iconName,
				B_FILE_NAME_LENGTH);
		}

		case B_GET_VECTOR_ICON:
		{
			device_icon *icon = &kKeyIconData;
			char vendor[sizeof(lun->vendor_name)+1];
			char product[sizeof(lun->product_name)+1];

			if (length != sizeof(device_icon))
				return B_BAD_VALUE;

			if (device->is_ufi) {
				// UFI is specific for floppy drives
				icon = &kFloppyIconData;
			} else {
				switch (lun->device_type) {
					case B_CD:
					case B_OPTICAL:
						icon = &kCDIconData;
						break;
					case B_TAPE:	// TODO
					default:
						snprintf(vendor, sizeof(vendor), "%.8s",
								lun->vendor_name);
						snprintf(product, sizeof(product), "%.16s",
								lun->product_name);
						for (int i = 0; kIconMatches[i].icon; i++) {
							if (kIconMatches[i].vendor != NULL
									&& strstr(vendor,
										kIconMatches[i].vendor) == NULL)
								continue;
							if (kIconMatches[i].product != NULL
									&& strstr(product,
										kIconMatches[i].product) == NULL)
								continue;
							icon = kIconMatches[i].icon;
						}
						break;
				}
			}

			device_icon iconData;
			if (user_memcpy(&iconData, buffer, sizeof(device_icon)) != B_OK)
				return B_BAD_ADDRESS;

			if (iconData.icon_size >= icon->icon_size) {
				if (user_memcpy(iconData.icon_data, icon->icon_data,
						(size_t)icon->icon_size) != B_OK)
					return B_BAD_ADDRESS;
			}

			iconData.icon_size = icon->icon_size;
			return user_memcpy(buffer, &iconData, sizeof(device_icon));
		}

		case B_GET_DEVICE_NAME:
		{
			size_t nameLength = sizeof(lun->vendor_name)
				+ sizeof(lun->product_name) + sizeof(lun->product_revision) + 3;

			char name[nameLength];
			snprintf(name, nameLength, "%.8s %.16s %.4s", lun->vendor_name,
				lun->product_name, lun->product_revision);

			normalize_name(name, nameLength);

			status_t result = user_strlcpy((char *)buffer, name, length);
			if (result > 0)
				result = B_OK;

			TRACE_ALWAYS("got device name \"%s\": %s\n", name,
				strerror(result));
			return result;
		}
	}

	TRACE_ALWAYS("unhandled ioctl %" B_PRId32 "\n", op);
	return B_DEV_INVALID_IOCTL;
}


static status_t
usb_disk_bounced_io(device_lun *lun, io_request *request)
{
	DMAResource* dmaResource = get_dma_resource(lun->device, lun->block_size);
	if (dmaResource == NULL)
		return B_NO_INIT;

	if (!request->Buffer()->IsPhysical()) {
		status_t status = request->Buffer()->LockMemory(request->TeamID(), request->IsWrite());
		if (status != B_OK) {
			TRACE_ALWAYS("failed to lock memory: %s\n", strerror(status));
			return status;
		}
		// SetStatusAndNotify() takes care of unlocking memory if necessary.
	}

	status_t status = B_OK;
	while (request->RemainingBytes() > 0) {
		IOOperation operation;
		status = dmaResource->TranslateNext(request, &operation, 0);
		if (status != B_OK)
			break;

		do {
			TRACE("%p: IOO offset: %" B_PRIdOFF ", length: %" B_PRIuGENADDR
				", write: %s\n", request, operation.Offset(),
				operation.Length(), operation.IsWrite() ? "yes" : "no");

			struct transfer_data data;
			data.physical = true;
			data.phys_vecs = (physical_entry*)operation.Vecs();
			data.vec_count = operation.VecCount();

			size_t length = operation.Length();
			const uint64 blockPosition = operation.Offset() / lun->block_size;
			const size_t blockCount = length / lun->block_size;
			if (operation.IsWrite()) {
				status = usb_disk_block_write(lun,
					blockPosition, blockCount, data, &length);
			} else {
				status = usb_disk_block_read(lun,
					blockPosition, blockCount, data, &length);
			}

			operation.SetStatus(status, length);
		} while (status == B_OK && !operation.Finish());

		if (status == B_OK && operation.Status() != B_OK) {
			TRACE_ALWAYS("I/O succeeded but IOOperation failed!\n");
			status = operation.Status();
		}

		request->OperationFinished(&operation);
		dmaResource->RecycleBuffer(operation.Buffer());

		TRACE("%p: status %s, remaining bytes %" B_PRIuGENADDR "\n", request,
			strerror(status), request->RemainingBytes());
		if (status != B_OK)
			break;
	}

	return status;
}


static status_t
usb_disk_direct_io(device_lun *lun, io_request *request)
{
	generic_io_vec* genericVecs = request->Buffer()->Vecs();
	const uint32 count = request->Buffer()->VecCount();
	BStackOrHeapArray<iovec, 16> vecs(count);
	for (uint32 i = 0; i < count; i++) {
		vecs[i].iov_base = (void*)genericVecs[i].base;
		vecs[i].iov_len = genericVecs[i].length;
	}
	struct transfer_data data;
	data.vecs = vecs;
	data.vec_count = count;

	size_t length = request->Length();
	const uint64 blockPosition = request->Offset() / lun->block_size;
	const size_t blockCount = length / lun->block_size;

	status_t status;
	if (request->IsWrite()) {
		 status = usb_disk_block_write(lun,
			blockPosition, blockCount, data, &length);
	} else {
		status = usb_disk_block_read(lun,
			blockPosition, blockCount, data, &length);
	}

	request->SetTransferredBytes(length != request->Length(), length);
	return status;
}


static status_t
usb_disk_io(void *cookie, io_request *request)
{
	TRACE("io(%p)\n", request);

	device_lun *lun = (device_lun *)cookie;
	disk_device *device = lun->device;

	RecursiveLocker ioLocker(device->io_lock);
	MutexLocker deviceLocker(device->lock);

	if (device->removed)
		return B_DEV_NOT_READY;

	status_t status;
	if (!usb_disk_needs_bounce(lun, request)) {
		status = usb_disk_direct_io(lun, request);
	} else {
		status = usb_disk_bounced_io(lun, request);
	}

	deviceLocker.Unlock();
	ioLocker.Unlock();

	if (request->Status() > 0)
		request->SetStatusAndNotify(status);
	else
		request->NotifyFinished();
	return status;
}


//	#pragma mark - driver module API


static float
usb_disk_supports_device(device_node *parent)
{
	CALLED();
	const char *bus;

	// make sure parent is really the usb bus manager
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;
	if (strcmp(bus, "usb") != 0)
		return 0.0;

	usb_device device;
	if (gDeviceManager->get_attr_uint32(parent, USB_DEVICE_ID_ITEM, &device, true) != B_OK)
		return -1;

	const usb_configuration_info *configuration = gUSBModule->get_configuration(device);
	if (configuration == NULL)
		return -1;

	static usb_support_descriptor supportedDevices[] = {
		{ USB_MASS_STORAGE_DEVICE_CLASS, 0x06 /* SCSI */, 0x50 /* bulk */, 0, 0 },
		{ USB_MASS_STORAGE_DEVICE_CLASS, 0x02 /* ATAPI */, 0x50 /* bulk */, 0, 0 },
		{ USB_MASS_STORAGE_DEVICE_CLASS, 0x05 /* ATAPI */, 0x50 /* bulk */, 0, 0 },
		{ USB_MASS_STORAGE_DEVICE_CLASS, 0x04 /* UFI */, 0x00, 0, 0 }
	};

	for (size_t i = 0; i < configuration->interface_count; i++) {
		usb_interface_info *interface = configuration->interface[i].active;
		if (interface == NULL)
			continue;

		for (size_t i = 0; i < B_COUNT_OF(supportedDevices); i++) {
			if (interface->descr->interface_class != supportedDevices[i].dev_class)
				continue;
			if (interface->descr->interface_subclass != supportedDevices[i].dev_subclass)
				continue;
			if (interface->descr->interface_protocol != supportedDevices[i].dev_protocol)
				continue;

			TRACE("USB disk device found!\n");
			return 0.6;
		}
	}

	return 0.0;
}


static status_t
usb_disk_register_device(device_node *node)
{
	CALLED();

	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "USB Disk"} },
		{ NULL }
	};

	return gDeviceManager->register_node(node, USB_DISK_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
usb_disk_init_driver(device_node *node, void **cookie)
{
	CALLED();

	usb_device usb_device;
	if (gDeviceManager->get_attr_uint32(node, USB_DEVICE_ID_ITEM, &usb_device, true) != B_OK)
		return B_BAD_VALUE;

	return usb_disk_attach(node, usb_device, cookie);
}


static void
usb_disk_uninit_driver(void *_cookie)
{
	CALLED();
	// Nothing to do.
}


static status_t
usb_disk_register_child_devices(void* _cookie)
{
	CALLED();
	disk_device *device = (disk_device *)_cookie;

	device->number = gDeviceManager->create_id(USB_DISK_DEVICE_ID_GENERATOR);
	if (device->number < 0)
		return device->number;

	status_t status = B_OK;
	for (uint8 i = 0; i < device->lun_count; i++) {
		sprintf(device->luns[i]->name, DEVICE_NAME, device->number, i);
		status = gDeviceManager->publish_device(device->node, device->luns[i]->name,
			USB_DISK_DEVICE_MODULE_NAME);
	}

	return status;
}


//	#pragma mark -


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{ B_USB_MODULE_NAME, (module_info**)&gUSBModule},
	{ NULL }
};

struct device_module_info sUsbDiskDevice = {
	{
		USB_DISK_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	usb_disk_init_device,
	usb_disk_uninit_device,
	usb_disk_device_removed,

	usb_disk_open,
	usb_disk_close,
	usb_disk_free,
	NULL,	// read
	NULL,	// write
	usb_disk_io,
	usb_disk_ioctl,

	NULL,	// select
	NULL,	// deselect
};

struct driver_module_info sUsbDiskDriver = {
	{
		USB_DISK_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	usb_disk_supports_device,
	usb_disk_register_device,
	usb_disk_init_driver,
	usb_disk_uninit_driver,
	usb_disk_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};

module_info* modules[] = {
	(module_info*)&sUsbDiskDriver,
	(module_info*)&sUsbDiskDevice,
	NULL
};
