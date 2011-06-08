/*
 * Copyright 2008-2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <ByteOrder.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "usb_disk.h"
#include "usb_disk_scsi.h"


#define DRIVER_NAME			"usb_disk"
#define DEVICE_NAME_BASE	"disk/usb/"
#define DEVICE_NAME			DEVICE_NAME_BASE"%ld/%d/raw"


//#define TRACE_USB_DISK
#ifdef TRACE_USB_DISK
#define TRACE(x...)			dprintf(DRIVER_NAME": "x)
#define TRACE_ALWAYS(x...)	dprintf(DRIVER_NAME": "x)
#else
#define TRACE(x...)			/* nothing */
#define TRACE_ALWAYS(x...)	dprintf(DRIVER_NAME": "x)
#endif


int32 api_version = B_CUR_DRIVER_API_VERSION;
static usb_module_info *gUSBModule = NULL;
static disk_device *gDeviceList = NULL;
static uint32 gDeviceCount = 0;
static uint32 gLunCount = 0;
static mutex gDeviceListLock;
static char **gDeviceNames = NULL;

static uint8 kDeviceIcon[] = {
	0x6e, 0x63, 0x69, 0x66, 0x0a, 0x04, 0x01, 0x73, 0x05, 0x01, 0x02, 0x01,
	0x06, 0x02, 0xb1, 0xf8, 0x5d, 0x3a, 0x2f, 0xbf, 0xbe, 0xdb, 0x67, 0xb6,
	0x98, 0x06, 0x4b, 0x22, 0x15, 0x47, 0x13, 0x02, 0x00, 0xed, 0xed, 0xed,
	0xff, 0xab, 0xbc, 0xc6, 0x02, 0x01, 0x06, 0x02, 0xb9, 0x82, 0x56, 0x32,
	0x7d, 0xfb, 0xb8, 0x06, 0x39, 0xbe, 0xd9, 0xb5, 0x4b, 0x7d, 0x31, 0x4a,
	0xa4, 0xe7, 0x00, 0xd1, 0xde, 0xe4, 0xff, 0x7a, 0x9c, 0xae, 0x02, 0x00,
	0x16, 0x02, 0x38, 0xe9, 0xaa, 0x3b, 0x7b, 0x1d, 0xbf, 0xb0, 0xa6, 0x3d,
	0x16, 0x76, 0x4b, 0x84, 0x81, 0x48, 0x37, 0x36, 0x00, 0x99, 0xff, 0x53,
	0x02, 0x00, 0x16, 0x02, 0xba, 0x38, 0x9a, 0xb8, 0xef, 0x79, 0x3e, 0x34,
	0x8b, 0xbf, 0x56, 0x52, 0x48, 0x2c, 0x61, 0x4c, 0x4e, 0xec, 0x00, 0x40,
	0xff, 0x01, 0x05, 0xff, 0x05, 0x46, 0x02, 0x01, 0x16, 0x02, 0x35, 0xc2,
	0x71, 0x3a, 0xf6, 0x84, 0xb9, 0xf3, 0x5b, 0x34, 0x81, 0xa0, 0x49, 0xc0,
	0x57, 0x49, 0x6e, 0x51, 0xff, 0xf3, 0x00, 0x52, 0x02, 0x01, 0x06, 0x02,
	0x38, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x80, 0x00,
	0x49, 0xa0, 0x00, 0x49, 0x80, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x06,
	0xe3, 0x06, 0x0c, 0x06, 0x09, 0xab, 0xaa, 0x03, 0x3e, 0x5e, 0x3c, 0x5f,
	0x3e, 0x5e, 0x60, 0x4d, 0x5a, 0x4a, 0x60, 0x47, 0x56, 0x42, 0x50, 0x45,
	0x4c, 0x43, 0x2a, 0x54, 0x36, 0x5d, 0x36, 0x5d, 0x3a, 0x60, 0x06, 0x08,
	0xfb, 0xea, 0x27, 0x49, 0x26, 0x48, 0x27, 0x49, 0x32, 0x56, 0x37, 0x59,
	0x37, 0x59, 0x38, 0x5a, 0x3b, 0x59, 0x3a, 0x5a, 0x3b, 0x59, 0x58, 0x3c,
	0x52, 0x36, 0x43, 0x29, 0x27, 0x45, 0x27, 0x45, 0x26, 0x46, 0x06, 0x05,
	0xab, 0x03, 0x27, 0x49, 0x26, 0x48, 0x27, 0x49, 0x32, 0x56, 0x52, 0x36,
	0x43, 0x29, 0x27, 0x45, 0x27, 0x45, 0x26, 0x46, 0x0a, 0x05, 0xc2, 0x1c,
	0xb8, 0xf9, 0x4f, 0x25, 0xc9, 0x4c, 0xb7, 0xc4, 0x5a, 0x30, 0x51, 0x39,
	0x0a, 0x04, 0xc5, 0x50, 0xbb, 0xc0, 0xc2, 0x1c, 0xb8, 0xf9, 0x4f, 0x25,
	0xc9, 0x4c, 0xb7, 0xc4, 0x0a, 0x04, 0x51, 0x39, 0xc5, 0x50, 0xbb, 0xc0,
	0xc9, 0x4c, 0xb7, 0xc4, 0x5a, 0x30, 0x0a, 0x04, 0x4f, 0x2f, 0x51, 0x31,
	0x53, 0x2f, 0x51, 0x2d, 0x06, 0x04, 0xee, 0x4f, 0x35, 0x53, 0x30, 0x51,
	0x32, 0x55, 0x2e, 0x58, 0x2c, 0x54, 0x31, 0x56, 0x2f, 0x52, 0x33, 0x06,
	0x04, 0xee, 0x31, 0x58, 0x40, 0x47, 0x39, 0x4e, 0x47, 0x40, 0x50, 0x38,
	0x41, 0x48, 0x48, 0x41, 0x3a, 0x4f, 0x08, 0x02, 0x3a, 0x40, 0x3e, 0x3c,
	0x02, 0x04, 0x3e, 0x3a, 0xbe, 0x48, 0x3a, 0xbf, 0x9f, 0x3a, 0x41, 0x3d,
	0x41, 0xbd, 0xe2, 0x41, 0xbf, 0x39, 0x3e, 0x40, 0xbf, 0x9f, 0x40, 0xbe,
	0x48, 0x40, 0x3b, 0x3d, 0x3b, 0xbf, 0x39, 0x3b, 0xbd, 0xe2, 0x06, 0x05,
	0xbe, 0x02, 0x32, 0x56, 0x36, 0x5a, 0x36, 0x5a, 0x37, 0x5b, 0x3a, 0x5a,
	0x39, 0x5b, 0x3a, 0x5a, 0x58, 0x3c, 0x52, 0x36, 0x11, 0x0a, 0x00, 0x01,
	0x00, 0x00, 0x0a, 0x01, 0x01, 0x03, 0x12, 0x3f, 0xe9, 0x8e, 0xbb, 0x54,
	0xe2, 0x3b, 0x54, 0xe2, 0x3f, 0xe9, 0x8e, 0xc6, 0x4d, 0xd9, 0x45, 0xc0,
	0xc5, 0x01, 0x17, 0x84, 0x00, 0x04, 0x0a, 0x02, 0x01, 0x04, 0x02, 0x3f,
	0xe9, 0x8e, 0xbb, 0x54, 0xe2, 0x3b, 0x54, 0xe2, 0x3f, 0xe9, 0x8e, 0xc6,
	0x4d, 0xd9, 0x45, 0xc0, 0xc5, 0x0a, 0x03, 0x01, 0x05, 0x02, 0x3f, 0xe9,
	0x8e, 0xbb, 0x54, 0xe2, 0x3b, 0x54, 0xe2, 0x3f, 0xe9, 0x8e, 0xc6, 0x4d,
	0xd9, 0x45, 0xc0, 0xc5, 0x0a, 0x01, 0x01, 0x01, 0x12, 0x3f, 0xe9, 0x8e,
	0xbb, 0x54, 0xe2, 0x3b, 0x54, 0xe2, 0x3f, 0xe9, 0x8e, 0xc6, 0xb0, 0x64,
	0x46, 0x78, 0x3b, 0x01, 0x17, 0x84, 0x00, 0x04, 0x0a, 0x04, 0x01, 0x02,
	0x02, 0x3f, 0xe9, 0x8e, 0xbb, 0x54, 0xe2, 0x3b, 0x54, 0xe2, 0x3f, 0xe9,
	0x8e, 0xc6, 0xb0, 0x64, 0x46, 0x78, 0x3b, 0x0a, 0x05, 0x01, 0x0b, 0x02,
	0x3f, 0xe9, 0x8e, 0xbb, 0x54, 0xe2, 0x3b, 0x54, 0xe2, 0x3f, 0xe9, 0x8e,
	0xc6, 0xb0, 0x64, 0x46, 0x78, 0x3b, 0x0a, 0x01, 0x01, 0x06, 0x02, 0x3f,
	0xe9, 0x8e, 0xbb, 0x54, 0xe2, 0x3b, 0x54, 0xe2, 0x3f, 0xe9, 0x8e, 0xc6,
	0x5b, 0x2d, 0x45, 0x43, 0x93, 0x0a, 0x01, 0x01, 0x06, 0x02, 0x3f, 0xe9,
	0x8e, 0xbb, 0x54, 0xe2, 0x3b, 0x54, 0xe2, 0x3f, 0xe9, 0x8e, 0xc7, 0x7d,
	0x8b, 0x44, 0x36, 0x9a, 0x0a, 0x06, 0x02, 0x07, 0x08, 0x02, 0x3f, 0xe9,
	0x8e, 0xbb, 0x54, 0xe2, 0x3b, 0x54, 0xe2, 0x3f, 0xe9, 0x8e, 0xc6, 0x4d,
	0xd9, 0x45, 0xc0, 0xc5, 0x0a, 0x01, 0x01, 0x09, 0x12, 0x3f, 0x6c, 0x5c,
	0xba, 0xea, 0x46, 0x3a, 0xea, 0x46, 0x3f, 0x6c, 0x5c, 0xc5, 0x19, 0x6c,
	0x46, 0x6b, 0x36, 0x01, 0x17, 0x8c, 0x22, 0x04, 0x0a, 0x07, 0x01, 0x09,
	0x12, 0x3f, 0xe9, 0x8e, 0xbb, 0x54, 0xe2, 0x3b, 0x54, 0xe2, 0x3f, 0xe9,
	0x8e, 0xc6, 0x4d, 0xd9, 0x45, 0xc0, 0xc5, 0x01, 0x17, 0x88, 0x22, 0x04,
	0x0a, 0x08, 0x01, 0x09, 0x12, 0x3f, 0xe9, 0x8e, 0xbb, 0x54, 0xe2, 0x3b,
	0x54, 0xe2, 0x3f, 0xe9, 0x8e, 0xc6, 0x01, 0xed, 0x46, 0x11, 0xa8, 0x01,
	0x17, 0x85, 0x22, 0x04, 0x0a, 0x01, 0x01, 0x0a, 0x12, 0x3f, 0xe9, 0x8e,
	0xbb, 0x54, 0xe2, 0x3a, 0xaa, 0x52, 0x3f, 0x21, 0x43, 0xc6, 0x59, 0xd0,
	0x46, 0xdb, 0x8c, 0x01, 0x17, 0x84, 0x00, 0x04, 0x0a, 0x06, 0x01, 0x0a,
	0x02, 0x3f, 0xe9, 0x8e, 0xbb, 0x54, 0xe2, 0x3b, 0x54, 0xe2, 0x3f, 0xe9,
	0x8e, 0xc6, 0x99, 0xc6, 0x45, 0x5e, 0x3a, 0x0a, 0x01, 0x01, 0x0a, 0x12,
	0x3f, 0x21, 0x43, 0xba, 0xaa, 0x52, 0x3a, 0xaa, 0x52, 0x3f, 0x21, 0x43,
	0xc7, 0xd2, 0xa7, 0x49, 0x5f, 0xed, 0x01, 0x17, 0x84, 0x00, 0x04, 0x0a,
	0x09, 0x01, 0x0a, 0x02, 0x3f, 0xe9, 0x8e, 0xbb, 0x54, 0xe2, 0x3b, 0x54,
	0xe2, 0x3f, 0xe9, 0x8e, 0xc8, 0xa5, 0xc8, 0x48, 0xeb, 0x05
};


