/*
	Driver for USB RNDIS Network devices
	Copyright (C) 2022 Adrien Destugues <pulkomandy@pulkomandy.tk>
	Distributed under the terms of the MIT license.
*/
#include <ether_driver.h>
#include <net/if_media.h>
#include <string.h>
#include <stdlib.h>

#include "RNDISDevice.h"
#include "Driver.h"


const uint32 OID_GEN_MAXIMUM_FRAME_SIZE = 0x00010106;
const uint32 OID_GEN_LINK_SPEED = 0x00010107;
const uint32 OID_GEN_CURRENT_PACKET_FILTER = 0x0001010E;
const uint32 OID_GEN_MEDIA_CONNECT_STATUS = 0x00010114;
const uint32 OID_802_3_PERMANENT_ADDRESS = 0x01010101;


enum RndisCommands {
	REMOTE_NDIS_PACKET_MSG = 1,
	REMOTE_NDIS_INITIALIZE_MSG = 2,
	REMOTE_NDIS_INITIALIZE_CMPLT = 0x80000002,
	REMOTE_NDIS_HALT_MSG = 3,
	REMOTE_NDIS_QUERY_MSG = 4,
	REMOTE_NDIS_QUERY_CMPLT = 0x80000004,
	REMOTE_NDIS_SET_MSG = 5,
	REMOTE_NDIS_SET_CMPLT = 0x80000005,
	REMOTE_NDIS_RESET_MSG = 6,
	REMOTE_NDIS_RESET_CMPLT = 0x80000006,
	REMOTE_NDIS_INDICATE_STATUS_MSG = 7,
	REMOTE_NDIS_KEEPALIVE_MSG = 8,
	REMOTE_NDIS_KEEPALIVE_CMPLT = 0x80000008
};


enum Status
{
	RNDIS_STATUS_SUCCESS = 0,
	RNDIS_STATUS_FAILURE = 0xC0000001,
	RNDIS_STATUS_INVALID_DATA = 0xC0010015,
	RNDIS_STATUS_NOT_SUPPORTED = 0xC00000BB,
	RNDIS_STATUS_MEDIA_CONNECT = 0x4001000B,
	RNDIS_STATUS_MEDIA_DISCONNECT = 0x4001000C,
};


enum MediaConnectStatus
{
	MEDIA_STATE_UNKNOWN,
	MEDIA_STATE_CONNECTED,
	MEDIA_STATE_DISCONNECTED
};


const uint32 NDIS_PACKET_TYPE_ALL_MULTICAST = 0x00000004;
const uint32 NDIS_PACKET_TYPE_BROADCAST = 0x00000008;



RNDISDevice::RNDISDevice(usb_device device)
	:	fStatus(B_ERROR),
		fOpen(false),
		fRemoved(false),
		fInsideNotify(0),
		fDevice(device),
		fDataInterfaceIndex(0),
		fMaxSegmentSize(0),
		fNotifyEndpoint(0),
		fReadEndpoint(0),
		fWriteEndpoint(0),
		fNotifyReadSem(-1),
		fNotifyWriteSem(-1),
		fLockWriteSem(-1),
		fNotifyControlSem(-1),
		fReadHeader(NULL),
		fLinkStateChangeSem(-1),
		fMediaConnectState(MEDIA_STATE_UNKNOWN),
		fDownstreamSpeed(0)
{
	const usb_device_descriptor *deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	if (deviceDescriptor == NULL) {
		TRACE_ALWAYS("failed to get device descriptor\n");
		return;
	}

	fVendorID = deviceDescriptor->vendor_id;
	fProductID = deviceDescriptor->product_id;

	fNotifyReadSem = create_sem(0, DRIVER_NAME"_notify_read");
	if (fNotifyReadSem < B_OK) {
		TRACE_ALWAYS("failed to create read notify sem\n");
		return;
	}

	fNotifyWriteSem = create_sem(0, DRIVER_NAME"_notify_write");
	if (fNotifyWriteSem < B_OK) {
		TRACE_ALWAYS("failed to create write notify sem\n");
		return;
	}

	fLockWriteSem = create_sem(1, DRIVER_NAME"_lock_write");
	if (fNotifyWriteSem < B_OK) {
		TRACE_ALWAYS("failed to create write lock sem\n");
		return;
	}

	fNotifyControlSem = create_sem(0, DRIVER_NAME"_notify_control");
	if (fNotifyControlSem < B_OK) {
		TRACE_ALWAYS("failed to create control notify sem\n");
		return;
	}

	if (_SetupDevice() != B_OK) {
		TRACE_ALWAYS("failed to setup device\n");
		return;
	}

	fStatus = B_OK;
}


