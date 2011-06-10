/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#include "KLSI.h"
#include <ByteOrder.h>

KLSIDevice::KLSIDevice(usb_device device, uint16 vendorID, uint16 productID,
	const char *description)
	:	SerialDevice(device, vendorID, productID, description)
{
}


status_t
KLSIDevice::AddDevice(const usb_configuration_info *config)
{
	TRACE_FUNCALLS("> KLSIDevice::AddDevice(%08x, %08x)\n", this, config);

	int32 pipesSet = 0;
	status_t status = ENODEV;
	if (config->interface_count > 0) {
		usb_interface_info *interface = config->interface[0].active;
		for (size_t i = 0; i < interface->endpoint_count; i++) {
			usb_endpoint_info *endpoint = &interface->endpoint[i];
			if (endpoint->descr->attributes == USB_ENDPOINT_ATTR_INTERRUPT) {
				if (endpoint->descr->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN) {
					SetControlPipe(endpoint->handle);
					SetInterruptBufferSize(endpoint->descr->max_packet_size);
				}
			} else if (endpoint->descr->attributes == USB_ENDPOINT_ATTR_BULK) {
				if (endpoint->descr->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN) {
					SetReadBufferSize(ROUNDUP(endpoint->descr->max_packet_size, 16));
					SetReadPipe(endpoint->handle);
				} else {
					SetWriteBufferSize(ROUNDUP(endpoint->descr->max_packet_size, 16));
					SetWritePipe(endpoint->handle);
				}
			}
			if (++pipesSet >= 3)
				break;
		}
	}

	if (pipesSet >= 3)
		status = B_OK;

	TRACE_FUNCRET("< KLSIDevice::AddDevice() returns: 0x%08x\n", status);
	return status;
}


status_t
KLSIDevice::ResetDevice()
{
	TRACE_FUNCALLS("> KLSIDevice::ResetDevice(%08x)\n", this);

	size_t length = 0;
	status_t status;
	usb_cdc_line_coding lineCoding = { 9600, 1, 0, 8 };
	uint8 linestate[2];

	status = SetLineCoding(&lineCoding);
	status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		KLSI_CONF_REQUEST,
		KLSI_CONF_REQUEST_READ_ON, 0, 0, NULL, &length);
	TRACE("= KLSIDevice::ResetDevice(): set conf read_on returns: 0x%08x\n",
		status);

	linestate[0] = 0xff;
	linestate[1] = 0xff;
	length = 0;
	status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN,
		KLSI_POLL_REQUEST,
		0, 0, 2, linestate, &length);

	TRACE_FUNCRET("< KLSIDevice::ResetDevice() returns: 0x%08x\n", status);
	return status;
}


status_t
KLSIDevice::SetLineCoding(usb_cdc_line_coding *lineCoding)
{
	TRACE_FUNCALLS("> KLSIDevice::SetLineCoding(0x%08x, {%d, 0x%02x, 0x%02x, 0x%02x})\n",
		this, lineCoding->speed, lineCoding->stopbits, lineCoding->parity,
		lineCoding->databits);

	uint8 rate;
	switch (lineCoding->speed) {
		case 300: rate = klsi_sio_b300; break;
		case 600: rate = klsi_sio_b600; break;
		case 1200: rate = klsi_sio_b1200; break;
		case 2400: rate = klsi_sio_b2400; break;
		case 4800: rate = klsi_sio_b4800; break;
		case 9600: rate = klsi_sio_b9600; break;
		case 19200: rate = klsi_sio_b19200; break;
		case 38400: rate = klsi_sio_b38400; break;
		case 57600: rate = klsi_sio_b57600; break;
		case 115200: rate = klsi_sio_b115200; break;
		default:
			rate = klsi_sio_b19200;
			TRACE_ALWAYS("= KLSIDevice::SetLineCoding(): Datarate: %d is not "
				"supported by this hardware. Defaulted to %d\n",
				lineCoding->speed, rate);
		break;
	}

	uint8 codingPacket[5];
	codingPacket[0] = 5; /* size */
	codingPacket[1] = rate;
	codingPacket[2] = lineCoding->databits; /* only 7,8 */
	codingPacket[3] = 0; /* unknown */
	codingPacket[4] = 1; /* unknown */

	size_t length = 0;
	status_t status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		KLSI_SET_REQUEST, 0, 0,
		sizeof(codingPacket), codingPacket, &length);

	if (status != B_OK)
		TRACE_ALWAYS("= KLSIDevice::SetLineCoding(): datarate set request failed: 0x%08x\n", status);

	TRACE_FUNCRET("< KLSIDevice::SetLineCoding() returns: 0x%08x\n", status);
	return status;
}


void
KLSIDevice::OnRead(char **buffer, size_t *numBytes)
{
	if (*numBytes <= 2) {
		*numBytes = 0;
		return;
	}

	size_t bytes = B_LENDIAN_TO_HOST_INT16(*(uint16 *)(*buffer));
	*numBytes = MIN(bytes, *numBytes - 2);
	*buffer += 2;
}


void
KLSIDevice::OnWrite(const char *buffer, size_t *numBytes, size_t *packetBytes)
{
	if (*numBytes > KLSI_BUFFER_SIZE)
		*numBytes = *packetBytes = KLSI_BUFFER_SIZE;

	if (*numBytes > WriteBufferSize() - 2)
		*numBytes = *packetBytes = WriteBufferSize() - 2;

	char *writeBuffer = WriteBuffer();
	*((uint16 *)writeBuffer) = B_HOST_TO_LENDIAN_INT16(*numBytes);
	memcpy(writeBuffer + 2, buffer, *packetBytes);
	*packetBytes += 2;
}


void
KLSIDevice::OnClose()
{
	TRACE_FUNCALLS("> KLSIDevice::OnClose(%08x)\n", this);

	size_t length = 0;
	status_t status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		KLSI_CONF_REQUEST,
		KLSI_CONF_REQUEST_READ_OFF, 0, 0,
		NULL, &length);

	TRACE_FUNCRET("< KLSIDevice::OnClose() returns: 0x%08x\n", status);
}
