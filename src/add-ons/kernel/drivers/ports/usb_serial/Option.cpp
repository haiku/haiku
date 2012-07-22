/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "Option.h"


OptionDevice::OptionDevice(usb_device device, uint16 vendorID,
	uint16 productID, const char *description)
	:
	ACMDevice(device, vendorID, productID, description)
{
	TRACE_FUNCALLS("> OptionDevice found: %s\n", description);
}


status_t
OptionDevice::AddDevice(const usb_configuration_info *config)
{
	TRACE_FUNCALLS("> OptionDevice::AddDevice(%08x, %08x)\n", this, config);

	int portsFound = 0;
	if (config->interface_count > 0) {
		for (size_t index = 0; index < config->interface_count; index++) {
			usb_interface_info *interface = config->interface[index].active;

			int txEndpointID = -1;
			int rxEndpointID = -1;
			int irEndpointID = -1;

			for (size_t i = 0; i < interface->endpoint_count; i++) {
				usb_endpoint_info *endpoint = &interface->endpoint[i];

				// Find our Interrupt endpoint
				if (endpoint->descr->attributes == USB_ENDPOINT_ATTR_INTERRUPT
					&& (endpoint->descr->endpoint_address
						& USB_ENDPOINT_ADDR_DIR_IN) != 0) {
					irEndpointID = i;
					continue;
				}

				// Find our Transmit / Receive endpoints
				if (endpoint->descr->attributes == USB_ENDPOINT_ATTR_BULK) {
					if ((endpoint->descr->endpoint_address
						& USB_ENDPOINT_ADDR_DIR_IN) != 0) {
						rxEndpointID = i;
					} else {
						txEndpointID = i;
					}
					continue;
				}
			}

			TRACE("> OptionDevice::%s: endpoint %d, tx: %d, rx: %d, ir: %d\n",
				__func__, index, txEndpointID, rxEndpointID, irEndpointID);

			if (txEndpointID < 0 || rxEndpointID < 0 || irEndpointID < 0)
				continue;

			TRACE("> OptionDevice::%s: found port at interface %d\n", __func__,
				index);
			portsFound++;

			usb_endpoint_info *irEndpoint = &interface->endpoint[irEndpointID];
			usb_endpoint_info *txEndpoint = &interface->endpoint[txEndpointID];
			usb_endpoint_info *rxEndpoint = &interface->endpoint[rxEndpointID];
			SetControlPipe(irEndpoint->handle);
			SetReadPipe(rxEndpoint->handle);
			SetWritePipe(txEndpoint->handle);
		}

		// TODO: We need to handle multiple ports
		// We use the last found serial port for now
		if (portsFound > 0) {
			if (portsFound > 1) {
				TRACE_ALWAYS("> OptionDevice::%s: Warning: Found more than one "
					"serial port on this device (%d). Only the last one is "
					"is used.\n", __func__, portsFound);
			}
			return B_OK;
		}
	}
	return ENODEV;
}


status_t
OptionDevice::ResetDevice()
{
	TRACE_FUNCALLS("> OptionDevice::ResetDevice(%08x)\n", this);
	return B_OK;
}