RNDISDevice::~RNDISDevice()
{
	if (fNotifyReadSem >= B_OK)
		delete_sem(fNotifyReadSem);
	if (fNotifyWriteSem >= B_OK)
		delete_sem(fNotifyWriteSem);
	if (fLockWriteSem >= B_OK)
		delete_sem(fLockWriteSem);
	if (fNotifyControlSem >= B_OK)
		delete_sem(fNotifyControlSem);

	if (!fRemoved)
		gUSBModule->cancel_queued_transfers(fNotifyEndpoint);
}


status_t
RNDISDevice::Open()
{
	if (fOpen)
		return B_BUSY;
	if (fRemoved)
		return B_ERROR;

	// reset the device by switching the data interface to the disabled first
	// interface and then enable it by setting the second actual data interface
	const usb_configuration_info *config
		= gUSBModule->get_configuration(fDevice);

	usb_interface_info *interface = config->interface[fDataInterfaceIndex].active;
	if (interface->endpoint_count < 2) {
		TRACE_ALWAYS("setting the data alternate interface failed\n");
		return B_ERROR;
	}

	if (!(interface->endpoint[0].descr->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN))
		fWriteEndpoint = interface->endpoint[0].handle;
	else
		fReadEndpoint = interface->endpoint[0].handle;

	if (interface->endpoint[1].descr->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN)
		fReadEndpoint = interface->endpoint[1].handle;
	else
		fWriteEndpoint = interface->endpoint[1].handle;

	if (fReadEndpoint == 0 || fWriteEndpoint == 0) {
		TRACE_ALWAYS("no read and write endpoints found\n");
		return B_ERROR;
	}

	if (gUSBModule->queue_interrupt(fNotifyEndpoint, &fNotifyBuffer,
		sizeof(fNotifyBuffer), _NotifyCallback, this) != B_OK) {
		TRACE_ALWAYS("failed to setup notification interrupt\n");
		return B_ERROR;
	}

	status_t status = _RNDISInitialize();
	if (status != B_OK) {
		TRACE_ALWAYS("failed to initialize RNDIS device\n");
		return status;
	}

	status = _ReadMACAddress(fDevice, fMACAddress);
	if (status != B_OK) {
		TRACE_ALWAYS("failed to read mac address\n");
		return status;
	}

	// TODO these are non-fatal but make sure we have sane defaults for them
	status = _ReadMaxSegmentSize(fDevice);
	if (status != B_OK) {
		TRACE_ALWAYS("failed to read fragment size\n");
	}

	status = _ReadMediaState(fDevice);
	if (status != B_OK) {
		fMediaConnectState = MEDIA_STATE_CONNECTED;
		TRACE_ALWAYS("failed to read media state\n");
	}

	status = _ReadLinkSpeed(fDevice);
	if (status != B_OK) {
		fDownstreamSpeed = 1000 * 100; // 10Mbps
		TRACE_ALWAYS("failed to read link speed\n");
	}

	// Tell the device to connect
	status = _EnableBroadcast(fDevice);
	TRACE("Initialization result: %s\n", strerror(status));

	// the device should now be ready
	if (status == B_OK)
		fOpen = true;
	return status;
}


status_t
RNDISDevice::Close()
{
	if (fRemoved) {
		fOpen = false;
		return B_OK;
	}

	// TODO tell the device to disconnect?

	gUSBModule->cancel_queued_transfers(fNotifyEndpoint);
	gUSBModule->cancel_queued_transfers(fReadEndpoint);
	gUSBModule->cancel_queued_transfers(fWriteEndpoint);

	fOpen = false;
	return B_OK;
}


status_t
RNDISDevice::Free()
{
	return B_OK;
}


