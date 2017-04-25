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
	usb_cdc_cm_functional_descriptor* cmDesc = NULL;
	usb_cdc_union_functional_descriptor* unionDesc = NULL;

	// Search ACM Communication Interface
	for (size_t i = 0; i < config->interface_count && status < B_OK; i++) {
		usb_interface_info *interface = config->interface[i].active;
		usb_interface_descriptor *descriptor = interface->descr;
		if (descriptor->interface_class != USB_CDC_COMMUNICATION_INTERFACE_CLASS
			|| descriptor->interface_subclass != USB_CDC_COMMUNICATION_INTERFACE_ACM_SUBCLASS)
			continue;

		// Found ACM Communication Interface!
		// Get functional descriptors of some interest, if any
		for (size_t j = 0; j < interface->generic_count; j++) {
			usb_generic_descriptor *generic = &interface->generic[j]->generic;
			switch (generic->data[0]) {
				case USB_CDC_CM_FUNCTIONAL_DESCRIPTOR:
					cmDesc = (usb_cdc_cm_functional_descriptor*)generic;
					break;

				case USB_CDC_ACM_FUNCTIONAL_DESCRIPTOR:
					break;

				case USB_CDC_UNION_FUNCTIONAL_DESCRIPTOR:
					unionDesc = (usb_cdc_union_functional_descriptor*)generic;
					break;
			}
		}

		masterIndex = unionDesc ? unionDesc->master_interface : i;
		slaveIndex = cmDesc ? cmDesc->data_interface
			: unionDesc ? unionDesc->slave_interfaces[0] : 0;

		TRACE("ACM device found on configuration #%d: master itf: %d, "
				"slave/data itf: %d\n", config->descr->configuration,
				masterIndex, slaveIndex);

		// Some ACM USB devices report the wrong unions which rightfully
		// breaks probing. Some drivers keep a list of these devices,
		// for now we just assume identical indexes are wrong.
		if (masterIndex == slaveIndex) {
			TRACE_ALWAYS("Command interface matches data interface, "
				"assuming broken union quirk!\n");
			masterIndex = 0;
			slaveIndex = 1;
		}

		status = B_OK;
		break;
	}

	if (status == B_OK && masterIndex < config->interface_count) {
		// check that the indicated master interface fits our need
		usb_interface_info *interface = config->interface[masterIndex].active;
		usb_interface_descriptor *descriptor = interface->descr;
		if ((descriptor->interface_class == USB_CDC_COMMUNICATION_INTERFACE_CLASS
			|| descriptor->interface_class == USB_CDC_DATA_INTERFACE_CLASS)
			&& interface->endpoint_count >= 1) {
			SetControlPipe(interface->endpoint[0].handle);
		} else {
			TRACE("Indicated command interface doesn't fit our needs!\n");
			status = ENODEV;
		}
	}

	if (status == B_OK && slaveIndex < config->interface_count) {
		// check that the indicated slave interface fits our need
		usb_interface_info *interface = config->interface[slaveIndex].active;
		usb_interface_descriptor *descriptor = interface->descr;
		if (descriptor->interface_class == USB_CDC_DATA_INTERFACE_CLASS
			&& interface->endpoint_count >= 2) {
			if (!(interface->endpoint[0].descr->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN))
				SetWritePipe(interface->endpoint[0].handle);
			else
				SetReadPipe(interface->endpoint[0].handle);

			if (interface->endpoint[1].descr->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN)
				SetReadPipe(interface->endpoint[1].handle);
			else
				SetWritePipe(interface->endpoint[1].handle);
		} else {
			TRACE("Indicated data interface doesn't fit our needs!\n");
			status = ENODEV;
		}
	}

	TRACE_FUNCRET("< ACMDevice::AddDevice() returns: 0x%08x\n", status);
	return status;
}


status_t
ACMDevice::SetLineCoding(usb_cdc_line_coding *lineCoding)
{
	TRACE_FUNCALLS("> ACMDevice::SetLineCoding(0x%08x, {%d, 0x%02x, 0x%02x, 0x%02x})\n",
		this, lineCoding->speed, lineCoding->stopbits, lineCoding->parity,
		lineCoding->databits);

	size_t length = 0;
	status_t status = gUSBModule->send_request(Device(),
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT,
		USB_CDC_SET_LINE_CODING, 0, 0,
		sizeof(usb_cdc_line_coding),
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
		USB_CDC_SET_CONTROL_LINE_STATE, state, 0, 0, NULL, &length);

	TRACE_FUNCRET("< ACMDevice::SetControlLineState() returns: 0x%08x\n", status);
	return status;
}