//
//#pragma mark - Forward Declarations
//


static void	usb_disk_callback(void *cookie, status_t status, void *data,
				size_t actualLength);

status_t	usb_disk_mass_storage_reset(disk_device *device);
uint8		usb_disk_get_max_lun(disk_device *device);
void		usb_disk_reset_recovery(disk_device *device);
status_t	usb_disk_transfer_data(disk_device *device, bool directionIn,
				void *data, size_t dataLength);
status_t	usb_disk_receive_csw(disk_device *device,
				command_status_wrapper *status);
status_t	usb_disk_operation(device_lun *lun, uint8 operation,
				uint8 opLength, uint32 logicalBlockAddress,
				uint16 transferLength, void *data, uint32 *dataLength,
				bool directionIn);

status_t	usb_disk_request_sense(device_lun *lun);
status_t	usb_disk_mode_sense(device_lun *lun);
status_t	usb_disk_test_unit_ready(device_lun *lun);
status_t	usb_disk_inquiry(device_lun *lun);
status_t	usb_disk_reset_capacity(device_lun *lun);
status_t	usb_disk_update_capacity(device_lun *lun);
status_t	usb_disk_synchronize(device_lun *lun, bool force);


//
//#pragma mark - Device Allocation Helper Functions
//


void
usb_disk_free_device_and_luns(disk_device *device)
{
	mutex_lock(&device->lock);
	mutex_destroy(&device->lock);
	delete_sem(device->notify);
	for (uint8 i = 0; i < device->lun_count; i++)
		free(device->luns[i]);
	free(device->luns);
	free(device);
}