status_t
RNDISDevice::Read(uint8 *buffer, size_t *numBytes)
{
	if (fRemoved) {
		*numBytes = 0;
		return B_DEVICE_NOT_FOUND;
	}

	// The read funcion can return only one packet at a time, but we can receive multiple ones in
	// a single USB transfer. So we need to buffer them, and check if we have something in our
	// buffer for each Read() call before scheduling a new USB transfer. This would be more
	// efficient if the network stack had a way to read multiple frames at once.
	if (fReadHeader == NULL) {
		status_t result = gUSBModule->queue_bulk(fReadEndpoint, fReadBuffer, sizeof(fReadBuffer),
			_ReadCallback, this);
		if (result != B_OK) {
			*numBytes = 0;
			return result;
		}

		result = acquire_sem_etc(fNotifyReadSem, 1, B_CAN_INTERRUPT, 0);
		if (result < B_OK) {
			*numBytes = 0;
			return result;
		}

		if (fStatusRead != B_OK && fStatusRead != B_CANCELED && !fRemoved) {
			TRACE_ALWAYS("device status error 0x%08" B_PRIx32 "\n", fStatusRead);
			result = gUSBModule->clear_feature(fReadEndpoint,
				USB_FEATURE_ENDPOINT_HALT);
			if (result != B_OK) {
				TRACE_ALWAYS("failed to clear halt state on read\n");
				*numBytes = 0;
				return result;
			}
		}
		fReadHeader = (uint32*)fReadBuffer;
	}

	if (fReadHeader[0] != REMOTE_NDIS_PACKET_MSG) {
		TRACE_ALWAYS("Received unexpected packet type %08" B_PRIx32 " on data link\n",
			fReadHeader[0]);
		*numBytes = 0;
		fReadHeader = NULL;
		return B_BAD_VALUE;
	}

	if (fReadHeader[1] + ((uint8*)fReadHeader - fReadBuffer) > fActualLengthRead) {
		TRACE_ALWAYS("Received frame at %ld length %08" B_PRIx32 " out of bounds of receive buffer"
			"%08" B_PRIx32 "\n", (uint8*) fReadHeader - fReadBuffer, fReadHeader[1],
			fActualLengthRead);
	}

	if (fReadHeader[2] + fReadHeader[3] > fReadHeader[1]) {
		TRACE_ALWAYS("Received frame data goes past end of frame: %" B_PRIu32 " + %" B_PRIu32
			" > %" B_PRIu32, fReadHeader[2], fReadHeader[3], fReadHeader[1]);
	}

	if (fReadHeader[4] != 0 || fReadHeader[5] != 0 || fReadHeader[6] != 0) {
		TRACE_ALWAYS("Received frame has out of band data: off %08" B_PRIx32 " len %08" B_PRIx32
			" count %08" B_PRIx32 "\n", fReadHeader[4], fReadHeader[5], fReadHeader[6]);
	}

	if (fReadHeader[7] != 0 || fReadHeader[8] != 0) {
		TRACE_ALWAYS("Received frame has per-packet info: off %08" B_PRIx32 " len %08" B_PRIx32
			"\n", fReadHeader[7], fReadHeader[8]);
	}

	if (fReadHeader[9] != 0) {
		TRACE_ALWAYS("Received frame has non-0 reserved field %08" B_PRIx32 "\n", fReadHeader[9]);
	}

	*numBytes = fReadHeader[3];
	int offset = fReadHeader[2] + 2 * sizeof(uint32);
	memcpy(buffer, (uint8*)fReadHeader + offset, fReadHeader[3]);

	TRACE("Received data packet len %08" B_PRIx32 " data [off %08" B_PRIx32 " len %08" B_PRIx32 "]\n",
		fReadHeader[1], fReadHeader[2], fReadHeader[3]);

	// Advance to next packet
	fReadHeader = (uint32*)((uint8*)fReadHeader + fReadHeader[1]);

	// Are we past the end of the buffer? If so, prepare to receive another one on the next read
	if ((uint32)((uint8*)fReadHeader - fReadBuffer) >= fActualLengthRead)
		fReadHeader = NULL;

	return B_OK;
}


class SemLocker {
public:
	SemLocker(sem_id sem)
		: fSem(sem)
	{
		fStatus = acquire_sem(fSem);
	}

	~SemLocker()
	{
		if (fStatus == B_OK)
			release_sem(fSem);
	}

	status_t fStatus;
private:
	sem_id fSem;
};


