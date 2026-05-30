/*
 * Originally released under the Be Sample Code License.
 * Copyright 2000, Be Incorporated. All rights reserved.
 *
 * Modified for Haiku by François Revol and Michael Lotz.
 * Copyright 2007-2016, Haiku Inc. All rights reserved.
 */

#include <USBKit.h>
#include <stdio.h>

#include <usb/USB_cdc.h>

#include "listusb.h"

void
DumpCDCDescriptor(const usb_generic_descriptor* descriptor, const BUSBInterface* interface)
{
	if (descriptor->descriptor_type == 0x24) {
		printf("                    Type ............. CDC interface descriptor\n");
		printf("                    Subtype .......... ");
		switch (descriptor->data[0]) {
			case USB_CDC_HEADER_FUNCTIONAL_DESCRIPTOR:
				printf("Header\n");
				printf("                    CDC Version ...... %x.%x\n",
					descriptor->data[2], descriptor->data[1]);
				return;
			case USB_CDC_CM_FUNCTIONAL_DESCRIPTOR:
			{
				printf("Call management\n");
				const usb_cdc_cm_functional_descriptor* cmDesc
					= (const usb_cdc_cm_functional_descriptor*)descriptor;
				printf("                    Capabilities ..... ");
				bool somethingPrinted = false;
				if ((cmDesc->capabilities & USB_CDC_CM_DOES_CALL_MANAGEMENT) != 0) {
					printf("Call management");
					somethingPrinted = true;
				}
				if ((cmDesc->capabilities & USB_CDC_CM_OVER_DATA_INTERFACE) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("Over data interface");
					somethingPrinted = true;
				}
				if (!somethingPrinted)
					printf("None");
				printf("\n");
				printf("                    Data interface ... %d\n", cmDesc->data_interface);
				return;
			}
			case USB_CDC_ACM_FUNCTIONAL_DESCRIPTOR:
			{
				printf("Abstract control management\n");
				const usb_cdc_acm_functional_descriptor* acmDesc
					= (const usb_cdc_acm_functional_descriptor*)descriptor;
				printf("                    Capabilities ..... ");
				bool somethingPrinted = false;
				if ((acmDesc->capabilities & USB_CDC_ACM_HAS_COMM_FEATURES) != 0) {
					printf("Communication features");
					somethingPrinted = true;
				}
				if ((acmDesc->capabilities & USB_CDC_ACM_HAS_LINE_CONTROL) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("Line control");
					somethingPrinted = true;
				}
				if ((acmDesc->capabilities & USB_CDC_ACM_HAS_SEND_BREAK) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("Send break");
					somethingPrinted = true;
				}
				if ((acmDesc->capabilities & USB_CDC_ACM_HAS_NETWORK_CONNECTION) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("Network connection");
					somethingPrinted = true;
				}
				if (!somethingPrinted)
					printf("None");
				printf("\n");
				return;
			}
			case 0x03:
				printf("Direct line management\n");
				break;
			case 0x04:
				printf("Telephone ringer management\n");
				break;
			case 0x05:
				printf("Telephone call and line state reporting\n");
				break;
			case USB_CDC_UNION_FUNCTIONAL_DESCRIPTOR:
				printf("Union\n");
				printf("                    Control interface  %d\n", descriptor->data[1]);
				for (int32 i = 2; i < descriptor->length - 2; i++)
					printf("                    Subordinate .....  %d\n", descriptor->data[i]);
				return;
			case 0x07:
				printf("Country selection\n");
				break;
			case 0x08:
				printf("Telephone operational mode\n");
				break;
			case 0x09:
				printf("USB Terminal\n");
				break;
			case 0x0A:
				printf("Network channel\n");
				break;
			case 0x0B:
				printf("Protocol init\n");
				break;
			case 0x0C:
				printf("Extension unit\n");
				break;
			case 0x0D:
				printf("Multi-channel management\n");
				break;
			case 0x0E:
				printf("CAPI control\n");
				break;
			case 0x0F:
			{
				printf("Ethernet\n");
				printf("                    MAC Address ...... %s\n",
					interface->Device()->DecodeStringDescriptor(descriptor->data[1]));
				printf("                    Statistics ....... ");
				bool somethingPrinted = false;
				if ((descriptor->data[2] & 0x01) != 0) {
					printf("XMIT_OK");
					somethingPrinted = true;
				}
				if ((descriptor->data[2] & 0x02) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("RCV_OK");
					somethingPrinted = true;
				}
				if ((descriptor->data[2] & 0x04) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("XMIT_ERROR");
					somethingPrinted = true;
				}
				if ((descriptor->data[2] & 0x08) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("RCV_ERROR");
					somethingPrinted = true;
				}
				if ((descriptor->data[2] & 0x10) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("RCV_NO_BUFFER");
					somethingPrinted = true;
				}
				if ((descriptor->data[2] & 0x20) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("DIRECTED_BYTES_XMIT");
					somethingPrinted = true;
				}
				if ((descriptor->data[2] & 0x40) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("DIRECTED_FRAMES_XMIT");
					somethingPrinted = true;
				}
				if ((descriptor->data[2] & 0x80) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("MULTICAST_BYTES_XMIT");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x01) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("MULTICAST_FRAMES_XMIT");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x02) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("BROADCAST_BYTES_XMIT");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x04) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("BROADCAST_FRAMES_XMIT");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x08) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("DIRECTED_BYTES_RCV");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x10) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("DIRECTED_FRAMES_RCV");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x20) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("MULTICAST_BYTES_RCV");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x40) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("MULTICAST_FRAMES_RCV");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x80) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("BROADCAST_BYTES_RCV");
					somethingPrinted = true;
				}
				if ((descriptor->data[4] & 0x01) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("BROADCAST_FRAMES_RCV");
					somethingPrinted = true;
				}
				if ((descriptor->data[4] & 0x02) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("RCV_CRC_ERROR");
					somethingPrinted = true;
				}
				if ((descriptor->data[4] & 0x04) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("TRANSMIT_QUEUE_LENGTH");
					somethingPrinted = true;
				}
				if ((descriptor->data[4] & 0x08) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("RCV_ERROR_ALIGNMENT");
					somethingPrinted = true;
				}
				if ((descriptor->data[4] & 0x10) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("XMIT_ONE_COLLISION");
					somethingPrinted = true;
				}
				if ((descriptor->data[4] & 0x20) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("XMIT_MORE_COLLISIONS");
					somethingPrinted = true;
				}
				if ((descriptor->data[4] & 0x40) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("XMIT_DEFERRED");
					somethingPrinted = true;
				}
				if ((descriptor->data[4] & 0x80) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("XMIT_MAX_COLLISIONS");
					somethingPrinted = true;
				}
				if ((descriptor->data[5] & 0x01) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("RCV_OVERRUN");
					somethingPrinted = true;
				}
				if ((descriptor->data[5] & 0x02) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("XMIT_UNDERRUN");
					somethingPrinted = true;
				}
				if ((descriptor->data[5] & 0x04) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("XMIT_HEARTBEAT_FAILURE");
					somethingPrinted = true;
				}
				if ((descriptor->data[5] & 0x08) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("XMIT_TIMES_CRS_LOST");
					somethingPrinted = true;
				}
				if ((descriptor->data[5] & 0x10) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("XMIT_LATE_COLLISIONS");
					somethingPrinted = true;
				}
				// 4 bytes - Bitfield indicating collected statistics
				if (!somethingPrinted)
					printf("None");
				printf("\n");
				printf("                    Max. Segment Size  %d\n",
					descriptor->data[6] | descriptor->data[7] << 8);
				printf("                    Multicast filters  %d\n",
					descriptor->data[8] | descriptor->data[9] << 8);
				printf("                    Power filters ...  %d\n",
					descriptor->data[10]);
				return;
			}
			case 0x10:
				printf("ATM\n");
				break;
			case 0x11:
				printf("Wireless handset\n");
				break;
			case 0x12:
				printf("Mobile direct line\n");
				break;
			case 0x13:
				printf("Mobile direct line detail\n");
				break;
			case 0x14:
				printf("Device management\n");
				break;
			case 0x15:
				printf("Object Exchange\n");
				break;
			case 0x16:
				printf("Command set\n");
				break;
			case 0x17:
				printf("Command set detail\n");
				break;
			case 0x18:
				printf("Telephone control\n");
				break;
			case 0x19:
				printf("Object Exchange service identifier\n");
				break;
			case 0x1A:
			{
				printf("Network Control Model (NCM)\n");
				printf("                    Version .......... %d.%d\n",
					descriptor->data[2], descriptor->data[1]);
				printf("                    Capabilities ..... ");
				bool somethingPrinted = false;
				if ((descriptor->data[3] & 0x80) != 0) {
					printf("Extended 1.1 capabilitiies");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x20) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("8-bit Get/SetNbInputSize");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x10) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("Get/SetCrcMode");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x08) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("Get/SetMaxDatagramSize");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x04) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("SendEncapsulatedCommand/GetEncapsulatedResponse");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x02) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("Get/SetNetAddress");
					somethingPrinted = true;
				}
				if ((descriptor->data[3] & 0x01) != 0) {
					if (somethingPrinted)
						printf(", ");
					printf("SetEthernetPacketFilter");
					somethingPrinted = true;
				}
				if (!somethingPrinted)
					printf("None");
				printf("\n");
				return;
			}
			default:
				printf("0x%02x\n", descriptor->data[0]);
		}

		printf("                    Data ............. ");
		// len includes len and descriptor_type field
		// start at i = 1 because we already dumped the first byte as subtype
		for (int32 i = 1; i < descriptor->length - 2; i++)
			printf("%02x ", descriptor->data[i]);
		printf("\n");
		return;
	}

#if 0
	if (descriptor->descriptor_type == 0x25) {
		printf("                    Type ............. CDC endpoint descriptor\n",
		return;
	}
#endif

	DumpDescriptorData(descriptor);
}
