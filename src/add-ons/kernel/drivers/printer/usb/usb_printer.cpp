/*
 * Copyright 2008-2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#include <ByteOrder.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "usb_printer.h"
#include "USB_printer.h"

#define DRIVER_NAME			"usb_printer"
#define DEVICE_NAME_BASE	"printer/usb/"
#define DEVICE_NAME			DEVICE_NAME_BASE"%ld"


#define TRACE_USB_PRINTER 1
#ifdef TRACE_USB_PRINTER
#define TRACE(x...)			dprintf(DRIVER_NAME": "x)
#define TRACE_ALWAYS(x...)	dprintf(DRIVER_NAME": "x)
#else
#define TRACE(x...)			/* nothing */
#define TRACE_ALWAYS(x...)	dprintf(DRIVER_NAME": "x)
#endif


int32 api_version = B_CUR_DRIVER_API_VERSION;
static usb_module_info *gUSBModule = NULL;
static printer_device *gDeviceList = NULL;
static uint32 gDeviceCount = 0;
static mutex gDeviceListLock;
static char **gDeviceNames = NULL;


//
//#pragma mark - Forward Declarations
//


static void	usb_printer_callback(void *cookie, status_t status, void *data,
				size_t actualLength);

status_t	usb_printer_transfer_data(printer_device *device, bool directionIn,
				void *data, size_t dataLength);

#if 0
status_t	usb_printer_mass_storage_reset(printer_device *device);
uint8		usb_printer_get_max_lun(printer_device *device);
void		usb_printer_reset_recovery(printer_device *device);
status_t	usb_printer_receive_csw(printer_device *device,
				command_status_wrapper *status);
status_t	usb_printer_operation(device_lun *lun, uint8 operation,
				uint8 opLength, uint32 logicalBlockAddress,
				uint16 transferLength, void *data, uint32 *dataLength,
				bool directionIn);

status_t	usb_printer_request_sense(device_lun *lun);
status_t	usb_printer_mode_sense(device_lun *lun);
status_t	usb_printer_test_unit_ready(device_lun *lun);
status_t	usb_printer_inquiry(device_lun *lun);
status_t	usb_printer_reset_capacity(device_lun *lun);
status_t	usb_printer_update_capacity(device_lun *lun);
status_t	usb_printer_synchronize(device_lun *lun, bool force);
#endif

//
//#pragma mark - Device Allocation Helper Functions
//


void
usb_printer_free_device(printer_device *device)
{
	mutex_lock(&device->lock);
	mutex_destroy(&device->lock);
	delete_sem(device->notify);
	free(device);
}


//
//#pragma mark - Bulk-only Functions
//


void
usb_printer_reset_recovery(printer_device *device)
{
	gUSBModule->clear_feature(device->bulk_in, USB_FEATURE_ENDPOINT_HALT);
	gUSBModule->clear_feature(device->bulk_out, USB_FEATURE_ENDPOINT_HALT);
}