status_t
RNDISDevice::Write(const uint8 *buffer, size_t *numBytes)
{
	if (fRemoved) {
		*numBytes = 0;
		return B_DEVICE_NOT_FOUND;
	}

	iovec vec[2];

	uint32 header[11] = { 0 };
	header[0] = REMOTE_NDIS_PACKET_MSG;
	header[1] = *numBytes + sizeof(header);
	header[2] = 0x24;
	header[3] = *numBytes;

	vec[0].iov_base = &header;
	vec[0].iov_len = sizeof(header);

	vec[1].iov_base = (void*)buffer;
	vec[1].iov_len = *numBytes;

	SemLocker mutex(fLockWriteSem);
	status_t result = mutex.fStatus;
	if (result < B_OK) {
		*numBytes = 0;
		return result;
	}

	result = gUSBModule->queue_bulk_v(fWriteEndpoint, vec, 2, _WriteCallback, this);
	if (result != B_OK) {
		*numBytes = 0;
		return result;
	}

	do {
		result = acquire_sem_etc(fNotifyWriteSem, 1, B_CAN_INTERRUPT, 0);
	} while (result == B_INTERRUPTED);

	if (result < B_OK) {
		*numBytes = 0;
		return result;
	}

	if (fStatusWrite != B_OK && fStatusWrite != B_CANCELED && !fRemoved) {
		TRACE_ALWAYS("device status error 0x%08" B_PRIx32 "\n", fStatusWrite);
		result = gUSBModule->clear_feature(fWriteEndpoint,
			USB_FEATURE_ENDPOINT_HALT);
		if (result != B_OK) {
			TRACE_ALWAYS("failed to clear halt state on write\n");
			*numBytes = 0;
			return result;
		}
	}

	*numBytes = fActualLengthWrite;

	return B_OK;
}


status_t
RNDISDevice::Control(uint32 op, void *buffer, size_t length)
{
	switch (op) {
		case ETHER_INIT:
			return B_OK;

		case ETHER_GETADDR:
			memcpy(buffer, &fMACAddress, sizeof(fMACAddress));
			return B_OK;

		case ETHER_GETFRAMESIZE:
			*(uint32 *)buffer = fMaxSegmentSize;
			return B_OK;

		case ETHER_SET_LINK_STATE_SEM:
			fLinkStateChangeSem = *(sem_id *)buffer;
			return B_OK;

		case ETHER_GET_LINK_STATE:
		{
			ether_link_state *state = (ether_link_state *)buffer;
			// FIXME get media duplex state from OID_GEN_LINK_STATE if supported
			state->media = IFM_ETHER | IFM_FULL_DUPLEX;
			if (fMediaConnectState != MEDIA_STATE_DISCONNECTED)
				state->media |= IFM_ACTIVE;
			state->quality = 1000;
			state->speed = fDownstreamSpeed * 100;
			return B_OK;
		}

		default:
			TRACE_ALWAYS("unsupported ioctl %" B_PRIu32 "\n", op);
	}

	return B_DEV_INVALID_IOCTL;
}


void
RNDISDevice::Removed()
{
	fRemoved = true;
	fMediaConnectState = MEDIA_STATE_DISCONNECTED;
	fDownstreamSpeed = 0;

	// the notify hook is different from the read and write hooks as it does
	// itself schedule traffic (while the other hooks only release a semaphore
	// to notify another thread which in turn safly checks for the removed
	// case) - so we must ensure that we are not inside the notify hook anymore
	// before returning, as we would otherwise violate the promise not to use
	// any of the pipes after returning from the removed hook
	while (atomic_add(&fInsideNotify, 0) != 0)
		snooze(100);

	gUSBModule->cancel_queued_transfers(fNotifyEndpoint);
	gUSBModule->cancel_queued_transfers(fReadEndpoint);
	gUSBModule->cancel_queued_transfers(fWriteEndpoint);

	if (fLinkStateChangeSem >= B_OK)
		release_sem_etc(fLinkStateChangeSem, 1, B_DO_NOT_RESCHEDULE);
}


status_t
RNDISDevice::_SendCommand(const void* data, size_t length)
{
	size_t actualLength;
	return gUSBModule->send_request(fDevice, USB_REQTYPE_INTERFACE_OUT | USB_REQTYPE_CLASS,
		USB_CDC_SEND_ENCAPSULATED_COMMAND, 0, 0, length, (void*)data, &actualLength);
}


