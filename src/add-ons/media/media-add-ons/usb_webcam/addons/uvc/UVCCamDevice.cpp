/*
 * Copyright 2011, Gabriel Hartmann, gabriel.hartmann@gmail.com.
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2009, Ithamar Adema, <ithamar.adema@team-embedded.nl>.
 * Distributed under the terms of the MIT License.
 */


#include "UVCCamDevice.h"
#include "UVCDeframer.h"

#include <stdio.h>
#include <stdlib.h>
#include <ParameterWeb.h>
#include <media/Buffer.h>


usb_webcam_support_descriptor kSupportedDevices[] = {
	// ofcourse we support a generic UVC device...
	{{ USB_VIDEO_DEVICE_CLASS, USB_VIDEO_INTERFACE_VIDEOCONTROL_SUBCLASS, 0, 0, 0 }, "Generic UVC", "Video Class", "??" },
	{{ 0xEF, 0x02, 0, 0, 0 }, "Miscellaneous device", "Interface association", "??" },
	// ...whilst the following IDs were 'stolen' from a recent Linux driver:
	{{ 0, 0, 0, 0x045e, 0x00f8, }, "Microsoft",     "Lifecam NX-6000",                 "??" },
	{{ 0, 0, 0, 0x045e, 0x0723, }, "Microsoft",     "Lifecam VX-7000",                 "??" },
	{{ 0, 0, 0, 0x046d, 0x08c1, }, "Logitech",      "QuickCam Fusion",                 "??" },
	{{ 0, 0, 0, 0x046d, 0x08c2, }, "Logitech",      "QuickCam Orbit MP",               "??" },
	{{ 0, 0, 0, 0x046d, 0x08c3, }, "Logitech",      "QuickCam Pro for Notebook",       "??" },
	{{ 0, 0, 0, 0x046d, 0x08c5, }, "Logitech",      "QuickCam Pro 5000",               "??" },
	{{ 0, 0, 0, 0x046d, 0x08c6, }, "Logitech",      "QuickCam OEM Dell Notebook",      "??" },
	{{ 0, 0, 0, 0x046d, 0x08c7, }, "Logitech",      "QuickCam OEM Cisco VT Camera II", "??" },
	{{ 0, 0, 0, 0x046d, 0x0821, }, "Logitech",      "HD Pro Webcam C910",              "??" },
	{{ 0, 0, 0, 0x05ac, 0x8501, }, "Apple",         "Built-In iSight",                 "??" },
	{{ 0, 0, 0, 0x05e3, 0x0505, }, "Genesys Logic", "USB 2.0 PC Camera",               "??" },
	{{ 0, 0, 0, 0x0e8d, 0x0004, }, "N/A",           "MT6227",                          "??" },
	{{ 0, 0, 0, 0x174f, 0x5212, }, "Syntek",        "(HP Spartan)",                    "??" },
	{{ 0, 0, 0, 0x174f, 0x5931, }, "Syntek",        "(Samsung Q310)",                  "??" },
	{{ 0, 0, 0, 0x174f, 0x8a31, }, "Syntek",        "Asus F9SG",                       "??" },
	{{ 0, 0, 0, 0x174f, 0x8a33, }, "Syntek",        "Asus U3S",                        "??" },
	{{ 0, 0, 0, 0x17ef, 0x480b, }, "N/A",           "Lenovo Thinkpad SL500",           "??" },
	{{ 0, 0, 0, 0x18cd, 0xcafe, }, "Ecamm",         "Pico iMage",                      "??" },
	{{ 0, 0, 0, 0x19ab, 0x1000, }, "Bodelin",       "ProScopeHR",                      "??" },
	{{ 0, 0, 0, 0x1c4f, 0x3000, }, "SiGma Micro",   "USB Web Camera",                  "??" },
	{{ 0, 0, 0, 0, 0}, NULL, NULL, NULL }
};

/* Table 2-1 Compression Formats of USB Video Payload Uncompressed */
usbvc_guid kYUY2Guid = {0x59, 0x55, 0x59, 0x32, 0x00, 0x00, 0x10, 0x00, 0x80,
	0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};
usbvc_guid kNV12Guid = {0x4e, 0x56, 0x31, 0x32, 0x00, 0x00, 0x10, 0x00, 0x80,
	0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};

static void
print_guid(const usbvc_guid guid)
{
	if (!memcmp(guid, kYUY2Guid, sizeof(usbvc_guid)))
		printf("YUY2");
	else if (!memcmp(guid, kNV12Guid, sizeof(usbvc_guid)))
		printf("NV12");
	else {
		printf("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:"
			"%02x:%02x:%02x:%02x", guid[0], guid[1], guid[2], guid[3], guid[4],
			guid[5], guid[6], guid[7], guid[8], guid[9], guid[10], guid[11],
			guid[12], guid[13], guid[14], guid[15]);
	}
}


UVCCamDevice::UVCCamDevice(CamDeviceAddon& _addon, BUSBDevice* _device)
	: CamDevice(_addon, _device),
	fHeaderDescriptor(NULL),
	fInterruptIn(NULL),
	fUncompressedFormatIndex(1),
	fUncompressedFrameIndex(1)
{
	fDeframer = new UVCDeframer(this);
	SetDataInput(fDeframer);

	const BUSBConfiguration* config;
	const BUSBInterface* interface;
	usb_descriptor* generic;
	uint8 buffer[1024];

	generic = (usb_descriptor*)buffer;

	for (uint32 i = 0; i < _device->CountConfigurations(); i++) {
		config = _device->ConfigurationAt(i);
		if (config == NULL)
			continue;
		_device->SetConfiguration(config);
		for (uint32 j = 0; j < config->CountInterfaces(); j++) {
			interface = config->InterfaceAt(j);
			if (interface == NULL)
				continue;

			if (interface->Class() == USB_VIDEO_DEVICE_CLASS && interface->Subclass()
				== USB_VIDEO_INTERFACE_VIDEOCONTROL_SUBCLASS) {
				printf("UVCCamDevice: (%" B_PRIu32 ",%" B_PRIu32 "): Found Video Control "
					"interface.\n", i, j);

				// look for class specific interface descriptors and parse them
				for (uint32 k = 0; interface->OtherDescriptorAt(k, generic,
					sizeof(buffer)) == B_OK; k++) {
					if (generic->generic.descriptor_type != (USB_REQTYPE_CLASS
						| USB_DESCRIPTOR_INTERFACE))
						continue;
					fControlIndex = interface->Index();
					_ParseVideoControl((const usbvc_class_descriptor*)generic,
						generic->generic.length);
				}
				for (uint32 k = 0; k < interface->CountEndpoints(); k++) {
					const BUSBEndpoint* e = interface->EndpointAt(i);
					if (e && e->IsInterrupt() && e->IsInput()) {
						fInterruptIn = e;
						break;
					}
				}
				fInitStatus = B_OK;
			} else if (interface->Class() == USB_VIDEO_DEVICE_CLASS && interface->Subclass()
				== USB_VIDEO_INTERFACE_VIDEOSTREAMING_SUBCLASS) {
				printf("UVCCamDevice: (%" B_PRIu32 ",%" B_PRIu32 "): Found Video Streaming "
					"interface.\n", i, j);

				// look for class specific interface descriptors and parse them
				for (uint32 k = 0; interface->OtherDescriptorAt(k, generic,
					sizeof(buffer)) == B_OK; k++) {
					if (generic->generic.descriptor_type != (USB_REQTYPE_CLASS
						| USB_DESCRIPTOR_INTERFACE))
						continue;
					fStreamingIndex = interface->Index();
					_ParseVideoStreaming((const usbvc_class_descriptor*)generic,
						generic->generic.length);
				}

				for (uint32 k = 0; k < interface->CountEndpoints(); k++) {
					const BUSBEndpoint* e = interface->EndpointAt(i);
					if (e && e->IsIsochronous() && e->IsInput()) {
						fIsoIn = e;
						break;
					}
				}
			}
		}
	}
}


