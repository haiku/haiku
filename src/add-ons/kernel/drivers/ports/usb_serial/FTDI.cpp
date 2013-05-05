/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "FTDI.h"
#include "FTDIRegs.h"


FTDIDevice::FTDIDevice(usb_device device, uint16 vendorID, uint16 productID,
	const char *description)
	:	SerialDevice(device, vendorID, productID, description),
		fHeaderLength(0),
		fStatusMSR(0),
		fStatusLSR(0)
{
}


status_t
FTDIDevice::AddDevice(const usb_configuration_info *config)
{
	TRACE_FUNCALLS("> FTDIDevice::AddDevice(%08x, %08x)\n", this, config);
	status_t status = ENODEV;
	if (config->interface_count > 0) {
		int32 pipesSet = 0;
		usb_interface_info *interface = config->interface[0].active;
		for (size_t i = 0; i < interface->endpoint_count; i++) {
			usb_endpoint_info *endpoint = &interface->endpoint[i];
			if (endpoint->descr->attributes == USB_ENDPOINT_ATTR_BULK) {
				if (endpoint->descr->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN) {
					SetReadPipe(endpoint->handle);
					if (++pipesSet >= 3)
						break;
				} else {
					if (endpoint->descr->endpoint_address) {
						SetControlPipe(endpoint->handle);
						SetWritePipe(endpoint->handle);
						pipesSet += 2;
						if (pipesSet >= 3)
							break;
					}
				}
			}
		}

		if (pipesSet >= 3) {
			if (ProductID() == 0x8372) // AU100AX
				fHeaderLength = 1;
			else
				fHeaderLength = 0;

			status = B_OK;
		}
	}

	TRACE_FUNCRET("< FTDIDevice::AddDevice() returns: 0x%08x\n", status);
	return status;
}


status_t
FTDIDevice::ResetDevice()
{
	TRACE_FUNCALLS("> FTDIDevice::ResetDevice(0x%08x)\n", this);

	size_t length = 0;
	status_t status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		FTDI_SIO_RESET, FTDI_SIO_RESET_SIO,
		FTDI_PIT_DEFAULT, 0, NULL, &length);

	TRACE_FUNCRET("< FTDIDevice::ResetDevice() returns:%08x\n", status);
	return status;
}


status_t
FTDIDevice::SetLineCoding(usb_cdc_line_coding *lineCoding)
{
	TRACE_FUNCALLS("> FTDIDevice::SetLineCoding(0x%08x, {%d, 0x%02x, 0x%02x, 0x%02x})\n",
		this, lineCoding->speed, lineCoding->stopbits, lineCoding->parity,
		lineCoding->databits);

	int32 rate = 0;
	if (ProductID() == 0x8372) {
		// AU100AX
		switch (lineCoding->speed) {
			case 300: rate = ftdi_sio_b300; break;
			case 600: rate = ftdi_sio_b600; break;
			case 1200: rate = ftdi_sio_b1200; break;
			case 2400: rate = ftdi_sio_b2400; break;
			case 4800: rate = ftdi_sio_b4800; break;
			case 9600: rate = ftdi_sio_b9600; break;
			case 19200: rate = ftdi_sio_b19200; break;
			case 38400: rate = ftdi_sio_b38400; break;
			case 57600: rate = ftdi_sio_b57600; break;
			case 115200: rate = ftdi_sio_b115200; break;
			default:
				rate = ftdi_sio_b19200;
				TRACE_ALWAYS("= FTDIDevice::SetLineCoding(): Datarate: %d is "
					"not supported by this hardware. Defaulted to %d\n",
					lineCoding->speed, rate);
			break;
		}
	} else {
		switch (lineCoding->speed) {
			case 300: rate = ftdi_8u232am_b300; break;
			case 600: rate = ftdi_8u232am_b600; break;
			case 1200: rate = ftdi_8u232am_b1200; break;
			case 2400: rate = ftdi_8u232am_b2400; break;
			case 4800: rate = ftdi_8u232am_b4800; break;
			case 9600: rate = ftdi_8u232am_b9600; break;
			case 19200: rate = ftdi_8u232am_b19200; break;
			case 38400: rate = ftdi_8u232am_b38400; break;
			case 57600: rate = ftdi_8u232am_b57600; break;
			case 115200: rate = ftdi_8u232am_b115200; break;
			case 230400: rate = ftdi_8u232am_b230400; break;
			case 460800: rate = ftdi_8u232am_b460800; break;
			case 921600: rate = ftdi_8u232am_b921600; break;
			default:
				rate = ftdi_sio_b19200;
				TRACE_ALWAYS("= FTDIDevice::SetLineCoding(): Datarate: %d is "
					"not supported by this hardware. Defaulted to %d\n",
					lineCoding->speed, rate);
				break;
		}
	}

	size_t length = 0;
	status_t status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		FTDI_SIO_SET_BAUD_RATE, rate,
		FTDI_PIT_DEFAULT, 0, NULL, &length);
	if (status != B_OK)
		TRACE_ALWAYS("= FTDIDevice::SetLineCoding(): datarate set request failed: 0x%08x\n", status);

	int32 data = 0;
	switch (lineCoding->stopbits) {
		case USB_CDC_LINE_CODING_1_STOPBIT: data = FTDI_SIO_SET_DATA_STOP_BITS_2; break;
		case USB_CDC_LINE_CODING_2_STOPBITS: data = FTDI_SIO_SET_DATA_STOP_BITS_1; break;
		default:
			TRACE_ALWAYS("= FTDIDevice::SetLineCoding(): Wrong stopbits param: %d\n",
				lineCoding->stopbits);
			break;
	}

	switch (lineCoding->parity) {
		case USB_CDC_LINE_CODING_NO_PARITY: data |= FTDI_SIO_SET_DATA_PARITY_NONE; break;
		case USB_CDC_LINE_CODING_EVEN_PARITY: data |= FTDI_SIO_SET_DATA_PARITY_EVEN; break;
		case USB_CDC_LINE_CODING_ODD_PARITY:	data |= FTDI_SIO_SET_DATA_PARITY_ODD; break;
		default:
			TRACE_ALWAYS("= FTDIDevice::SetLineCoding(): Wrong parity param: %d\n",
				lineCoding->parity);
			break;
	}

	switch (lineCoding->databits) {
		case 8: data |= FTDI_SIO_SET_DATA_BITS(8); break;
		case 7: data |= FTDI_SIO_SET_DATA_BITS(7); break;
		default:
			TRACE_ALWAYS("= FTDIDevice::SetLineCoding(): Wrong databits param: %d\n",
				lineCoding->databits);
			break;
	}

	length = 0;
	status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		FTDI_SIO_SET_DATA, data,
		FTDI_PIT_DEFAULT, 0, NULL, &length);
	if (status != B_OK)
		TRACE_ALWAYS("= FTDIDevice::SetLineCoding(): data set request failed: %08x\n", status);

	TRACE_FUNCRET("< FTDIDevice::SetLineCoding() returns: 0x%08x\n", status);
	return status;
}