status_t
RNDISDevice::_ReadResponse(void* data, size_t length)
{
	size_t actualLength;
	return gUSBModule->send_request(fDevice, USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_CLASS,
		USB_CDC_GET_ENCAPSULATED_RESPONSE, 0, 0, length, data, &actualLength);
}


status_t
RNDISDevice::_RNDISInitialize()
{
	uint32 request[] = {
		REMOTE_NDIS_INITIALIZE_MSG,
		6 * sizeof(uint32), // MessageLength
		0x00000000, // RequestID
		0x00000001, 0x00000000, // Version (major, minor)
		0x00004000 // MaxTransferSize
	};

	status_t result = _SendCommand(request, sizeof(request));
	TRACE("Send init command results in %s\n", strerror(result));

	acquire_sem(fNotifyControlSem);

	TRACE("Received notification after init command\n");

	uint32 response[0x34 / 4];

	result = _ReadResponse(response, sizeof(response));
	TRACE("Read init command results in %s\n", strerror(result));
	if (result != B_OK)
		return result;

	TRACE("Type      %" B_PRIx32 "\n", response[0]);
	TRACE("Length    %" B_PRIx32 "\n", response[1]);
	TRACE("Req ID    %" B_PRIx32 "\n", response[2]);
	TRACE("Status    %" B_PRIx32 "\n", response[3]);
	TRACE("Vers Maj  %" B_PRIx32 "\n", response[4]);
	TRACE("Vers Min  %" B_PRIx32 "\n", response[5]);
	TRACE("DevFlags  %" B_PRIx32 "\n", response[6]);
	TRACE("Medium    %" B_PRIx32 "\n", response[7]);
	TRACE("Max Pkts  %" B_PRIx32 "\n", response[8]);
	TRACE("Max Bytes %" B_PRIx32 "\n", response[9]);
	TRACE("Alignment %" B_PRIx32 "\n", response[10]);
	TRACE("Reserved  ");
	for (int i = 11; i < 0x34 / 4; i++)
		TRACE("%" B_PRIx32 " ", response[i]);
	TRACE("\n");

	// TODO configure stuff until we get a SET_CPLT message meaning everything is configured
	// TODO set up a notification sytem to be notified if these change? Do we have OIDs for them?
	// OID_GEN_HARDWARE_STATUS, OID_GEN_MEDIA_IN_USE

	return B_OK;
}


