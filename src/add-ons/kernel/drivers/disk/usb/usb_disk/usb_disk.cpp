/*
 * Copyright 2008, Haiku Inc. All rights reserved.
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
#define DEVICE_NAME			DEVICE_NAME_BASE"%ld/%ld/raw"


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
static benaphore gDeviceListLock;
static char **gDeviceNames = NULL;


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
status_t	usb_disk_operation(disk_device *device, uint8 operation,
				uint8 opLength, uint32 logicalBlockAddress,
				uint16 transferLength, void *data, uint32 *dataLength,
				bool directionIn);

status_t	usb_disk_request_sense(disk_device *device);
status_t	usb_disk_test_unit_ready(disk_device *device);
status_t	usb_disk_inquiry(disk_device *device);
status_t	usb_disk_update_capacity(disk_device *device);
status_t	usb_disk_synchronize(disk_device *device, bool force);


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

	if (result > 15) {
		// invalid count, only up to 15 LUNs are possible
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
		result = acquire_sem(device->notify);
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
usb_disk_operation(disk_device *device, uint8 operation, uint8 opLength,
	uint32 logicalBlockAddress, uint16 transferLength, void *data,
	uint32 *dataLength, bool directionIn)
{
	command_block_wrapper command;
	command.signature = CBW_SIGNATURE;
	command.tag = device->current_tag++;
	command.data_transfer_length = (dataLength != NULL ? *dataLength : 0);
	command.flags = (directionIn ? CBW_DATA_INPUT : CBW_DATA_OUTPUT);
	command.lun = device->lun;
	command.command_block_length = opLength;
	memset(command.command_block, 0, sizeof(command.command_block));

	switch (opLength) {
		case 6: {
			scsi_command_6 *commandBlock = (scsi_command_6 *)command.command_block;
			commandBlock->operation = operation;
			commandBlock->lun = device->lun << 5;
			commandBlock->allocation_length = (uint8)transferLength;
			break;
		}

		case 10: {
			scsi_command_10 *commandBlock = (scsi_command_10 *)command.command_block;
			commandBlock->operation = operation;
			commandBlock->lun_flags = device->lun << 5;
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

	if (data != NULL && dataLength != NULL && *dataLength > 0) {
		// we have data to transfer in a data stage
		result = usb_disk_transfer_data(device, directionIn, data, *dataLength);
		if (result != B_OK)
			return result;

		if (device->status != B_OK || device->actual_length != *dataLength) {
			// sending or receiving of the data failed
			if (device->status == B_DEV_STALLED) {
				TRACE("stall while transfering data\n");
				gUSBModule->clear_feature(directionIn ? device->bulk_in
					: device->bulk_out, USB_FEATURE_ENDPOINT_HALT);
			} else {
				TRACE_ALWAYS("sending or receiving of the data failed\n");
				return B_ERROR;
			}
		}
	}

	command_status_wrapper status;
	result =  usb_disk_receive_csw(device, &status);
	if (result != B_OK && device->status == B_DEV_STALLED) {
		// in case of a stall clear the stall and try again
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
		case CSW_STATUS_COMMAND_FAILED: {
			if (dataLength != NULL && status.data_residue <= *dataLength)
				*dataLength -= status.data_residue;

			if (status.status == CSW_STATUS_COMMAND_PASSED) {
				// the operation is complete and has succeeded
				result = B_OK;
			} else {
				// the operation is complete but has failed at the SCSI level
				TRACE_ALWAYS("operation failed at the SCSI level\n");
				usb_disk_request_sense(device);
				result = B_ERROR;
			}
			break;
		}

		case CSW_STATUS_PHASE_ERROR: {
			// a protocol or device error occured
			TRACE_ALWAYS("phase error in operation\n");
			usb_disk_reset_recovery(device);
			if (dataLength != NULL)
				*dataLength = 0;
			result = B_ERROR;
		}
	}

	return result;
}


//
//#pragma mark - Helper/Convenience Functions
//


status_t
usb_disk_request_sense(disk_device *device)
{
	uint32 dataLength = sizeof(scsi_request_sense_6_parameter);
	scsi_request_sense_6_parameter parameter;
	status_t result = usb_disk_operation(device, SCSI_REQUEST_SENSE_6, 6, 0,
		dataLength, &parameter, &dataLength, true);
	if (result != B_OK) {
		TRACE_ALWAYS("getting request sense data failed\n");
		return result;
	}

	if (parameter.sense_key >= 2) {
		TRACE_ALWAYS("request_sense: key: 0x%02x; asc: 0x%02x; ascq: 0x%02x;\n",
			parameter.sense_key, parameter.additional_sense_code,
			parameter.additional_sense_code_qualifier);
	}

	switch (parameter.sense_key) {
		case SCSI_SENSE_KEY_NO_SENSE:
		case SCSI_SENSE_KEY_RECOVERED_ERROR:
			return B_OK;

		case SCSI_SENSE_KEY_NOT_READY:
			TRACE_ALWAYS("request_sense: device not ready\n");
			return B_DEV_NOT_READY;

		case SCSI_SENSE_KEY_MEDIUM_ERROR:
		case SCSI_SENSE_KEY_HARDWARE_ERROR:
			TRACE_ALWAYS("request_sense: no media or hardware error\n");
			return B_DEV_NO_MEDIA;

		case SCSI_SENSE_KEY_ILLEGAL_REQUEST:
			TRACE_ALWAYS("request_sense: illegal request\n");
			return B_DEV_INVALID_IOCTL;

		case SCSI_SENSE_KEY_UNIT_ATTENTION:
			TRACE_ALWAYS("request_sense: media changed\n");
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
usb_disk_test_unit_ready(disk_device *device)
{
	status_t result = usb_disk_operation(device, SCSI_TEST_UNIT_READY_6, 6, 0,
		0, NULL, NULL, true);
	if (result == B_OK)
		return B_OK;

	return usb_disk_request_sense(device);
}


status_t
usb_disk_inquiry(disk_device *device)
{
	uint32 dataLength = sizeof(scsi_inquiry_6_parameter);
	scsi_inquiry_6_parameter parameter;
	status_t result = usb_disk_operation(device, SCSI_INQUIRY_6, 6, 0,
		dataLength, &parameter, &dataLength, true);
	if (result != B_OK) {
		TRACE_ALWAYS("getting inquiry data failed\n");
		device->removable = true;
		return result;
	}

	TRACE("peripherial_device_type. 0x%02x\n", parameter.peripherial_device_type);
	TRACE("peripherial_qualifier... 0x%02x\n", parameter.peripherial_qualifier);
	TRACE("removable_medium........ %s\n", parameter.removable_medium ? "yes" : "no");
	TRACE("version................. 0x%02x\n", parameter.version);
	TRACE("response_data_format.... 0x%02x\n", parameter.response_data_format);	
	TRACE("vendor_identification... \"%.8s\"\n", parameter.vendor_identification);	
	TRACE("product_identification.. \"%.16s\"\n", parameter.product_identification);	
	TRACE("product_revision_level.. \"%.4s\"\n", parameter.product_revision_level);	
	device->device_type = parameter.peripherial_device_type; /* 1:1 mapping */
	device->removable = (parameter.removable_medium == 1);
	return B_OK;
}