status_t
usb_printer_transfer_data(printer_device *device, bool directionIn, void *data,
	size_t dataLength)
{
	status_t result = gUSBModule->queue_bulk(directionIn ? device->bulk_in
		: device->bulk_out, data, dataLength, usb_printer_callback, device);
	if (result != B_OK) {
		TRACE_ALWAYS("failed to queue data transfer\n");
		return result;
	}

	do {
		result = acquire_sem_etc(device->notify, 1, B_RELATIVE_TIMEOUT, 2000000);
		if (result == B_TIMED_OUT) {
			// Cancel the transfer and collect the sem that should now be
			// released through the callback on cancel. Handling of device
			// reset is done in usb_printer_operation() when it detects that
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

#if 0
status_t
usb_printer_mass_storage_reset(printer_device *device)
{
	return gUSBModule->send_request(device->device, USB_REQTYPE_INTERFACE_OUT
		| USB_REQTYPE_CLASS, REQUEST_MASS_STORAGE_RESET, 0x0000,
		device->interface, 0, NULL, NULL);
}


uint8
usb_printer_get_max_lun(printer_device *device)
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


status_t
usb_printer_receive_csw(printer_device *device, command_status_wrapper *status)
{
	status_t result = usb_printer_transfer_data(device, true, status,
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
usb_printer_operation(device_lun *lun, uint8 operation, uint8 opLength,
	uint32 logicalBlockAddress, uint16 transferLength, void *data,
	uint32 *dataLength, bool directionIn)
{
	TRACE("operation: lun: %u; op: %u; oplen: %u; lba: %lu; tlen: %u; data: %p; dlen: %p (%lu); in: %c\n",
		lun->logical_unit_number, operation, opLength, logicalBlockAddress,
		transferLength, data, dataLength, dataLength ? *dataLength : 0,
		directionIn ? 'y' : 'n');

	printer_device *device = lun->device;
	command_block_wrapper command;
	command.signature = CBW_SIGNATURE;
	command.tag = device->current_tag++;
	command.data_transfer_length = (dataLength != NULL ? *dataLength : 0);
	command.flags = (directionIn ? CBW_DATA_INPUT : CBW_DATA_OUTPUT);
	command.lun = lun->logical_unit_number;
	command.command_block_length = opLength;
	memset(command.command_block, 0, sizeof(command.command_block));

	switch (opLength) {
		case 6: {
			scsi_command_6 *commandBlock = (scsi_command_6 *)command.command_block;
			commandBlock->operation = operation;
			commandBlock->lun = lun->logical_unit_number << 5;
			commandBlock->allocation_length = (uint8)transferLength;
			if (operation == SCSI_MODE_SENSE_6) {
				// we hijack the lba argument to transport the desired page
				commandBlock->reserved[1] = (uint8)logicalBlockAddress;
			}
			break;
		}

		case 10: {
			scsi_command_10 *commandBlock = (scsi_command_10 *)command.command_block;
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

	status_t result = usb_printer_transfer_data(device, false, &command,
		sizeof(command_block_wrapper));
	if (result != B_OK)
		return result;

	if (device->status != B_OK ||
		device->actual_length != sizeof(command_block_wrapper)) {
		// sending the command block wrapper failed
		TRACE_ALWAYS("sending the command block wrapper failed\n");
		usb_printer_reset_recovery(device);
		return B_ERROR;
	}

	size_t transferedData = 0;
	if (data != NULL && dataLength != NULL && *dataLength > 0) {
		// we have data to transfer in a data stage
		result = usb_printer_transfer_data(device, directionIn, data, *dataLength);
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
				usb_printer_reset_recovery(device);
				return B_ERROR;
			}
		}
	}

	command_status_wrapper status;
	result =  usb_printer_receive_csw(device, &status);
	if (result != B_OK) {
		// in case of a stall or error clear the stall and try again
		gUSBModule->clear_feature(device->bulk_in, USB_FEATURE_ENDPOINT_HALT);
		result = usb_printer_receive_csw(device, &status);
	}

	if (result != B_OK) {
		TRACE_ALWAYS("receiving the command status wrapper failed\n");
		usb_printer_reset_recovery(device);
		return result;
	}

	if (status.signature != CSW_SIGNATURE || status.tag != command.tag) {
		// the command status wrapper is not valid
		TRACE_ALWAYS("command status wrapper is not valid\n");
		usb_printer_reset_recovery(device);
		return B_ERROR;
	}

	switch (status.status) {
		case CSW_STATUS_COMMAND_PASSED:
		case CSW_STATUS_COMMAND_FAILED: {
			if (status.data_residue > command.data_transfer_length) {
				// command status wrapper is not meaningful
				TRACE_ALWAYS("command status wrapper has invalid residue\n");
				usb_printer_reset_recovery(device);
				return B_ERROR;
			}

			if (dataLength != NULL) {
				*dataLength -= status.data_residue;
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
				TRACE_ALWAYS("operation 0x%02x failed at the SCSI level\n",
					operation);
				result = usb_printer_request_sense(lun);
				return result == B_OK ? B_ERROR : result;
			}
		}

		case CSW_STATUS_PHASE_ERROR: {
			// a protocol or device error occured
			TRACE_ALWAYS("phase error in operation 0x%02x\n", operation);
			usb_printer_reset_recovery(device);
			return B_ERROR;
		}

		default: {
			// command status wrapper is not meaningful
			TRACE_ALWAYS("command status wrapper has invalid status\n");
			usb_printer_reset_recovery(device);
			return B_ERROR;
		}
	}
}


//
//#pragma mark - Helper/Convenience Functions
//


status_t
usb_printer_request_sense(device_lun *lun)
{
	uint32 dataLength = sizeof(scsi_request_sense_6_parameter);
	scsi_request_sense_6_parameter parameter;
	status_t result = usb_printer_operation(lun, SCSI_REQUEST_SENSE_6, 6, 0,
		dataLength, &parameter, &dataLength, true);
	if (result != B_OK) {
		TRACE_ALWAYS("getting request sense data failed\n");
		return result;
	}

	if (parameter.sense_key > SCSI_SENSE_KEY_NOT_READY) {
		TRACE_ALWAYS("request_sense: key: 0x%02x; asc: 0x%02x; ascq: 0x%02x;\n",
			parameter.sense_key, parameter.additional_sense_code,
			parameter.additional_sense_code_qualifier);
	}

	switch (parameter.sense_key) {
		case SCSI_SENSE_KEY_NO_SENSE:
		case SCSI_SENSE_KEY_RECOVERED_ERROR:
			return B_OK;

		case SCSI_SENSE_KEY_NOT_READY:
			TRACE("request_sense: device not ready (asc 0x%02x ascq 0x%02x)\n",
				parameter.additional_sense_code,
				parameter.additional_sense_code_qualifier);
			lun->media_present = false;
			usb_printer_reset_capacity(lun);
			return B_DEV_NO_MEDIA;

		case SCSI_SENSE_KEY_HARDWARE_ERROR:
		case SCSI_SENSE_KEY_MEDIUM_ERROR:
			TRACE_ALWAYS("request_sense: media or hardware error\n");
			return B_DEV_UNREADABLE;

		case SCSI_SENSE_KEY_ILLEGAL_REQUEST:
			TRACE_ALWAYS("request_sense: illegal request\n");
			return B_DEV_INVALID_IOCTL;

		case SCSI_SENSE_KEY_UNIT_ATTENTION:
			TRACE_ALWAYS("request_sense: media changed\n");
			lun->media_changed = true;
			lun->media_present = true;
			return B_DEV_MEDIA_CHANGED;

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
usb_printer_mode_sense(device_lun *lun)
{
	uint32 dataLength = sizeof(scsi_mode_sense_6_parameter);
	scsi_mode_sense_6_parameter parameter;
	status_t result = usb_printer_operation(lun, SCSI_MODE_SENSE_6, 6,
		SCSI_MODE_PAGE_DEVICE_CONFIGURATION, dataLength, &parameter,
		&dataLength, true);
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
usb_printer_test_unit_ready(device_lun *lun)
{
	return usb_printer_operation(lun, SCSI_TEST_UNIT_READY_6, 6, 0, 0, NULL, NULL,
		true);
}


status_t
usb_printer_inquiry(device_lun *lun)
{
	uint32 dataLength = sizeof(scsi_inquiry_6_parameter);
	scsi_inquiry_6_parameter parameter;
	status_t result = B_ERROR;
	for (uint32 tries = 0; tries < 3; tries++) {
		result = usb_printer_operation(lun, SCSI_INQUIRY_6, 6, 0, dataLength,
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
usb_printer_reset_capacity(device_lun *lun)
{
	lun->block_size = 512;
	lun->block_count = 0;
	return B_OK;
}


status_t
usb_printer_update_capacity(device_lun *lun)
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
		result = usb_printer_operation(lun, SCSI_READ_CAPACITY_10, 10, 0, 0,
			&parameter, &dataLength, true);
		if (result == B_OK)
			break;
	}

	if (result != B_OK) {
		TRACE_ALWAYS("failed to update capacity\n");
		lun->media_present = false;
		lun->media_changed = false;
		usb_printer_reset_capacity(lun);
		return result;
	}

	lun->media_present = true;
	lun->media_changed = false;
	lun->block_size = ntohl(parameter.logical_block_length);
	lun->block_count = ntohl(parameter.last_logical_block_address) + 1;
	return B_OK;
}


status_t
usb_printer_synchronize(device_lun *lun, bool force)
{
	if (lun->device->sync_support == 0) {
		// this device reported an illegal request when syncing or repeatedly
		// returned an other error, it apparently does not support syncing...
		return B_UNSUPPORTED;
	}

	if (!lun->should_sync && !force)
		return B_OK;

	status_t result = usb_printer_operation(lun, SCSI_SYNCHRONIZE_CACHE_10, 10,
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
#endif

//
//#pragma mark - Device Attach/Detach Notifications and Callback
//


static void
usb_printer_callback(void *cookie, status_t status, void *data,
	size_t actualLength)
{
	//TRACE("callback()\n");
	printer_device *device = (printer_device *)cookie;
	device->status = status;
	device->actual_length = actualLength;
	release_sem(device->notify);
}


static status_t
usb_printer_device_added(usb_device newDevice, void **cookie)
{
	TRACE("device_added(0x%08lx)\n", newDevice);
	printer_device *device = (printer_device *)malloc(sizeof(printer_device));
	device->device = newDevice;
	device->removed = false;
	device->open_count = 0;
	device->interface = 0xff;
	device->current_tag = 0;
	device->sync_support = SYNC_SUPPORT_RELOAD;
	device->block_size = 4096;

	// scan through the interfaces to find our bulk-only data interface
	const usb_configuration_info *configuration = gUSBModule->get_configuration(newDevice);
	if (configuration == NULL) {
		free(device);
		return B_ERROR;
	}

	for (size_t i = 0; i < configuration->interface_count; i++) {
		usb_interface_info *interface = configuration->interface[i].active;
		if (interface == NULL)
			continue;

		if (interface->descr->interface_class == PRINTER_INTERFACE_CLASS
			&& interface->descr->interface_subclass == PRINTER_INTERFACE_SUBCLASS
			&& (interface->descr->interface_protocol == PIT_UNIDIRECTIONAL
				|| interface->descr->interface_protocol == PIT_BIDIRECTIONAL
				|| interface->descr->interface_protocol == PIT_1284_4_COMPATIBLE)) {

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
			break;
		}
	}

	if (device->interface == 0xff) {
		TRACE_ALWAYS("no valid interface found\n");
		free(device);
		return B_ERROR;
	}

	mutex_init(&device->lock, "usb_printer device lock");

	device->notify = create_sem(0, "usb_printer callback notify");
	if (device->notify < B_OK) {
		mutex_destroy(&device->lock);
		free(device);
		return device->notify;
	}

	mutex_lock(&gDeviceListLock);
	uint32 freeDeviceMask = 0;
	device->device_number = 0;
	printer_device *other = gDeviceList;
	while (other != NULL) {
		// set max device number + 1
		if (other->device_number >= device->device_number)
			device->device_number = other->device_number + 1;

		// record used devices
		if (other->device_number < 32) {
			freeDeviceMask |= 1 << other->device_number;
		}

		other = other->link;
	}
	if (freeDeviceMask != (uint32)-1) {
		// reuse device number of first 32 devices
		for (int32 device_number = 0; device_number < 32; device_number ++) {
			if (freeDeviceMask & (1 << device_number) == 0) {
				device->device_number = device_number;
				break;
			}
		}
	}

	device->link = gDeviceList;
	gDeviceList = device;
	uint32 deviceNumber = gDeviceCount++;
	sprintf(device->name, DEVICE_NAME, deviceNumber);
	mutex_unlock(&gDeviceListLock);

	TRACE("new device: 0x%08lx\n", (uint32)device);
	*cookie = (void *)device;
	return B_OK;
}


static status_t
usb_printer_device_removed(void *cookie)
{
	TRACE("device_removed(0x%08lx)\n", (uint32)cookie);
	printer_device *device = (printer_device *)cookie;

	mutex_lock(&gDeviceListLock);
	if (gDeviceList == device) {
		gDeviceList = device->link;
	} else {
		printer_device *element = gDeviceList;
		while (element) {
			if (element->link == device) {
				element->link = device->link;
				break;
			}

			element = element->link;
		}
	}
	gDeviceCount--;

	device->removed = true;
	gUSBModule->cancel_queued_transfers(device->bulk_in);
	gUSBModule->cancel_queued_transfers(device->bulk_out);
	if (device->open_count == 0)
		usb_printer_free_device(device);

	mutex_unlock(&gDeviceListLock);
	return B_OK;
}


//
//#pragma mark - Partial Buffer Functions
//

#if 0
static bool
usb_printer_needs_partial_buffer(printer_device *device, off_t position, size_t length,
	uint32 &blockPosition, uint16 &blockCount)
{
	blockPosition = (uint32)(position / device->block_size);
	if ((off_t)blockPosition * device->block_size != position)
		return true;

	blockCount = (uint16)(length / device->block_size);
	if ((size_t)blockCount * device->block_size != length)
		return true;

	return false;
}


static status_t
usb_printer_block_read(printer_device *device, uint32 blockPosition, uint16 blockCount,
	void *buffer, size_t *length)
{
	status_t result = usb_printer_operation(device, SCSI_READ_10, 10, blockPosition,
		blockCount, buffer, length, true);
	return result;
}


static status_t
usb_printer_block_write(printer_device *device, uint32 blockPosition, uint16 blockCount,
	void *buffer, size_t *length)
{
	status_t result = usb_printer_operation(device, SCSI_WRITE_10, 10, blockPosition,
		blockCount, buffer, length, false);
	if (result == B_OK)
		lun->should_sync = true;
	return result;
}


static status_t
usb_printer_prepare_partial_buffer(printer_device *device, off_t position, size_t length,
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

	status_t result = usb_printer_block_read(device, blockPosition, blockCount,
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
#endif

//
//#pragma mark - Driver Hooks
//


static status_t
usb_printer_open(const char *name, uint32 flags, void **cookie)
{
	TRACE("open(%s)\n", name);
	if (strncmp(name, DEVICE_NAME_BASE, strlen(DEVICE_NAME_BASE)) != 0)
		return B_NAME_NOT_FOUND;

	mutex_lock(&gDeviceListLock);
	printer_device *device = gDeviceList;
	while (device) {
		if (strncmp(name, device->name, 32) == 0) {
			if (device->removed) {
				mutex_unlock(&gDeviceListLock);
				return B_ERROR;
			}

			device->open_count++;
			*cookie = device;
			mutex_unlock(&gDeviceListLock);
			return B_OK;
		}

		device = device->link;
	}

	mutex_unlock(&gDeviceListLock);
	return B_NAME_NOT_FOUND;
}


static status_t
usb_printer_close(void *cookie)
{
	TRACE("close()\n");
	return B_OK;
}


static status_t
usb_printer_free(void *cookie)
{
	TRACE("free()\n");
	mutex_lock(&gDeviceListLock);

	printer_device *device = (printer_device *)cookie;
	device->open_count--;
	if (device->open_count == 0 && device->removed) {
		// we can simply free the device here as it has been removed from
		// the device list in the device removed notification hook
		usb_printer_free_device(device);
	}

	mutex_unlock(&gDeviceListLock);
	return B_OK;
}


static status_t
usb_printer_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	printer_device *device = (printer_device *)cookie;
	mutex_lock(&device->lock);
	if (device->removed) {
		mutex_unlock(&device->lock);
		return B_DEV_NOT_READY;
	}

	status_t result = B_DEV_INVALID_IOCTL;
	switch (op) {
		case USB_PRINTER_GET_DEVICE_ID: {
			// TODO implement
			strcpy(buffer, "Not implemented");
			result = B_OK;
			break;
		}

		default:
			TRACE_ALWAYS("unhandled ioctl %ld\n", op);
			break;
	}

	mutex_unlock(&device->lock);
	return result;
}


static status_t
usb_printer_transfer(printer_device* device, bool directionIn, void* buffer, size_t* length)
{
	status_t result = B_ERROR;
	if (buffer == NULL || length == NULL || *length <= 0) {
		return result;
	}

	result = usb_printer_transfer_data(device, true, buffer, *length);
	if (result != B_OK) {
		return result;
	}
	size_t transferedData = device->actual_length;
	if (device->status != B_OK || transferedData != *length) {
		// sending or receiving of the data failed
		if (device->status == B_DEV_STALLED) {
			TRACE("stall while transfering data\n");
			gUSBModule->clear_feature(directionIn ? device->bulk_in
				: device->bulk_out, USB_FEATURE_ENDPOINT_HALT);
		} else {
			TRACE_ALWAYS("sending or receiving of the data failed\n");
			usb_printer_reset_recovery(device);
			return B_ERROR;
		}
	}
	*length = transferedData;

	return result;
}

static status_t
usb_printer_read(void *cookie, off_t position, void *buffer, size_t *length)
{
	if (buffer == NULL || length == NULL)
		return B_BAD_VALUE;

	TRACE("read(%lld, %ld)\n", position, *length);
	printer_device *device = (printer_device *)cookie;
	mutex_lock(&device->lock);
	if (device->removed) {
		*length = 0;
		mutex_unlock(&device->lock);
		return B_DEV_NOT_READY;
	}

	status_t result = usb_printer_transfer(device, true, buffer, length);

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
usb_printer_write(void *cookie, off_t position, const void *buffer,
	size_t *length)
{
	if (buffer == NULL || length == NULL)
		return B_BAD_VALUE;

	TRACE("write(%lld, %ld)\n", position, *length);
	printer_device *device = (printer_device *)cookie;
	mutex_lock(&device->lock);
	if (device->removed) {
		*length = 0;
		mutex_unlock(&device->lock);
		return B_DEV_NOT_READY;
	}

	status_t result = usb_printer_transfer(device, false, (void*)buffer, length);

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
		&usb_printer_device_added,
		&usb_printer_device_removed
	};

	static usb_support_descriptor supportedDevices = {
		PRINTER_INTERFACE_CLASS,
		PRINTER_INTERFACE_SUBCLASS,
		0, // any protocol
		0, // any product
		0  // any vendor
	};

	gDeviceList = NULL;
	gDeviceCount = 0;
	mutex_init(&gDeviceListLock, "usb_printer device list lock");

	TRACE("trying module %s\n", B_USB_MODULE_NAME);
	status_t result = get_module(B_USB_MODULE_NAME,
		(module_info **)&gUSBModule);
	if (result < B_OK) {
		TRACE_ALWAYS("getting module failed 0x%08lx\n", result);
		mutex_destroy(&gDeviceListLock);
		return result;
	}

	gUSBModule->register_driver(DRIVER_NAME, &supportedDevices, 1, NULL);
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

	gDeviceNames = (char **)malloc(sizeof(char *) * (gDeviceCount + 1));
	if (gDeviceNames == NULL)
		return NULL;

	int32 index = 0;
	mutex_lock(&gDeviceListLock);
	printer_device *device = gDeviceList;
	while (device) {
		gDeviceNames[index++] = strdup(device->name);

		device = device->link;
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
		&usb_printer_open,
		&usb_printer_close,
		&usb_printer_free,
		&usb_printer_ioctl,
		&usb_printer_read,
		&usb_printer_write,
		NULL,
		NULL,
		NULL,
		NULL
	};

	return &hooks;
}