status_t
FTDIDevice::SetControlLineState(uint16 state)
{
	TRACE_FUNCALLS("> FTDIDevice::SetControlLineState(0x%08x, 0x%04x)\n", this, state);

	int32 control;
	control = (state & USB_CDC_CONTROL_SIGNAL_STATE_RTS) ? FTDI_SIO_SET_RTS_HIGH
		: FTDI_SIO_SET_RTS_LOW;

	size_t length = 0;
	status_t status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		FTDI_SIO_MODEM_CTRL, control,
		FTDI_PIT_DEFAULT, 0, NULL, &length);

	if (status != B_OK)
		TRACE_ALWAYS("= FTDIDevice::SetControlLineState(): control set request failed: 0x%08x\n", status);

	control = (state & USB_CDC_CONTROL_SIGNAL_STATE_DTR) ? FTDI_SIO_SET_DTR_HIGH
		: FTDI_SIO_SET_DTR_LOW;

	status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		FTDI_SIO_MODEM_CTRL, control,
		FTDI_PIT_DEFAULT, 0, NULL, &length);

	if (status != B_OK)
		TRACE_ALWAYS("= FTDIDevice::SetControlLineState(): control set request failed: 0x%08x\n", status);

	TRACE_FUNCRET("< FTDIDevice::SetControlLineState() returns: 0x%08x\n", status);
	return status;
}


void
FTDIDevice::OnRead(char **buffer, size_t *numBytes)
{
	fStatusMSR = FTDI_GET_MSR(*buffer);
	fStatusLSR = FTDI_GET_LSR(*buffer);
	TRACE("FTDIDevice::OnRead(): MSR: 0x%02x LSR: 0x%02x\n", fStatusMSR, fStatusLSR);
	*buffer += 2;
	*numBytes -= 2;
}


void
FTDIDevice::OnWrite(const char *buffer, size_t *numBytes, size_t *packetBytes)
{
	if (*numBytes > FTDI_BUFFER_SIZE)
		*numBytes = *packetBytes = FTDI_BUFFER_SIZE;

	char *writeBuffer = WriteBuffer();
	if (fHeaderLength > 0) {
		if (*numBytes > WriteBufferSize() - fHeaderLength)
			*numBytes = *packetBytes = WriteBufferSize() - fHeaderLength;

		*writeBuffer = FTDI_OUT_TAG(*numBytes, FTDI_PIT_DEFAULT);
	}

	memcpy(writeBuffer + fHeaderLength, buffer, *packetBytes);
	*packetBytes += fHeaderLength;
}
