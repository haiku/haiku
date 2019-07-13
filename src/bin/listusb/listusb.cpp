/*
 * Originally released under the Be Sample Code License.
 * Copyright 2000, Be Incorporated. All rights reserved.
 *
 * Modified for Haiku by François Revol and Michael Lotz.
 * Copyright 2007-2008, Haiku Inc. All rights reserved.
 */

#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <String.h>
#include <stdio.h>
#include <usb/USB_audio.h>
#include <usb/USB_video.h>

#include "usbspec_private.h"
#include "usb-utils.h"

#include "listusb.h"


const char*
ClassName(int classNumber) {
	switch (classNumber) {
		case 0:
			return "Per-interface classes";
		case USB_AUDIO_DEVICE_CLASS:
			return "Audio";
		case 2:
			return "Communication";
		case 3:
			return "HID";
		case 5:
			return "Physical";
		case 6:
			return "Image";
		case 7:
			return "Printer";
		case 8:
			return "Mass storage";
		case 9:
			return "Hub";
		case 10:
			return "CDC-Data";
		case 11:
			return "Smart card";
		case 13:
			return "Content security";
		case USB_VIDEO_DEVICE_CLASS:
			return "Video";
		case 15:
			return "Personal Healthcare";
		case 0xDC:
			return "Diagnostic device";
		case 0xE0:
			return "Wireless controller";
		case 0xEF:
			return "Miscellaneous";
		case 0xFE:
			return "Application specific";
		case 0xFF:
			return "Vendor specific";
	}

	return "Unknown";
}


const char*
SubclassName(int classNumber, int subclass)
{
	if (classNumber == 0xEF) {
		if (subclass == 0x02)
			return " (Common)";
		if (subclass == 0x04)
			return " (RNDIS)";
		if (subclass == 0x05)
			return " (USB3 Vision)";
		if (subclass == 0x06)
			return " (STEP)";
		if (subclass == 0x07)
			return " (DVB Command Interface)";
	}

	if (classNumber == USB_VIDEO_DEVICE_CLASS) {
		switch (subclass) {
			case USB_VIDEO_INTERFACE_UNDEFINED_SUBCLASS:
				return " (Undefined)";
			case USB_VIDEO_INTERFACE_VIDEOCONTROL_SUBCLASS:
				return " (Control)";
			case USB_VIDEO_INTERFACE_VIDEOSTREAMING_SUBCLASS:
				return " (Streaming)";
			case USB_VIDEO_INTERFACE_COLLECTION_SUBCLASS:
				return " (Collection)";
		}
	}

	if (classNumber == 0xFE) {
		if (subclass == 0x01)
			return " (Firmware Upgrade)";
		if (subclass == 0x02)
			return " (IrDA)";
		if (subclass == 0x03)
			return " (Test and measurement)";
	}

	return "";
}


const char*
ProtocolName(int classNumber, int subclass, int protocol)
{
	switch (classNumber) {
		case 0x09:
			if (subclass == 0x00)
			{
				switch (protocol) {
					case 0x00:
						return " (Full speed)";
					case 0x01:
						return " (Hi-speed, single TT)";
					case 0x02:
						return " (Hi-speed, multiple TT)";
					case 0x03:
						return " (Super speed)";
				}
			}
		case 0xE0:
			if (subclass == 0x01 && protocol == 0x01)
				return " (Bluetooth)";
		case 0xEF:
			if (subclass == 0x02 && protocol == 0x01)
				return " (Interface Association)";
			break;
	}
	return "";
}


void
DumpDescriptorData(const usb_generic_descriptor* descriptor)
{
	printf("                    Type ............. 0x%02x\n",
		descriptor->descriptor_type);

	printf("                    Data ............. ");
	// len includes len and descriptor_type field
	for (int32 i = 0; i < descriptor->length - 2; i++)
		printf("%02x ", descriptor->data[i]);
	printf("\n");
}