status_t
RNDISDevice::_SetupDevice()
{
	const usb_device_descriptor *deviceDescriptor
                = gUSBModule->get_device_descriptor(fDevice);

	if (deviceDescriptor == NULL) {
		TRACE_ALWAYS("failed to get device descriptor\n");
		return B_ERROR;
	}

	uint8 controlIndex = 0;
	uint8 dataIndex = 0;
	bool foundUnionDescriptor = false;
	bool foundCMDescriptor = false;
	bool found = false;
	const usb_configuration_info *config = NULL;
	for (int i = 0; i < deviceDescriptor->num_configurations && !found; i++) {
		config = gUSBModule->get_nth_configuration(fDevice, i);
		if (config == NULL)
			continue;

		for (size_t j = 0; j < config->interface_count && !found; j++) {
			const usb_interface_info *interface = config->interface[j].active;
			usb_interface_descriptor *descriptor = interface->descr;
			if (descriptor->interface_class != USB_COMMUNICATION_WIRELESS_DEVICE_CLASS
				|| descriptor->interface_subclass != 0x01
				|| descriptor->interface_protocol != 0x03
				|| interface->generic_count == 0) {
				continue;
			}

			// try to find and interpret the union and call management functional
			// descriptors (they allow us to locate the data interface)
			foundUnionDescriptor = foundCMDescriptor = false;
			for (size_t k = 0; k < interface->generic_count; k++) {
				usb_generic_descriptor *generic = &interface->generic[k]->generic;
				if (generic->length >= sizeof(usb_cdc_union_functional_descriptor)
					&& generic->data[0] == USB_CDC_UNION_FUNCTIONAL_DESCRIPTOR) {
					controlIndex = generic->data[1];
					foundUnionDescriptor = true;
				} else if (generic->length >= sizeof(usb_cdc_cm_functional_descriptor)
					&& generic->data[0] == USB_CDC_CM_FUNCTIONAL_DESCRIPTOR) {
					usb_cdc_cm_functional_descriptor *cm
						= (usb_cdc_cm_functional_descriptor *)generic;
					dataIndex = cm->data_interface;
					foundCMDescriptor = true;
				}

				if (foundUnionDescriptor && foundCMDescriptor) {
					found = true;
					break;
				}
			}
		}
	}

	if (!foundUnionDescriptor) {
		TRACE_ALWAYS("did not find a union descriptor\n");
		return B_ERROR;
	}

	if (!foundCMDescriptor) {
		TRACE_ALWAYS("did not find an ethernet descriptor\n");
		return B_ERROR;
	}

	// set the current configuration
	gUSBModule->set_configuration(fDevice, config);
	if (controlIndex >= config->interface_count) {
		TRACE_ALWAYS("control interface index invalid\n");
		return B_ERROR;
	}

	// check that the indicated control interface fits our needs
	usb_interface_info *interface = config->interface[controlIndex].active;
	usb_interface_descriptor *descriptor = interface->descr;
	if ((descriptor->interface_class != 0xE0
		|| descriptor->interface_subclass != 0x01
		|| descriptor->interface_protocol != 0x03)
		|| interface->endpoint_count == 0) {
		TRACE_ALWAYS("control interface invalid\n");
		return B_ERROR;
	}

	fNotifyEndpoint = interface->endpoint[0].handle;
	if (interface->endpoint[0].descr->max_packet_size > sizeof(fNotifyBuffer)) {
		TRACE_ALWAYS("Notify buffer is too small, need at least %d bytes\n",
			interface->endpoint[0].descr->max_packet_size);
		return B_ERROR;
	}

	if (dataIndex >= config->interface_count) {
		TRACE_ALWAYS("data interface index %d out of range %" B_PRIuSIZE "\n", dataIndex,
			config->interface_count);
		return B_ERROR;
	}

	interface = &config->interface[dataIndex].alt[0];
	descriptor = interface->descr;
	if (descriptor->interface_class != USB_CDC_DATA_INTERFACE_CLASS
		|| interface->endpoint_count < 2) {
		TRACE_ALWAYS("data interface %d invalid (class %x, %" B_PRIuSIZE " endpoints)\n", dataIndex,
			descriptor->interface_class, interface->endpoint_count);
		return B_ERROR;
	}

	fDataInterfaceIndex = dataIndex;
	return B_OK;
}


status_t
RNDISDevice::_GetOID(uint32 oid, void* buffer, size_t length)
{
	uint32 request[] = {
		REMOTE_NDIS_QUERY_MSG,
		7 * sizeof(uint32), // Length of the request
		0x00000001, // Request ID (FIXME generate this dynamically if we need multiple requests in
					// flight, so we can match up the replies with the different requests)
		oid,
		0, 0, 0
	};

	status_t result = _SendCommand(request, sizeof(request));
	if (result != B_OK)
		return result;

	acquire_sem(fNotifyControlSem);

	uint8 response[length + 24];
	result = _ReadResponse(response, length + 24);
	memcpy(buffer, &response[24], length);
	return result;
}