//
//#pragma mark - Bulk-only Mass Storage Functions
//


status_t
usb_disk_mass_storage_reset(disk_device *device)
{
	return gUSBModule->send_request(device->device, USB_REQTYPE_INTERFACE_OUT
		| USB_REQTYPE_CLASS, REQUEST_MASS_STORAGE_RESET, 0x0000,
		device->interface, 0, NULL, NULL);
}


uint8
usb_disk_get_max_lun(disk_device *device)
{
	uint8 result = 0;
	size_t actualLength = 0;

	// devices that do not support multiple LUNs may stall this request
	if (gUSBModule->send_request(device->device, USB_REQTYPE_INTERFACE_IN
		| USB_REQTYPE_CLASS, REQUEST_GET_MAX_LUN, 0x0000, device->interface,
		1, &result, &actualLength) != B_OK || actualLength != 1)
		return 0;

	if (result > MAX_LOGICAL_UNIT_NUMBER) {
		// invalid max lun
		return 0;
	}

	return result;
}


void
usb_disk_reset_recovery(disk_device *device)
{
	usb_disk_mass_storage_reset(device);
	gUSBModule->clear_feature(device->bulk_in, USB_FEATURE_ENDPOINT_HALT);
	gUSBModule->clear_feature(device->bulk_out, USB_FEATURE_ENDPOINT_HALT);
}


status_t
usb_disk_transfer_data(disk_device *device, bool directionIn, void *data,
	size_t dataLength)
{
	status_t result = gUSBModule->queue_bulk(directionIn ? device->bulk_in
		: device->bulk_out, data, dataLength, usb_disk_callback, device);
	if (result != B_OK) {
		TRACE_ALWAYS("failed to queue data transfer\n");
		return result;
	}

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

	if (result != B_OK) {
		TRACE_ALWAYS("acquire_sem failed while waiting for data transfer\n");
		return result;
	}

	return B_OK;
}


status_t
usb_disk_receive_csw(disk_device *device, command_status_wrapper *status)
{
	status_t result = usb_disk_transfer_data(device, true, status,
		sizeof(command_status_wrapper));
	if (result != B_OK)
		return result;

	if (device->status != B_OK
		|| device->actual_length != sizeof(command_status_wrapper)) {
		// receiving the command status wrapper failed
		return B_ERROR;
	}

	return B_OK;
}


