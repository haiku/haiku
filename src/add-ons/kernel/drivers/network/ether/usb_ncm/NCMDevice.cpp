/*
	Driver for USB NCM Network devices
	Copyright (C) 2022 Adrien Destugues <pulkomandy@pulkomandy.tk>
	Distributed under the terms of the MIT license.
*/
#include "NCMDevice.h"

#include <ether_driver.h>
#include <net/if_media.h>
#include <sys/sockio.h>
#include <stdlib.h>
#include <string.h>

#include "Driver.h"


enum MediaConnectStatus {
	MEDIA_STATE_UNKNOWN,
	MEDIA_STATE_CONNECTED,
	MEDIA_STATE_DISCONNECTED
};


NCMDevice::NCMDevice(usb_device device)
	: fStatus(B_ERROR),
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
	fReadHeader(NULL),
	fReadHeader2(NULL),
	fSequence(0),
	fLinkStateChangeSem(-1),
	fMediaConnectState(MEDIA_STATE_UNKNOWN),
	fDownstreamSpeed(0),
	fInputMaxSize(sizeof(fReadBuffer))
{
	const usb_device_descriptor *deviceDescriptor
		= gUSBModule->get_device_descriptor(device);

	if (deviceDescriptor == NULL) {
		TRACE_ALWAYS("failed to get device descriptor\n");
		return;
	}

	fNotifyReadSem = create_sem(0, DRIVER_NAME "_notify_read");
	if (fNotifyReadSem < B_OK) {
		TRACE_ALWAYS("failed to create read notify sem\n");
		return;
	}

	fNotifyWriteSem = create_sem(0, DRIVER_NAME "_notify_write");
	if (fNotifyWriteSem < B_OK) {
		TRACE_ALWAYS("failed to create write notify sem\n");
		return;
	}

	fLockWriteSem = create_sem(1, DRIVER_NAME "_lock_write");
	if (fNotifyWriteSem < B_OK) {
		TRACE_ALWAYS("failed to create write lock sem\n");
		return;
	}

	if (_SetupDevice() != B_OK) {
		TRACE_ALWAYS("failed to setup device\n");
		return;
	}

	fStatus = B_OK;
}


NCMDevice::~NCMDevice()
{
	if (fNotifyReadSem >= B_OK)
		delete_sem(fNotifyReadSem);
	if (fNotifyWriteSem >= B_OK)
		delete_sem(fNotifyWriteSem);
	if (fLockWriteSem >= B_OK)
		delete_sem(fLockWriteSem);

	if (!fRemoved)
		gUSBModule->cancel_queued_transfers(fNotifyEndpoint);
}


status_t
NCMDevice::Open()
{
	if (fOpen)
		return B_BUSY;
	if (fRemoved)
		return B_ERROR;

	// reset the device by switching the data interface to the disabled first
	// interface and then enable it by setting the second actual data interface
	const usb_configuration_info *config = gUSBModule->get_configuration(fDevice);

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

	// TODO these are non-fatal but make sure we have sane defaults for them
	status_t status = B_OK;
#if 0
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
#endif

	// the device should now be ready
	if (status == B_OK)
		fOpen = true;
	return status;
}


