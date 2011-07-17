/*
 * Copyright 2008-2009, Haiku Inc. All rights reserved.
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


#define DRIVER_NAME			"usb_floppy"
#define DEVICE_NAME_BASE	"disk/ufi/"
#define DEVICE_NAME			DEVICE_NAME_BASE"%ld/%d/raw"


//#define TRACE_USB_DISK
#ifdef TRACE_USB_DISK
#define TRACE(x...)			dprintf("\x1B[35m"DRIVER_NAME"\x1B[0m: "x)
#define TRACE_ALWAYS(x...)	dprintf("\x1B[35m"DRIVER_NAME"\x1B[0m: "x)
#else
#define TRACE(x...)			/* nothing */
#define TRACE_ALWAYS(x...)	dprintf("\x1B[35m"DRIVER_NAME"\x1B[0m: "x)
#endif


int32 api_version = B_CUR_DRIVER_API_VERSION;
static usb_module_info *gUSBModule = NULL;
static disk_device *gDeviceList = NULL;
static uint32 gDeviceCount = 0;
static uint32 gLunCount = 0;
static mutex gDeviceListLock;
static char **gDeviceNames = NULL;

static uint8 kDeviceIcon[] = {
	0x6e, 0x63, 0x69, 0x66, 0x0d, 0x03, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00,
	0x00, 0x6a, 0x02, 0x00, 0x16, 0x02, 0x38, 0x6a, 0xad, 0x38, 0xeb, 0x7a,
	0xbb, 0x77, 0x73, 0x3a, 0xde, 0x88, 0x48, 0xce, 0xdc, 0x4a, 0x75, 0xed,
	0x00, 0x7a, 0xff, 0x4b, 0x02, 0x00, 0x16, 0x02, 0x38, 0x6a, 0xad, 0x38,
	0xeb, 0x7a, 0xbb, 0x77, 0x73, 0x3a, 0xde, 0x88, 0x48, 0xce, 0xdc, 0x4a,
	0x75, 0xed, 0x00, 0x8a, 0xff, 0x2c, 0x05, 0x7d, 0x02, 0x00, 0x16, 0x02,
	0x39, 0xc6, 0x30, 0x36, 0xa8, 0x1b, 0xb9, 0x51, 0xe6, 0x3c, 0x5b, 0xb3,
	0x4b, 0x4d, 0xa8, 0x4a, 0x1a, 0xa1, 0x00, 0x3a, 0xff, 0x5d, 0x02, 0x00,
	0x16, 0x02, 0xb7, 0x16, 0xa4, 0xba, 0x87, 0xfe, 0x3c, 0xb2, 0x07, 0xb9,
	0x49, 0xed, 0x48, 0x87, 0xd8, 0x4a, 0x1e, 0x62, 0x01, 0x75, 0xfe, 0xc4,
	0x02, 0x00, 0x16, 0x02, 0xb7, 0x16, 0xa4, 0xba, 0x87, 0xfe, 0x3c, 0xb2,
	0x07, 0xb9, 0x49, 0xed, 0x48, 0x87, 0xd8, 0x4a, 0x1e, 0x62, 0x00, 0x5c,
	0xfe, 0x9b, 0x02, 0x00, 0x16, 0x05, 0x36, 0xef, 0x60, 0x38, 0xe2, 0xe5,
	0xbd, 0x22, 0x41, 0x3b, 0x2f, 0xce, 0x4a, 0x0e, 0x78, 0x4a, 0x5a, 0x6c,
	0x00, 0xb8, 0x38, 0xe6, 0x77, 0xb8, 0xbd, 0xd2, 0xff, 0x93, 0x02, 0x00,
	0x16, 0x05, 0x34, 0x0a, 0x8f, 0x38, 0xd2, 0xa2, 0xba, 0xb4, 0xc5, 0x35,
	0xe9, 0xec, 0x49, 0xfd, 0x24, 0x4a, 0x64, 0x62, 0x00, 0xe1, 0x38, 0xff,
	0x75, 0xd7, 0xba, 0xf3, 0xfd, 0xd0, 0x02, 0x00, 0x16, 0x02, 0x36, 0x67,
	0xbe, 0x39, 0xdd, 0xbc, 0xbe, 0x50, 0x04, 0x3a, 0xe0, 0x9f, 0x4b, 0x85,
	0x01, 0x49, 0x2a, 0x3f, 0x00, 0xff, 0xff, 0xd8, 0x03, 0x38, 0x6d, 0xbe,
	0x02, 0x00, 0x16, 0x02, 0x3c, 0x40, 0xef, 0x3b, 0x82, 0xd1, 0xba, 0xeb,
	0x42, 0x3b, 0xbf, 0x4d, 0x4a, 0xa7, 0xce, 0x49, 0x5d, 0xc1, 0xff, 0x01,
	0x00, 0x4a, 0x12, 0x0a, 0x05, 0x44, 0x5a, 0x44, 0x40, 0x5e, 0x40, 0x5f,
	0x45, 0x49, 0x5a, 0x0a, 0x06, 0x45, 0x58, 0x5c, 0x42, 0x5c, 0x40, 0x3d,
	0x34, 0xb5, 0x6b, 0xc2, 0x2e, 0xb5, 0x63, 0xc3, 0xbb, 0x0a, 0x04, 0x44,
	0x58, 0x26, 0x4a, 0x26, 0x47, 0x44, 0x54, 0x0a, 0x04, 0x44, 0x59, 0x5c,
	0x42, 0x5c, 0x3e, 0x44, 0x54, 0x0a, 0x05, 0x44, 0x56, 0x5c, 0x40, 0x3d,
	0x34, 0xb5, 0x6b, 0xc2, 0x2e, 0xb5, 0x43, 0xc3, 0x3b, 0x0a, 0x04, 0x2a,
	0x4c, 0x3f, 0x56, 0x3f, 0x53, 0x2a, 0x4a, 0x0a, 0x04, 0x2a, 0x4b, 0x39,
	0x52, 0x39, 0x50, 0x2a, 0x4a, 0x0a, 0x04, 0x31, 0x42, 0x45, 0x4c, 0x3f,
	0x53, 0x2a, 0x4a, 0x0a, 0x04, 0x3f, 0x49, 0x38, 0x50, 0x2a, 0x4a, 0x31,
	0x43, 0x0a, 0x04, 0x3f, 0x36, 0x57, 0x3e, 0x49, 0x4b, 0x32, 0x40, 0x08,
	0x02, 0x3c, 0x3b, 0x4d, 0x43, 0x00, 0x02, 0x3b, 0x3b, 0x31, 0x36, 0x43,
	0x3f, 0x48, 0x3d, 0x3f, 0x36, 0xc4, 0x82, 0xbf, 0xc7, 0x00, 0x02, 0x3e,
	0x3c, 0xbc, 0x58, 0xbd, 0x93, 0x47, 0x3e, 0x45, 0x44, 0x3d, 0x43, 0x4d,
	0x45, 0x02, 0x04, 0x39, 0x3b, 0x39, 0x3d, 0x39, 0x39, 0x3c, 0x38, 0x3b,
	0x38, 0x3e, 0x38, 0x3e, 0x3a, 0x3f, 0x3a, 0x41, 0x37, 0x3c, 0x3d, 0x3c,
	0x42, 0x3c, 0x40, 0x02, 0x04, 0x46, 0x3c, 0x46, 0x3e, 0x46, 0x3a, 0x48,
	0x3a, 0x46, 0x3a, 0x4a, 0x3a, 0x4a, 0x3c, 0x4a, 0x3a, 0x4a, 0x3e, 0x48,
	0x3e, 0x4a, 0x3e, 0x46, 0x3e, 0x0a, 0x04, 0x45, 0x42, 0x42, 0x45, 0x45,
	0x47, 0x48, 0x44, 0x0a, 0x03, 0x4e, 0x43, 0x4d, 0x3f, 0x48, 0x44, 0x0a,
	0x04, 0x32, 0x4b, 0x36, 0x48, 0x32, 0x46, 0x2e, 0x4a, 0x0d, 0x0a, 0x01,
	0x01, 0x00, 0x20, 0x1e, 0x20, 0x0a, 0x00, 0x01, 0x01, 0x30, 0x1e, 0x20,
	0x01, 0x17, 0x84, 0x00, 0x04, 0x0a, 0x02, 0x01, 0x02, 0x20, 0x1e, 0x20,
	0x0a, 0x05, 0x01, 0x03, 0x20, 0x1e, 0x20, 0x0a, 0x06, 0x01, 0x04, 0x20,
	0x1e, 0x20, 0x0a, 0x03, 0x01, 0x05, 0x20, 0x1e, 0x20, 0x0a, 0x07, 0x01,
	0x07, 0x20, 0x1e, 0x20, 0x0a, 0x08, 0x01, 0x06, 0x20, 0x1e, 0x20, 0x0a,
	0x09, 0x01, 0x08, 0x20, 0x1e, 0x20, 0x0a, 0x0a, 0x01, 0x09, 0x20, 0x1e,
	0x20, 0x0a, 0x0c, 0x03, 0x0a, 0x0b, 0x0c, 0x1a, 0x40, 0x1d, 0x05, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x9d, 0xc9, 0xc2, 0x91, 0x98, 0x43,
	0x6d, 0xc2, 0x20, 0xff, 0x01, 0x17, 0x81, 0x00, 0x04, 0x0a, 0x0c, 0x04,
	0x0d, 0x0e, 0x0f, 0x10, 0x0a, 0x3f, 0xff, 0xbd, 0x34, 0xaf, 0xbc, 0xb4,
	0xe0, 0x2c, 0x3f, 0xbc, 0x62, 0x3e, 0x74, 0x62, 0x41, 0xfe, 0xe7, 0x20,
	0xff, 0x0a, 0x02, 0x01, 0x11, 0x20, 0x1e, 0x20
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
status_t	usb_disk_operation(device_lun *lun, uint8* operation,
				void *data, uint32 *dataLength,
				bool directionIn);

status_t	usb_disk_send_diagnostic(device_lun *lun);
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
	delete_sem(device->interruptLock);
	for (uint8 i = 0; i < device->lun_count; i++)
		free(device->luns[i]);
	free(device->luns);
	free(device);
}