UVCCamDevice::~UVCCamDevice()
{
	free(fHeaderDescriptor);
}


void
UVCCamDevice::_ParseVideoStreaming(const usbvc_class_descriptor* _descriptor,
	size_t len)
{
	switch (_descriptor->descriptorSubtype) {
		case USB_VIDEO_VS_INPUT_HEADER:
		{
			const usb_video_class_specific_vs_interface_input_header_descriptor* descriptor
				= (const usb_video_class_specific_vs_interface_input_header_descriptor*)_descriptor;
			printf("VS_INPUT_HEADER:\t#fmts=%d,ept=0x%x (%s)\n", descriptor->num_formats,
				descriptor->_endpoint_address.endpoint_number,
				descriptor->_endpoint_address.direction ? "IN" : "OUT");
			if (descriptor->_info.dynamic_format_change_support)
				printf("\tDynamic Format Change supported\n");
			printf("\toutput terminal id=%d\n", descriptor->terminal_link);
			printf("\tstill capture method=%d\n", descriptor->still_capture_method);
			if (descriptor->trigger_support) {
				printf("\ttrigger button fixed to still capture=%s\n",
					descriptor->trigger_usage ? "no" : "yes");
			}
			const struct usb_video_class_specific_vs_interface_input_header_descriptor::ma_controls*
				controls = descriptor->_ma_controls;
			for (uint8 i = 0; i < descriptor->num_formats; i++,
				controls =
				(const struct usb_video_class_specific_vs_interface_input_header_descriptor
					::ma_controls*)((const char*)controls + descriptor->control_size)) {
				printf("\tfmt%d: %s %s %s %s - %s %s\n", i,
					(controls->key_frame_rate) ? "wKeyFrameRate" : "",
					(controls->p_frame_rate) ? "wPFrameRate" : "",
					(controls->comp_quality) ? "wCompQuality" : "",
					(controls->comp_window_size) ? "wCompWindowSize" : "",
					(controls->generate_key_frame) ? "<Generate Key Frame>" : "",
					(controls->update_frame_segment) ? "<Update Frame Segment>" : "");
			}
			break;
		}
		case USB_VIDEO_VS_FORMAT_UNCOMPRESSED:
		{
			const usbvc_format_descriptor* descriptor
				= (const usbvc_format_descriptor*)_descriptor;
			fUncompressedFormatIndex = descriptor->formatIndex;
			printf("VS_FORMAT_UNCOMPRESSED:\tbFormatIdx=%d,#frmdesc=%d,guid=",
				descriptor->formatIndex, descriptor->numFrameDescriptors);
			print_guid(descriptor->uncompressed.format);
			printf("\n\t#bpp=%d,optfrmidx=%d,aspRX=%d,aspRY=%d\n",
				descriptor->uncompressed.bytesPerPixel,
				descriptor->uncompressed.defaultFrameIndex,
				descriptor->uncompressed.aspectRatioX,
				descriptor->uncompressed.aspectRatioY);
			printf("\tbmInterlaceFlags:\n");
			if (descriptor->uncompressed.interlaceFlags & 1)
				printf("\tInterlaced stream or variable\n");
			printf("\t%d fields per frame\n",
				(descriptor->uncompressed.interlaceFlags & 2) ? 1 : 2);
			if (descriptor->uncompressed.interlaceFlags & 4)
				printf("\tField 1 first\n");
			printf("\tField Pattern: ");
			switch ((descriptor->uncompressed.interlaceFlags & 0x30) >> 4) {
				case 0: printf("Field 1 only\n"); break;
				case 1: printf("Field 2 only\n"); break;
				case 2: printf("Regular pattern of fields 1 and 2\n"); break;
				case 3: printf("Random pattern of fields 1 and 2\n"); break;
			}
			if (descriptor->uncompressed.copyProtect)
				printf("\tRestrict duplication\n");
			break;
		}
		case USB_VIDEO_VS_FRAME_MJPEG:
		case USB_VIDEO_VS_FRAME_UNCOMPRESSED:
		{
			const usb_video_frame_descriptor* descriptor
				= (const usb_video_frame_descriptor*)_descriptor;
			if (_descriptor->descriptorSubtype == USB_VIDEO_VS_FRAME_UNCOMPRESSED) {
				printf("VS_FRAME_UNCOMPRESSED:");
				fUncompressedFrames.AddItem(
					new usb_video_frame_descriptor(*descriptor));
			} else {
				printf("VS_FRAME_MJPEG:");
				fMJPEGFrames.AddItem(new usb_video_frame_descriptor(*descriptor));
			}
			printf("\tbFrameIdx=%d,stillsupported=%s,"
				"fixedframerate=%s\n", descriptor->frame_index,
				(descriptor->capabilities & 1) ? "yes" : "no",
				(descriptor->capabilities & 2) ? "yes" : "no");
			printf("\twidth=%u,height=%u,min/max bitrate=%" B_PRIu32 "/%" B_PRIu32 ", maxbuf=%" B_PRIu32 "\n",
				descriptor->width, descriptor->height,
				descriptor->min_bit_rate, descriptor->max_bit_rate,
				descriptor->max_video_frame_buffer_size);
			printf("\tdefault frame interval: %" B_PRIu32 ", #intervals(0=cont): %d\n",
				descriptor->default_frame_interval, descriptor->frame_interval_type);
			if (descriptor->frame_interval_type == 0) {
				printf("min/max frame interval=%" B_PRIu32 "/%" B_PRIu32 ", step=%" B_PRIu32 "\n",
					descriptor->continuous.min_frame_interval,
					descriptor->continuous.max_frame_interval,
					descriptor->continuous.frame_interval_step);
			} else for (uint8 i = 0; i < descriptor->frame_interval_type; i++) {
				printf("\tdiscrete frame interval: %" B_PRIu32 "\n",
					descriptor->discrete_frame_intervals[i]);
			}
			break;
		}
		case USB_VIDEO_VS_COLORFORMAT:
		{
			const usb_video_color_matching_descriptor* descriptor
				= (const usb_video_color_matching_descriptor*)_descriptor;
			printf("VS_COLORFORMAT:\n\tbColorPrimaries: ");
			switch (descriptor->color_primaries) {
				case 0: printf("Unspecified\n"); break;
				case 1: printf("BT.709,sRGB\n"); break;
				case 2: printf("BT.470-2(M)\n"); break;
				case 3: printf("BT.470-2(B,G)\n"); break;
				case 4: printf("SMPTE 170M\n"); break;
				case 5: printf("SMPTE 240M\n"); break;
				default: printf("Invalid (%d)\n", descriptor->color_primaries);
			}
			printf("\tbTransferCharacteristics: ");
			switch (descriptor->transfer_characteristics) {
				case 0: printf("Unspecified\n"); break;
				case 1: printf("BT.709\n"); break;
				case 2: printf("BT.470-2(M)\n"); break;
				case 3: printf("BT.470-2(B,G)\n"); break;
				case 4: printf("SMPTE 170M\n"); break;
				case 5: printf("SMPTE 240M\n"); break;
				case 6: printf("Linear (V=Lc)\n"); break;
				case 7: printf("sRGB\n"); break;
				default: printf("Invalid (%d)\n",
					descriptor->transfer_characteristics);
			}
			printf("\tbMatrixCoefficients: ");
			switch (descriptor->matrix_coefficients) {
				case 0: printf("Unspecified\n"); break;
				case 1: printf("BT.709\n"); break;
				case 2: printf("FCC\n"); break;
				case 3: printf("BT.470-2(B,G)\n"); break;
				case 4: printf("SMPTE 170M (BT.601)\n"); break;
				case 5: printf("SMPTE 240M\n"); break;
				default: printf("Invalid (%d)\n", descriptor->matrix_coefficients);
			}
			break;
		}
		case USB_VIDEO_VS_OUTPUT_HEADER:
		{
			const usb_video_class_specific_vs_interface_output_header_descriptor* descriptor
				= (const usb_video_class_specific_vs_interface_output_header_descriptor*)_descriptor;
			printf("VS_OUTPUT_HEADER:\t#fmts=%d,ept=0x%x (%s)\n",
				descriptor->num_formats, descriptor->_endpoint_address.endpoint_number,
				descriptor->_endpoint_address.direction ? "IN" : "OUT");
			printf("\toutput terminal id=%d\n", descriptor->terminal_link);
			const struct usb_video_class_specific_vs_interface_output_header_descriptor::ma_controls*
				controls = descriptor->_ma_controls;
			for (uint8 i = 0; i < descriptor->num_formats; i++,
				controls
					= (const struct usb_video_class_specific_vs_interface_output_header_descriptor
					::ma_controls*)((const char*)controls + descriptor->control_size)) {
				printf("\tfmt%d: %s %s %s %s\n", i,
					(controls->key_frame_rate) ? "wKeyFrameRate" : "",
					(controls->p_frame_rate) ? "wPFrameRate" : "",
					(controls->comp_quality) ? "wCompQuality" : "",
					(controls->comp_window_size) ? "wCompWindowSize" : "");
			}
			break;
		}
		case USB_VIDEO_VS_STILL_IMAGE_FRAME:
		{
			const usb_video_still_image_frame_descriptor* descriptor
				= (const usb_video_still_image_frame_descriptor*)_descriptor;
			printf("VS_STILL_IMAGE_FRAME:\t#imageSizes=%d,compressions=%d,"
				"ept=0x%x\n", descriptor->num_image_size_patterns,
				descriptor->NumCompressionPatterns(),
				descriptor->endpoint_address);
			for (uint8 i = 0; i < descriptor->num_image_size_patterns; i++) {
				printf("imageSize%d: %dx%d\n", i,
					descriptor->_pattern_size[i].width,
					descriptor->_pattern_size[i].height);
			}
			for (uint8 i = 0; i < descriptor->NumCompressionPatterns(); i++) {
				printf("compression%d: %d\n", i,
					descriptor->CompressionPatterns()[i]);
			}
			break;
		}
		case USB_VIDEO_VS_FORMAT_MJPEG:
		{
			const usbvc_format_descriptor* descriptor
				= (const usbvc_format_descriptor*)_descriptor;
			fMJPEGFormatIndex = descriptor->formatIndex;
			printf("VS_FORMAT_MJPEG:\tbFormatIdx=%d,#frmdesc=%d\n",
				descriptor->formatIndex, descriptor->numFrameDescriptors);
			printf("\t#flgs=%d,optfrmidx=%d,aspRX=%d,aspRY=%d\n",
				descriptor->mjpeg.flags,
				descriptor->mjpeg.defaultFrameIndex,
				descriptor->mjpeg.aspectRatioX,
				descriptor->mjpeg.aspectRatioY);
			printf("\tbmInterlaceFlags:\n");
			if (descriptor->mjpeg.interlaceFlags & 1)
				printf("\tInterlaced stream or variable\n");
			printf("\t%d fields per frame\n",
				(descriptor->mjpeg.interlaceFlags & 2) ? 1 : 2);
			if (descriptor->mjpeg.interlaceFlags & 4)
				printf("\tField 1 first\n");
			printf("\tField Pattern: ");
			switch ((descriptor->mjpeg.interlaceFlags & 0x30) >> 4) {
				case 0: printf("Field 1 only\n"); break;
				case 1: printf("Field 2 only\n"); break;
				case 2: printf("Regular pattern of fields 1 and 2\n"); break;
				case 3: printf("Random pattern of fields 1 and 2\n"); break;
			}
			if (descriptor->mjpeg.copyProtect)
				printf("\tRestrict duplication\n");
			break;
		}
		case USB_VIDEO_VS_FORMAT_MPEG2TS:
			printf("VS_FORMAT_MPEG2TS:\t\n");
			break;
		case USB_VIDEO_VS_FORMAT_DV:
			printf("VS_FORMAT_DV:\t\n");
			break;
		case USB_VIDEO_VS_FORMAT_FRAME_BASED:
			printf("VS_FORMAT_FRAME_BASED:\t\n");
			break;
		case USB_VIDEO_VS_FRAME_FRAME_BASED:
			printf("VS_FRAME_FRAME_BASED:\t\n");
			break;
		case USB_VIDEO_VS_FORMAT_STREAM_BASED:
			printf("VS_FORMAT_STREAM_BASED:\t\n");
			break;
		default:
			printf("INVALID STREAM UNIT TYPE=%d!\n",
				_descriptor->descriptorSubtype);
	}
}