void
DumpDescriptor(const usb_generic_descriptor* descriptor,
	int classNum, int subclass)
{
	switch (classNum) {
		case USB_AUDIO_DEVICE_CLASS:
			DumpAudioDescriptor(descriptor, subclass);
			break;
		case USB_VIDEO_DEVICE_CLASS:
			DumpVideoDescriptor(descriptor, subclass);
			break;
		default:
			DumpDescriptorData(descriptor);
			break;
	}
}


static void
DumpInterface(const BUSBInterface* interface)
{
	if (!interface)
		return;

	printf("                Class .............. 0x%02x (%s)\n",
		interface->Class(), ClassName(interface->Class()));
	printf("                Subclass ........... 0x%02x%s\n",
		interface->Subclass(),
		SubclassName(interface->Class(), interface->Subclass()));
	printf("                Protocol ........... 0x%02x%s\n",
		interface->Protocol(), ProtocolName(interface->Class(),
			interface->Subclass(), interface->Protocol()));
	printf("                Interface String ... \"%s\"\n",
		interface->InterfaceString());

	for (uint32 i = 0; i < interface->CountEndpoints(); i++) {
		const BUSBEndpoint* endpoint = interface->EndpointAt(i);
		if (!endpoint)
			continue;

		printf("                [Endpoint %" B_PRIu32 "]\n", i);
		printf("                    MaxPacketSize .... %d\n",
			endpoint->MaxPacketSize());
		printf("                    Interval ......... %d\n",
			endpoint->Interval());

		if (endpoint->IsControl())
			printf("                    Type ............. Control\n");
		else if (endpoint->IsBulk())
			printf("                    Type ............. Bulk\n");
		else if (endpoint->IsIsochronous())
			printf("                    Type ............. Isochronous\n");
		else if (endpoint->IsInterrupt())
			printf("                    Type ............. Interrupt\n");

		if (endpoint->IsInput())
			printf("                    Direction ........ Input\n");
		else
			printf("                    Direction ........ Output\n");
	}

	char buffer[256];
	usb_descriptor* generic = (usb_descriptor*)buffer;
	for (uint32 i = 0;
			interface->OtherDescriptorAt(i, generic, 256) == B_OK; i++) {
		printf("                [Descriptor %" B_PRIu32 "]\n", i);
		DumpDescriptor(&generic->generic, interface->Class(), interface->Subclass());
	}
}


static void
DumpConfiguration(const BUSBConfiguration* configuration)
{
	if (!configuration)
		return;

	printf("        Configuration String . \"%s\"\n",
		configuration->ConfigurationString());
	for (uint32 i = 0; i < configuration->CountInterfaces(); i++) {
		printf("        [Interface %" B_PRIu32 "]\n", i);
		const BUSBInterface* interface = configuration->InterfaceAt(i);

		for (uint32 j = 0; j < interface->CountAlternates(); j++) {
			const BUSBInterface* alternate = interface->AlternateAt(j);
			printf("            [Alternate %" B_PRIu32 "%s]\n", j,
				j == interface->AlternateIndex() ? " active" : "");
			DumpInterface(alternate);
		}
	}
}


