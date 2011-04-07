/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2009, Ithamar Adema, <ithamar.adema@team-embedded.nl>.
 * Distributed under the terms of the MIT License.
 */


#include "UVCCamDevice.h"

#include <stdio.h>


usb_webcam_support_descriptor kSupportedDevices[] = {
	// ofcourse we support a generic UVC device...
	{{ CC_VIDEO, SC_VIDEOCONTROL, 0, 0, 0 }, "USB", "Video Class", "??" },
	// ...whilst the following IDs were 'stolen' from a recent Linux driver:
	{{ 0, 0, 0, 0x045e, 0x00f8, }, "Microsoft",     "Lifecam NX-6000",                 "??" },
	{{ 0, 0, 0, 0x045e, 0x0723, }, "Microsoft",     "Lifecam VX-7000",                 "??" },
	{{ 0, 0, 0, 0x046d, 0x08c1, }, "Logitech",      "QuickCam Fusion",                 "??" },
	{{ 0, 0, 0, 0x046d, 0x08c2, }, "Logitech",      "QuickCam Orbit MP",               "??" },
	{{ 0, 0, 0, 0x046d, 0x08c3, }, "Logitech",      "QuickCam Pro for Notebook",       "??" },
	{{ 0, 0, 0, 0x046d, 0x08c5, }, "Logitech",      "QuickCam Pro 5000",               "??" },
	{{ 0, 0, 0, 0x046d, 0x08c6, }, "Logitech",      "QuickCam OEM Dell Notebook",      "??" },
	{{ 0, 0, 0, 0x046d, 0x08c7, }, "Logitech",      "QuickCam OEM Cisco VT Camera II", "??" },
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


UVCCamDevice::UVCCamDevice(CamDeviceAddon &_addon, BUSBDevice* _device)
	: CamDevice(_addon, _device)
{
	const BUSBConfiguration* config;
	const BUSBInterface* interface;
	usb_descriptor* generic;
	uint8 buffer[1024];

	generic = (usb_descriptor *)buffer;

	for (uint32 i = 0; i < _device->CountConfigurations(); i++) {
		config = _device->ConfigurationAt(i);
		_device->SetConfiguration(config);
		for (uint32 j = 0; j < config->CountInterfaces(); j++) {
			interface = config->InterfaceAt(j);

			if (interface->Class() == CC_VIDEO && interface->Subclass()
				== SC_VIDEOCONTROL) {
				printf("UVCCamDevice: (%lu,%lu): Found Video Control "
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
					const BUSBEndpoint *e = interface->EndpointAt(i);
					if (e && e->IsInterrupt() && e->IsInput()) {
						fInterruptIn = e;
						break;
					}
				}
				fInitStatus = B_OK;
			} else if (interface->Class() == CC_VIDEO && interface->Subclass()
				== SC_VIDEOSTREAMING) {
				printf("UVCCamDevice: (%lu,%lu): Found Video Streaming "
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
					const BUSBEndpoint *e = interface->EndpointAt(i);
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
}


void
UVCCamDevice::_ParseVideoStreaming(const usbvc_class_descriptor* _descriptor,
	size_t len)
{
	switch(_descriptor->descriptorSubtype) {
		case VS_INPUT_HEADER:
		{
			const usbvc_input_header_descriptor* descriptor =
				(const usbvc_input_header_descriptor*)_descriptor;
			printf("VS_INPUT_HEADER:\t#fmts=%d,ept=0x%x\n", descriptor->numFormats,
				descriptor->endpointAddress);
			if (descriptor->info & 1)
				printf("\tDynamic Format Change supported\n");
			printf("\toutput terminal id=%d\n", descriptor->terminalLink);
			printf("\tstill capture method=%d\n", descriptor->stillCaptureMethod);
			if (descriptor->triggerSupport) {
				printf("\ttrigger button fixed to still capture=%s\n",
					descriptor->triggerUsage ? "no" : "yes");
			}
			const uint8 *controls = descriptor->controls;
			for (uint8 i = 0; i < descriptor->numFormats; i++,
				controls += descriptor->controlSize) {
				printf("\tfmt%d: %s %s %s %s - %s %s\n", i,
					(*controls & 1) ? "wKeyFrameRate" : "",
					(*controls & 2) ? "wPFrameRate" : "",
					(*controls & 4) ? "wCompQuality" : "",
					(*controls & 8) ? "wCompWindowSize" : "",
					(*controls & 16) ? "<Generate Key Frame>" : "",
					(*controls & 32) ? "<Update Frame Segment>" : "");
			}
			break;
		}
		case VS_FORMAT_UNCOMPRESSED:
		{		
			const usbvc_format_descriptor* descriptor =
				(const usbvc_format_descriptor*)_descriptor;
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
			switch((descriptor->uncompressed.interlaceFlags & 0x30) >> 4) {
				case 0: printf("Field 1 only\n"); break;
				case 1: printf("Field 2 only\n"); break;
				case 2: printf("Regular pattern of fields 1 and 2\n"); break;
				case 3: printf("Random pattern of fields 1 and 2\n"); break;
			}
			if (descriptor->uncompressed.copyProtect)
				printf("\tRestrict duplication\n");
			break;
		}
		case VS_FRAME_MJPEG:
			printf("VS_FRAME_MJPEG:");	// fall through
		case VS_FRAME_UNCOMPRESSED:
		{
			if (_descriptor->descriptorSubtype == VS_FRAME_UNCOMPRESSED)
				printf("VS_FRAME_UNCOMPRESSED:");
			const usbvc_frame_descriptor* descriptor =
				(const usbvc_frame_descriptor*)_descriptor;
			printf("\tbFrameIdx=%d,stillsupported=%s,"
				"fixedframerate=%s\n", descriptor->frameIndex,
				(descriptor->capabilities & 1) ? "yes" : "no",
				(descriptor->capabilities & 2) ? "yes" : "no");
			printf("\twidth=%u,height=%u,min/max bitrate=%lu/%lu, maxbuf=%lu\n",
				descriptor->width, descriptor->height,
				descriptor->minBitRate, descriptor->maxBitRate,
				descriptor->maxVideoFrameBufferSize);
			printf("\tdefault frame interval: %lu, #intervals(0=cont): %d\n", 
				descriptor->defaultFrameInterval, descriptor->frameIntervalType);
			if (descriptor->frameIntervalType == 0) {
				printf("min/max frame interval=%lu/%lu, step=%lu\n",
					descriptor->continuous.minFrameInterval,
					descriptor->continuous.maxFrameInterval,
					descriptor->continuous.frameIntervalStep);
			} else for (uint8 i = 0; i < descriptor->frameIntervalType; i++) {
				printf("discrete frame interval: %lu\n",
					descriptor->discreteFrameIntervals[i]);
			}
			break;
		}
		case VS_COLORFORMAT:
		{
			const usbvc_color_matching_descriptor* descriptor =
				(const usbvc_color_matching_descriptor*)_descriptor;
			printf("VS_COLORFORMAT:\n\tbColorPrimaries: ");
			switch(descriptor->colorPrimaries) {
				case 0: printf("Unspecified\n"); break;
				case 1: printf("BT.709,sRGB\n"); break;
				case 2: printf("BT.470-2(M)\n"); break;
				case 3: printf("BT.470-2(B,G)\n"); break;
				case 4: printf("SMPTE 170M\n"); break;
				case 5: printf("SMPTE 240M\n"); break;
				default: printf("Invalid (%d)\n", descriptor->colorPrimaries);
			}
			printf("\tbTransferCharacteristics: ");
			switch(descriptor->transferCharacteristics) {
				case 0: printf("Unspecified\n"); break;
				case 1: printf("BT.709\n"); break;
				case 2: printf("BT.470-2(M)\n"); break;
				case 3: printf("BT.470-2(B,G)\n"); break;
				case 4: printf("SMPTE 170M\n"); break;
				case 5: printf("SMPTE 240M\n"); break;
				case 6: printf("Linear (V=Lc)\n"); break;
				case 7: printf("sRGB\n"); break;
				default: printf("Invalid (%d)\n",
					descriptor->transferCharacteristics);
			}
			printf("\tbMatrixCoefficients: ");
			switch(descriptor->matrixCoefficients) {
				case 0: printf("Unspecified\n"); break;
				case 1: printf("BT.709\n"); break;
				case 2: printf("FCC\n"); break;
				case 3: printf("BT.470-2(B,G)\n"); break;
				case 4: printf("SMPTE 170M (BT.601)\n"); break;
				case 5: printf("SMPTE 240M\n"); break;
				default: printf("Invalid (%d)\n", descriptor->matrixCoefficients);
			}
			break;
		}
		case VS_OUTPUT_HEADER:
		{
			const usbvc_output_header_descriptor* descriptor =
				(const usbvc_output_header_descriptor*)_descriptor;
			printf("VS_OUTPUT_HEADER:\t#fmts=%d,ept=0x%x\n",
				descriptor->numFormats, descriptor->endpointAddress);
			printf("\toutput terminal id=%d\n", descriptor->terminalLink);
			const uint8 *controls = descriptor->controls;
			for (uint8 i = 0; i < descriptor->numFormats; i++,
				controls += descriptor->controlSize) {
				printf("\tfmt%d: %s %s %s %s\n", i,
					(*controls & 1) ? "wKeyFrameRate" : "",
					(*controls & 2) ? "wPFrameRate" : "",
					(*controls & 4) ? "wCompQuality" : "",
					(*controls & 8) ? "wCompWindowSize" : "");
			}
			break;
		}
		case VS_STILL_IMAGE_FRAME:
		{
			const usbvc_still_image_frame_descriptor* descriptor =
				(const usbvc_still_image_frame_descriptor*)_descriptor;
			printf("VS_STILL_IMAGE_FRAME:\t#imageSizes=%d,compressions=%d,"
				"ept=0x%x\n", descriptor->numImageSizePatterns,
				descriptor->NumCompressionPatterns(),
				descriptor->endpointAddress);
			for (uint8 i = 0; i < descriptor->numImageSizePatterns; i++) {
				printf("imageSize%d: %dx%d\n", i,
					descriptor->imageSizePatterns[i].width,
					descriptor->imageSizePatterns[i].height);
			}
			for (uint8 i = 0; i < descriptor->NumCompressionPatterns(); i++) {
				printf("compression%d: %d\n", i,
					descriptor->CompressionPatterns()[i]);
			}
			break;
		}
		case VS_FORMAT_MJPEG:
		{		
			const usbvc_format_descriptor* descriptor =
				(const usbvc_format_descriptor*)_descriptor;
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
			switch((descriptor->mjpeg.interlaceFlags & 0x30) >> 4) {
				case 0: printf("Field 1 only\n"); break;
				case 1: printf("Field 2 only\n"); break;
				case 2: printf("Regular pattern of fields 1 and 2\n"); break;
				case 3: printf("Random pattern of fields 1 and 2\n"); break;
			}
			if (descriptor->mjpeg.copyProtect)
				printf("\tRestrict duplication\n");
			break;
		}
		case VS_FORMAT_MPEG2TS:
			printf("VS_FORMAT_MPEG2TS:\t\n");
			break;
		case VS_FORMAT_DV:
			printf("VS_FORMAT_DV:\t\n");
			break;
		case VS_FORMAT_FRAME_BASED:
			printf("VS_FORMAT_FRAME_BASED:\t\n");
			break;
		case VS_FRAME_FRAME_BASED:
			printf("VS_FRAME_FRAME_BASED:\t\n");
			break;
		case VS_FORMAT_STREAM_BASED:
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
	switch(_descriptor->descriptorSubtype) {
		case VC_HEADER:
		{
			fHeaderDescriptor = (usbvc_interface_header_descriptor*)_descriptor;
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
		case VC_INPUT_TERMINAL:
		{
			const usbvc_input_terminal_descriptor* descriptor =
				(const usbvc_input_terminal_descriptor*)_descriptor;
			printf("VC_INPUT_TERMINAL:\tid=%d,type=%04x,associated terminal="
				"%d\n", descriptor->terminalID, descriptor->terminalType,
				descriptor->associatedTerminal);
			printf("\tDesc: %s\n",
				fDevice->DecodeStringDescriptor(descriptor->terminal));
			if (descriptor->terminalType == 0x201) {
				const usbvc_camera_terminal_descriptor* desc =
					(const usbvc_camera_terminal_descriptor*)descriptor;
				printf("\tObjectiveFocalLength Min/Max %d/%d\n",
					desc->objectiveFocalLengthMin,
					desc->objectiveFocalLengthMax);
				printf("\tOcularFocalLength %d\n", desc->ocularFocalLength);
				printf("\tControlSize %d\n", desc->controlSize);
			}
			break;
		}
		case VC_OUTPUT_TERMINAL:
		{
			const usbvc_output_terminal_descriptor* descriptor =
				(const usbvc_output_terminal_descriptor*)_descriptor;
			printf("VC_OUTPUT_TERMINAL:\tid=%d,type=%04x,associated terminal="
				"%d, src id=%d\n", descriptor->terminalID,
				descriptor->terminalType, descriptor->associatedTerminal,
				descriptor->sourceID);
			printf("\tDesc: %s\n",
				fDevice->DecodeStringDescriptor(descriptor->terminal));
			break;
		}
		case VC_SELECTOR_UNIT:
		{
			const usbvc_selector_unit_descriptor* descriptor =
				(const usbvc_selector_unit_descriptor*)_descriptor;
			printf("VC_SELECTOR_UNIT:\tid=%d,#pins=%d\n",
				descriptor->unitID, descriptor->numInputPins);
			printf("\t");
			for (uint8 i = 0; i < descriptor->numInputPins; i++)
				printf("%d ", descriptor->sourceID[i]);
			printf("\n");
			printf("\tDesc: %s\n",
				fDevice->DecodeStringDescriptor(descriptor->Selector()));
			break;
		}
		case VC_PROCESSING_UNIT:
		{
			const usbvc_processing_unit_descriptor* descriptor =
				(const usbvc_processing_unit_descriptor*)_descriptor;
			printf("VC_PROCESSING_UNIT:\tid=%d,src id=%d, digmul=%d\n",
				descriptor->unitID, descriptor->sourceID,
				descriptor->maxMultiplier);
			printf("\tbControlSize=%d\n", descriptor->controlSize);
			if (descriptor->controlSize >= 1) {
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
			if (descriptor->controlSize >= 2) {
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
			if (descriptor->controlSize >= 3) {
				if (descriptor->controls[2] & 1)
					printf("\tAnalog Video Standard\n");
				if (descriptor->controls[2] & 2)
					printf("\tAnalog Video Lock Status\n");
			}
			printf("\tDesc: %s\n",
				fDevice->DecodeStringDescriptor(descriptor->Processing()));
			if (descriptor->VideoStandards() & 2)
				printf("\tNTSC  525/60\n");
			if (descriptor->VideoStandards() & 4)
				printf("\tPAL   625/50\n");
			if (descriptor->VideoStandards() & 8)
				printf("\tSECAM 625/50\n");
			if (descriptor->VideoStandards() & 16)
				printf("\tNTSC  625/50\n");
			if (descriptor->VideoStandards() & 32)
				printf("\tPAL   525/60\n");
			break;
		}
		case VC_EXTENSION_UNIT:
		{
			const usbvc_extension_unit_descriptor* descriptor =
				(const usbvc_extension_unit_descriptor*)_descriptor;
			printf("VC_EXTENSION_UNIT:\tid=%d, guid=", descriptor->unitID);
			print_guid(descriptor->guidExtensionCode);
			printf("\n\t#ctrls=%d, #pins=%d\n", descriptor->numControls,
				descriptor->numInputPins);
			printf("\t");
			for (uint8 i = 0; i < descriptor->numInputPins; i++)
				printf("%d ", descriptor->sourceID[i]);
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
	return CamDevice::StopTransfer();
}


status_t
UVCCamDevice::SuggestVideoFrame(uint32 &width, uint32 &height)
{
	width = 320;
	height = 240;
	return B_OK;
}


status_t
UVCCamDevice::AcceptVideoFrame(uint32 &width, uint32 &height)
{
	width = 320;
	height = 240;
	
	SetVideoFrame(BRect(0, 0, width - 1, height - 1));
	return B_OK;
}


status_t
UVCCamDevice::_ProbeCommitFormat()
{
	usbvc_probecommit request;
	memset(&request, 0, sizeof(request));
	request.hint = 1 << 8;
	request.SetFrameInterval(333333);
	request.formatIndex = 1;
	request.frameIndex = 3;
	size_t length = fHeaderDescriptor->version > 0x100 ? 34 : 26;
	size_t actualLength = fDevice->ControlTransfer(
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT, SET_CUR,
		VS_PROBE_CONTROL << 8, fStreamingIndex, length, &request);
	if (actualLength != length) {
		fprintf(stderr, "UVCCamDevice::_ProbeFormat() SET_CUR ProbeControl1"
			" failed %ld\n", actualLength);
		return B_ERROR;
	}

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

	actualLength = fDevice->ControlTransfer(
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT, SET_CUR,
		VS_PROBE_CONTROL << 8, fStreamingIndex, length, &request);
	if (actualLength != length) {
		fprintf(stderr, "UVCCamDevice::_ProbeFormat() SetCur ProbeControl2"
			" failed\n");
		return B_ERROR;
	}

	actualLength = fDevice->ControlTransfer(
		USB_REQTYPE_CLASS | USB_REQTYPE_INTERFACE_OUT, SET_CUR,
		VS_COMMIT_CONTROL << 8, fStreamingIndex, length, &request);
	if (actualLength != length) {
		fprintf(stderr, "UVCCamDevice::_ProbeFormat() SetCur CommitControl"
			" failed\n");
		return B_ERROR;
	}
	
	fMaxVideoFrameSize = response.maxVideoFrameSize;
	fMaxPayloadTransferSize = response.maxPayloadTransferSize;	
	printf("usbvc_probecommit setup done maxVideoFrameSize:%ld"
		" maxPayloadTransferSize:%ld\n", fMaxVideoFrameSize,
		fMaxPayloadTransferSize);
	
	return B_OK;
}


status_t
UVCCamDevice::_SelectBestAlternate()
{
	const BUSBConfiguration *config = fDevice->ActiveConfiguration();
	const BUSBInterface *streaming = config->InterfaceAt(fStreamingIndex);
	
	uint32 bestBandwidth = 0;
	uint32 alternateIndex = 0;
	uint32 endpointIndex = 0;
	
	for (uint32 i = 0; i < streaming->CountAlternates(); i++) {
		const BUSBInterface *alternate = streaming->AlternateAt(i);
		for (uint32 j = 0; j < alternate->CountEndpoints(); j++) {
			const BUSBEndpoint *endpoint = alternate->EndpointAt(j);
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
	
	printf("UVCCamDevice::_SelectBestAlternate() %ld\n", bestBandwidth);
	if (((BUSBInterface *)streaming)->SetAlternate(alternateIndex) != B_OK) {
		fprintf(stderr, "UVCCamDevice::_SelectBestAlternate()"
			" selecting alternate failed\n");
		return B_ERROR;
	}
	
	fIsoIn = streaming->EndpointAt(endpointIndex);
	
	return B_OK;
}


UVCCamDeviceAddon::UVCCamDeviceAddon(WebCamMediaAddOn* webcam)
	: CamDeviceAddon(webcam)
{
	SetSupportedDevices(kSupportedDevices);
}


UVCCamDeviceAddon::~UVCCamDeviceAddon()
{
}


const char *
UVCCamDeviceAddon::BrandName()
{
	return "USB Video Class";
}


UVCCamDevice *
UVCCamDeviceAddon::Instantiate(CamRoster &roster, BUSBDevice *from)
{
	return new UVCCamDevice(*this, from);
}


extern "C" status_t
B_WEBCAM_MKINTFUNC(uvccam)
(WebCamMediaAddOn* webcam, CamDeviceAddon **addon)
{
	*addon = new UVCCamDeviceAddon(webcam);
	return B_OK;
}
