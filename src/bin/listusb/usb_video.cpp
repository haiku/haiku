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
			fputs(ProcessingControlString(i), stdout);
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
DumpVideoControlCSInterruptEndpointDescriptor(const usb_generic_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x (Endpoint)\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Interrupt)\n",
		(uint8)descriptor->data[0]);
	printf("                    Max Transfer Size . %u\n",
		(uint16)((descriptor->data[1] << 8) | descriptor->data[2]));
}


void
DumpVideoControlCSEndpointDescriptor(const usb_generic_descriptor* descriptor)
{
	uint8 descriptorSubtype = descriptor->data[0];
	switch (descriptorSubtype) {
		case EP_SUBTYPE_INTERRUPT:
			DumpVideoControlCSInterruptEndpointDescriptor(descriptor);
			break;
		default:
			DumpDescriptorData(descriptor);
	}
}


void
DumpVideoStreamInputHeaderDescriptor(const usb_generic_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x (VideoStream Interface)\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Input header)\n",
		(uint8)descriptor->data[0]);
	printf("                    Format count ...... %u\n",
		(uint8)descriptor->data[1]);
	printf("                    Total length ...... %u\n",
		(uint16)((descriptor->data[2] << 8) | descriptor->data[3]));
	printf("                    Endpoint .......... 0x%02x\n",
		(uint8)descriptor->data[4]);
	printf("                    Info .............. 0x%02x\n",
		(uint8)descriptor->data[5]);
	printf("                    Terminal Link ..... 0x%02x\n",
		(uint8)descriptor->data[6]);
	printf("                    Still capture ..... 0x%02x\n",
		(uint8)descriptor->data[7]);
	printf("                    Trigger support ... %u\n",
		(uint8)descriptor->data[8]);
	printf("                    Trigger usage ..... %u\n",
		(uint8)descriptor->data[9]);

	uint8 nformat = descriptor->data[1];
	uint8 formatsize = descriptor->data[10];
	uint8 i, j;

	for (i = 0; i < nformat; i++)
	{
		printf("                    Format %2d ......... 0x", i);
		for (j = 0; j < formatsize; j++)
			printf("%02x", (uint8)descriptor->data[11 + i * formatsize + j]);
		printf("\n");
	}
}


void
DumpVideoStillImageDescriptor(const usb_generic_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x (VideoStream Interface)\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (Still Image)\n",
		(uint8)descriptor->data[0]);
	printf("                    Endpoint .......... %u\n",
		(uint8)descriptor->data[1]);

	uint8 npatterns = descriptor->data[2];
	uint8 i;
	printf("                    Resolutions ....... ");
	for (i = 0; i < npatterns; i++)
	{
		// FIXME these are reverse-endian compared to everything else.
		// Is my webcam wrong, or is it some quirk in the spec?
		printf("%ux%u, ",
			(uint16)((descriptor->data[i * 4 + 4] << 8) | (descriptor->data[i * 4 + 3])),
			(uint16)((descriptor->data[i * 4 + 6] << 8) | (descriptor->data[i * 4 + 5])));
	}
	printf("\n");

	i = i * 4 + 3;

	npatterns = descriptor->data[i];
	while (npatterns > 0)
	{
		printf("                    Compression ....... %u\n",
			(uint8)descriptor->data[i]);
		npatterns--;
	}
}


static const char*
VSInterfaceString(int subtype)
{
	switch(subtype) {
		case USB_VIDEO_VS_UNDEFINED:
			return "Undefined";
		case USB_VIDEO_VS_INPUT_HEADER:
			return "Input header";
		case USB_VIDEO_VS_OUTPUT_HEADER:
			return "Output header";
		case USB_VIDEO_VS_STILL_IMAGE_FRAME:
			return "Still image";
		case USB_VIDEO_VS_FORMAT_UNCOMPRESSED:
			return "Uncompressed format";
		case USB_VIDEO_VS_FRAME_UNCOMPRESSED:
			return "Uncompressed frame";
		case USB_VIDEO_VS_FORMAT_MJPEG:
			return "MJPEG format";
		case USB_VIDEO_VS_FRAME_MJPEG:
			return "MJPEG frame";
		case USB_VIDEO_VS_FORMAT_MPEG2TS:
			return "MPEG2TS format";
		case USB_VIDEO_VS_FORMAT_DV:
			return "DV format";
		case USB_VIDEO_VS_COLORFORMAT:
			return "Color format";
		case USB_VIDEO_VS_FORMAT_FRAME_BASED:
			return "Frame based format";
		case USB_VIDEO_VS_FRAME_FRAME_BASED:
			return "Frame based frame";
		case USB_VIDEO_VS_FORMAT_STREAM_BASED:
			return "Stream based format";
		case USB_VIDEO_VS_FORMAT_H264:
			return "H264 format";
		case USB_VIDEO_VS_FRAME_H264:
			return "H264 frame";
		case USB_VIDEO_VS_FORMAT_H264_SIMULCAST:
			return "H264 simulcast";
		case USB_VIDEO_VS_FORMAT_VP8:
			return "VP8 format";
		case USB_VIDEO_VS_FRAME_VP8:
			return "VP8 frame";
		case USB_VIDEO_VS_FORMAT_VP8_SIMULCAST:
			return "VP8 simulcast";
		default:
			return "Unknown";
	};
}