void
UVCCamDevice::_ParseVideoControl(const usbvc_class_descriptor* _descriptor,
	size_t len)
{
	switch (_descriptor->descriptorSubtype) {
		case USB_VIDEO_VC_HEADER:
		{
			if (fHeaderDescriptor != NULL) {
				printf("ERROR: multiple VC_HEADER! Skipping...\n");
				break;
			}
			fHeaderDescriptor = (usbvc_interface_header_descriptor*)malloc(len);
			memcpy(fHeaderDescriptor, _descriptor, len);
			printf("VC_HEADER:\tUVC v%x.%02x, clk %.5f MHz\n",
				fHeaderDescriptor->version >> 8,
				fHeaderDescriptor->version & 0xff,
				fHeaderDescriptor->clockFrequency / 1000000.0);
			for (uint8 i = 0; i < fHeaderDescriptor->numInterfacesNumbers; i++) {
				printf("\tStreaming Interface %d\n",
					fHeaderDescriptor->interfaceNumbers[i]);
			}
			break;
		}
		case USB_VIDEO_VC_INPUT_TERMINAL:
		{
			const usbvc_input_terminal_descriptor* descriptor
				= (const usbvc_input_terminal_descriptor*)_descriptor;
			printf("VC_INPUT_TERMINAL:\tid=%d,type=%04x,associated terminal="
				"%d\n", descriptor->terminalID, descriptor->terminalType,
				descriptor->associatedTerminal);
			printf("\tDesc: %s\n",
				fDevice->DecodeStringDescriptor(descriptor->terminal));
			if (descriptor->terminalType == 0x201) {
				const usb_video_camera_terminal_descriptor* desc
					= (const usb_video_camera_terminal_descriptor*)descriptor;
				printf("\tObjectiveFocalLength Min/Max %d/%d\n",
					desc->objective_focal_length_min,
					desc->objective_focal_length_max);
				printf("\tOcularFocalLength %d\n", desc->ocular_focal_length);
				printf("\tControlSize %d\n", desc->control_size);
			}
			break;
		}
		case USB_VIDEO_VC_OUTPUT_TERMINAL:
		{
			const usb_video_output_terminal_descriptor* descriptor
				= (const usb_video_output_terminal_descriptor*)_descriptor;
			printf("VC_OUTPUT_TERMINAL:\tid=%d,type=%04x,associated terminal="
				"%d, src id=%d\n", descriptor->terminal_id,
				descriptor->terminal_type, descriptor->associated_terminal,
				descriptor->source_id);
			printf("\tDesc: %s\n",
				fDevice->DecodeStringDescriptor(descriptor->terminal));
			break;
		}
		case USB_VIDEO_VC_SELECTOR_UNIT:
		{
			const usb_video_selector_unit_descriptor* descriptor
				= (const usb_video_selector_unit_descriptor*)_descriptor;
			printf("VC_SELECTOR_UNIT:\tid=%d,#pins=%d\n",
				descriptor->unit_id, descriptor->num_input_pins);
			printf("\t");
			for (uint8 i = 0; i < descriptor->num_input_pins; i++)
				printf("%d ", descriptor->source_id[i]);
			printf("\n");
			printf("\tDesc: %s\n",
				fDevice->DecodeStringDescriptor(descriptor->Selector()));
			break;
		}
		case USB_VIDEO_VC_PROCESSING_UNIT:
		{
			const usb_video_processing_unit_descriptor* descriptor
				= (const usb_video_processing_unit_descriptor*)_descriptor;
			fControlRequestIndex = fControlIndex + (descriptor->unit_id << 8);
			printf("VC_PROCESSING_UNIT:\t unit id=%d,src id=%d, digmul=%d\n",
				descriptor->unit_id, descriptor->source_id,
				descriptor->max_multiplier);
			printf("\tbControlSize=%d\n", descriptor->control_size);
			if (descriptor->control_size >= 1) {
				if (descriptor->controls[0] & 1)
					printf("\tBrightness\n");
				if (descriptor->controls[0] & 2)
					printf("\tContrast\n");
				if (descriptor->controls[0] & 4)
					printf("\tHue\n");
				if (descriptor->controls[0] & 8)
					printf("\tSaturation\n");
				if (descriptor->controls[0] & 16)
					printf("\tSharpness\n");
				if (descriptor->controls[0] & 32)
					printf("\tGamma\n");
				if (descriptor->controls[0] & 64)
					printf("\tWhite Balance Temperature\n");
				if (descriptor->controls[0] & 128)
					printf("\tWhite Balance Component\n");
			}
			if (descriptor->control_size >= 2) {
				if (descriptor->controls[1] & 1)
					printf("\tBacklight Compensation\n");
				if (descriptor->controls[1] & 2)
					printf("\tGain\n");
				if (descriptor->controls[1] & 4)
					printf("\tPower Line Frequency\n");
				if (descriptor->controls[1] & 8)
					printf("\t[AUTO] Hue\n");
				if (descriptor->controls[1] & 16)
					printf("\t[AUTO] White Balance Temperature\n");
				if (descriptor->controls[1] & 32)
					printf("\t[AUTO] White Balance Component\n");
				if (descriptor->controls[1] & 64)
					printf("\tDigital Multiplier\n");
				if (descriptor->controls[1] & 128)
					printf("\tDigital Multiplier Limit\n");
			}
			if (descriptor->control_size >= 3) {
				if (descriptor->controls[2] & 1)
					printf("\tAnalog Video Standard\n");
				if (descriptor->controls[2] & 2)
					printf("\tAnalog Video Lock Status\n");
			}
			printf("\tDesc: %s\n",
				fDevice->DecodeStringDescriptor(descriptor->Processing()));
			if (descriptor->VideoStandards()._video_standards.ntsc_525_60)
				printf("\tNTSC  525/60\n");
			if (descriptor->VideoStandards()._video_standards.pal_625_50)
				printf("\tPAL   625/50\n");
			if (descriptor->VideoStandards()._video_standards.secam_625_50)
				printf("\tSECAM 625/50\n");
			if (descriptor->VideoStandards()._video_standards.ntsc_625_50)
				printf("\tNTSC  625/50\n");
			if (descriptor->VideoStandards()._video_standards.pal_525_60)
				printf("\tPAL   525/60\n");
			break;
		}
		case USB_VIDEO_VC_EXTENSION_UNIT:
		{
			const usb_video_extension_unit_descriptor* descriptor
				= (const usb_video_extension_unit_descriptor*)_descriptor;
			printf("VC_EXTENSION_UNIT:\tid=%d, guid=", descriptor->unit_id);
			print_guid(descriptor->guid_extension_code);
			printf("\n\t#ctrls=%d, #pins=%d\n", descriptor->num_controls,
				descriptor->num_input_pins);
			printf("\t");
			for (uint8 i = 0; i < descriptor->num_input_pins; i++)
				printf("%d ", descriptor->source_id[i]);
			printf("\n");
			printf("\tDesc: %s\n",
				fDevice->DecodeStringDescriptor(descriptor->Extension()));
			break;
		}
		default:
			printf("Unknown control %d\n", _descriptor->descriptorSubtype);
	}
}