status_t
usb_disk_operation(device_lun *lun, uint8 operation, uint8 opLength,
	uint32 logicalBlockAddress, uint16 transferLength, void *data,
	uint32 *dataLength, bool directionIn)
{
	TRACE("operation: lun: %u; op: %u; oplen: %u; lba: %lu; tlen: %u; data: "
		"%p; dlen: %p (%lu); in: %c\n",
		lun->logical_unit_number, operation, opLength, logicalBlockAddress,
		transferLength, data, dataLength, dataLength ? *dataLength : 0,
		directionIn ? 'y' : 'n');

	disk_device *device = lun->device;
	command_block_wrapper command;
	command.signature = CBW_SIGNATURE;
	command.tag = device->current_tag++;
	command.data_transfer_length = (dataLength != NULL ? *dataLength : 0);
	command.flags = (directionIn ? CBW_DATA_INPUT : CBW_DATA_OUTPUT);
	command.lun = lun->logical_unit_number;
	command.command_block_length
		= device->is_atapi ? ATAPI_COMMAND_LENGTH : opLength;
	memset(command.command_block, 0, sizeof(command.command_block));

	switch (opLength) {
		case 6:
		{
			scsi_command_6 *commandBlock
				= (scsi_command_6 *)command.command_block;
			commandBlock->operation = operation;
			commandBlock->lun = lun->logical_unit_number << 5;
			commandBlock->allocation_length = (uint8)transferLength;
			if (operation == SCSI_MODE_SENSE_6) {
				// we hijack the lba argument to transport the desired page
				commandBlock->reserved[1] = (uint8)logicalBlockAddress;
			}
			break;
		}

		case 10:
		{
			scsi_command_10 *commandBlock
				= (scsi_command_10 *)command.command_block;
			commandBlock->operation = operation;
			commandBlock->lun_flags = lun->logical_unit_number << 5;
			commandBlock->logical_block_address = htonl(logicalBlockAddress);
			commandBlock->transfer_length = htons(transferLength);
			break;
		}

		default:
			TRACE_ALWAYS("unsupported operation length %d\n", opLength);
			return B_BAD_VALUE;
	}

	status_t result = usb_disk_transfer_data(device, false, &command,
		sizeof(command_block_wrapper));
	if (result != B_OK)
		return result;

	if (device->status != B_OK ||
		device->actual_length != sizeof(command_block_wrapper)) {
		// sending the command block wrapper failed
		TRACE_ALWAYS("sending the command block wrapper failed\n");
		usb_disk_reset_recovery(device);
		return B_ERROR;
	}

	size_t transferedData = 0;
	if (data != NULL && dataLength != NULL && *dataLength > 0) {
		// we have data to transfer in a data stage
		result = usb_disk_transfer_data(device, directionIn, data,
			*dataLength);
		if (result != B_OK)
			return result;

		transferedData = device->actual_length;
		if (device->status != B_OK || transferedData != *dataLength) {
			// sending or receiving of the data failed
			if (device->status == B_DEV_STALLED) {
				TRACE("stall while transfering data\n");
				gUSBModule->clear_feature(directionIn ? device->bulk_in
					: device->bulk_out, USB_FEATURE_ENDPOINT_HALT);
			} else {
				TRACE_ALWAYS("sending or receiving of the data failed\n");
				usb_disk_reset_recovery(device);
				return B_ERROR;
			}
		}
	}

	command_status_wrapper status;
	result =  usb_disk_receive_csw(device, &status);
	if (result != B_OK) {
		// in case of a stall or error clear the stall and try again
		gUSBModule->clear_feature(device->bulk_in, USB_FEATURE_ENDPOINT_HALT);
		result = usb_disk_receive_csw(device, &status);
	}

	if (result != B_OK) {
		TRACE_ALWAYS("receiving the command status wrapper failed\n");
		usb_disk_reset_recovery(device);
		return result;
	}

	if (status.signature != CSW_SIGNATURE || status.tag != command.tag) {
		// the command status wrapper is not valid
		TRACE_ALWAYS("command status wrapper is not valid\n");
		usb_disk_reset_recovery(device);
		return B_ERROR;
	}

	switch (status.status) {
		case CSW_STATUS_COMMAND_PASSED:
		case CSW_STATUS_COMMAND_FAILED:
		{
			// The residue from "status.data_residue" is not maintained
			// correctly by some devices, so calculate it instead.
			uint32 residue = command.data_transfer_length - transferedData;

			if (dataLength != NULL) {
				*dataLength -= residue;
				if (transferedData < *dataLength) {
					TRACE_ALWAYS("less data transfered than indicated\n");
					*dataLength = transferedData;
				}
			}

			if (status.status == CSW_STATUS_COMMAND_PASSED) {
				// the operation is complete and has succeeded
				return B_OK;
			} else {
				// the operation is complete but has failed at the SCSI level
				if (operation != SCSI_TEST_UNIT_READY_6) {
					TRACE_ALWAYS("operation 0x%02x failed at the SCSI level\n",
						operation);
				}

				result = usb_disk_request_sense(lun);
				return result == B_OK ? B_ERROR : result;
			}
		}

		case CSW_STATUS_PHASE_ERROR:
		{
			// a protocol or device error occured
			TRACE_ALWAYS("phase error in operation 0x%02x\n", operation);
			usb_disk_reset_recovery(device);
			return B_ERROR;
		}

		default:
		{
			// command status wrapper is not meaningful
			TRACE_ALWAYS("command status wrapper has invalid status\n");
			usb_disk_reset_recovery(device);
			return B_ERROR;
		}
	}
}


//
//#pragma mark - Helper/Convenience Functions
//


status_t
usb_disk_request_sense(device_lun *lun)
{
	uint32 dataLength = sizeof(scsi_request_sense_6_parameter);
	scsi_request_sense_6_parameter parameter;
	status_t result = usb_disk_operation(lun, SCSI_REQUEST_SENSE_6, 6, 0,
		dataLength, &parameter, &dataLength, true);
	if (result != B_OK) {
		TRACE_ALWAYS("getting request sense data failed\n");
		return result;
	}

	if (parameter.sense_key > SCSI_SENSE_KEY_NOT_READY
		&& parameter.sense_key != SCSI_SENSE_KEY_UNIT_ATTENTION) {
		TRACE_ALWAYS("request_sense: key: 0x%02x; asc: 0x%02x; ascq: "
			"0x%02x;\n", parameter.sense_key, parameter.additional_sense_code,
			parameter.additional_sense_code_qualifier);
	}

	switch (parameter.sense_key) {
		case SCSI_SENSE_KEY_NO_SENSE:
		case SCSI_SENSE_KEY_RECOVERED_ERROR:
			return B_OK;

		case SCSI_SENSE_KEY_HARDWARE_ERROR:
		case SCSI_SENSE_KEY_MEDIUM_ERROR:
			TRACE_ALWAYS("request_sense: media or hardware error\n");
			return B_DEV_UNREADABLE;

		case SCSI_SENSE_KEY_ILLEGAL_REQUEST:
			TRACE_ALWAYS("request_sense: illegal request\n");
			return B_DEV_INVALID_IOCTL;

		case SCSI_SENSE_KEY_UNIT_ATTENTION:
			if (parameter.additional_sense_code
					!= SCSI_ASC_MEDIUM_NOT_PRESENT) {
				TRACE_ALWAYS("request_sense: media changed\n");
				lun->media_changed = true;
				lun->media_present = true;
				return B_DEV_MEDIA_CHANGED;
			}
			// fall through

		case SCSI_SENSE_KEY_NOT_READY:
			TRACE("request_sense: device not ready (asc 0x%02x ascq 0x%02x)\n",
				parameter.additional_sense_code,
				parameter.additional_sense_code_qualifier);
			lun->media_present = false;
			usb_disk_reset_capacity(lun);
			return B_DEV_NO_MEDIA;

		case SCSI_SENSE_KEY_DATA_PROTECT:
			TRACE_ALWAYS("request_sense: write protected\n");
			return B_READ_ONLY_DEVICE;

		case SCSI_SENSE_KEY_ABORTED_COMMAND:
			TRACE_ALWAYS("request_sense: command aborted\n");
			return B_CANCELED;
	}

	return B_ERROR;
}


