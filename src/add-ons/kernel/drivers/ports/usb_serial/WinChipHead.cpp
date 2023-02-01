/*
 * Copyright 2022, Gerasim Troeglazov <3dEyes@gmail.com>
 * Distributed under the terms of the MIT License.
 */

#include "WinChipHead.h"

WCHDevice::WCHDevice(usb_device device, uint16 vendorID, uint16 productID,
	const char *description)
	:	SerialDevice(device, vendorID, productID, description),
		fChipVersion(0),
		fStatusMCR(0),
		fStatusLCR(CH34X_LCR_ENABLE_RX | CH34X_LCR_ENABLE_TX | CH34X_LCR_CS8),
		fDataRate(CH34X_SIO_9600)
{
}


// Called for each configuration of the device. Return B_OK if the given
// configuration sounds like it is the usb serial one.
status_t
WCHDevice::AddDevice(const usb_configuration_info *config)
{
	TRACE_FUNCALLS("> WCHDevice::AddDevice(%08x, %08x)\n", this, config);

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
			status = B_OK;
		}
	}

	TRACE_FUNCRET("< WCHDevice::AddDevice() returns: 0x%08x\n", status);

	return status;
}


status_t
WCHDevice::ResetDevice()
{
	TRACE_FUNCALLS("> WCHDevice::ResetDevice(0x%08x)\n", this);
	size_t length = 0;

	uint8 inputBuffer[CH34X_INPUT_BUF_SIZE];
	status_t status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_IN,
		CH34X_REQ_READ_VERSION, 0, 0, sizeof(inputBuffer), inputBuffer, &length);

	if (status == B_OK) {
		fChipVersion = inputBuffer[0];
		TRACE_ALWAYS("= WCHDevice::ResetDevice(): Chip version: 0x%02x\n", fChipVersion);
	} else {
		TRACE_ALWAYS("= WCHDevice::ResetDevice(): Can't get chip version: 0x%08x\n",
			status);
		return status;
	}

	status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		CH34X_REQ_SERIAL_INIT, 0, 0, 0, NULL, &length);

	if (status != B_OK) {
		TRACE_ALWAYS("= WCHDevice::ResetDevice(): init failed\n");
		return status;
	}

	status = WriteConfig(fDataRate, fStatusLCR, fStatusMCR);

	TRACE_FUNCRET("< WCHDevice::ResetDevice() returns:%08x\n", status);
	return status;
}

status_t
WCHDevice::SetLineCoding(usb_cdc_line_coding *lineCoding)
{
	TRACE_FUNCALLS("> WCHDevice::SetLineCoding(0x%08x, {%d, 0x%02x, 0x%02x, 0x%02x})\n",
		this, lineCoding->speed, lineCoding->stopbits, lineCoding->parity,
		lineCoding->databits);

	fStatusLCR = CH34X_LCR_ENABLE_RX | CH34X_LCR_ENABLE_TX;

	switch (lineCoding->stopbits) {
		case USB_CDC_LINE_CODING_1_STOPBIT:
			break;
		case USB_CDC_LINE_CODING_2_STOPBITS:
			fStatusLCR |= CH34X_LCR_STOP_BITS_2;
			break;
		default:
			TRACE_ALWAYS("= WCHDevice::SetLineCoding(): Wrong stopbits param: %d\n",
				lineCoding->stopbits);
			break;
	}

	switch (lineCoding->parity) {
		case USB_CDC_LINE_CODING_NO_PARITY:
			break;
		case USB_CDC_LINE_CODING_EVEN_PARITY:
			fStatusLCR |= CH34X_LCR_ENABLE_PAR | CH34X_LCR_PAR_EVEN;
			break;
		case USB_CDC_LINE_CODING_ODD_PARITY:
			fStatusLCR |= CH34X_LCR_ENABLE_PAR;
			break;
		default:
			TRACE_ALWAYS("= WCHDevice::SetLineCoding(): Wrong parity param: %d\n",
				lineCoding->parity);
			break;
	}

	switch (lineCoding->databits) {
		case 5:
			fStatusLCR |= CH34X_LCR_CS5;
			break;
		case 6:
			fStatusLCR |= CH34X_LCR_CS6;
			break;
		case 7:
			fStatusLCR |= CH34X_LCR_CS7;
			break;
		case 8:
			fStatusLCR |= CH34X_LCR_CS8;
			break;
		default:
			fStatusLCR |= CH34X_LCR_CS8;
			TRACE_ALWAYS("= WCHDevice::SetLineCoding(): Wrong databits param: %d\n",
				lineCoding->databits);
			break;
	}

	switch (lineCoding->speed) {
		case 600: fDataRate = CH34X_SIO_600; break;
		case 1200: fDataRate = CH34X_SIO_1200; break;
		case 1800: fDataRate = CH34X_SIO_1800; break;
		case 2400: fDataRate = CH34X_SIO_2400; break;
		case 4800: fDataRate = CH34X_SIO_4800; break;
		case 9600: fDataRate = CH34X_SIO_9600; break;
		case 19200: fDataRate = CH34X_SIO_19200; break;
		case 31250: fDataRate = CH34X_SIO_31250; break;
		case 38400: fDataRate = CH34X_SIO_38400; break;
		case 57600: fDataRate = CH34X_SIO_57600; break;
		case 115200: fDataRate = CH34X_SIO_115200; break;
		case 230400: fDataRate = CH34X_SIO_230400; break;
		default:
			fDataRate = CH34X_SIO_9600;
			TRACE_ALWAYS("= WCHDevice::SetLineCoding(): Datarate: %d is not "
				"supported by this hardware. Defaulted to %d\n",
				lineCoding->speed, CH34X_DEFAULT_BAUD_RATE);
			break;
	}

	status_t status = WriteConfig(fDataRate, fStatusLCR, fStatusMCR);

	if (status != B_OK)
		TRACE_ALWAYS("= WCHDevice::SetLineCoding(): WriteConfig failed\n");

	TRACE_FUNCRET("< WCHDevice::SetLineCoding() returns: 0x%08x\n", status);

	return status;
}