bool
UVCCamDevice::SupportsIsochronous()
{
	return true;
}


status_t
UVCCamDevice::StartTransfer()
{
	if (_ProbeCommitFormat() != B_OK || _SelectBestAlternate() != B_OK)
		return B_ERROR;
	return CamDevice::StartTransfer();
}


status_t
UVCCamDevice::StopTransfer()
{
	_SelectIdleAlternate();
	return CamDevice::StopTransfer();
}


status_t
UVCCamDevice::SuggestVideoFrame(uint32& width, uint32& height)
{
	printf("UVCCamDevice::SuggestVideoFrame(%" B_PRIu32 ", %" B_PRIu32 ")\n", width, height);
	// As in AcceptVideoFrame(), the suggestion should probably just be the
	// first advertised uncompressed format, but current applications prefer
	// 320x240, so this is tried first here as a suggestion.
	width = 320;
	height = 240;
	if (!AcceptVideoFrame(width, height)) {
		const usb_video_frame_descriptor* descriptor
			= (const usb_video_frame_descriptor*)fUncompressedFrames.FirstItem();
		width  = (*descriptor).width;
		height = (*descriptor).height;
	}
	return B_OK;
}


status_t
UVCCamDevice::AcceptVideoFrame(uint32& width, uint32& height)
{
	printf("UVCCamDevice::AcceptVideoFrame(%" B_PRIu32 ", %" B_PRIu32 ")\n", width, height);
	if (width <= 0 || height <= 0) {
		// Uncomment below when applications support dimensions other than 320x240
		// This code selects the first listed available uncompressed frame format
		/*
		const usbvc_frame_descriptor* descriptor
			= (const usbvc_frame_descriptor*)fUncompressedFrames.FirstItem();
		width = (*descriptor).width;
		height = (*descriptor).height;
		SetVideoFrame(BRect(0, 0, width - 1, height - 1));
		return B_OK;
		*/

		width  = 320;
		height = 240;
	}

	for (int i = 0; i<fUncompressedFrames.CountItems(); i++) {
		const usb_video_frame_descriptor* descriptor
			= (const usb_video_frame_descriptor*)fUncompressedFrames.ItemAt(i);
		if ((*descriptor).width == width && (*descriptor).height == height) {
			fUncompressedFrameIndex = i;
			SetVideoFrame(BRect(0, 0, width - 1, height - 1));
			return B_OK;
		}
	}

	fprintf(stderr, "UVCCamDevice::AcceptVideoFrame() Invalid frame dimensions"
		"\n");
	return B_ERROR;
}