void
DumpVideoFormatDescriptor(const usb_generic_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x (VideoStream Interface)\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (%s)\n",
		(uint8)descriptor->data[0], VSInterfaceString(descriptor->data[0]));
	printf("                    Index ............. 0x%02x\n",
		(uint8)descriptor->data[1]);
	printf("                    Frame number ...... 0x%02x\n",
		(uint8)descriptor->data[2]);

	printf("                    GUID .............. ");
	for (uint8 i = 3; i < 16 + 3; i++)
		printf("%02x ", descriptor->data[i]);
	printf("\n");

	printf("                    Bits per pixel .... %u\n",
		(uint8)descriptor->data[19]);
	printf("                    Default frame idx . 0x%02x\n",
		(uint8)descriptor->data[20]);
	printf("                    Aspect ratio ...... %u:%u\n",
		(uint8)descriptor->data[21], (uint8)descriptor->data[22]);
	printf("                    Interlace flags ... 0x%02x\n",
		(uint8)descriptor->data[23]);
	printf("                    Copy protect ...... %u\n",
		(uint8)descriptor->data[24]);
}


void
DumpVideoFrameDescriptor(const usb_video_frame_descriptor* descriptor)
{
	printf("                    Type .............. 0x%02x (VideoStream Interface)\n",
		descriptor->descriptor_type);
	printf("                    Subtype ........... 0x%02x (%s)\n",
		descriptor->descriptor_subtype,
		VSInterfaceString(descriptor->descriptor_subtype));
	printf("                    Index ............. 0x%02x\n",
		descriptor->frame_index);
	printf("                    Capabilities ...... 0x%02x\n",
		descriptor->capabilities);
	printf("                    Resolution ........ %u x %u\n",
		descriptor->width, descriptor->height);
	printf("                    Bit rates ......... %" B_PRIu32 " - %" B_PRIu32 "\n",
		descriptor->min_bit_rate, descriptor->max_bit_rate);
	printf("                    Frame buffer size . %" B_PRIu32 "\n",
		descriptor->max_video_frame_buffer_size);
	printf("                    Frame interval .... %.4fms\n",
		descriptor->default_frame_interval / 10000.f);
	for (uint8 i = 0; i < descriptor->frame_interval_type; i++)
	{
		printf("                    Frame interval %2d . %.4fms\n",
			i, descriptor->discrete_frame_intervals[i] / 10000.f);
	}
	// TODO if frame__interval_type is 0, dump continuous frame intervals
}


void
DumpVideoStreamCSInterfaceDescriptor(const usb_generic_descriptor* descriptor)
{
	uint8 subtype = descriptor->data[0];
	switch (subtype) {
		case USB_VIDEO_VS_INPUT_HEADER:
			DumpVideoStreamInputHeaderDescriptor(descriptor);
			break;
		case USB_VIDEO_VS_STILL_IMAGE_FRAME:
			DumpVideoStillImageDescriptor(descriptor);
			break;
		case USB_VIDEO_VS_FORMAT_UNCOMPRESSED:
		case USB_VIDEO_VS_FORMAT_MJPEG:
			DumpVideoFormatDescriptor(descriptor);
			break;
		case USB_VIDEO_VS_FRAME_UNCOMPRESSED:
		case USB_VIDEO_VS_FRAME_MJPEG:
			DumpVideoFrameDescriptor((usb_video_frame_descriptor*)descriptor);
			break;
		default:
			DumpDescriptorData(descriptor);
			break;
	}
}



void
DumpVideoDescriptor(const usb_generic_descriptor* descriptor, int subclass)
{
	switch (subclass) {
		case USB_VIDEO_INTERFACE_VIDEOCONTROL_SUBCLASS:
			switch (descriptor->descriptor_type) {
				case USB_VIDEO_CS_INTERFACE:
					DumpVideoControlCSInterfaceDescriptor(descriptor);
					break;
				case USB_VIDEO_CS_ENDPOINT:
					DumpVideoControlCSEndpointDescriptor(descriptor);
					break;
				default:
					DumpDescriptorData(descriptor);
					break;
			}
			break;
		case USB_VIDEO_INTERFACE_VIDEOSTREAMING_SUBCLASS:
			switch (descriptor->descriptor_type) {
				case USB_VIDEO_CS_INTERFACE:
					DumpVideoStreamCSInterfaceDescriptor(descriptor);
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