status_t
NCMDevice::Close()
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
NCMDevice::Free()
{
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
NCMDevice::Read(uint8 *buffer, size_t *numBytes)
{
	if (fRemoved) {
		TRACE("Reading, but device is removed\n");
		*numBytes = 0;
		return B_DEVICE_NOT_FOUND;
	}

	// The read function can return only one packet at a time, but we can receive multiple ones in
	// a single USB transfer. So we need to buffer them, and check if we have something in our
	// buffer for each Read() call before scheduling a new USB transfer. This would be more
	// efficient if the network stack had a way to read multiple frames at once.
	if (fReadHeader == NULL) {
		SemLocker mutex(fLockWriteSem);
		TRACE("-> Reading next packet batch\n");
		status_t result = gUSBModule->queue_bulk(fReadEndpoint, fReadBuffer, fInputMaxSize,
			_ReadCallback, this);
		if (result != B_OK) {
			TRACE_ALWAYS("failed to schedule read transfer: %s\n", strerror(result));
			fReadHeader = NULL;
			fReadHeader2 = NULL;
			*numBytes = 0;
			return result;
		}

try_again:
		result = acquire_sem_etc(fNotifyReadSem, 1, B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT, 10000000);
		if (result < B_OK) {
			TRACE_ALWAYS("error while waiting for frame: %s\n", strerror(result));
			fReadHeader = NULL;
			fReadHeader2 = NULL;
			*numBytes = 0;
			return result;
		}
		TRACE("-> Reading received: %s\n", strerror(result));

		if (result == B_TIMED_OUT) {
			gUSBModule->cancel_queued_transfers(fReadEndpoint);
			result = gUSBModule->clear_feature(fReadEndpoint, USB_FEATURE_ENDPOINT_HALT);
			goto try_again;
		}

		if (fStatusRead == B_CANCELED) {
			TRACE_ALWAYS("request was cancelled: %s\n", strerror(result));
			// The transfer was canceled, so no data was actually received.
			fReadHeader = NULL;
			fReadHeader2 = NULL;
			*numBytes = 0;
			return fStatusRead;
		}

		if ((fStatusRead != B_OK) && !fRemoved) {
			// In other error cases (triggered by the device), we need to clear the "halt" feature
			// so that the next transfers will work.
			TRACE_ALWAYS("device read status error: %s\n", strerror(fStatusRead));

			gUSBModule->cancel_queued_transfers(fReadEndpoint);

			result = gUSBModule->clear_feature(fReadEndpoint, USB_FEATURE_ENDPOINT_HALT);
			if (result != B_OK) {
				TRACE_ALWAYS("failed to clear halt state on read\n");
			}
			fReadHeader = NULL;
			fReadHeader2 = NULL;
			*numBytes = 0;
			return fStatusRead;
		}

		fReadHeader = fReadBuffer;

		if (memcmp(fReadHeader, "NCMH\x0C", 6) != 0) {
			TRACE_ALWAYS("Received unexpected packet type %s on data link\n", fReadHeader);
			fReadHeader = NULL;
			fReadHeader2 = NULL;
			*numBytes = 0;
			return B_BAD_VALUE;
		}

		TRACE("Receive read sequence: %x length in header %x length read %x\n",
			(fReadHeader[7] << 8) | fReadHeader[6],
			(fReadHeader[9] << 8) | fReadHeader[8],
			fActualLengthRead);

		// Point to the first NCMx packet after the main header
		uint16 offset = fReadHeader[10] | (fReadHeader[11] << 8);
		fReadHeader += offset;
	}

	TRACE("-> Check main header\n");
	if (memcmp(fReadHeader, "NCM0", 4) != 0) {
		TRACE_ALWAYS("Received invalid NCMx at fReadHeader %lx\n", fReadHeader - fReadBuffer);
		fReadHeader = NULL;
		fReadHeader2 = NULL;
		*numBytes = 0;
		return B_BAD_VALUE;
	}

	if (fReadHeader2 == NULL)
		fReadHeader2 = fReadHeader + 8;

	uint16 offset = fReadHeader2[0] | (fReadHeader2[1] << 8);
	uint16 len = fReadHeader2[2] | (fReadHeader2[3] << 8);

	if (offset == 0 && len == 0) {
		uint16 nextNCM = fReadHeader[6] | (fReadHeader[7] << 8);
		if (nextNCM != 0) {
			fReadHeader = fReadBuffer + nextNCM;
			fReadHeader2 = fReadHeader + 8;
			offset = fReadHeader2[0] | (fReadHeader2[1] << 8);
			len = fReadHeader2[2] | (fReadHeader2[3] << 8);
		} else {
			TRACE("-> No more data, please read again\n");
			fReadHeader = NULL;
			fReadHeader2 = NULL;
			*numBytes = 0;
			return EAGAIN;
		}
	}

	TRACE("-> Return buffer\n");
	*numBytes = len;
	memcpy(buffer, fReadBuffer + offset, len);

	// Advance to the next Ethernet packet in the NCMx (if there is one)
	fReadHeader2 += 4;

	return B_OK;
}


status_t
NCMDevice::Write(const uint8 *buffer, size_t *numBytes)
{
	if (fRemoved) {
		*numBytes = 0;
		return B_DEVICE_NOT_FOUND;
	}

	// This makes sure there is only one write going on at a time.
	// Also synchronizes incrementation of fSequence.
	SemLocker mutex(fLockWriteSem);
	status_t result = mutex.fStatus;
	if (result < B_OK) {
		TRACE_ALWAYS("Can't acquire write semaphore: %s\n", strerror(result));
		*numBytes = 0;
		return result;
	}

	// Write only one ethernet packet at a time. This loses the interest of NCM which is to fit
	// multiple packets into a single USB transaction to get better throughput and less latency.
	// But for an efficient multi-packet write, we would need a way to know if the network stack
	// has more packets pending to be sent, which is not provided in any way in Haiku AFAIK.
	// For now just forge a static header for a single ethernet packet.
	uint8 header[28] = { 0 };
	header[0] = 'N';
	header[1] = 'C';
	header[2] = 'M';
	header[3] = 'H';
	header[4] = 0x0C; // size of NCMH header (LSB)
	header[5] = 0x00; // size of NCMH header (MSB)
	header[6] = fSequence & 0xFF;
	header[7] = fSequence >> 8;
	header[8] = (*numBytes + sizeof(header)) & 0xFF;
	header[9] = (*numBytes + sizeof(header)) >> 8;
	header[10] = 0x0C; // pointer to NCMx header (LSB)
	header[11] = 0x00; //                        (MSB)
	header[12] = 'N';
	header[13] = 'C';
	header[14] = 'M';
	header[15] = '0'; // CRC not included, should be computed by hardware
	header[16] = 0x10; // Size of NCMx header (LSB)
	header[17] = 0x00; //                     (MSB)
	header[18] = 0x00; // This is the last NCMx
	header[19] = 0x00; //
	header[20] = sizeof(header) & 0xFF; // Offset to data (LSB)
	header[21] = sizeof(header) >> 8;   // Offset to data (MSB)
	header[22] = (*numBytes) & 0xFF; // Offset to data (LSB)
	header[23] = (*numBytes) >> 8;   // Offset to data (MSB)
	header[24] = 0x00; // No more frames
	header[25] = 0x00; //
	header[26] = 0x00; //
	header[27] = 0x00; //

	TRACE("Writing with sequence %x\n", fSequence);
	fSequence++;

	iovec vec[2];
	vec[0].iov_base = &header;
	vec[0].iov_len = sizeof(header);

	vec[1].iov_base = (void*)buffer;
	vec[1].iov_len = *numBytes;

	result = gUSBModule->queue_bulk_v(fWriteEndpoint, vec, 2, _WriteCallback, this);
	if (result != B_OK) {
		*numBytes = 0;
		return result;
	}

	do {
		result = acquire_sem_etc(fNotifyWriteSem, 1, B_CAN_INTERRUPT, 0);
	} while (result == B_INTERRUPTED);

	if (result < B_OK) {
		TRACE_ALWAYS("Can't write: %s, %s\n", strerror(result), strerror(fStatusWrite));
		*numBytes = 0;
		return result;
	}

	if (fStatusWrite == B_CANCELED) {
		TRACE_ALWAYS("Write transfer cancelled: %s\n", strerror(result));
		// The transfer was canceled, so no data was actually sent.
		*numBytes = 0;
		return fStatusWrite;
	}

	if ((fStatusWrite != B_OK) && !fRemoved) {
		TRACE_ALWAYS("device write status error 0x%08" B_PRIx32 "\n", fStatusWrite);

		gUSBModule->cancel_queued_transfers(fWriteEndpoint);

		result = gUSBModule->clear_feature(fWriteEndpoint, USB_FEATURE_ENDPOINT_HALT);
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
NCMDevice::Control(uint32 op, void *buffer, size_t length)
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

		case ETHER_SEND_NET_BUFFER:
		case ETHER_RECEIVE_NET_BUFFER:
			// Ignored for now, we use the old read/write interface instead
			return B_DEV_INVALID_IOCTL;

		case SIOCGIFSTATS:
			// Parameter is a struct ifreq, fill in request.ifr_stats
			// Number of received and sent packets and number of collisions
			// Not needed since there are no collisions and the network stack can already keep
			// track of sent/received packets on its own.
			break;

		default:
			TRACE_ALWAYS("unsupported ioctl %" B_PRIu32 "\n", op);
	}

	return B_DEV_INVALID_IOCTL;
}


void
NCMDevice::Removed()
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
NCMDevice::_OutputRequest(uint16 request, void* data, size_t length)
{
	size_t actualLength;
	status_t status = gUSBModule->send_request(fDevice,
		USB_REQTYPE_INTERFACE_OUT | USB_REQTYPE_CLASS, request, 0, 0, length, data, &actualLength);

	if (status != B_OK)
		return status;

	if (actualLength != length)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
NCMDevice::_InputRequest(uint16 request, void* data, size_t length)
{
	size_t actualLength;
	status_t status =  gUSBModule->send_request(fDevice,
		USB_REQTYPE_INTERFACE_IN | USB_REQTYPE_CLASS, request, 0, 0, length, data, &actualLength);

	if (status != B_OK)
		return status;

	if (actualLength != length)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
NCMDevice::_SetupDevice()
{
	const usb_device_descriptor *deviceDescriptor = gUSBModule->get_device_descriptor(fDevice);

	if (deviceDescriptor == NULL) {
		TRACE_ALWAYS("failed to get device descriptor\n");
		return B_ERROR;
	}

	uint8 controlIndex = 0;
	uint8 dataIndex = 0;
	bool foundUnionDescriptor = false;
	bool foundEthernetDescriptor = false;
	bool found = false;
	const usb_configuration_info *config = NULL;
	for (int i = 0; i < deviceDescriptor->num_configurations && !found; i++) {
		config = gUSBModule->get_nth_configuration(fDevice, i);
		if (config == NULL)
			continue;

		for (size_t j = 0; j < config->interface_count && !found; j++) {
			const usb_interface_info *interface = config->interface[j].active;
			usb_interface_descriptor *descriptor = interface->descr;
			// Locate the interface that is CDC-NCM
			if (descriptor->interface_class != USB_COMMUNICATION_DEVICE_CLASS
				|| descriptor->interface_subclass != USB_CDC_COMMUNICATION_INTERFACE_NCM_SUBCLASS
				|| descriptor->interface_protocol != 0x00
				|| interface->generic_count == 0) {
				continue;
			}

			// try to find and interpret the union and call management functional
			// descriptors (they allow us to locate the data interface)
			foundUnionDescriptor = false;
			foundEthernetDescriptor = false;
			for (size_t k = 0; k < interface->generic_count; k++) {
				usb_generic_descriptor *generic = &interface->generic[k]->generic;
				if (generic->length >= sizeof(usb_cdc_union_functional_descriptor)
					&& generic->data[0] == USB_CDC_UNION_FUNCTIONAL_DESCRIPTOR) {
					controlIndex = generic->data[1];
					dataIndex = generic->data[2]; // "Subordinate" interface
					foundUnionDescriptor = true;
				}

				if (generic->length >= sizeof(usb_cdc_ethernet_functional_descriptor)
					&& generic->data[0] == USB_CDC_ETHERNET_FUNCTIONAL_DESCRIPTOR) {
					foundEthernetDescriptor = true;
					usb_cdc_ethernet_functional_descriptor *ethernet
						= (usb_cdc_ethernet_functional_descriptor*)generic;

					size_t size = 0;
					char macString[26] = {0};
					uint8 langid[4];

					fMaxSegmentSize = ethernet->max_segment_size;
					// TODO handle language ID (see usb_ecm)
					status_t status = gUSBModule->get_descriptor(fDevice, USB_DESCRIPTOR_STRING,
						0, 0, langid, sizeof(langid), &size);
					if (status != B_OK)
						TRACE_ALWAYS("Failed to get language descriptor: %s\n", strerror(status));
					status = gUSBModule->get_descriptor(fDevice, USB_DESCRIPTOR_STRING,
						ethernet->mac_address_index, ((langid[3] << 8) | langid[2]),
						macString, sizeof(macString), &size);
					if (status != B_OK)
						TRACE_ALWAYS("Failed to get MAC descriptor: %s\n", strerror(status));
					if (size != sizeof(macString))
						TRACE_ALWAYS("MAC descriptor invalid size: %lu\n", size);

					char macPart[3];
					macPart[2] = 0;
					for (int32 i = 0; i < 6; i++) {
						macPart[0] = macString[2 + i * 4 + 0];
						macPart[1] = macString[2 + i * 4 + 2];
						fMACAddress[i] = strtol(macPart, NULL, 16);
					}

					TRACE_ALWAYS("read mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
						fMACAddress[0], fMACAddress[1], fMACAddress[2], fMACAddress[3],
						fMACAddress[4], fMACAddress[5]);
				}

				// TODO we may need the Header (just for the CDC spec version?), the NCM (for the
				// NCM version) descriptors as well

				if (foundUnionDescriptor && foundEthernetDescriptor) {
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

	if (!foundEthernetDescriptor) {
		TRACE_ALWAYS("did not find an ethernet descriptor\n");
		return B_ERROR;
	}

	// set the current configuration
	status_t status = gUSBModule->set_configuration(fDevice, config);
	if (status != B_OK) {
		TRACE_ALWAYS("Failed to set device configuration: %s\n", strerror(status));
		return status;
	}


	if (controlIndex >= config->interface_count) {
		TRACE_ALWAYS("control interface index invalid\n");
		return B_ERROR;
	}

	// check that the indicated control interface fits our needs
	usb_interface_info *interface = config->interface[controlIndex].active;
	usb_interface_descriptor *descriptor = interface->descr;
	if ((descriptor->interface_class != USB_COMMUNICATION_DEVICE_CLASS
		|| descriptor->interface_subclass != USB_CDC_COMMUNICATION_INTERFACE_NCM_SUBCLASS
		|| descriptor->interface_protocol != 0x00)
		|| interface->endpoint_count == 0) {
		TRACE_ALWAYS("control interface invalid\n");
		return B_ERROR;
	}

	// The notify buffer in the driver is static. If a device can send larger notifications, the
	// buffer needs to be enlarged to accomodate for this. Otherwise, the notify events will stall
	// and bad things will happen (complete loss of USB communications on all devices in my case).
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

	interface = &config->interface[dataIndex].alt[1];
	descriptor = interface->descr;
	if (descriptor->interface_class != USB_CDC_DATA_INTERFACE_CLASS
		|| descriptor->interface_subclass != 0
		|| descriptor->interface_protocol != 1
		|| interface->endpoint_count < 2) {
		TRACE_ALWAYS("data interface %d invalid (class %x, %" B_PRIuSIZE " endpoints)\n", dataIndex,
			descriptor->interface_class, interface->endpoint_count);
		return B_ERROR;
	}

	int16 buffer[14];
	status = _InputRequest(USB_CDC_GET_NTB_PARAMETERS, buffer, sizeof(buffer));
	if (status != B_OK) {
		TRACE_ALWAYS("Failed to get NTB parameters: %s\n", strerror(status));
		return status;
	}

	fInputMaxSize = (buffer[3] << 16) | buffer[2];

	TRACE("Formats supported:    %04x\n", buffer[1]);
	TRACE("Input max size:       %08x\n", fInputMaxSize);
	TRACE("Input divisor:        %04x\n", buffer[4]);
	TRACE("Input remainder:      %04x\n", buffer[5]);
	TRACE("Input alignment:      %04x\n", buffer[6]);
	TRACE("(reserved):           %04x\n", buffer[7]);
	TRACE("Output max size:      %04x %04x\n", buffer[9], buffer[8]);
	TRACE("Output divisor:       %04x\n", buffer[10]);
	TRACE("Output remainder:     %04x\n", buffer[11]);
	TRACE("Output alignment:     %04x\n", buffer[12]);
	TRACE("Output max datagrams: %04x\n", buffer[13]);

	if (fInputMaxSize > sizeof(fReadBuffer)) {
		TRACE_ALWAYS("Input buffer size needed by device is too large, clipping to %lx\n",
			sizeof(fReadBuffer));
		fInputMaxSize = sizeof(fReadBuffer);

		buffer[0] = fInputMaxSize & 0xFFFF;
		buffer[1] = (fInputMaxSize >> 16) & 0xFFFF;
		status = _OutputRequest(USB_CDC_SET_NTB_INPUT_SIZE, buffer, 4);
		if (status != B_OK) {
			TRACE_ALWAYS("Failed to set NTB input size: %s\n", strerror(status));
			return status;
		}
	}

	// Activate the "alternate 1" interface to finalize the initialization phase.
	status = gUSBModule->set_alt_interface(fDevice, interface);
	if (status != B_OK) {
		TRACE_ALWAYS("Failed to enable data interface: %s\n", strerror(status));
		return status;
	}

	fDataInterfaceIndex = dataIndex;
	return B_OK;
}


void
NCMDevice::_ReadCallback(void *cookie, int32 status, void *data,
	size_t actualLength)
{
	NCMDevice *device = (NCMDevice *)cookie;
	device->fActualLengthRead = actualLength;
	device->fStatusRead = status;
	release_sem_etc(device->fNotifyReadSem, 1, B_DO_NOT_RESCHEDULE);
}


void
NCMDevice::_WriteCallback(void *cookie, int32 status, void *data,
	size_t actualLength)
{
	NCMDevice *device = (NCMDevice *)cookie;
	device->fActualLengthWrite = actualLength;
	device->fStatusWrite = status;
	release_sem_etc(device->fNotifyWriteSem, 1, B_DO_NOT_RESCHEDULE);
}


void
NCMDevice::_NotifyCallback(void *cookie, int32 status, void *_data,
	size_t actualLength)
{
	NCMDevice *device = (NCMDevice *)cookie;
	atomic_add(&device->fInsideNotify, 1);
	if (status == B_CANCELED || device->fRemoved) {
		atomic_add(&device->fInsideNotify, -1);
		return;
	}

	if (status != B_OK) {
		TRACE_ALWAYS("device notify status error: %s\n", strerror(status));

		if (gUSBModule->clear_feature(device->fNotifyEndpoint,
			USB_FEATURE_ENDPOINT_HALT) != B_OK)
			TRACE_ALWAYS("failed to clear halt state in notify hook\n");
	} else if (actualLength < 8) {
		TRACE_ALWAYS("Received notification with unexpected number of bytes %" B_PRIuSIZE "\n",
			actualLength);
	} else {
		uint8* data = (uint8*)_data;
		switch (data[1]) {
			case USB_CDC_NETWORK_CONNECTION:
			{
				uint16 up = *(uint16*)(data + 2);
				if (up)
					device->fMediaConnectState = MEDIA_STATE_CONNECTED;
				else
					device->fMediaConnectState = MEDIA_STATE_DISCONNECTED;
				TRACE_ALWAYS("Link state change to %d\n", device->fMediaConnectState);
				if (device->fLinkStateChangeSem >= B_OK)
					release_sem_etc(device->fLinkStateChangeSem, 1, B_DO_NOT_RESCHEDULE);
				break;
			}
			case USB_CDC_CONNECTION_SPEED_CHANGE:
			{
				device->fDownstreamSpeed = (*(uint32*)(data + 8)) / 100;
				TRACE_ALWAYS("Link speed changed to %d\n", device->fDownstreamSpeed);
				if (device->fLinkStateChangeSem >= B_OK)
					release_sem_etc(device->fLinkStateChangeSem, 1, B_DO_NOT_RESCHEDULE);
				break;
			}
			default:
			{
				uint32 notification = data[0];
				uint32 reserved = data[1];
				TRACE_ALWAYS("Received notification %" B_PRIx32 " %" B_PRIx32 "\n",
					notification, reserved);
				break;
			}
		}
	}

	// schedule next notification buffer
	gUSBModule->queue_interrupt(device->fNotifyEndpoint, device->fNotifyBuffer,
		sizeof(device->fNotifyBuffer), _NotifyCallback, device);
	atomic_add(&device->fInsideNotify, -1);
}
