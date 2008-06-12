/*
 * Copyright (c) 2007-2008 by Michael Lotz
 * Heavily based on the original usb_serial driver which is:
 *
 * Copyright (c) 2003 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the MIT License.
 */
#include "ACM.h"
#include "Driver.h"

ACMDevice::ACMDevice(usb_device device, uint16 vendorID, uint16 productID,
	const char *description)
	:	SerialDevice(device, vendorID, productID, description)
{
}


status_t
ACMDevice::AddDevice(const usb_configuration_info *config)
{
	TRACE_FUNCALLS("> ACMDevice::AddDevice(0x%08x, 0x%08x)\n", this, config);

	status_t status = ENODEV;
	uint8 masterIndex = 0;
	uint8 slaveIndex = 0;
	for (size_t i = 0; i < config->interface_count && status < B_OK; i++) {
		usb_interface_info *interface = config->interface[i].active;
		usb_interface_descriptor *descriptor = interface->descr;
		if (descriptor->interface_class == USB_INT_CLASS_CDC
			&& descriptor->interface_subclass == USB_INT_SUBCLASS_ACM
			&& interface->generic_count > 0) {
			// try to find and interpret the union functional descriptor
			for (size_t j = 0; j < interface->generic_count; j++) {
				usb_generic_descriptor *generic = &interface->generic[j]->generic;
				if (generic->length >= 5
					&& generic->data[0] == FUNCTIONAL_SUBTYPE_UNION) {
					masterIndex = generic->data[1];
					slaveIndex = generic->data[2];
					status = B_OK;
					break;
				}
			}
		}
	}

	if (status == B_OK && masterIndex < config->interface_count) {
		// check that the indicated master interface fits our need
		usb_interface_info *interface = config->interface[masterIndex].active;
		usb_interface_descriptor *descriptor = interface->descr;
		if ((descriptor->interface_class == USB_INT_CLASS_CDC
			|| descriptor->interface_class == USB_INT_CLASS_CDC_DATA)
			&& interface->endpoint_count >= 1) {
			SetControlPipe(interface->endpoint[0].handle);
		} else
			status = ENODEV;
	}

	if (status == B_OK && slaveIndex < config->interface_count) {
		// check that the indicated slave interface fits our need
		usb_interface_info *interface = config->interface[slaveIndex].active;
		usb_interface_descriptor *descriptor = interface->descr;
		if (descriptor->interface_class == USB_INT_CLASS_CDC_DATA
			&& interface->endpoint_count >= 2) {
			if (!(interface->endpoint[0].descr->endpoint_address & USB_EP_ADDR_DIR_IN))
				SetWritePipe(interface->endpoint[0].handle);
			else
				SetReadPipe(interface->endpoint[0].handle);

			if (interface->endpoint[1].descr->endpoint_address & USB_EP_ADDR_DIR_IN)
				SetReadPipe(interface->endpoint[1].handle);
			else
				SetWritePipe(interface->endpoint[1].handle);
		} else
			status = ENODEV;
	}

	TRACE_FUNCRET("< ACMDevice::AddDevice() returns: 0x%08x\n", status);
	return status;
}


status_t
ACMDevice::SetLineCoding(usb_serial_line_coding *lineCoding)
{
	TRACE_FUNCALLS("> ACMDevice::SetLineCoding(0x%08x, {%d, 0x%02x, 0x%02x, 0x%02x})\n",
		this, lineCoding->speed, lineCoding->stopbits, lineCoding->parity,
		lineCoding->databits);

	size_t length = 0;
	status_t status = gUSBModule->send_request(Device(),
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT,
		SET_LINE_CODING, 0, 0,
		sizeof(usb_serial_line_coding),
		lineCoding, &length);

	TRACE_FUNCRET("< ACMDevice::SetLineCoding() returns: 0x%08x\n", status);
	return status;
}


status_t
ACMDevice::SetControlLineState(uint16 state)
{
	TRACE_FUNCALLS("> ACMDevice::SetControlLineState(0x%08x, 0x%04x)\n", this, state);

	size_t length = 0;
	status_t status = gUSBModule->send_request(Device(),
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT,
		SET_CONTROL_LINE_STATE, state, 0, 0, NULL, &length);

	TRACE_FUNCRET("< ACMDevice::SetControlLineState() returns: 0x%08x\n", status);
	return status;
}


void
ACMDevice::OnWrite(const char *buffer, size_t *numBytes, size_t *packetBytes)
{
	memcpy(WriteBuffer(), buffer, *numBytes);
}
