/*
 * Copyright 2016, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>

#include <usb/USB_video.h>

#include "listusb.h"


void
DumpVideoCSInterfaceDescriptorHeader(
	const usb_videocontrol_header_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Header)\n",
		descriptor->descriptor_subtype);
	printf("                    UVC Release ....... %d.%d\n",
		descriptor->bcd_release_no >> 8, descriptor->bcd_release_no & 0xFF);
	printf("                    Total Length ...... %u\n",
		descriptor->total_length);
	printf("                    Clock Frequency ... %" B_PRIu32 "\n",
		descriptor->clock_frequency);
	printf("                    Interfaces ........ ");

	for (uint8 i = 0; i < descriptor->in_collection; i++)
		printf("%u, ", descriptor->interface_numbers[i]);
	printf("\n");
}


static const char*
TerminalTypeName(uint16 terminalType)
{
	switch (terminalType) {
		case USB_VIDEO_VENDOR_USB_IO:
			return "Vendor specific";
		case USB_VIDEO_STREAMING_USB_IO:
			return "Streaming";

		case USB_VIDEO_VENDOR_IN:
			return "Vendor specific input";
		case USB_VIDEO_CAMERA_IN:
			return "Camera";
		case USB_VIDEO_MEDIA_TRANSPORT_IN:
			return "Media transport input";

		case USB_VIDEO_VENDOR_OUT:
			return "Vendor specific output";
		case USB_VIDEO_DISPLAY_OUT:
			return "Display";
		case USB_VIDEO_MEDIA_TRANSPORT_OUT:
			return "Media transport output";

		case USB_VIDEO_VENDOR_EXT:
			return "Vendor specific format";
		case USB_VIDEO_COMPOSITE_EXT:
			return "Composite";
		case USB_VIDEO_SVIDEO_EXT:
			return "S-Video";
		case USB_VIDEO_COMPONENT_EXT:
			return "Component";

		default:
			return "Unknown";
	}
}


void
DumpVideoCSInterfaceDescriptorOutputTerminal(
	const usb_video_output_terminal_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Output Terminal)\n",
		descriptor->descriptor_subtype);
	printf("                    Terminal ID ....... %u\n",
		descriptor->terminal_id);
	printf("                    Terminal Type ..... 0x%04x (%s)\n",
		descriptor->terminal_type,
			TerminalTypeName(descriptor->terminal_type));
	printf("                    Associated Terminal %u\n",
		descriptor->assoc_terminal);
	printf("                    Source ID ......... %u\n",
		descriptor->source_id);
	printf("                    Terminal .......... %u\n",
		descriptor->terminal);
}


void
DumpVideoCSInterfaceDescriptorInputTerminal(
	const usb_video_input_terminal_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Input Terminal)\n",
		descriptor->descriptor_subtype);
	printf("                    Terminal ID ....... %u\n",
		descriptor->terminal_id);
	printf("                    Terminal Type ..... 0x%04x (%s)\n",
		descriptor->terminal_type,
			TerminalTypeName(descriptor->terminal_type));
	printf("                    Terminal .......... %u\n",
		descriptor->terminal);

	if (descriptor->terminal_type == USB_VIDEO_CAMERA_IN)
	{
		printf("                    Min. Focal length . %u\n",
			descriptor->camera.focal_length_min);
		printf("                    Max. Focal length . %u\n",
			descriptor->camera.focal_length_min);
		printf("                    Focal length ...... %u\n",
			descriptor->camera.focal_length);
		printf("                    Controls .......... %02x%02x%02x\n",
			descriptor->camera.controls[0],
			descriptor->camera.controls[1],
			descriptor->camera.controls[2]);
	}
}


static const char*
ProcessingControlString(int index)
{
	switch(index)
	{
		case 0:
			return "Brightness, ";
		case 1:
			return "Contrast, ";
		case 2:
			return "Hue, ";
		case 3:
			return "Saturation, ";
		case 4:
			return "Sharpness, ";
		case 5:
			return "Gamma, ";
		case 6:
			return "White balance temp., ";
		case 7:
			return "White balance component, ";
		case 8:
			return "Backlight compensation, ";
		case 9:
			return "Gain, ";
		case 10:
			return "Power line frequency, ";
		case 11:
			return "Automatic hue, ";
		case 12:
			return "Automatic white balance temp., ";
		case 13:
			return "Automatic white balance component, ";
		case 14:
			return "Digital multiplier, ";
		case 15:
			return "Digital multiplier limit, ";
		case 16:
			return "Analog video standard, ";
		case 17:
			return "Analog video lock status, ";
		case 18:
			return "Automatic contrast, ";
		default:
			return "Unknown, ";
	}
}


void
DumpVideoCSInterfaceDescriptorProcessingUnit(
	const usb_video_processing_unit_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Processing unit)\n",
		descriptor->descriptor_subtype);
	printf("                    Unit ID ........... %u\n",
		descriptor->unit_id);
	printf("                    Source ID ......... %u\n",
		descriptor->source_id);
	printf("                    Max Multiplier .... %f\n",
		descriptor->max_multiplier / 100.f);
	printf("                    Controls .......... ");
	uint32_t controls = (descriptor->controls[0] << 16)
		| (descriptor->controls[1] << 8)
		| descriptor->controls[2];
	for (int i = 0; i < 19; i++)
	{
		if (controls & (1 << (23 - i))) {
			printf(ProcessingControlString(i));
		}
	}
	printf("\n");
	printf("                    Processing ........ %u\n",
		descriptor->processing);
	printf("                    Video Standards ... 0x%02x\n",
		descriptor->video_standards);
}


void
DumpVideoCSInterfaceDescriptorExtensionUnit(
	const usb_generic_descriptor* descriptor)
{
	uint8 i = 0;

	printf("                    Type .............. 0x%02x\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Extension unit)\n",
		(uint8)descriptor->data[i++]);
	printf("                    Unit ID ........... %u\n",
		(uint8)descriptor->data[i++]);

	printf("                    GUID .............. ");
	for (i = 2; i < 16 + 2; i++)
		printf("%02x ", descriptor->data[i]);
	printf("\n");

	printf("                    Control count ..... %u\n",
		(uint8)descriptor->data[i++]);

	printf("                    Input pins ........ ");
	i = 20; // Skip the input pin count
	for (; i - 20 < descriptor->data[19]; i++)
		printf("%u, ", descriptor->data[i]);
	printf("\n");

	printf("                    Controls .......... ");
	uint8_t end = descriptor->data[i++];
	uint8_t start = i;
	for (; i - start < end; i++)
		printf("%02x", (uint8)descriptor->data[i]);
	printf("\n");

	printf("                    Extension ......... %u\n",
		(uint8)descriptor->data[i++]);
}


void
DumpVideoControlCSInterfaceDescriptor(const usb_generic_descriptor* descriptor)
{
	uint8 descriptorSubtype = descriptor->data[0];
	switch (descriptorSubtype) {
		case USB_VIDEO_VC_HEADER:
			DumpVideoCSInterfaceDescriptorHeader(
				(usb_videocontrol_header_descriptor*)descriptor);
			break;
		case USB_VIDEO_VC_INPUT_TERMINAL:
			DumpVideoCSInterfaceDescriptorInputTerminal(
				(usb_video_input_terminal_descriptor*)descriptor);
			break;
		case USB_VIDEO_VC_OUTPUT_TERMINAL:
			DumpVideoCSInterfaceDescriptorOutputTerminal(
				(usb_video_output_terminal_descriptor*)descriptor);
			break;
		case USB_VIDEO_VC_PROCESSING_UNIT:
			DumpVideoCSInterfaceDescriptorProcessingUnit(
				(usb_video_processing_unit_descriptor*)descriptor);
			break;
		case USB_VIDEO_VC_EXTENSION_UNIT:
			DumpVideoCSInterfaceDescriptorExtensionUnit(descriptor);
			break;
		default:
			DumpDescriptorData(descriptor);
	}
}


void
DumpAudioStreamCSInterfaceDescriptor(const usb_generic_descriptor* descriptor);

void
DumpVideoDescriptor(const usb_generic_descriptor* descriptor, int subclass)
{
	switch (subclass) {
		case USB_VIDEO_INTERFACE_VIDEOCONTROL_SUBCLASS:
			switch (descriptor->descriptor_type) {
				case USB_VIDEO_CS_INTERFACE:
					DumpVideoControlCSInterfaceDescriptor(descriptor);
					break;
				default:
					DumpDescriptorData(descriptor);
					break;
			}
			break;
		case USB_VIDEO_INTERFACE_VIDEOSTREAMING_SUBCLASS:
			switch (descriptor->descriptor_type) {
				case USB_VIDEO_CS_INTERFACE:
					DumpAudioStreamCSInterfaceDescriptor(descriptor);
					break;
				default:
					DumpDescriptorData(descriptor);
					break;
			}
			break;
		case USB_VIDEO_INTERFACE_COLLECTION_SUBCLASS:
			switch (descriptor->descriptor_type) {
				case USB_VIDEO_CS_INTERFACE:
					// TODO
					DumpDescriptorData(descriptor);
					break;
				default:
					DumpDescriptorData(descriptor);
					break;
			}
			break;
		default:
			DumpDescriptorData(descriptor);
			break;
	}
}