status_t
UVCCamDevice::_ProbeCommitFormat()
{
	printf("UVCCamDevice::_ProbeCommitFormat()\n");
	printf("UVCCamDevice::fStreamingIndex = %" B_PRIu32 "\n", fStreamingIndex);

	/*
	char error;
	printf("BEFORE ERROR CODE CHECK.\n");
	fDevice->ControlTransfer(
			USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_IN, GET_CUR,
			VS_STREAM_ERROR_CODE_CONTROL << 8, fStreamingIndex, 1, &error);
	printf("Error code = Ox%x\n", error);
	*/

	usb_video_probe_and_commit_controls request;
	memset(&request, 0, sizeof(request));
	request._hint.frame_interval = 1;
	request.frame_interval = 333333;
	request.format_index = fUncompressedFormatIndex;
	request.frame_index = fUncompressedFrameIndex;
	size_t length = fHeaderDescriptor->version > 0x100 ? 34 : 26;
	size_t actualLength = fDevice->ControlTransfer(
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT, USB_VIDEO_RC_SET_CUR,
		USB_VIDEO_VS_PROBE_CONTROL << 8, fStreamingIndex, length, &request);
	if (actualLength != length) {
		fprintf(stderr, "UVCCamDevice::_ProbeFormat() SET_CUR ProbeControl1"
			" failed %ld\n", actualLength);
		return B_ERROR;
	}

	/*
	usbvc_probecommit response;
	actualLength = fDevice->ControlTransfer(
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_IN, GET_MAX,
		VS_PROBE_CONTROL << 8, fStreamingIndex, sizeof(response), &response);
	if (actualLength != sizeof(response)) {
		fprintf(stderr, "UVCCamDevice::_ProbeFormat() GetMax ProbeControl"
			" failed\n");
		return B_ERROR;
	}

	printf("usbvc_probecommit response.compQuality %d\n", response.compQuality);
	request.compQuality = response.compQuality;
	*/


	usb_video_probe_and_commit_controls response;
	memset(&response, 0, sizeof(response));
	actualLength = fDevice->ControlTransfer(
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_IN, USB_VIDEO_RC_GET_CUR,
		USB_VIDEO_VS_PROBE_CONTROL << 8, fStreamingIndex, length, &response);

	/*
	actualLength = fDevice->ControlTransfer(
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT, SET_CUR,
		VS_PROBE_CONTROL << 8, fStreamingIndex, length, &request);
	if (actualLength != length) {
		fprintf(stderr, "UVCCamDevice::_ProbeFormat() SetCur ProbeControl2"
			" failed\n");
		return B_ERROR;
	}
	*/

	actualLength = fDevice->ControlTransfer(
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT, USB_VIDEO_RC_SET_CUR,
		USB_VIDEO_VS_COMMIT_CONTROL << 8, fStreamingIndex, length, &request);
	if (actualLength != length) {
		fprintf(stderr, "UVCCamDevice::_ProbeFormat() SetCur CommitControl"
			" failed\n");
		return B_ERROR;
	}


	fMaxVideoFrameSize = response.max_video_frame_size;
	fMaxPayloadTransferSize = response.max_payload_transfer_size;
	printf("usbvc_probecommit setup done maxVideoFrameSize:%" B_PRIu32 ""
		" maxPayloadTransferSize:%" B_PRIu32 "\n", fMaxVideoFrameSize,
		fMaxPayloadTransferSize);

	printf("UVCCamDevice::_ProbeCommitFormat()\n --> SUCCESSFUL\n");
	return B_OK;
}


status_t
UVCCamDevice::_SelectBestAlternate()
{
	printf("UVCCamDevice::_SelectBestAlternate()\n");
	const BUSBConfiguration* config = fDevice->ActiveConfiguration();
	const BUSBInterface* streaming = config->InterfaceAt(fStreamingIndex);
	if (streaming == NULL)
		return B_BAD_INDEX;

	uint32 bestBandwidth = 0;
	uint32 alternateIndex = 0;
	uint32 endpointIndex = 0;

	for (uint32 i = 0; i < streaming->CountAlternates(); i++) {
		const BUSBInterface* alternate = streaming->AlternateAt(i);
		for (uint32 j = 0; j < alternate->CountEndpoints(); j++) {
			const BUSBEndpoint* endpoint = alternate->EndpointAt(j);
			if (!endpoint->IsIsochronous() || !endpoint->IsInput())
				continue;
			if (fMaxPayloadTransferSize > endpoint->MaxPacketSize())
				continue;
			if (bestBandwidth != 0
				&& bestBandwidth < endpoint->MaxPacketSize())
				continue;
			bestBandwidth = endpoint->MaxPacketSize();
			endpointIndex = j;
			alternateIndex = i;
		}
	}

	if (bestBandwidth == 0) {
		fprintf(stderr, "UVCCamDevice::_SelectBestAlternate()"
			" couldn't find a valid alternate\n");
		return B_ERROR;
	}

	printf("UVCCamDevice::_SelectBestAlternate() %" B_PRIu32 "\n", bestBandwidth);
	if (((BUSBInterface*)streaming)->SetAlternate(alternateIndex) != B_OK) {
		fprintf(stderr, "UVCCamDevice::_SelectBestAlternate()"
			" selecting alternate failed\n");
		return B_ERROR;
	}

	fIsoIn = streaming->EndpointAt(endpointIndex);

	return B_OK;
}


status_t
UVCCamDevice::_SelectIdleAlternate()
{
	printf("UVCCamDevice::_SelectIdleAlternate()\n");
	const BUSBConfiguration* config = fDevice->ActiveConfiguration();
	const BUSBInterface* streaming = config->InterfaceAt(fStreamingIndex);
	if (streaming == NULL)
		return B_BAD_INDEX;
	if (((BUSBInterface*)streaming)->SetAlternate(0) != B_OK) {
		fprintf(stderr, "UVCCamDevice::_SelectIdleAlternate()"
			" selecting alternate failed\n");
		return B_ERROR;
	}

	fIsoIn = NULL;

	return B_OK;
}