status_t
RNDISDevice::_ReadMACAddress(usb_device device, uint8 *buffer)
{
	status_t result = _GetOID(OID_802_3_PERMANENT_ADDRESS, buffer, 6);
	if (result != B_OK)
		return result;

	TRACE_ALWAYS("mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
		buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
	return B_OK;
}


status_t
RNDISDevice::_ReadMaxSegmentSize(usb_device device)
{
	status_t result = _GetOID(OID_GEN_MAXIMUM_FRAME_SIZE, &fMaxSegmentSize,
		sizeof(fMaxSegmentSize));
	if (result != B_OK)
		return result;

	TRACE_ALWAYS("max frame size: %" B_PRId32 "\n", fMaxSegmentSize);
	return B_OK;
}


status_t
RNDISDevice::_ReadMediaState(usb_device device)
{
	status_t result = _GetOID(OID_GEN_MEDIA_CONNECT_STATUS, &fMediaConnectState,
		sizeof(fMediaConnectState));
	if (result != B_OK)
		return result;

	TRACE_ALWAYS("media connect state: %" B_PRId32 "\n", fMediaConnectState);
	return B_OK;
}


status_t
RNDISDevice::_ReadLinkSpeed(usb_device device)
{
	status_t result = _GetOID(OID_GEN_LINK_SPEED, &fDownstreamSpeed,
		sizeof(fDownstreamSpeed));
	if (result != B_OK)
		return result;

	TRACE_ALWAYS("link speed: %" B_PRId32 " * 100bps\n", fDownstreamSpeed);
	return B_OK;
}


status_t
RNDISDevice::_EnableBroadcast(usb_device device)
{
	uint32 request[] = {
		REMOTE_NDIS_SET_MSG,
		8 * sizeof(uint32), // Length of the request
		0x00000001, // Request ID (FIXME generate this dynamically if we need multiple requests in
					// flight, so we can match up the replies with the different requests)
		OID_GEN_CURRENT_PACKET_FILTER,
		0x14, // buffer length
		0x14, // buffer offset
		0, // reserved
		NDIS_PACKET_TYPE_ALL_MULTICAST | NDIS_PACKET_TYPE_BROADCAST
	};

	status_t result = _SendCommand(request, sizeof(request));
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to start traffic (set oid: %s)\n", strerror(result));
		return result;
	}

	acquire_sem(fNotifyControlSem);

	uint32 response[4];
	result = _ReadResponse(response, 4 * sizeof(uint32));
	if (result != B_OK) {
		TRACE_ALWAYS("Failed to start traffic (response: %s)\n", strerror(result));
		return result;
	}

	// TODO check other fields in the response (message type, length, request id) match our request

	switch (response[3])
	{
		case RNDIS_STATUS_SUCCESS:
			return B_OK;
		case RNDIS_STATUS_FAILURE:
			return B_ERROR;
		case RNDIS_STATUS_INVALID_DATA:
			return B_BAD_DATA;
		case RNDIS_STATUS_NOT_SUPPORTED:
			return B_NOT_SUPPORTED;
		case RNDIS_STATUS_MEDIA_CONNECT:
			return EISCONN;
		case RNDIS_STATUS_MEDIA_DISCONNECT:
			return ENOTCONN;
		default:
			TRACE_ALWAYS("Unexpected error code %" B_PRIx32 "\n", response[3]);
			return B_IO_ERROR;
	}
}


void
RNDISDevice::_ReadCallback(void *cookie, int32 status, void *data,
	size_t actualLength)
{
	RNDISDevice *device = (RNDISDevice *)cookie;
	device->fActualLengthRead = actualLength;
	device->fStatusRead = status;
	release_sem_etc(device->fNotifyReadSem, 1, B_DO_NOT_RESCHEDULE);
}


void
RNDISDevice::_WriteCallback(void *cookie, int32 status, void *data,
	size_t actualLength)
{
	RNDISDevice *device = (RNDISDevice *)cookie;
	device->fActualLengthWrite = actualLength;
	device->fStatusWrite = status;
	release_sem_etc(device->fNotifyWriteSem, 1, B_DO_NOT_RESCHEDULE);
}


void
RNDISDevice::_NotifyCallback(void *cookie, int32 status, void *_data,
	size_t actualLength)
{
	RNDISDevice *device = (RNDISDevice *)cookie;
	atomic_add(&device->fInsideNotify, 1);
	if (status == B_CANCELED || device->fRemoved) {
		atomic_add(&device->fInsideNotify, -1);
		return;
	}

	if (status != B_OK) {
		TRACE_ALWAYS("device status error 0x%08" B_PRIx32 "\n", status);
		if (gUSBModule->clear_feature(device->fNotifyEndpoint,
			USB_FEATURE_ENDPOINT_HALT) != B_OK)
			TRACE_ALWAYS("failed to clear halt state in notify hook\n");
	} else if (actualLength != 8) {
		TRACE_ALWAYS("Received notification with unexpected number of bytes %" B_PRIuSIZE "\n",
			actualLength);
	} else {
#ifdef TRACE_RNDIS
		uint32* data = (uint32*)_data;
		uint32 notification = data[0];
		uint32 reserved = data[1];
		TRACE("Received notification %" B_PRIx32 " %" B_PRIx32 "\n", notification, reserved);
#endif
		release_sem_etc(device->fNotifyControlSem, 1, B_DO_NOT_RESCHEDULE);
	}

	// schedule next notification buffer
	gUSBModule->queue_interrupt(device->fNotifyEndpoint, device->fNotifyBuffer,
		sizeof(device->fNotifyBuffer), _NotifyCallback, device);
	atomic_add(&device->fInsideNotify, -1);
}