status_t
usb_disk_update_capacity(disk_device *device)
{
	uint32 dataLength = sizeof(scsi_read_capacity_10_parameter);
	scsi_read_capacity_10_parameter parameter;
	status_t result = usb_disk_operation(device, SCSI_READ_CAPACITY_10, 10, 0,
		0, &parameter, &dataLength, true);
	if (result != B_OK) {
		TRACE_ALWAYS("failed to update capacity\n");
		device->block_size = 1;
		device->block_count = 0;
		return result;
	}

	device->block_size = ntohl(parameter.logical_block_length);
	device->block_count = ntohl(parameter.last_logical_block_address) + 1;
	return B_OK;
}


status_t
usb_disk_synchronize(disk_device *device, bool force)
{
	if (device->should_sync || force) {
		status_t result = usb_disk_operation(device, SCSI_SYNCHRONIZE_CACHE_10,
			10, 0, 0, NULL, NULL, false);
		device->should_sync = false;
		return result;
	}

	return B_OK;
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
	device->should_sync = false;
	device->lun = 0;

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

		if (interface->descr->interface_class == 0x08 /* mass storage */
			&& interface->descr->interface_subclass == 0x06 /* SCSI */
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
			break;
		}
	}

	if (device->interface == 0xff) {
		TRACE_ALWAYS("no valid bulk-only interface found\n");
		return B_ERROR;
	}

	status_t result = benaphore_init(&device->lock, "usb_disk device lock");
	if (result < B_OK) {
		free(device);
		return result;
	}

	device->notify = create_sem(0, "usb_disk callback notify");
	if (device->notify < B_OK) {
		benaphore_destroy(&device->lock);
		free(device);
		return B_NO_MORE_SEMS;
	}

	result = usb_disk_inquiry(device);
	for (uint32 tries = 0; tries < 3; tries++) {
		if (usb_disk_test_unit_ready(device) == B_OK)
			break;
		snooze(10000);
	}

	if (result != B_OK || usb_disk_update_capacity(device) != B_OK) {
		TRACE_ALWAYS("failed to read device data\n");
		delete_sem(device->notify);
		benaphore_destroy(&device->lock);
		free(device);
		return B_ERROR;
	}

	benaphore_lock(&gDeviceListLock);
	device->link = (void *)gDeviceList;
	gDeviceList = device;
	uint32 deviceNumber = gDeviceCount++;

	// ToDo: find out about LUNs and publish those too
	//uint8 maxLun = usb_disk_get_max_lun(device);
	//TRACE("device has max lun %d\n", maxLun);
	sprintf(device->name, DEVICE_NAME, deviceNumber, 0L);
	benaphore_unlock(&gDeviceListLock);

	TRACE("new device: 0x%08lx\n", (uint32)device);
	*cookie = (void *)device;
	return B_OK;
}


