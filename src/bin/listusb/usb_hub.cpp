
/*
 * Copyright 2026, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */
#include <stdio.h>
#include <stdlib.h>

#include <USBKit.h>
#include <usb/USB_hub.h>

#include "listusb.h"


void
DumpHubDescriptor(BUSBDevice& device)
{
	union {
		usb_hub_descriptor hub;
		usb_hub_ss_descriptor hub_ss;
	} _PACKED descriptor;

	usb_hub_descriptor* hub = &descriptor.hub;
	usb_hub_ss_descriptor* hub_ss = &descriptor.hub_ss;

	bool isUSB2 = device.Protocol() == USB_HUB_PROTOCOL_HIGHSPEED_TT_PER_PORT
		|| device.Protocol() == USB_HUB_PROTOCOL_HIGHSPEED_SINGLE_TT;
	bool isUSB3 = device.Protocol() == USB_HUB_PROTOCOL_USB_3;
	size_t descriptorSize = isUSB3 ? sizeof(usb_hub_ss_descriptor) : sizeof(usb_hub_descriptor);

	printf("    [Hub %sDescriptor]\n", isUSB3 ? "USB 3/SuperSpeed " : "");

#pragma push_macro("INDENT")
#define INDENT "        "

	size_t size = device.ControlTransfer(USB_REQTYPE_DEVICE_IN | USB_REQTYPE_CLASS,
		USB_REQUEST_GET_DESCRIPTOR, (isUSB3 ? USB_DESCRIPTOR_HUB_SS : USB_DESCRIPTOR_HUB) << 8,
		0, descriptorSize, &descriptor);

	if (size < descriptorSize || (isUSB3 && size != descriptorSize)) {
		printf(INDENT "**ERROR** Failed to get hub descriptor!\n");
		return;
	}

	printf(INDENT "Type .................... 0x%02x Hub%s Descriptor\n", hub->descriptor_type,
		hub->descriptor_type == USB_DESCRIPTOR_HUB_SS ? " SuperSpeed" : "");
	printf(INDENT "Length .................. 0x%02x (%u)\n", hub->length, hub->length);
	printf(INDENT "Controller Current ...... %u mA\n", hub->max_power);
	printf(INDENT "Characteristics ......... 0x%04x\n", hub->characteristics);

	if ((hub->characteristics & USB_HUB_NO_POWER_SWITCHING) != 0)
		printf(INDENT "    No Power Switching\n");
	else if ((hub->characteristics & USB_HUB_INDIVIDUAL_POWER_SWITCHING) != 0)
		printf(INDENT "    Individual Power Switching\n");
	else
		printf(INDENT "    Ganged Power Switching\n");

	if ((hub->characteristics & USB_HUB_COMPOUND_DEVICE) != 0)
		printf(INDENT "    Compound Device\n");

	if ((hub->characteristics & USB_HUB_OVERCURRENT_MODE_NONE) != 0)
		printf(INDENT "    No Over-Current Protection\n");
	else if ((hub->characteristics & USB_HUB_OVERCURRENT_MODE_INDIVIDUAL) != 0)
		printf(INDENT "    Individual Over-Current Protection\n");
	else
		printf(INDENT "    Global Over-Current Protection\n");

	if (isUSB2) {
		uint16 thinkTime = (hub->characteristics & USB_HUB_TT_THINK_TIME_MASK) >> 5;
		printf(INDENT "    TT Think Time %u\n", (thinkTime + 1) * 8);
	}

	if ((hub->characteristics & USB_HUB_PORTS_INDICATORS) != 0)
		printf(INDENT "    Ports Indicators\n");

	printf(INDENT "Power On to Power Good .. %u ms\n", hub->power_on_to_power_good * 2);
	printf(INDENT "Max Power ............... %u mA\n", hub->max_power);
	if (isUSB3) {
		printf(INDENT "Decode Latency .......... %u ns\n", hub_ss->decode_latency * 100);
		printf(INDENT "Delay ................... %u ns\n", hub_ss->delay);
	}
	printf(INDENT "Ports Count.............. %u\n", hub->num_ports);

	for (int i = 1; i <= hub->num_ports; i++) {
		usb_port_status portStatus;
		size_t actualLength = device.ControlTransfer(USB_REQTYPE_CLASS | USB_REQTYPE_OTHER_IN,
			USB_REQUEST_GET_STATUS, 0, i, sizeof(portStatus), (void*)&portStatus);
		if (actualLength != sizeof(portStatus))
			continue;

		printf(INDENT "    Port %3d status ..... 0x%04x.%04x", i, portStatus.status,
			portStatus.change);
		if ((portStatus.status & PORT_STATUS_CONNECTION) != 0)
			printf(" Connect");
		if ((portStatus.status & PORT_STATUS_ENABLE) != 0)
			printf(" Enable");
		if (!isUSB3 && (portStatus.status & PORT_STATUS_SUSPEND) != 0)
			printf(" Suspend");
		if ((portStatus.status & PORT_STATUS_OVER_CURRENT) != 0)
			printf(" Overcurrent");
		if ((portStatus.status & PORT_STATUS_RESET) != 0)
			printf(" Reset");
		if ((portStatus.status & (isUSB3 ? PORT_STATUS_SS_POWER : PORT_STATUS_POWER)) != 0)
			printf(" Power");

		if (isUSB3) {
			uint16 linkState = (portStatus.status & PORT_STATUS_SS_LINK_STATE) >> 5;
			const char* linkStateName[] = {
				"U0",
				"U1",
				"U2",
				"U3",
				"eSS.Disabled",
				"Rx.Detect",
				"eSS.Inactive",
				"Polling",
				"Recovery",
				"HotReset",
				"ComplianceMode",
				"Loopback",
				NULL,
				NULL,
				NULL,
				NULL
			};
			if (linkStateName[linkState] != NULL)
				printf(" %s", linkStateName[linkState]);

			uint16 negociatedSpeed = (portStatus.status & PORT_STATUS_SS_SPEED) >> 10;
			if (negociatedSpeed == 0)
				printf(" SuperSpeed+");

		} else {
			if ((portStatus.status & PORT_STATUS_TEST) != 0)
				printf(" Test");
			if ((portStatus.status & PORT_STATUS_INDICATOR) != 0)
				printf(" Indicator");
		}

		printf("\n");
	} // next port status

#pragma pop_macro("INDENT")
}