status_t
usb_disk_mode_sense(device_lun *lun)
{
	uint32 dataLength = sizeof(scsi_mode_sense_6_parameter);
	scsi_mode_sense_6_parameter parameter;
	status_t result = usb_disk_operation(lun, SCSI_MODE_SENSE_6, 6,
		SCSI_MODE_PAGE_DEVICE_CONFIGURATION, dataLength, &parameter,
		&dataLength, true);
	if (result != B_OK) {
		TRACE_ALWAYS("getting mode sense data failed\n");
		return result;
	}

	lun->write_protected
		= (parameter.device_specific & SCSI_DEVICE_SPECIFIC_WRITE_PROTECT)
			!= 0;
	TRACE_ALWAYS("write protected: %s\n", lun->write_protected ? "yes" : "no");
	return B_OK;
}


status_t
usb_disk_test_unit_ready(device_lun *lun)
{
	// if unsupported we assume the unit is fixed and therefore always ok
	if (!lun->device->tur_supported)
		return B_OK;

	status_t result;
	if (lun->device->is_atapi) {
		result = usb_disk_operation(lun, SCSI_START_STOP_UNIT_6, 6, 0, 1,
			NULL, NULL, false);
	} else {
		result = usb_disk_operation(lun, SCSI_TEST_UNIT_READY_6, 6, 0, 0,
			NULL, NULL, true);
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
	uint32 dataLength = sizeof(scsi_inquiry_6_parameter);
	scsi_inquiry_6_parameter parameter;
	status_t result = B_ERROR;
	for (uint32 tries = 0; tries < 3; tries++) {
		result = usb_disk_operation(lun, SCSI_INQUIRY_6, 6, 0, dataLength,
			&parameter, &dataLength, true);
		if (result == B_OK)
			break;
	}
	if (result != B_OK) {
		TRACE_ALWAYS("getting inquiry data failed\n");
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


status_t
usb_disk_update_capacity(device_lun *lun)
{
	uint32 dataLength = sizeof(scsi_read_capacity_10_parameter);
	scsi_read_capacity_10_parameter parameter;
	status_t result = B_ERROR;

	// Retry reading the capacity up to three times. The first try might only
	// yield a unit attention telling us that the device or media status
	// changed, which is more or less expected if it is the first operation
	// on the device or the device only clears the unit atention for capacity
	// reads.
	for (int32 i = 0; i < 3; i++) {
		result = usb_disk_operation(lun, SCSI_READ_CAPACITY_10, 10, 0, 0,
			&parameter, &dataLength, true);
		if (result == B_OK)
			break;
	}

	if (result != B_OK) {
		TRACE_ALWAYS("failed to update capacity\n");
		lun->media_present = false;
		lun->media_changed = false;
		usb_disk_reset_capacity(lun);
		return result;
	}

	lun->media_present = true;
	lun->media_changed = false;
	lun->block_size = ntohl(parameter.logical_block_length);
	lun->block_count = ntohl(parameter.last_logical_block_address) + 1;
	return B_OK;
}


status_t
usb_disk_synchronize(device_lun *lun, bool force)
{
	if (lun->device->sync_support == 0) {
		// this device reported an illegal request when syncing or repeatedly
		// returned an other error, it apparently does not support syncing...
		return B_UNSUPPORTED;
	}

	if (!lun->should_sync && !force)
		return B_OK;

	status_t result = usb_disk_operation(lun, SCSI_SYNCHRONIZE_CACHE_10, 10,
		0, 0, NULL, NULL, false);

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
usb_disk_device_added(usb_device newDevice, void **cookie)
{
	TRACE("device_added(0x%08lx)\n", newDevice);
	disk_device *device = (disk_device *)malloc(sizeof(disk_device));
	device->device = newDevice;
	device->removed = false;
	device->open_count = 0;
	device->interface = 0xff;
	device->current_tag = 0;
	device->sync_support = SYNC_SUPPORT_RELOAD;
	device->tur_supported = true;
	device->is_atapi = false;
	device->luns = NULL;

	// scan through the interfaces to find our bulk-only data interface
	const usb_configuration_info *configuration
		= gUSBModule->get_configuration(newDevice);
	if (configuration == NULL) {
		free(device);
		return B_ERROR;
	}

	for (size_t i = 0; i < configuration->interface_count; i++) {
		usb_interface_info *interface = configuration->interface[i].active;
		if (interface == NULL)
			continue;

		if (interface->descr->interface_class == 0x08 /* mass storage */
			&& (interface->descr->interface_subclass == 0x06 /* SCSI */
				|| interface->descr->interface_subclass == 0x02 /* ATAPI */
				|| interface->descr->interface_subclass == 0x05 /* ATAPI */)
			&& interface->descr->interface_protocol == 0x50 /* bulk-only */) {

			bool hasIn = false;
			bool hasOut = false;
			for (size_t j = 0; j < interface->endpoint_count; j++) {
				usb_endpoint_info *endpoint = &interface->endpoint[j];
				if (endpoint == NULL
					|| endpoint->descr->attributes != USB_ENDPOINT_ATTR_BULK)
					continue;

				if (!hasIn && (endpoint->descr->endpoint_address
					& USB_ENDPOINT_ADDR_DIR_IN)) {
					device->bulk_in = endpoint->handle;
					hasIn = true;
				} else if (!hasOut && (endpoint->descr->endpoint_address
					& USB_ENDPOINT_ADDR_DIR_IN) == 0) {
					device->bulk_out = endpoint->handle;
					hasOut = true;
				}

				if (hasIn && hasOut)
					break;
			}

			if (!(hasIn && hasOut))
				continue;

			device->interface = interface->descr->interface_number;
			device->is_atapi = interface->descr->interface_subclass != 0x06;
			break;
		}
	}

	if (device->interface == 0xff) {
		TRACE_ALWAYS("no valid bulk-only interface found\n");
		free(device);
		return B_ERROR;
	}

	mutex_init(&device->lock, "usb_disk device lock");

	device->notify = create_sem(0, "usb_disk callback notify");
	if (device->notify < B_OK) {
		mutex_destroy(&device->lock);
		status_t result = device->notify;
		free(device);
		return result;
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
		usb_disk_reset_capacity(lun);

		// initialize this lun
		result = usb_disk_inquiry(lun);
		for (uint32 tries = 0; tries < 8; tries++) {
			TRACE("usb lun %"B_PRIu8" inquiry attempt %"B_PRIu32" begin\n",
				i, tries);
			status_t ready = usb_disk_test_unit_ready(lun);
			if (ready == B_OK || ready == B_DEV_NO_MEDIA) {
				if (ready == B_OK) {
					if (lun->device_type == B_CD)
						lun->write_protected = true;
					// TODO: check for write protection; disabled since
					// some devices lock up when getting the mode sense
					else if (/*usb_disk_mode_sense(lun) != B_OK*/true)
						lun->write_protected = false;

					TRACE("usb lun %"B_PRIu8" ready. write protected = %c\n", i,
						lun->write_protected ? 'y' : 'n');

					break;
				}
				TRACE("usb lun %"B_PRIu8" not ready, attempt %"B_PRIu32"\n",
					i, tries);
			}
			TRACE("usb lun %"B_PRIu8" inquiry attempt %"B_PRIu32" failed\n", i,
				tries);

			bigtime_t snoozeTime = 1000000 * tries;
			TRACE("snoozing %"B_PRIu64" microseconds for usb lun\n",
				snoozeTime);
			snooze(snoozeTime);
		}

		if (result != B_OK)
			break;
	}

	if (result != B_OK) {
		TRACE_ALWAYS("failed to initialize logical units\n");
		usb_disk_free_device_and_luns(device);
		return result;
	}

	mutex_lock(&gDeviceListLock);
	device->device_number = 0;
	disk_device *other = gDeviceList;
	while (other != NULL) {
		if (other->device_number >= device->device_number)
			device->device_number = other->device_number + 1;

		other = (disk_device *)other->link;
	}

	device->link = (void *)gDeviceList;
	gDeviceList = device;
	gLunCount += device->lun_count;
	for (uint8 i = 0; i < device->lun_count; i++)
		sprintf(device->luns[i]->name, DEVICE_NAME, device->device_number, i);
	mutex_unlock(&gDeviceListLock);

	TRACE("new device: 0x%08lx\n", (uint32)device);
	*cookie = (void *)device;
	return B_OK;
}


static status_t
usb_disk_device_removed(void *cookie)
{
	TRACE("device_removed(0x%08lx)\n", (uint32)cookie);
	disk_device *device = (disk_device *)cookie;

	mutex_lock(&gDeviceListLock);
	if (gDeviceList == device) {
		gDeviceList = (disk_device *)device->link;
	} else {
		disk_device *element = gDeviceList;
		while (element) {
			if (element->link == device) {
				element->link = device->link;
				break;
			}

			element = (disk_device *)element->link;
		}
	}
	gLunCount -= device->lun_count;
	gDeviceCount--;

	device->removed = true;
	gUSBModule->cancel_queued_transfers(device->bulk_in);
	gUSBModule->cancel_queued_transfers(device->bulk_out);
	if (device->open_count == 0)
		usb_disk_free_device_and_luns(device);

	mutex_unlock(&gDeviceListLock);
	return B_OK;
}


//
//#pragma mark - Partial Buffer Functions
//


static bool
usb_disk_needs_partial_buffer(device_lun *lun, off_t position, size_t length,
	uint32 &blockPosition, uint16 &blockCount)
{
	blockPosition = (uint32)(position / lun->block_size);
	if ((off_t)blockPosition * lun->block_size != position)
		return true;

	blockCount = (uint16)(length / lun->block_size);
	if ((size_t)blockCount * lun->block_size != length)
		return true;

	return false;
}


static status_t
usb_disk_block_read(device_lun *lun, uint32 blockPosition, uint16 blockCount,
	void *buffer, size_t *length)
{
	status_t result = usb_disk_operation(lun, SCSI_READ_10, 10, blockPosition,
		blockCount, buffer, length, true);
	return result;
}


static status_t
usb_disk_block_write(device_lun *lun, uint32 blockPosition, uint16 blockCount,
	void *buffer, size_t *length)
{
	status_t result = usb_disk_operation(lun, SCSI_WRITE_10, 10, blockPosition,
		blockCount, buffer, length, false);
	if (result == B_OK)
		lun->should_sync = true;
	return result;
}


static status_t
usb_disk_prepare_partial_buffer(device_lun *lun, off_t position, size_t length,
	void *&partialBuffer, void *&blockBuffer, uint32 &blockPosition,
	uint16 &blockCount)
{
	blockPosition = (uint32)(position / lun->block_size);
	blockCount = (uint16)((uint32)((position + length + lun->block_size - 1)
		/ lun->block_size) - blockPosition);
	size_t blockLength = blockCount * lun->block_size;
	blockBuffer = malloc(blockLength);
	if (blockBuffer == NULL) {
		TRACE_ALWAYS("no memory to allocate partial buffer\n");
		return B_NO_MEMORY;
	}

	status_t result = usb_disk_block_read(lun, blockPosition, blockCount,
		blockBuffer, &blockLength);
	if (result != B_OK) {
		TRACE_ALWAYS("block read failed when filling partial buffer\n");
		free(blockBuffer);
		return result;
	}

	off_t offset = position - (blockPosition * lun->block_size);
	partialBuffer = (uint8 *)blockBuffer + offset;
	return B_OK;
}


//
//#pragma mark - Driver Hooks
//


static status_t
usb_disk_open(const char *name, uint32 flags, void **cookie)
{
	TRACE("open(%s)\n", name);
	if (strncmp(name, DEVICE_NAME_BASE, strlen(DEVICE_NAME_BASE)) != 0)
		return B_NAME_NOT_FOUND;

	int32 lastPart = 0;
	size_t nameLength = strlen(name);
	for (int32 i = nameLength - 1; i >= 0; i--) {
		if (name[i] == '/') {
			lastPart = i;
			break;
		}
	}

	char rawName[nameLength + 4];
	strncpy(rawName, name, lastPart + 1);
	rawName[lastPart + 1] = 0;
	strcat(rawName, "raw");
	TRACE("opening raw device %s for %s\n", rawName, name);

	mutex_lock(&gDeviceListLock);
	disk_device *device = gDeviceList;
	while (device) {
		for (uint8 i = 0; i < device->lun_count; i++) {
			device_lun *lun = device->luns[i];
			if (strncmp(rawName, lun->name, 32) == 0) {
				// found the matching device/lun
				if (device->removed) {
					mutex_unlock(&gDeviceListLock);
					return B_ERROR;
				}

				device->open_count++;
				*cookie = lun;
				mutex_unlock(&gDeviceListLock);
				return B_OK;
			}
		}

		device = (disk_device *)device->link;
	}

	mutex_unlock(&gDeviceListLock);
	return B_NAME_NOT_FOUND;
}


static status_t
usb_disk_close(void *cookie)
{
	TRACE("close()\n");
	device_lun *lun = (device_lun *)cookie;
	disk_device *device = lun->device;

	mutex_lock(&device->lock);
	if (!device->removed)
		usb_disk_synchronize(lun, false);
	mutex_unlock(&device->lock);

	return B_OK;
}


static status_t
usb_disk_free(void *cookie)
{
	TRACE("free()\n");
	mutex_lock(&gDeviceListLock);

	device_lun *lun = (device_lun *)cookie;
	disk_device *device = lun->device;
	device->open_count--;
	if (device->open_count == 0 && device->removed) {
		// we can simply free the device here as it has been removed from
		// the device list in the device removed notification hook
		usb_disk_free_device_and_luns(device);
	}

	mutex_unlock(&gDeviceListLock);
	return B_OK;
}


static status_t
usb_disk_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	device_lun *lun = (device_lun *)cookie;
	disk_device *device = lun->device;
	mutex_lock(&device->lock);
	if (device->removed) {
		mutex_unlock(&device->lock);
		return B_DEV_NOT_READY;
	}

	status_t result = B_DEV_INVALID_IOCTL;
	switch (op) {
		case B_GET_MEDIA_STATUS:
		{
			*(status_t *)buffer = usb_disk_test_unit_ready(lun);
			TRACE("B_GET_MEDIA_STATUS: 0x%08lx\n", *(status_t *)buffer);
			result = B_OK;
			break;
		}

		case B_GET_GEOMETRY:
		{
			if (lun->media_changed) {
				result = usb_disk_update_capacity(lun);
				if (result != B_OK)
					break;
			}

			device_geometry *geometry = (device_geometry *)buffer;
			geometry->bytes_per_sector = lun->block_size;
			geometry->cylinder_count = lun->block_count;
			geometry->sectors_per_track = geometry->head_count = 1;
			geometry->device_type = lun->device_type;
			geometry->removable = lun->removable;
			geometry->read_only = lun->write_protected;
			geometry->write_once = (lun->device_type == B_WORM);
			TRACE("B_GET_GEOMETRY: %ld sectors at %ld bytes per sector\n",
				geometry->cylinder_count, geometry->bytes_per_sector);
			result = B_OK;
			break;
		}

		case B_FLUSH_DRIVE_CACHE:
			TRACE("B_FLUSH_DRIVE_CACHE\n");
			result = usb_disk_synchronize(lun, true);
			break;

		case B_EJECT_DEVICE:
			result = usb_disk_operation(lun, SCSI_START_STOP_UNIT_6, 6, 0, 2,
				NULL, NULL, false);
			break;

		case B_LOAD_MEDIA:
			result = usb_disk_operation(lun, SCSI_START_STOP_UNIT_6, 6, 0, 3,
				NULL, NULL, false);
			break;

#if HAIKU_TARGET_PLATFORM_HAIKU
		case B_GET_ICON:
			// We don't support this legacy ioctl anymore, but the two other
			// icon ioctls below instead.
			break;

		case B_GET_ICON_NAME:
			result = user_strlcpy((char *)buffer,
				"devices/drive-removable-media-usb", B_FILE_NAME_LENGTH);
			break;

		case B_GET_VECTOR_ICON:
		{
			if (length != sizeof(device_icon)) {
				result = B_BAD_VALUE;
				break;
			}

			device_icon iconData;
			if (user_memcpy(&iconData, buffer, sizeof(device_icon)) != B_OK) {
				result = B_BAD_ADDRESS;
				break;
			}

			if (iconData.icon_size >= (int32)sizeof(kDeviceIcon)) {
				if (user_memcpy(iconData.icon_data, kDeviceIcon,
						sizeof(kDeviceIcon)) != B_OK) {
					result = B_BAD_ADDRESS;
					break;
				}
			}

			iconData.icon_size = sizeof(kDeviceIcon);
			result = user_memcpy(buffer, &iconData, sizeof(device_icon));
			break;
		}
#endif

		default:
			TRACE_ALWAYS("unhandled ioctl %ld\n", op);
			break;
	}

	mutex_unlock(&device->lock);
	return result;
}


static status_t
usb_disk_read(void *cookie, off_t position, void *buffer, size_t *length)
{
	if (buffer == NULL || length == NULL)
		return B_BAD_VALUE;

	TRACE("read(%lld, %ld)\n", position, *length);
	device_lun *lun = (device_lun *)cookie;
	disk_device *device = lun->device;
	mutex_lock(&device->lock);
	if (device->removed) {
		*length = 0;
		mutex_unlock(&device->lock);
		return B_DEV_NOT_READY;
	}

	status_t result = B_ERROR;
	uint32 blockPosition = 0;
	uint16 blockCount = 0;
	bool needsPartial = usb_disk_needs_partial_buffer(lun, position, *length,
		blockPosition, blockCount);
	if (needsPartial) {
		void *partialBuffer = NULL;
		void *blockBuffer = NULL;
		result = usb_disk_prepare_partial_buffer(lun, position, *length,
			partialBuffer, blockBuffer, blockPosition, blockCount);
		if (result == B_OK) {
			memcpy(buffer, partialBuffer, *length);
			free(blockBuffer);
		}
	} else {
		result = usb_disk_block_read(lun, blockPosition, blockCount, buffer,
			length);
	}

	mutex_unlock(&device->lock);
	if (result == B_OK) {
		TRACE("read successful with %ld bytes\n", *length);
		return B_OK;
	}

	*length = 0;
	TRACE_ALWAYS("read fails with 0x%08lx\n", result);
	return result;
}


static status_t
usb_disk_write(void *cookie, off_t position, const void *buffer,
	size_t *length)
{
	if (buffer == NULL || length == NULL)
		return B_BAD_VALUE;

	TRACE("write(%lld, %ld)\n", position, *length);
	device_lun *lun = (device_lun *)cookie;
	disk_device *device = lun->device;
	mutex_lock(&device->lock);
	if (device->removed) {
		*length = 0;
		mutex_unlock(&device->lock);
		return B_DEV_NOT_READY;
	}

	status_t result = B_ERROR;
	uint32 blockPosition = 0;
	uint16 blockCount = 0;
	bool needsPartial = usb_disk_needs_partial_buffer(lun, position,
		*length, blockPosition, blockCount);
	if (needsPartial) {
		void *partialBuffer = NULL;
		void *blockBuffer = NULL;
		result = usb_disk_prepare_partial_buffer(lun, position, *length,
			partialBuffer, blockBuffer, blockPosition, blockCount);
		if (result == B_OK) {
			memcpy(partialBuffer, buffer, *length);
			size_t blockLength = blockCount * lun->block_size;
			result = usb_disk_block_write(lun, blockPosition, blockCount,
				blockBuffer, &blockLength);
			free(blockBuffer);
		}
	} else {
		result = usb_disk_block_write(lun, blockPosition, blockCount,
			(void *)buffer, length);
	}

	mutex_unlock(&device->lock);
	if (result == B_OK) {
		TRACE("write successful with %ld bytes\n", *length);
		return B_OK;
	}

	*length = 0;
	TRACE_ALWAYS("write fails with 0x%08lx\n", result);
	return result;
}


//
//#pragma mark - Driver Entry Points
//


status_t
init_hardware()
{
	TRACE("init_hardware()\n");
	return B_OK;
}


status_t
init_driver()
{
	TRACE("init_driver()\n");
	static usb_notify_hooks notifyHooks = {
		&usb_disk_device_added,
		&usb_disk_device_removed
	};

	static usb_support_descriptor supportedDevices[] = {
		{ 0x08 /* mass storage */, 0x06 /* SCSI */, 0x50 /* bulk */, 0, 0 },
		{ 0x08 /* mass storage */, 0x02 /* ATAPI */, 0x50 /* bulk */, 0, 0 },
		{ 0x08 /* mass storage */, 0x05 /* ATAPI */, 0x50 /* bulk */, 0, 0 }
	};

	gDeviceList = NULL;
	gDeviceCount = 0;
	gLunCount = 0;
	mutex_init(&gDeviceListLock, "usb_disk device list lock");

	TRACE("trying module %s\n", B_USB_MODULE_NAME);
	status_t result = get_module(B_USB_MODULE_NAME,
		(module_info **)&gUSBModule);
	if (result < B_OK) {
		TRACE_ALWAYS("getting module failed 0x%08lx\n", result);
		mutex_destroy(&gDeviceListLock);
		return result;
	}

	gUSBModule->register_driver(DRIVER_NAME, supportedDevices, 3, NULL);
	gUSBModule->install_notify(DRIVER_NAME, &notifyHooks);
	return B_OK;
}


void
uninit_driver()
{
	TRACE("uninit_driver()\n");
	gUSBModule->uninstall_notify(DRIVER_NAME);
	mutex_lock(&gDeviceListLock);

	if (gDeviceNames) {
		for (int32 i = 0; gDeviceNames[i]; i++)
			free(gDeviceNames[i]);
		free(gDeviceNames);
		gDeviceNames = NULL;
	}

	mutex_destroy(&gDeviceListLock);
	put_module(B_USB_MODULE_NAME);
}


const char **
publish_devices()
{
	TRACE("publish_devices()\n");
	if (gDeviceNames) {
		for (int32 i = 0; gDeviceNames[i]; i++)
			free(gDeviceNames[i]);
		free(gDeviceNames);
		gDeviceNames = NULL;
	}

	gDeviceNames = (char **)malloc(sizeof(char *) * (gLunCount + 1));
	if (gDeviceNames == NULL)
		return NULL;

	int32 index = 0;
	mutex_lock(&gDeviceListLock);
	disk_device *device = gDeviceList;
	while (device) {
		for (uint8 i = 0; i < device->lun_count; i++)
			gDeviceNames[index++] = strdup(device->luns[i]->name);

		device = (disk_device *)device->link;
	}

	gDeviceNames[index++] = NULL;
	mutex_unlock(&gDeviceListLock);
	return (const char **)gDeviceNames;
}


device_hooks *
find_device(const char *name)
{
	TRACE("find_device()\n");
	static device_hooks hooks = {
		&usb_disk_open,
		&usb_disk_close,
		&usb_disk_free,
		&usb_disk_ioctl,
		&usb_disk_read,
		&usb_disk_write,
		NULL,
		NULL,
		NULL,
		NULL
	};

	return &hooks;
}