static status_t
usb_disk_device_removed(void *cookie)
{
	TRACE("device_removed(0x%08lx)\n", (uint32)cookie);
	disk_device *device = (disk_device *)cookie;

	benaphore_lock(&gDeviceListLock);
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
	gDeviceCount--;
	benaphore_unlock(&gDeviceListLock);

	device->device = 0;
	device->removed = true;
	if (device->open_count == 0) {
		benaphore_lock(&device->lock);
		benaphore_destroy(&device->lock);
		delete_sem(device->notify);
		free(device);
	}

	return B_OK;
}


//
//#pragma mark - Partial Buffer Functions
//


static bool
usb_disk_needs_partial_buffer(disk_device *device, off_t position,
	size_t length, uint32 &blockPosition, uint16 &blockCount)
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
usb_disk_block_read(disk_device *device, uint32 blockPosition,
	uint16 blockCount, void *buffer, size_t *length)
{
	status_t result = usb_disk_operation(device, SCSI_READ_10, 10,
		blockPosition, blockCount, buffer, length, true);
	return result;
}


static status_t
usb_disk_block_write(disk_device *device, uint32 blockPosition,
	uint16 blockCount, void *buffer, size_t *length)
{
	status_t result = usb_disk_operation(device, SCSI_WRITE_10, 10,
		blockPosition, blockCount, buffer, length, false);
	if (result == B_OK)
		device->should_sync = true;
	return result;
}


static status_t
usb_disk_prepare_partial_buffer(disk_device *device, off_t position,
	size_t length, void *&partialBuffer, void *&blockBuffer,
	uint32 &blockPosition, uint16 &blockCount)
{
	blockPosition = (uint32)(position / device->block_size);
	blockCount = (uint16)((uint32)((position + length + device->block_size - 1)
		/ device->block_size) - blockPosition);
	size_t blockLength = blockCount * device->block_size;
	blockBuffer = malloc(blockLength);
	if (blockBuffer == NULL) {
		TRACE_ALWAYS("no memory to allocate partial buffer\n");
		return B_NO_MEMORY;
	}

	status_t result = usb_disk_block_read(device, blockPosition, blockCount,
		blockBuffer, &blockLength);
	if (result != B_OK) {
		TRACE_ALWAYS("block read failed when filling partial buffer\n");
		free(blockBuffer);
		return result;
	}

	off_t offset = position - (blockPosition * device->block_size);
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
	for (int32 i = strlen(name) - 1; i >= 0; i--) {
		if (name[i] == '/') {
			lastPart = i;
			break;
		}
	}

	char rawName[32];
	strlcpy(rawName, name, lastPart + 2);
	strcat(rawName, "raw");
	TRACE("opening raw device %s for %s\n", rawName, name);

	benaphore_lock(&gDeviceListLock);
	disk_device *device = gDeviceList;
	while (device) {
		if (!device->removed && strncmp(rawName, device->name, 32) == 0) {
			device->open_count++;
			*cookie = device;
			benaphore_unlock(&gDeviceListLock);
			return B_OK;
		}

		device = (disk_device *)device->link;
	}

	benaphore_unlock(&gDeviceListLock);
	return B_NAME_NOT_FOUND;
}


static status_t
usb_disk_close(void *cookie)
{
	TRACE("close()\n");
	disk_device *device = (disk_device *)cookie;
	usb_disk_synchronize(device, false);
	return B_OK;
}


static status_t
usb_disk_free(void *cookie)
{
	TRACE("free()\n");
	benaphore_lock(&gDeviceListLock);

	disk_device *device = (disk_device *)cookie;
	device->open_count--;
	if (device->removed && device->open_count == 0) {
		benaphore_lock(&device->lock);
		benaphore_destroy(&device->lock);
		delete_sem(device->notify);
		free(device);
	}

	benaphore_unlock(&gDeviceListLock);
	return B_OK;
}