void
UVCCamDevice::_AddProcessingParameter(BParameterGroup* group,
	int32 index, const usb_video_processing_unit_descriptor* descriptor)
{
	BParameterGroup* subgroup;
	uint16 wValue = 0; // Control Selector
	float minValue = 0.0;
	float maxValue = 100.0;
	if (descriptor->control_size >= 1) {
		if (descriptor->controls[0] & 1) {
			// debug_printf("\tBRIGHTNESS\n");
			fBrightness = _AddParameter(group, &subgroup, index,
				USB_VIDEO_PU_BRIGHTNESS_CONTROL, "Brightness");
		}
		if (descriptor->controls[0] & 2) {
			// debug_printf("\tCONSTRAST\n");
			fContrast = _AddParameter(group, &subgroup, index + 1,
				USB_VIDEO_PU_CONTRAST_CONTROL, "Contrast");
		}
		if (descriptor->controls[0] & 4) {
			// debug_printf("\tHUE\n");
			fHue = _AddParameter(group, &subgroup, index + 2,
				USB_VIDEO_PU_HUE_CONTROL, "Hue");
			if (descriptor->control_size >= 2) {
				if (descriptor->controls[1] & 8) {
					fHueAuto = _AddAutoParameter(subgroup, index + 3,
						USB_VIDEO_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL);
				}
			}
		}
		if (descriptor->controls[0] & 8) {
			// debug_printf("\tSATURATION\n");
			fSaturation = _AddParameter(group, &subgroup, index + 4,
				USB_VIDEO_PU_SATURATION_CONTROL, "Saturation");
		}
		if (descriptor->controls[0] & 16) {
			// debug_printf("\tSHARPNESS\n");
			fSharpness = _AddParameter(group, &subgroup, index + 5,
				USB_VIDEO_PU_SHARPNESS_CONTROL, "Sharpness");
		}
		if (descriptor->controls[0] & 32) {
			// debug_printf("\tGamma\n");
			fGamma = _AddParameter(group, &subgroup, index + 6,
				USB_VIDEO_PU_GAMMA_CONTROL, "Gamma");
		}
		if (descriptor->controls[0] & 64) {
			// debug_printf("\tWHITE BALANCE TEMPERATURE\n");
			fWBTemp = _AddParameter(group, &subgroup, index + 7,
				USB_VIDEO_PU_WHITE_BALANCE_TEMPERATURE_CONTROL, "WB Temperature");
			if (descriptor->control_size >= 2) {
				if (descriptor->controls[1] & 16) {
					fWBTempAuto = _AddAutoParameter(subgroup, index + 8,
						USB_VIDEO_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL);
				}
			}
		}
		if (descriptor->controls[0] & 128) {
			// debug_printf("\tWhite Balance Component\n");
			fWBComponent = _AddParameter(group, &subgroup, index + 9,
				USB_VIDEO_PU_WHITE_BALANCE_COMPONENT_CONTROL, "WB Component");
			if (descriptor->control_size >= 2) {
				if (descriptor->controls[1] & 32) {
					fWBTempAuto = _AddAutoParameter(subgroup, index + 10,
						USB_VIDEO_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL);
				}
			}
		}
	}
	if (descriptor->control_size >= 2) {
		if (descriptor->controls[1] & 1) {
			// debug_printf("\tBACKLIGHT COMPENSATION\n");
			int16 data;
			wValue = USB_VIDEO_PU_BACKLIGHT_COMPENSATION_CONTROL << 8;
			fDevice->ControlTransfer(USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_IN,
				USB_VIDEO_RC_GET_MAX, wValue, fControlRequestIndex, sizeof(data), &data);
			maxValue = (float)data;
			fDevice->ControlTransfer(USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_IN,
				USB_VIDEO_RC_GET_MIN, wValue, fControlRequestIndex, sizeof(data), &data);
			minValue = (float)data;
			fDevice->ControlTransfer(USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_IN,
				USB_VIDEO_RC_GET_CUR, wValue, fControlRequestIndex, sizeof(data), &data);
			fBacklightCompensation = (float)data;
			subgroup = group->MakeGroup("Backlight Compensation");
			if (maxValue - minValue == 1) { // Binary Switch
				fBinaryBacklightCompensation = true;
				subgroup->MakeDiscreteParameter(index + 11,
					B_MEDIA_RAW_VIDEO, "Backlight Compensation",
					B_ENABLE);
			} else { // Range of values
				fBinaryBacklightCompensation = false;
				subgroup->MakeContinuousParameter(index + 11,
				B_MEDIA_RAW_VIDEO, "Backlight Compensation",
				B_GAIN, "", minValue, maxValue, 1.0 / (maxValue - minValue));
			}
		}
		if (descriptor->controls[1] & 2) {
			// debug_printf("\tGAIN\n");
			fGain = _AddParameter(group, &subgroup, index + 12, USB_VIDEO_PU_GAIN_CONTROL,
				"Gain");
		}
		if (descriptor->controls[1] & 4) {
			// debug_printf("\tPOWER LINE FREQUENCY\n");
			wValue = USB_VIDEO_PU_POWER_LINE_FREQUENCY_CONTROL << 8;
			int8 data;
			if (fDevice->ControlTransfer(USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_IN,
					USB_VIDEO_RC_GET_CUR, wValue, fControlRequestIndex, sizeof(data), &data)
				== sizeof(data)) {
				fPowerlineFrequency = data;
			}
			subgroup = group->MakeGroup("Power Line Frequency");
			subgroup->MakeContinuousParameter(index + 13,
				B_MEDIA_RAW_VIDEO, "Frequency", B_GAIN, "", 0, 60.0, 1.0 / 60.0);
		}
		// TODO Determine whether controls apply to these
		/*
		if (descriptor->controls[1] & 64)
			debug_printf("\tDigital Multiplier\n");
		if (descriptor->controls[1] & 128)
			debug_printf("\tDigital Multiplier Limit\n");
		*/
	}
	// TODO Determine whether controls apply to these
	/*
	if (descriptor->controlSize >= 3) {
		if (descriptor->controls[2] & 1)
			debug_printf("\tAnalog Video Standard\n");
		if (descriptor->controls[2] & 2)
			debug_printf("\tAnalog Video Lock Status\n");
	}
	*/

}



float
UVCCamDevice::_AddParameter(BParameterGroup* group,
	BParameterGroup** subgroup, int32 index, uint16 wValue, const char* name)
{
	float minValue = 0.0;
	float maxValue = 100.0;
	float currValue = 0.0;
	int16 data;

	wValue <<= 8;

	if (fDevice->ControlTransfer(USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_IN,
		USB_VIDEO_RC_GET_MAX, wValue, fControlRequestIndex, sizeof(data), &data)
		== sizeof(data)) {
		maxValue = (float)data;
	}
	if (fDevice->ControlTransfer(USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_IN,
		USB_VIDEO_RC_GET_MIN, wValue, fControlRequestIndex, sizeof(data), &data)
		== sizeof(data)) {
		minValue = (float)data;
	}
	if (fDevice->ControlTransfer(USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_IN,
		USB_VIDEO_RC_GET_CUR, wValue, fControlRequestIndex, sizeof(data), &data)
		== sizeof(data)) {
		currValue = (float)data;
	}

	*subgroup = group->MakeGroup(name);
	(*subgroup)->MakeContinuousParameter(index,
		B_MEDIA_RAW_VIDEO, name, B_GAIN, "", minValue, maxValue,
		1.0 / (maxValue - minValue));
	return currValue;
}


uint8
UVCCamDevice::_AddAutoParameter(BParameterGroup* subgroup, int32 index,
	uint16 wValue)
{
	uint8 data;
	wValue <<= 8;

	fDevice->ControlTransfer(USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_IN,
		USB_VIDEO_RC_GET_CUR, wValue, fControlRequestIndex, 1, &data);
	subgroup->MakeDiscreteParameter(index, B_MEDIA_RAW_VIDEO, "Auto",
		B_ENABLE);

	return data;
}


void
UVCCamDevice::AddParameters(BParameterGroup* group, int32& index)
{
	printf("UVCCamDevice::AddParameters()\n");
	fFirstParameterID = index;
//	debug_printf("fIndex = %d\n",fIndex);
	CamDevice::AddParameters(group, index);

	const BUSBConfiguration* config;
	const BUSBInterface* interface;
	uint8 buffer[1024];

	usb_descriptor* generic = (usb_descriptor*)buffer;

	for (uint32 i = 0; i < fDevice->CountConfigurations(); i++) {
		config = fDevice->ConfigurationAt(i);
		if (config == NULL)
			continue;
		fDevice->SetConfiguration(config);
		for (uint32 j = 0; j < config->CountInterfaces(); j++) {
			interface = config->InterfaceAt(j);
			if (interface == NULL)
				continue;
			if (interface->Class() != USB_VIDEO_DEVICE_CLASS || interface->Subclass()
				!= USB_VIDEO_INTERFACE_VIDEOCONTROL_SUBCLASS)
				continue;
			for (uint32 k = 0; interface->OtherDescriptorAt(k, generic,
				sizeof(buffer)) == B_OK; k++) {
				if (generic->generic.descriptor_type != (USB_REQTYPE_CLASS
					| USB_DESCRIPTOR_INTERFACE))
					continue;

				if (((const usbvc_class_descriptor*)generic)->descriptorSubtype
					== USB_VIDEO_VC_PROCESSING_UNIT) {
					_AddProcessingParameter(group, index,
						(const usb_video_processing_unit_descriptor*)generic);
				}
			}
		}
	}
}


