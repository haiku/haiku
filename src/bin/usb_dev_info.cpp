/*
 * Originally released under the Be Sample Code License.
 * Copyright 2000, Be Incorporated. All rights reserved.
 *
 * Modified for Haiku by François Revol and Michael Lotz.
 * Copyright 2007, Haiku Inc. All rights reserved.
 */

#include <USBKit.h>
#include <stdio.h>


static void
DumpInterface(const BUSBInterface *interface)
{
	if (!interface)
		return;

	printf("    Class .............. %d\n", interface->Class());
	printf("    Subclass ........... %d\n", interface->Subclass());
	printf("    Protocol ........... %d\n", interface->Protocol());
	printf("    Interface String ... \"%s\"\n", interface->InterfaceString());

	for (int32 i = 0; i < interface->CountEndpoints(); i++) {
		const BUSBEndpoint *endpoint = interface->EndpointAt(i);
		if (!endpoint)
			continue;

		printf("      [Endpoint %d]\n", i);
		printf("      MaxPacketSize .... %d\n", endpoint->MaxPacketSize());
		printf("      Interval ......... %d\n", endpoint->Interval());

		if (endpoint->IsControl())
			printf("      Type ............. Control\n");
		else if (endpoint->IsBulk())
			printf("      Type ............. Bulk\n");
		else if (endpoint->IsIsochronous())
			printf("      Type ............. Isochronous\n");
		else if (endpoint->IsInterrupt())
			printf("      Type ............. Interrupt\n");

		if(endpoint->IsInput())
			printf("      Direction ........ Input\n");
		else
			printf("      Direction ........ Output\n");
	}

	char buffer[256];
	usb_descriptor *generic = (usb_descriptor *)buffer;
	for (int32 i = 0; interface->OtherDescriptorAt(i, generic, 256) == B_OK; i++) {
		printf("      [Descriptor %d]\n", i);
		printf("      Type ............. 0x%02x\n", generic->generic.descriptor_type);

		printf("      Data ............. ");
		for(int32 j = 0; j < generic->generic.length; j++)
			printf("%02x ", generic->generic.data[j]);
		printf("\n");
	}
}


static void
DumpConfiguration(const BUSBConfiguration *configuration)
{
	if (!configuration)
		return;

	printf("  Configuration String . \"%s\"\n", configuration->ConfigurationString());
	for (int32 i = 0; i < configuration->CountInterfaces(); i++) {
		printf("    [Interface %d]\n", i);
		DumpInterface(configuration->InterfaceAt(i));
	}
}


static void
DumpInfo(BUSBDevice &device)
{
	printf("[Device]\n");
	printf("Class .................. %d\n", device.Class());
	printf("Subclass ............... %d\n", device.Subclass());
	printf("Protocol ............... %d\n", device.Protocol());
	printf("Max Endpoint 0 Packet .. %d\n", device.MaxEndpoint0PacketSize());
	printf("USB Version ............ 0x%04x\n", device.USBVersion());
	printf("Vendor ID .............. 0x%04x\n", device.VendorID());
	printf("Product ID ............. 0x%04x\n", device.ProductID());
	printf("Product Version ........ 0x%04x\n", device.Version());
	printf("Manufacturer String .... \"%s\"\n", device.ManufacturerString());
	printf("Product String ......... \"%s\"\n", device.ProductString());
	printf("Serial Number .......... \"%s\"\n", device.SerialNumberString());

	for (int32 i = 0; i < device.CountConfigurations(); i++) {
		printf("  [Configuration %d]\n", i);
		DumpConfiguration(device.ConfigurationAt(i));
	}
}


int
main(int argc, char *argv[])
{
	if(argc == 2) {
		BUSBDevice device(argv[1]);

		if (device.InitCheck() < B_OK) {
			printf("Cannot open USB device: %s\n", argv[1]);
			return 1;
		} else {
			DumpInfo(device);
			return 0;
		}
	} else {
		printf("Usage: info <device>\n");
		return 1;
	}
}
