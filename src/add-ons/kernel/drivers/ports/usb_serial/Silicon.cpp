/*
 * Copyright 2011, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "Silicon.h"


static const int kBaudrateGeneratorFrequency = 0x384000;


SiliconDevice::SiliconDevice(usb_device device, uint16 vendorID, uint16 productID,
	const char *description)
	:	SerialDevice(device, vendorID, productID, description)
{
}


// Called for each configuration of the device. Return B_OK if the given
// configuration sounds like it is the usb serial one.
status_t
SiliconDevice::AddDevice(const usb_configuration_info *config)
{
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
	return status;
}


// Called on opening the device - Good time to enable the UART ?
status_t
SiliconDevice::ResetDevice()
{
	uint16_t enableUart = 1;
	return WriteConfig(ENABLE_UART, &enableUart, 2);
}


status_t
SiliconDevice::SetLineCoding(usb_cdc_line_coding *lineCoding)
{
	uint16_t divider = kBaudrateGeneratorFrequency / lineCoding->speed ;
	status_t result = WriteConfig(SET_BAUDRATE_DIVIDER, &divider, 2);

	if (result != B_OK) return result;

	uint16_t data = 0;

	switch (lineCoding->stopbits) {
		case USB_CDC_LINE_CODING_1_STOPBIT: data = 0; break;
		case USB_CDC_LINE_CODING_2_STOPBITS: data = 2; break;
		default:
			TRACE_ALWAYS("= SiliconDevice::SetLineCoding(): Wrong stopbits param: %d\n",
				lineCoding->stopbits);
			break;
	}

	switch (lineCoding->parity) {
		case USB_CDC_LINE_CODING_NO_PARITY: data |= 0 << 4; break;
		case USB_CDC_LINE_CODING_EVEN_PARITY: data |= 2 << 4; break;
		case USB_CDC_LINE_CODING_ODD_PARITY:	data |= 1 << 4; break;
		default:
			TRACE_ALWAYS("= SiliconDevice::SetLineCoding(): Wrong parity param: %d\n",
				lineCoding->parity);
			break;
	}

	data |= lineCoding->databits << 8;

	return WriteConfig(SET_LINE_FORMAT, &data, 2);
}


status_t
SiliconDevice::SetControlLineState(uint16 state)
{
	uint16_t control = 0;
	control |= 0x0300; // We are updating DTR and RTS
	control |= (state & USB_CDC_CONTROL_SIGNAL_STATE_RTS) ? 2 : 0;
	control |= (state & USB_CDC_CONTROL_SIGNAL_STATE_DTR) ? 1 : 0;

	return WriteConfig(SET_STATUS, &control, 2);
}


status_t SiliconDevice::WriteConfig(CP210XRequest request, uint16_t* data,
	size_t size)
{
	size_t replyLength = 0;
	status_t result;
	// Small requests (16 bits and less) use the "value" field for their data.
	// Bigger ones use the actual buffer.
	if (size <= 2) {
		result = gUSBModule->send_request(Device(),
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT, request, data[0], 0, 0,
			NULL, &replyLength);
	} else {
		result = gUSBModule->send_request(Device(),
			USB_REQTYPE_VENDOR | USB_REQTYPE_DEVICE_OUT, request, 0x0000, 0,
			size, data, &replyLength);
	}

	if (result != B_OK) {
		TRACE_ALWAYS("= SiliconDevice request failed: 0x%08x (%s)\n",
			result, strerror(result));
	}

	return result;
}