status_t
UVCCamDevice::GetParameterValue(int32 id, bigtime_t* last_change, void* value,
	size_t* size)
{
	printf("UVCCAmDevice::GetParameterValue(%" B_PRId32 ")\n", id - fFirstParameterID);
	float* currValue;
	int* currValueInt;
	int16 data;
	uint16 wValue = 0;
	switch (id - fFirstParameterID) {
		case 0:
			// debug_printf("\tBrightness:\n");
			// debug_printf("\tValue = %f\n",fBrightness);
			*size = sizeof(float);
			currValue = (float*)value;
			*currValue = fBrightness;
			*last_change = fLastParameterChanges;
			return B_OK;
		case 1:
			// debug_printf("\tContrast:\n");
			// debug_printf("\tValue = %f\n",fContrast);
			*size = sizeof(float);
			currValue = (float*)value;
			*currValue = fContrast;
			*last_change = fLastParameterChanges;
			return B_OK;
		case 2:
			// debug_printf("\tHue:\n");
			// debug_printf("\tValue = %f\n",fHue);
			*size = sizeof(float);
			currValue = (float*)value;
			*currValue = fHue;
			*last_change = fLastParameterChanges;
			return B_OK;
		case 4:
			// debug_printf("\tSaturation:\n");
			// debug_printf("\tValue = %f\n",fSaturation);
			*size = sizeof(float);
			currValue = (float*)value;
			*currValue = fSaturation;
			*last_change = fLastParameterChanges;
			return B_OK;
		case 5:
			// debug_printf("\tSharpness:\n");
			// debug_printf("\tValue = %f\n",fSharpness);
			*size = sizeof(float);
			currValue = (float*)value;
			*currValue = fSharpness;
			*last_change = fLastParameterChanges;
			return B_OK;
		case 7:
			// debug_printf("\tWB Temperature:\n");
			*size = sizeof(float);
			currValue = (float*)value;
			wValue = USB_VIDEO_PU_WHITE_BALANCE_TEMPERATURE_CONTROL << 8;
			if (fDevice->ControlTransfer(USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_IN,
				USB_VIDEO_RC_GET_CUR, wValue, fControlRequestIndex, sizeof(data), &data)
				== sizeof(data)) {
				fWBTemp = (float)data;
			}
			// debug_printf("\tValue = %f\n",fWBTemp);
			*currValue = fWBTemp;
			*last_change = fLastParameterChanges;
			return B_OK;
		case 8:
			// debug_printf("\tWB Temperature Auto:\n");
			// debug_printf("\tValue = %d\n",fWBTempAuto);
			*size = sizeof(int);
			currValueInt = ((int*)value);
			*currValueInt = fWBTempAuto;
			*last_change = fLastParameterChanges;
			return B_OK;
		case 11:
			if (!fBinaryBacklightCompensation) {
				// debug_printf("\tBacklight Compensation:\n");
				// debug_printf("\tValue = %f\n",fBacklightCompensation);
				*size = sizeof(float);
				currValue = (float*)value;
				*currValue = fBacklightCompensation;
				*last_change = fLastParameterChanges;
			} else {
				// debug_printf("\tBacklight Compensation:\n");
				// debug_printf("\tValue = %d\n",fBacklightCompensationBinary);
				currValueInt = (int*)value;
				*currValueInt = fBacklightCompensationBinary;
				*last_change = fLastParameterChanges;
			}
			return B_OK;
		case 12:
			// debug_printf("\tGain:\n");
			// debug_printf("\tValue = %f\n",fGain);
			*size = sizeof(float);
			currValue = (float*)value;
			*currValue = fGain;
			*last_change = fLastParameterChanges;
			return B_OK;
		case 13:
			// debug_printf("\tPowerline Frequency:\n");
			// debug_printf("\tValue = %d\n",fPowerlineFrequency);
			*size = sizeof(float);
			currValue = (float*)value;
			switch (fPowerlineFrequency) {
				case 0:
					*currValue = 0.0;
					break;
				case 1:
					*currValue = 50.0;
					break;
				case 2:
					*currValue = 60.0;
					break;
			}
			*last_change = fLastParameterChanges;
			return B_OK;

	}
	return B_BAD_VALUE;
}


status_t
UVCCamDevice::SetParameterValue(int32 id, bigtime_t when, const void* value,
	size_t size)
{
	printf("UVCCamDevice::SetParameterValue(%" B_PRId32 ")\n", id - fFirstParameterID);
	switch (id - fFirstParameterID) {
		case 0:
			// debug_printf("\tBrightness:\n");
			if (!value || (size != sizeof(float)))
				return B_BAD_VALUE;
			fBrightness = *((float*)value);
			fLastParameterChanges = when;
			return _SetParameterValue(USB_VIDEO_PU_BRIGHTNESS_CONTROL, (int16)fBrightness);
		case 1:
			// debug_printf("\tContrast:\n");
			if (!value || (size != sizeof(float)))
				return B_BAD_VALUE;
			fContrast = *((float*)value);
			fLastParameterChanges = when;
			return _SetParameterValue(USB_VIDEO_PU_CONTRAST_CONTROL, (int16)fContrast);
		case 2:
			// debug_printf("\tHue:\n");
			if (!value || (size != sizeof(float)))
				return B_BAD_VALUE;
			fHue = *((float*)value);
			fLastParameterChanges = when;
			return _SetParameterValue(USB_VIDEO_PU_HUE_CONTROL, (int16)fHue);
		case 4:
			// debug_printf("\tSaturation:\n");
			if (!value || (size != sizeof(float)))
				return B_BAD_VALUE;
			fSaturation = *((float*)value);
			fLastParameterChanges = when;
			return _SetParameterValue(USB_VIDEO_PU_SATURATION_CONTROL, (int16)fSaturation);
		case 5:
			// debug_printf("\tSharpness:\n");
			if (!value || (size != sizeof(float)))
				return B_BAD_VALUE;
			fSharpness = *((float*)value);
			fLastParameterChanges = when;
			return _SetParameterValue(USB_VIDEO_PU_SHARPNESS_CONTROL, (int16)fSharpness);
		case 7:
			if (fWBTempAuto)
				return B_OK;
			// debug_printf("\tWB Temperature:\n");
			if (!value || (size != sizeof(float)))
				return B_BAD_VALUE;
			fWBTemp = *((float*)value);
			fLastParameterChanges = when;
			return _SetParameterValue(USB_VIDEO_PU_WHITE_BALANCE_TEMPERATURE_CONTROL,
				(int16)fWBTemp);
		case 8:
			// debug_printf("\tWB Temperature Auto:\n");
			if (!value || (size != sizeof(int)))
				return B_BAD_VALUE;
			fWBTempAuto = *((int*)value);
			fLastParameterChanges = when;
			return _SetParameterValue(
				USB_VIDEO_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL, (int8)fWBTempAuto);
		case 11:
			if (!fBinaryBacklightCompensation) {
				// debug_printf("\tBacklight Compensation:\n");
				if (!value || (size != sizeof(float)))
					return B_BAD_VALUE;
				fBacklightCompensation = *((float*)value);
			} else {
				// debug_printf("\tBacklight Compensation:\n");
				if (!value || (size != sizeof(int)))
					return B_BAD_VALUE;
				fBacklightCompensationBinary = *((int*)value);
			}
			fLastParameterChanges = when;
			return _SetParameterValue(USB_VIDEO_PU_BACKLIGHT_COMPENSATION_CONTROL,
				(int16)fBacklightCompensationBinary);
		case 12:
			// debug_printf("\tGain:\n");
			if (!value || (size != sizeof(float)))
				return B_BAD_VALUE;
			fGain = *((float*)value);
			fLastParameterChanges = when;
			return _SetParameterValue(USB_VIDEO_PU_GAIN_CONTROL, (int16)fGain);
		case 13:
			// debug_printf("\tPowerline Frequency:\n");
			// debug_printf("\tValue = %f\n",*((float*)value));
			if (!value || (size != sizeof(float)))
				return B_BAD_VALUE;
			float inValue = *((float*)value);
			fPowerlineFrequency = 0;
			if (inValue > 45.0 && inValue < 55.0) {
				fPowerlineFrequency = 1;
			}
			if (inValue >= 55.0) {
				fPowerlineFrequency = 2;
			}
			fLastParameterChanges = when;
			return _SetParameterValue(USB_VIDEO_PU_POWER_LINE_FREQUENCY_CONTROL,
				(int8)fPowerlineFrequency);

	}
	return B_BAD_VALUE;
}