static status_t
usb_disk_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	disk_device *device = (disk_device *)cookie;
	if (device->removed)
		return B_DEV_NOT_READY;

	benaphore_lock(&device->lock);
	status_t result = B_DEV_INVALID_IOCTL;

	switch (op) {
		case B_GET_MEDIA_STATUS: {
			*(status_t *)buffer = usb_disk_test_unit_ready(device);
			TRACE("B_GET_MEDIA_STATUS: 0x%08lx\n", *(status_t *)buffer);
			result = B_OK;
			break;
		}

		case B_GET_GEOMETRY: {
			device_geometry *geometry = (device_geometry *)buffer;
			geometry->bytes_per_sector = device->block_size;
			geometry->cylinder_count = device->block_count;
			geometry->sectors_per_track = geometry->head_count = 1;
			geometry->device_type = device->device_type;
			geometry->removable = device->removable;
			geometry->read_only = (device->device_type == B_CD);
			geometry->write_once = (device->device_type == B_WORM);
			TRACE("B_GET_GEOMETRY: %ld sectors at %ld bytes per sector\n",
				geometry->cylinder_count, geometry->bytes_per_sector);
			result = B_OK;
			break;
		}

		case B_FLUSH_DRIVE_CACHE:
			usb_disk_synchronize(device, true);
			break;

		default:
			TRACE_ALWAYS("unhandled ioctl %ld\n", op);
			break;
	}

	benaphore_unlock(&device->lock);
	return result;
}


static status_t
usb_disk_read(void *cookie, off_t position, void *buffer, size_t *length)
{
	TRACE("read(%lld, %ld)\n", position, *length);
	disk_device *device = (disk_device *)cookie;
	if (device->removed) {
		*length = 0;
		return B_DEV_NOT_READY;
	}

	if (buffer == NULL || length == NULL)
		return B_BAD_VALUE;

	benaphore_lock(&device->lock);
	status_t result = B_ERROR;
	uint32 blockPosition = 0;
	uint16 blockCount = 0;
	bool needsPartial = usb_disk_needs_partial_buffer(device, position,
		*length, blockPosition, blockCount);
	if (needsPartial) {
		void *partialBuffer = NULL;
		void *blockBuffer = NULL;
		result = usb_disk_prepare_partial_buffer(device, position, *length,
			partialBuffer, blockBuffer, blockPosition, blockCount);
		if (result == B_OK) {
			memcpy(buffer, partialBuffer, *length);
			free(blockBuffer);
		}
	} else {
		result = usb_disk_block_read(device, blockPosition, blockCount,
			buffer, length);
	}

	benaphore_unlock(&device->lock);
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
	TRACE("write(%lld, %ld)\n", position, *length);
	disk_device *device = (disk_device *)cookie;
	if (device->removed) {
		*length = 0;
		return B_DEV_NOT_READY;
	}

	if (buffer == NULL || length == NULL)
		return B_BAD_VALUE;

	benaphore_lock(&device->lock);
	status_t result = B_ERROR;
	uint32 blockPosition = 0;
	uint16 blockCount = 0;
	bool needsPartial = usb_disk_needs_partial_buffer(device, position,
		*length, blockPosition, blockCount);
	if (needsPartial) {
		void *partialBuffer = NULL;
		void *blockBuffer = NULL;
		result = usb_disk_prepare_partial_buffer(device, position, *length,
			partialBuffer, blockBuffer, blockPosition, blockCount);
		if (result == B_OK) {
			memcpy(partialBuffer, buffer, *length);
			size_t blockLength = blockCount * device->block_size;
			result = usb_disk_block_write(device, blockPosition, blockCount,
				blockBuffer, &blockLength);
			free(blockBuffer);
		}
	} else {
		result = usb_disk_block_write(device, blockPosition, blockCount,
			(void *)buffer, length);
	}

	benaphore_unlock(&device->lock);
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

	static usb_support_descriptor supportedDevices = {
		0x08 /* mass storage */, 0x06 /* SCSI */, 0x50 /* bulk only */, 0, 0
	};

	gDeviceList = NULL;
	gDeviceCount = 0;
	status_t result = benaphore_init(&gDeviceListLock, "usb_disk device list lock");
	if (result < B_OK) {
		TRACE("failed to create device list lock\n");
		return result;
	}

	TRACE("trying module %s\n", B_USB_MODULE_NAME);
	result = get_module(B_USB_MODULE_NAME, (module_info **)&gUSBModule);
	if (result < B_OK) {
		TRACE_ALWAYS("getting module failed 0x%08lx\n", result);
		benaphore_destroy(&gDeviceListLock);
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
	benaphore_lock(&gDeviceListLock);

	if (gDeviceNames) {
		for (int32 i = 0; gDeviceNames[i]; i++)
			free(gDeviceNames[i]);
		free(gDeviceNames);
		gDeviceNames = NULL;
	}

	benaphore_destroy(&gDeviceListLock);
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
	benaphore_lock(&gDeviceListLock);
	disk_device *device = gDeviceList;
	while (device) {
		gDeviceNames[index++] = strdup(device->name);
		device = (disk_device *)device->link;
	}

	gDeviceNames[index++] = NULL;
	benaphore_unlock(&gDeviceListLock);
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
