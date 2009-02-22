/*
 * Copyright 2009, Ithamar Adema, <ithamar.adema@team-embedded.nl>.
 * Distributed under the terms of the MIT License.
 */

#include "UVCCamDevice.h"
#include "USB_video.h"
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

static void print_guid(const uint8* buf)
{
	printf("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
		buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
}


UVCCamDevice::UVCCamDevice(CamDeviceAddon &_addon, BUSBDevice* _device)
	: CamDevice(_addon, _device)
{
	const BUSBConfiguration* uc;
	const BUSBInterface* ui;
	usb_descriptor *generic;
	uint32 cfg, intf, di;
	uint8 buffer[1024];

	generic = (usb_descriptor *)buffer;

	for (cfg=0; cfg < _device->CountConfigurations(); cfg++) {
		uc = _device->ConfigurationAt(cfg);
		for (intf=0; intf < uc->CountInterfaces(); intf++) {
			ui = uc->InterfaceAt(intf);

			if (ui->Class() == CC_VIDEO && ui->Subclass() == SC_VIDEOCONTROL) {
				printf("UVCCamDevice: (%lu,%lu): Found Video Control interface.\n", cfg, intf);

				// look for class specific interface descriptors and parse them
				for (di=0; ui->OtherDescriptorAt(di,generic,sizeof(buffer)) == B_OK; ++di)
					if (generic->generic.descriptor_type == (USB_REQTYPE_CLASS | USB_DESCRIPTOR_INTERFACE))
						ParseVideoControl(buffer, generic->generic.length);

				fInitStatus = B_OK;
			} else if (ui->Class() == CC_VIDEO && ui->Subclass() == SC_VIDEOSTREAMING) {
				printf("UVCCamDevice: (%lu,%lu): Found Video Control interface.\n", cfg, intf);

				// look for class specific interface descriptors and parse them
				for (di=0; ui->OtherDescriptorAt(di,generic,sizeof(buffer)) == B_OK; ++di)
					if (generic->generic.descriptor_type == (USB_REQTYPE_CLASS | USB_DESCRIPTOR_INTERFACE))
						ParseVideoStreaming(buffer, generic->generic.length);
			}
		}
	}
}

void UVCCamDevice::ParseVideoStreaming(const uint8* buffer, size_t len)
{
	int c, i, sz;

	switch(buffer[2]) {
		case VS_INPUT_HEADER:
			c = buffer[3];
			printf("VS_INPUT_HEADER:\t#fmts=%d,ept=0x%x\n", c, buffer[6]);
			if (buffer[7] & 1) printf("\tDynamic Format Change supported\n");
			printf("\toutput terminal id=%d\n", buffer[8]);
			printf("\tstill capture method=%d\n", buffer[9]);
			if (buffer[10])
				printf("\ttrigger button fixed to still capture=%s\n", buffer[11] ? "no" : "yes");
			sz = buffer[12];
			for (i=0; i < c; i++) {
				printf("\tfmt%d: %s %s %s %s - %s %s\n", i,
					(buffer[13+(sz*i)] & 1) ? "wKeyFrameRate" : "",
					(buffer[13+(sz*i)] & 2) ? "wPFrameRate" : "",
					(buffer[13+(sz*i)] & 4) ? "wCompQuality" : "",
					(buffer[13+(sz*i)] & 8) ? "wCompWindowSize" : "",
					(buffer[13+(sz*i)] & 16) ? "<Generate Key Frame>" : "",
					(buffer[13+(sz*i)] & 32) ? "<Update Frame Segment>" : "");
			}
			break;
		case VS_FORMAT_UNCOMPRESSED:
			c = buffer[4];
			printf("VS_FORMAT_UNCOMPRESSED:\tbFormatIdx=%d,#frmdesc=%d,guid=", buffer[3], c);
			print_guid(buffer+5);
			printf("\n\t#bpp=%d,optfrmidx=%d,aspRX=%d,aspRY=%d\n", buffer[21], buffer[22], 
				buffer[23], buffer[24]);
			printf("\tbmInterlaceFlags:\n");
			if (buffer[25] & 1) printf("\tInterlaced stream or variable\n");
			printf("\t%d fields per frame\n", (buffer[25] & 2) ? 1 : 2);
			if (buffer[25] & 4) printf("\tField 1 first\n");
			printf("\tField Pattern: ");
			switch((buffer[25] & 0x30) >> 4) {
				case 0: printf("Field 1 only\n"); break;
				case 1: printf("Field 2 only\n"); break;
				case 2: printf("Regular pattern of fields 1 and 2\n"); break;
				case 3: printf("Random pattern of fields 1 and 2\n"); break;
			}
			if (buffer[26]) printf("\tRestrict duplication\n"); break;
			break;
		case VS_FRAME_UNCOMPRESSED:
			printf("VS_FRAME_UNCOMPRESSED:\tbFrameIdx=%d,stillsupported=%s,fixedfrmrate=%s\n",
				buffer[3], (buffer[4]&1)?"yes":"no", (buffer[4]&2)?"yes":"no");
			printf("\twidth=%u,height=%u,min/max bitrate=%lu/%lu, maxbuf=%lu\n",
				*(uint16*)(buffer+5), *(uint16*)(buffer+7),
				*(uint32*)(buffer+9), *(uint32*)(buffer+13),
				*(uint32*)(buffer+17));
			printf("\tframe interval: %lu, #intervals(0=cont): %d\n", *(uint32*)(buffer+21), buffer[25]);
			//TODO print interval table
			break;
		case VS_COLORFORMAT:
			printf("VS_COLORFORMAT:\n\tbColorPrimaries: ");
			switch(buffer[3]) {
				case 0: printf("Unspecified\n"); break;
				case 1: printf("BT.709,sRGB\n"); break;
				case 2: printf("BT.470-2(M)\n"); break;
				case 3: printf("BT.470-2(B,G)\n"); break;
				case 4: printf("SMPTE 170M\n"); break;
				case 5: printf("SMPTE 240M\n"); break;
				default: printf("Invalid (%d)\n", buffer[3]);
			}
			printf("\tbTransferCharacteristics: ");
			switch(buffer[4]) {
				case 0: printf("Unspecified\n"); break;
				case 1: printf("BT.709\n"); break;
				case 2: printf("BT.470-2(M)\n"); break;
				case 3: printf("BT.470-2(B,G)\n"); break;
				case 4: printf("SMPTE 170M\n"); break;
				case 5: printf("SMPTE 240M\n"); break;
				case 6: printf("Linear (V=Lc)\n"); break;
				case 7: printf("sRGB\n"); break;
				default: printf("Invalid (%d)\n", buffer[4]);
			}
			printf("\tbMatrixCoefficients: ");
			switch(buffer[5]) {
				case 0: printf("Unspecified\n"); break;
				case 1: printf("BT.709\n"); break;
				case 2: printf("FCC\n"); break;
				case 3: printf("BT.470-2(B,G)\n"); break;
				case 4: printf("SMPTE 170M (BT.601)\n"); break;
				case 5: printf("SMPTE 240M\n"); break;
				default: printf("Invalid (%d)\n", buffer[5]);
			}
			break;

		case VS_OUTPUT_HEADER:
			printf("VS_OUTPUT_HEADER:\t\n");
			break;
		case VS_STILL_IMAGE_FRAME:
			printf("VS_STILL_IMAGE_FRAME:\t\n");
			break;
		case VS_FORMAT_MJPEG:
			printf("VS_FORMAT_MJPEG:\t\n");
			break;
		case VS_FRAME_MJPEG:
			printf("VS_FRAME_MJPEG:\t\n");
			break;
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
			printf("INVALID STREAM UNIT TYPE=%d!\n", buffer[2]);
	}
}

void UVCCamDevice::ParseVideoControl(const uint8* buffer, size_t len)
{
	int c, i;

	switch(buffer[2]) {
		case VC_HEADER:
			printf("VC_HEADER:\tUVC v%04x, clk %lu Hz\n", *(uint16*)(buffer+3), *(uint32*)(buffer+7));
			c = (len >= 12) ? buffer[11] : 0;
			for (i=0; i < c; i++)
				printf("\tStreaming Interface %d\n", buffer[12+i]);
			break;

		case VC_INPUT_TERMINAL:
			printf("VC_INPUT_TERMINAL:\tid=%d,type=%04x,associated terminal=%d\n", buffer[3], *(uint16*)(buffer+4), buffer[6]);
			printf("\tDesc: %s\n", fDevice->DecodeStringDescriptor(buffer[7]));
			break;

		case VC_OUTPUT_TERMINAL:
			printf("VC_OUTPUT_TERMINAL:\tid=%d,type=%04x,associated terminal=%d, src id=%d\n", buffer[3], *(uint16*)(buffer+4), buffer[6], buffer[7]);
			printf("\tDesc: %s\n", fDevice->DecodeStringDescriptor(buffer[8]));
			break;

		case VC_SELECTOR_UNIT:
			printf("VC_SELECTOR_UNIT:\tid=%d,#pins=%d\n", buffer[3], buffer[4]);
			printf("\t");
			for (i=0; i < buffer[4]; i++)
				printf("%d ", buffer[5+i]);
			printf("\n");
			printf("\tDesc: %s\n", fDevice->DecodeStringDescriptor(buffer[5+buffer[4]]));
			break;

		case VC_PROCESSING_UNIT:
			printf("VC_PROCESSING_UNIT:\tid=%d,src id=%d, digmul=%d\n", buffer[3], buffer[4], *(uint16*)(buffer+5));
			c = buffer[7];
			printf("\tbControlSize=%d\n", c);
			if (c >= 1) {
				if (buffer[8] & 1) printf("\tBrightness\n");
				if (buffer[8] & 2) printf("\tContrast\n");
				if (buffer[8] & 4) printf("\tHue\n");
				if (buffer[8] & 8) printf("\tSaturation\n");
				if (buffer[8] & 16) printf("\tSharpness\n");
				if (buffer[8] & 32) printf("\tGamma\n");
				if (buffer[8] & 64) printf("\tWhite Balance Temperature\n");
				if (buffer[8] & 128) printf("\tWhite Balance Component\n");
			}
			if (c >= 2) {
				if (buffer[9] & 1) printf("\tBacklight Compensation\n");
				if (buffer[9] & 2) printf("\tGain\n");
				if (buffer[9] & 4) printf("\tPower Line Frequency\n");
				if (buffer[9] & 8) printf("\t[AUTO] Hue\n");
				if (buffer[9] & 16) printf("\t[AUTO] White Balance Temperature\n");
				if (buffer[9] & 32) printf("\t[AUTO] White Balance Component\n");
				if (buffer[9] & 64) printf("\tDigital Multiplier\n");
				if (buffer[9] & 128) printf("\tDigital Multiplier Limit\n");
			}
			if (c >= 3) {
				if (buffer[10] & 1) printf("\tAnalog Video Standard\n");
				if (buffer[10] & 2) printf("\tAnalog Video Lock Status\n");
			}
			printf("\tDesc: %s\n", fDevice->DecodeStringDescriptor(buffer[8+c]));
			i=buffer[9+c];
			if (i & 2)  printf("\tNTSC  525/60\n");
			if (i & 4)  printf("\tPAL   625/50\n");
			if (i & 8)  printf("\tSECAM 625/50\n");
			if (i & 16) printf("\tNTSC  625/50\n");
			if (i & 32) printf("\tPAL   525/60\n");
			break;

		case VC_EXTENSION_UNIT:
			printf("VC_EXTENSION_UNIT:\tid=%d, guid=", buffer[3]);
			print_guid(buffer+4);
			printf("\n\t#ctrls=%d, #pins=%d\n", buffer[20], buffer[21]);
			c = buffer[21];
			printf("\t");
			for (i=0; i < c; i++)
				printf("%d ", buffer[22+i]);
			printf("\n");
			printf("\tDesc: %s\n", fDevice->DecodeStringDescriptor(buffer[23+c+buffer[22+c]]));
			break;

		default:
			printf("Unknown control %d\n", buffer[2]);
	}
}

UVCCamDevice::~UVCCamDevice()
{
}

bool UVCCamDevice::SupportsIsochronous()
{
	return true;
}

status_t UVCCamDevice::StartTransfer()
{
	return CamDevice::StartTransfer();
}

status_t UVCCamDevice::StopTransfer()
{
	return CamDevice::StopTransfer();
}

UVCCamDeviceAddon::UVCCamDeviceAddon(WebCamMediaAddOn* webcam)
	: CamDeviceAddon(webcam)
{
	SetSupportedDevices(kSupportedDevices);
}


UVCCamDeviceAddon::~UVCCamDeviceAddon()
{
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