status_t
UVCCamDevice::_SetParameterValue(uint16 wValue, int16 setValue)
{
	return (fDevice->ControlTransfer(USB_REQTYPE_CLASS
		| USB_REQTYPE_INTERFACE_OUT, USB_VIDEO_RC_SET_CUR, wValue << 8, fControlRequestIndex,
		sizeof(setValue), &setValue)) == sizeof(setValue);
}


status_t
UVCCamDevice::_SetParameterValue(uint16 wValue, int8 setValue)
{
	return (fDevice->ControlTransfer(USB_REQTYPE_CLASS
		| USB_REQTYPE_INTERFACE_OUT, USB_VIDEO_RC_SET_CUR, wValue << 8, fControlRequestIndex,
		sizeof(setValue), &setValue)) == sizeof(setValue);
}


status_t
UVCCamDevice::FillFrameBuffer(BBuffer* buffer, bigtime_t* stamp)
{
	memset(buffer->Data(), 0, buffer->SizeAvailable());
	status_t err = fDeframer->WaitFrame(2000000);
	if (err < B_OK) {
		fprintf(stderr, "WaitFrame: %" B_PRIx32 "\n", err);
		return err;
	}

	CamFrame* f;
	err = fDeframer->GetFrame(&f, stamp);
	if (err < B_OK) {
		fprintf(stderr, "GetFrame: %" B_PRIx32 "\n", err);
		return err;
	}

	long int w = (long)(VideoFrame().right - VideoFrame().left + 1);
	long int h = (long)(VideoFrame().bottom - VideoFrame().top + 1);

	if (buffer->SizeAvailable() >= (size_t)w * h * 4) {
		// TODO: The Video Producer only outputs B_RGB32.  This is OK for most
		// applications.  This could be leveraged if applications can
		// consume B_YUV422.
		_DecodeColor((unsigned char*)buffer->Data(),
			(unsigned char*)f->Buffer(), w, h);
	}
	delete f;
	return B_OK;
}


void
UVCCamDevice::_DecodeColor(unsigned char* dst, unsigned char* src,
	int32 width, int32 height)
{
	long int i;
	unsigned char* rawpt, * scanpt;
	long int size;

	rawpt = src;
	scanpt = dst;
	size = width*height;

	for ( i = 0; i < size; i++ ) {
	if ( (i/width) % 2 == 0 ) {
		if ( (i % 2) == 0 ) {
		/* B */
		if ( (i > width) && ((i % width) > 0) ) {
			*scanpt++ = (*(rawpt-width-1)+*(rawpt-width+1)
				+ *(rawpt+width-1)+*(rawpt+width+1))/4;	/* R */
			*scanpt++ = (*(rawpt-1)+*(rawpt+1)
				+ *(rawpt+width)+*(rawpt-width))/4;	/* G */
			*scanpt++ = *rawpt;					/* B */
		} else {
			/* first line or left column */
			*scanpt++ = *(rawpt+width+1);		/* R */
			*scanpt++ = (*(rawpt+1)+*(rawpt+width))/2;	/* G */
			*scanpt++ = *rawpt;				/* B */
		}
		} else {
		/* (B)G */
		if ( (i > width) && ((i % width) < (width-1)) ) {
			*scanpt++ = (*(rawpt+width)+*(rawpt-width))/2;	/* R */
			*scanpt++ = *rawpt;					/* G */
			*scanpt++ = (*(rawpt-1)+*(rawpt+1))/2;		/* B */
		} else {
			/* first line or right column */
			*scanpt++ = *(rawpt+width);	/* R */
			*scanpt++ = *rawpt;		/* G */
			*scanpt++ = *(rawpt-1);	/* B */
		}
		}
	} else {
		if ( (i % 2) == 0 ) {
		/* G(R) */
		if ( (i < (width*(height-1))) && ((i % width) > 0) ) {
			*scanpt++ = (*(rawpt-1)+*(rawpt+1))/2;		/* R */
			*scanpt++ = *rawpt;					/* G */
			*scanpt++ = (*(rawpt+width)+*(rawpt-width))/2;	/* B */
		} else {
			/* bottom line or left column */
			*scanpt++ = *(rawpt+1);		/* R */
			*scanpt++ = *rawpt;			/* G */
			*scanpt++ = *(rawpt-width);		/* B */
		}
		} else {
		/* R */
		if ( i < (width*(height-1)) && ((i % width) < (width-1)) ) {
			*scanpt++ = *rawpt;					/* R */
			*scanpt++ = (*(rawpt-1)+*(rawpt+1)
				+ *(rawpt-width)+*(rawpt+width))/4;	/* G */
			*scanpt++ = (*(rawpt-width-1)+*(rawpt-width+1)
				+ *(rawpt+width-1)+*(rawpt+width+1))/4;	/* B */
		} else {
			/* bottom line or right column */
			*scanpt++ = *rawpt;				/* R */
			*scanpt++ = (*(rawpt-1)+*(rawpt-width))/2;	/* G */
			*scanpt++ = *(rawpt-width-1);		/* B */
		}
		}
	}
	rawpt++;
	}
}




UVCCamDeviceAddon::UVCCamDeviceAddon(WebCamMediaAddOn* webcam)
	: CamDeviceAddon(webcam)
{
	printf("UVCCamDeviceAddon::UVCCamDeviceAddon(WebCamMediaAddOn* webcam)\n");
	SetSupportedDevices(kSupportedDevices);
}


UVCCamDeviceAddon::~UVCCamDeviceAddon()
{
}


const char *
UVCCamDeviceAddon::BrandName()
{
	printf("UVCCamDeviceAddon::BrandName()\n");
	return "USB Video Class";
}


UVCCamDevice *
UVCCamDeviceAddon::Instantiate(CamRoster& roster, BUSBDevice* from)
{
	printf("UVCCamDeviceAddon::Instantiate()\n");
	return new UVCCamDevice(*this, from);
}


extern "C" status_t
B_WEBCAM_MKINTFUNC(uvccam)
(WebCamMediaAddOn* webcam, CamDeviceAddon **addon)
{
	*addon = new UVCCamDeviceAddon(webcam);
	return B_OK;
}