//
//#pragma mark - Bulk-only Mass Storage Functions
//


void
usb_disk_reset_recovery(disk_device *device)
{
	gUSBModule->clear_feature(device->bulk_in, USB_FEATURE_ENDPOINT_HALT);
	gUSBModule->clear_feature(device->bulk_out, USB_FEATURE_ENDPOINT_HALT);
	gUSBModule->clear_feature(device->interrupt, USB_FEATURE_ENDPOINT_HALT);
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


void
usb_disk_interrupt(void* cookie, int32 status, void* data, uint32 length)
{
	disk_device* dev = (disk_device*)cookie;
	// We release the lock even if the interrupt is invalid. This way there
	// is at least a chance for the driver to terminate properly.
	release_sem(dev->interruptLock);

	if (length != 2) {
		TRACE_ALWAYS("interrupt of length %ld! (expected 2)\n", length);
		// In this case we do not reschedule the interrupt. This means the
		// driver will be locked. The interrupt should perhaps be scheduled
		// when starting a transfer instead. But getting there means something
		// is really broken, so...
		return;
	}

	// Reschedule the interrupt for next time
	gUSBModule->queue_interrupt(dev->interrupt, dev->interruptBuffer, 2,
		usb_disk_interrupt, cookie);
}


status_t
usb_disk_receive_csw(disk_device *device, command_status_wrapper *status)
{
	TRACE("Waiting for result...\n");
	gUSBModule->queue_interrupt(device->interrupt,
		device->interruptBuffer, 2, usb_disk_interrupt, device);

	acquire_sem(device->interruptLock);

	status->status = device->interruptBuffer[0];
	status->misc = device->interruptBuffer[1];

	return B_OK;
}


status_t
usb_disk_operation(device_lun *lun, uint8* operation,
	void *data,	uint32 *dataLength, bool directionIn)
{
	// TODO: remove transferLength
	TRACE("operation: lun: %u; op: 0x%x; data: %p; dlen: %p (%lu); in: %c\n",
		lun->logical_unit_number, operation[0], data, dataLength,
		dataLength ? *dataLength : 0, directionIn ? 'y' : 'n');

	disk_device* device = lun->device;

	// Step 1 : send the SCSI operation as a class specific request
	size_t actualLength = 12;
	status_t result = gUSBModule->send_request(device->device,
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT, 0 /*request*/, 0/*value*/,
		device->interface/*index*/, 12, operation, &actualLength);

	if (result != B_OK || actualLength != 12) {
		TRACE("Command stage: wrote %ld bytes (error: %s)\n",
			actualLength, strerror(result));

		// There was an error, we have to do a request sense to reset the device
		if (operation[0] != SCSI_REQUEST_SENSE_6) {
			usb_disk_request_sense(lun);
		}
		return result;
	}

	// Step 2 : data phase : send or receive data
	size_t transferedData = 0;
	if (data != NULL && dataLength != NULL && *dataLength > 0) {
		// we have data to transfer in a data stage
		result = usb_disk_transfer_data(device, directionIn, data, *dataLength);
		if (result != B_OK) {
			TRACE("Error %s in data phase\n", strerror(result));
			return result;
		}

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

	// step 3 : wait for the device to send the interrupt ACK
	if (operation[0] != SCSI_REQUEST_SENSE_6) {
		command_status_wrapper status;
		result =  usb_disk_receive_csw(device, &status);
		if (result != B_OK) {
			// in case of a stall or error clear the stall and try again
			TRACE("Error receiving interrupt: %s. Retrying...\n", strerror(result));
			gUSBModule->clear_feature(device->bulk_in, USB_FEATURE_ENDPOINT_HALT);
			result = usb_disk_receive_csw(device, &status);
		}

		if (result != B_OK) {
			TRACE_ALWAYS("receiving the command status interrupt failed\n");
			usb_disk_reset_recovery(device);
			return result;
		}

		// wait for the device to finish the operation.
		result = usb_disk_request_sense(lun);
	}
	return result;
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

	status_t result = usb_disk_operation(lun, commandBlock, NULL,
		NULL, false);

	int retry = 100;
	while(result == B_DEV_NO_MEDIA && retry > 0) {
		snooze(10000);
		result = usb_disk_request_sense(lun);
		retry--;
	}

	if (result != B_OK)
		TRACE("Send Diagnostic failed: %s\n", strerror(result));
	return result;
}


status_t
usb_disk_request_sense(device_lun *lun)
{
	uint32 dataLength = sizeof(scsi_request_sense_6_parameter);
	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

	commandBlock[0] = SCSI_REQUEST_SENSE_6;
	commandBlock[1] = lun->logical_unit_number << 5;
	commandBlock[2] = 0; // page code
	commandBlock[4] = dataLength;

	scsi_request_sense_6_parameter parameter;
	status_t result = usb_disk_operation(lun, commandBlock, &parameter,
		&dataLength, true);
	if (result != B_OK) {
		TRACE_ALWAYS("getting request sense data failed\n");
		return result;
	}

	if (parameter.sense_key > SCSI_SENSE_KEY_NOT_READY
		&& parameter.sense_key != SCSI_SENSE_KEY_UNIT_ATTENTION) {
		TRACE_ALWAYS("request_sense: key: 0x%02x; asc: 0x%02x; ascq: 0x%02x;\n",
			parameter.sense_key, parameter.additional_sense_code,
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
			if (parameter.additional_sense_code != SCSI_ASC_MEDIUM_NOT_PRESENT) {
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

	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

	commandBlock[0] = SCSI_MODE_SENSE_6;
	commandBlock[1] = lun->logical_unit_number << 5;
	commandBlock[2] = 0; // Current values
	commandBlock[7] = dataLength >> 8;
	commandBlock[8] = dataLength;

	scsi_mode_sense_6_parameter parameter;
	status_t result = usb_disk_operation(lun, commandBlock,
		&parameter, &dataLength, true);
	if (result != B_OK) {
		TRACE_ALWAYS("getting mode sense data failed\n");
		return result;
	}

	lun->write_protected
		= (parameter.device_specific & SCSI_DEVICE_SPECIFIC_WRITE_PROTECT) != 0;
	TRACE_ALWAYS("write protected: %s\n", lun->write_protected ? "yes" : "no");
	return B_OK;
}


status_t
usb_disk_test_unit_ready(device_lun *lun)
{
	return B_OK;

	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

	commandBlock[0] = SCSI_TEST_UNIT_READY_6;
	commandBlock[1] = lun->logical_unit_number << 5;

	status_t result = usb_disk_operation(lun, commandBlock, NULL, NULL, false);

	return result;
}


status_t
usb_disk_inquiry(device_lun *lun)
{
	uint32 dataLength = sizeof(scsi_inquiry_6_parameter);

	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

	commandBlock[0] = SCSI_INQUIRY_6;
	commandBlock[1] = lun->logical_unit_number << 5;
	commandBlock[2] = 0; // page code
	commandBlock[4] = dataLength;

	scsi_inquiry_6_parameter parameter;
	status_t result = B_ERROR;
	for (uint32 tries = 0; tries < 3; tries++) {
		result = usb_disk_operation(lun, commandBlock, &parameter, &dataLength,
			true);
		if (result == B_OK)
			break;
	}
	if (result != B_OK) {
		TRACE_ALWAYS("getting inquiry data failed\n");
		lun->device_type = B_DISK;
		lun->removable = true;
		return result;
	}

	TRACE("peripherial_device_type  0x%02x\n", parameter.peripherial_device_type);
	TRACE("peripherial_qualifier    0x%02x\n", parameter.peripherial_qualifier);
	TRACE("removable_medium         %s\n", parameter.removable_medium ? "yes" : "no");
	TRACE("version                  0x%02x\n", parameter.version);
	TRACE("response_data_format     0x%02x\n", parameter.response_data_format);
	TRACE_ALWAYS("vendor_identification    \"%.8s\"\n", parameter.vendor_identification);
	TRACE_ALWAYS("product_identification   \"%.16s\"\n", parameter.product_identification);
	TRACE_ALWAYS("product_revision_level   \"%.4s\"\n", parameter.product_revision_level);
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

	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

	commandBlock[0] = SCSI_READ_CAPACITY_10;
	commandBlock[1] = lun->logical_unit_number << 5;

	for (int tries = 0; tries < 5; tries++) {
		result = usb_disk_operation(lun, commandBlock, &parameter, &dataLength,
			true);
		if (result == B_DEV_NO_MEDIA || result == B_TIMED_OUT
				|| result == B_DEV_STALLED)
			snooze(10000);
		else
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
	return B_UNSUPPORTED;
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
	device->is_atapi = false;
	device->luns = NULL;

	// scan through the interfaces to find our data interface
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
			&& interface->descr->interface_subclass == 0x04 /* UFI (floppy) */
			&& interface->descr->interface_protocol == 0x00) {

			bool hasIn = false;
			bool hasOut = false;
			bool hasInt = false;
			for (size_t j = 0; j < interface->endpoint_count; j++) {
				usb_endpoint_info *endpoint = &interface->endpoint[j];
				if (endpoint == NULL)
					continue;

				if (!hasIn && (endpoint->descr->endpoint_address
					& USB_ENDPOINT_ADDR_DIR_IN)
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
					// TODO:ensure there is only one interrupt endpoint and
					// stop enumerating
					device->interrupt = endpoint->handle;
					hasInt = true;
				}

				if (hasIn && hasOut && hasInt)
					break;
			}

			if (!(hasIn && hasOut && hasInt)) {
				TRACE("in: %d, out: %d, int: %d\n", hasIn, hasOut, hasInt);
				continue;
			}

			device->interface = interface->descr->interface_number;
			device->is_atapi = interface->descr->interface_subclass != 0x06;
			break;
		}
	}

	if (device->interface == 0xff) {
		TRACE_ALWAYS("no valid CBI interface found\n");
		free(device);
		return B_ERROR;
	}

	mutex_init(&device->lock, "usb_disk device lock");

	device->notify = create_sem(0, "usb_disk callback notify");
	if (device->notify < B_OK) {
		mutex_destroy(&device->lock);
		free(device);
		return device->notify;
	}

	device->interruptLock = create_sem(0, "usb_disk interrupt lock");
	if (device->interruptLock < B_OK) {
		mutex_destroy(&device->lock);
		free(device);
		return device->interruptLock;
	}

	// TODO: handle more than 1 unit
	device->lun_count = 1;
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

		// Reset the device
		// If we don't do it all the other commands except inquiry and send
		// diagnostics will be stalled.
		result = usb_disk_send_diagnostic(lun);
		for (uint32 tries = 0; tries < 3; tries++) {
			status_t ready = usb_disk_test_unit_ready(lun);
			if (ready == B_OK || ready == B_DEV_NO_MEDIA) {
				if (ready == B_OK) {
					if (lun->device_type == B_CD)
						lun->write_protected = true;
					// TODO: check for write protection; disabled since
					// some devices lock up when getting the mode sense
					else if (/*usb_disk_mode_sense(lun) != B_OK*/true)
						lun->write_protected = false;
				}

				break;
			}

			snooze(10000);
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
	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

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
		result = usb_disk_operation(lun, commandBlock, buffer, length,
			true);
		if (result == B_OK)
			break;
		else
			snooze(10000);
	}
	return result;
}


static status_t
usb_disk_block_write(device_lun *lun, uint32 blockPosition, uint16 blockCount,
	void *buffer, size_t *length)
{
	uint8 commandBlock[12];
	memset(commandBlock, 0, sizeof(commandBlock));

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
	result = usb_disk_operation(lun, commandBlock, buffer, length,
		false);

	int retry = 10;
	while (result == B_DEV_NO_MEDIA && retry > 0) {
		snooze(10000);
		result = usb_disk_request_sense(lun);
		retry--;
	}

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
		case B_GET_MEDIA_STATUS: {
			*(status_t *)buffer = usb_disk_test_unit_ready(lun);
			TRACE("B_GET_MEDIA_STATUS: 0x%08lx\n", *(status_t *)buffer);
			result = B_OK;
			break;
		}

		case B_GET_GEOMETRY: {
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
		{
			uint8 commandBlock[12];
			memset(commandBlock, 0, sizeof(commandBlock));

			commandBlock[0] = SCSI_START_STOP_UNIT_6;
			commandBlock[1] = lun->logical_unit_number << 5;
			commandBlock[4] = 2;

			result = usb_disk_operation(lun, commandBlock, NULL, NULL, false);
			break;
		}

		case B_LOAD_MEDIA:
		{
			uint8 commandBlock[12];
			memset(commandBlock, 0, sizeof(commandBlock));

			commandBlock[0] = SCSI_START_STOP_UNIT_6;
			commandBlock[1] = lun->logical_unit_number << 5;
			commandBlock[4] = 3;

			result = usb_disk_operation(lun, commandBlock, NULL, NULL, false);
			break;
		}

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
		{ 0x08 /* mass storage */, 0x04 /* UFI */, 0x00, 0, 0 }
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

	gUSBModule->register_driver(DRIVER_NAME, supportedDevices, 1, NULL);
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