static void
DumpInfo(BUSBDevice& device, bool verbose)
{
	const char* vendorName = NULL;
	const char* deviceName = NULL;
	usb_get_vendor_info(device.VendorID(), &vendorName);
	usb_get_device_info(device.VendorID(), device.ProductID(), &deviceName);

	if (!verbose) {
		printf("%04x:%04x /dev/bus/usb%s \"%s\" \"%s\" ver. %04x\n",
			device.VendorID(), device.ProductID(), device.Location(),
			vendorName ? vendorName : device.ManufacturerString(),
			deviceName ? deviceName : device.ProductString(),
			device.Version());
		return;
	}

	printf("[Device /dev/bus/usb%s]\n", device.Location());
	printf("    Class .................. 0x%02x (%s)\n", device.Class(),
		ClassName(device.Class()));
	printf("    Subclass ............... 0x%02x%s\n", device.Subclass(),
		SubclassName(device.Class(), device.Subclass()));
	printf("    Protocol ............... 0x%02x%s\n", device.Protocol(),
		ProtocolName(device.Class(), device.Subclass(), device.Protocol()));
	printf("    Max Endpoint 0 Packet .. %d\n", device.MaxEndpoint0PacketSize());
	uint32_t version = device.USBVersion();
	printf("    USB Version ............ %d.%d\n", version >> 8, version & 0xFF);
	printf("    Vendor ID .............. 0x%04x", device.VendorID());
	if (vendorName != NULL)
		printf(" (%s)", vendorName);
	printf("\n    Product ID ............. 0x%04x", device.ProductID());
	if (deviceName != NULL)
		printf(" (%s)", deviceName);
	printf("\n    Product Version ........ 0x%04x\n", device.Version());
	printf("    Manufacturer String .... \"%s\"\n", device.ManufacturerString());
	printf("    Product String ......... \"%s\"\n", device.ProductString());
	printf("    Serial Number .......... \"%s\"\n", device.SerialNumberString());

	for (uint32 i = 0; i < device.CountConfigurations(); i++) {
		printf("    [Configuration %" B_PRIu32 "]\n", i);
		DumpConfiguration(device.ConfigurationAt(i));
	}

	if (device.Class() != 0x09)
		return;

	usb_hub_descriptor hubDescriptor;
	size_t size = device.GetDescriptor(USB_DESCRIPTOR_HUB, 0, 0,
		(void*)&hubDescriptor, sizeof(usb_hub_descriptor));
	if (size == sizeof(usb_hub_descriptor)) {
		printf("    Hub ports count......... %d\n", hubDescriptor.num_ports);
		printf("    Hub Controller Current.. %dmA\n", hubDescriptor.max_power);

		for (int index = 1; index <= hubDescriptor.num_ports; index++) {
			usb_port_status portStatus;
			size_t actualLength = device.ControlTransfer(USB_REQTYPE_CLASS
				| USB_REQTYPE_OTHER_IN, USB_REQUEST_GET_STATUS, 0,
				index, sizeof(portStatus), (void*)&portStatus);
			if (actualLength != sizeof(portStatus))
				continue;
			printf("      Port %d status....... %04x.%04x%s%s%s%s%s%s%s%s\n",
				index, portStatus.status, portStatus.change,
				portStatus.status & PORT_STATUS_CONNECTION ? " Connect": "",
				portStatus.status & PORT_STATUS_ENABLE ? " Enable": "",
				portStatus.status & PORT_STATUS_SUSPEND ? " Suspend": "",
				portStatus.status & PORT_STATUS_OVER_CURRENT ? " Overcurrent": "",
				portStatus.status & PORT_STATUS_RESET ? " Reset": "",
				portStatus.status & PORT_STATUS_POWER ? " Power": "",
				portStatus.status & PORT_STATUS_TEST ? " Test": "",
				portStatus.status & PORT_STATUS_INDICATOR ? " Indicator": "");
		}
	}
}


class DumpRoster : public BUSBRoster {
public:
					DumpRoster(bool verbose)
						:	fVerbose(verbose)
					{
					}


virtual	status_t	DeviceAdded(BUSBDevice* device)
					{
						DumpInfo(*device, fVerbose);
						return B_OK;
					}


virtual	void		DeviceRemoved(BUSBDevice* device)
					{
					}

private:
		bool		fVerbose;
};



int
main(int argc, char* argv[])
{
	bool verbose = false;
	BString devname = "";
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'v')
				verbose = true;
			else {
				printf("Usage: listusb [-v] [device]\n\n");
				printf("-v: Show more detailed information including "
					"interfaces, configurations, etc.\n\n");
				printf("If a device is not specified, "
					"all devices found on the bus will be listed\n");
				return 1;
			}
		} else
			devname = argv[i];
	}

	if (devname.Length() > 0) {
		BUSBDevice device(devname.String());
		if (device.InitCheck() < B_OK) {
			printf("Cannot open USB device: %s\n", devname.String());
			return 1;
		} else {
				DumpInfo(device, verbose);
				return 0;
		}
	} else {
		DumpRoster roster(verbose);
		roster.Start();
		roster.Stop();
	}

	return 0;
}