status_t
WCHDevice::SetControlLineState(uint16 state)
{
	TRACE_FUNCALLS("> WCHDevice::SetControlLineState(0x%08x, 0x%04x)\n",
		this, state);

	fStatusMCR = 0;

	if (state & USB_CDC_CONTROL_SIGNAL_STATE_RTS)
		fStatusMCR |= CH34X_BIT_RTS;

	if (state & USB_CDC_CONTROL_SIGNAL_STATE_DTR)
		fStatusMCR |= CH34X_BIT_DTR;

	size_t length = 0;
	status_t status = 0;

	if (fChipVersion < CH34X_VER_20) {
		status = gUSBModule->send_request(Device(),
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
			CH34X_REQ_WRITE_REG, CH34X_REG_STAT1 | (CH34X_REG_STAT1 << 8),
			~fStatusMCR | (~fStatusMCR << 8), 0, NULL, &length);
	} else {
		status = gUSBModule->send_request(Device(),
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
			CH34X_REQ_MODEM_CTRL, ~fStatusMCR, 0, 0, NULL, &length);
	}

	TRACE_FUNCRET("< WCHDevice::SetControlLineState() returns: 0x%08x\n",
		status);

	return status;
}

status_t
WCHDevice::WriteConfig(uint16 dataRate, uint8 lcr, uint8 mcr)
{
	size_t length = 0;
	status_t status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		CH34X_REQ_WRITE_REG, CH34X_REG_BPS_PRE | (CH34X_REG_BPS_DIV << 8),
		dataRate | CH34X_BPS_PRE_IMM, 0, NULL, &length);

	if (status != B_OK) {
		TRACE_ALWAYS("= WCHDevice::WriteConfig(): datarate request failed: 0x%08x\n",
			status);
		return status;
	}

	status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		CH34X_REQ_WRITE_REG, CH34X_REG_LCR | (CH34X_REG_LCR2 << 8),
		lcr, 0, NULL, &length);

	if (status != B_OK) {
		TRACE_ALWAYS("= WCHDevice::WriteConfig(): LCR request failed: 0x%08x\n",
			status);
		return status;
	}

	status = gUSBModule->send_request(Device(),
		USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT,
		CH34X_REQ_MODEM_CTRL, ~mcr, 0, 0, NULL, &length);

	if (status != B_OK)
		TRACE_ALWAYS("= WCHDevice::WriteConfig(): handshake failed: 0x%08x\n", status);

	return status;
}
