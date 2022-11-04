/*
 * Copyright 2003, Thomas Kurschel. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "edid.h"
#if !defined(_KERNEL_MODE) && !defined(_BOOT_MODE)
#	include "ddc_int.h"
#endif

#include <KernelExport.h>


const edid1_std_timing cta_mode1_timings[] = {
	/* VIC 1 */
	{  640,  480,   0, 1, 60 },
	{  720,  480,   0, 1, 60 },
	{  720,  480,   0, 3, 60 },
	{ 1280,  720,   0, 3, 60 },
	{ 1920, 1080,   0, 3, 60 },
	{ 1440,  480,   0, 1, 60 },
	{ 1440,  480,   0, 3, 60 },
	{ 1440,  240,   0, 1, 60 },
	{ 1440,  240,   0, 3, 60 },
	{ 2880,  480,   0, 1, 60 },
	/* VIC 11 */
	{ 2880,  480,   0, 3, 60 },
	{ 2880,  240,   0, 1, 60 },
	{ 2880,  240,   0, 3, 60 },
	{ 1440,  480,   0, 1, 60 },
	{ 1440,  480,   0, 3, 60 },
	{ 1920, 1080,   0, 3, 60 },
	{  720,  576,   0, 1, 50 },
	{  720,  576,   0, 3, 50 },
	{ 1280,  720,   0, 3, 50 },
	{ 1920, 1080,   0, 3, 50 },
	/* VIC 21 */
	{ 1440,  576,   0, 1, 50 },
	{ 1440,  576,   0, 3, 50 },
	{ 1440,  288,   0, 1, 50 },
	{ 1440,  288,   0, 3, 50 },
	{ 2880,  576,   0, 1, 50 },
	{ 2880,  576,   0, 3, 50 },
	{ 2880,  288,   0, 1, 50 },
	{ 2880,  288,   0, 3, 50 },
	{ 1440,  576,   0, 1, 50 },
	{ 1440,  576,   0, 3, 50 },
	/* VIC 31 */
	{ 1920, 1080,   0, 3, 50 },
	{ 1920, 1080,   0, 3, 24 },
	{ 1920, 1080,   0, 3, 25 },
	{ 1920, 1080,   0, 3, 30 },
	{ 2880,  480,   0, 1, 60 },
	{ 2880,  480,   0, 3, 60 },
	{ 2880,  576,   0, 1, 50 },
	{ 2880,  576,   0, 3, 50 },
	{ 1920, 1080,   0, 3, 50 },
	{ 1920, 1080,   0, 3, 100 },
	/* VIC 41 */
	{ 1280,  720,   0, 3, 100 },
	{  720,  576,   0, 1, 100 },
	{  720,  576,   0, 3, 100 },
	{ 1440,  576,   0, 1, 100 },
	{ 1440,  576,   0, 3, 100 },
	{ 1920, 1080,   0, 3, 120 },
	{ 1280,  720,   0, 3, 120 },
	{  720,  480,   0, 1, 120 },
	{  720,  480,   0, 3, 120 },
	{ 1440,  480,   0, 1, 120 },
	/* VIC 51 */
	{ 1440,  480,   0, 3, 120 },
	{  720,  576,   0, 1, 200 },
	{  720,  576,   0, 3, 200 },
	{ 1440,  576,   0, 1, 200 },
	{ 1440,  576,   0, 3, 200 },
	{  720,  480,   0, 1, 240 },
	{  720,  480,   0, 3, 240 },
	{ 1440,  480,   0, 1, 240 },
	{ 1440,  480,   0, 3, 240 },
	{ 1280,  720,   0, 3, 24 },
	/* VIC 61 */
	{ 1280,  720,   0, 3, 25 },
	{ 1280,  720,   0, 3, 30 },
	{ 1920, 1080,   0, 3, 120 },
	{ 1920, 1080,   0, 3, 100 },
	{ 1280,  720,   0, 4, 24 },
	{ 1280,  720,   0, 4, 25 },
	{ 1280,  720,   0, 4, 30 },
	{ 1280,  720,   0, 4, 50 },
	{ 1280,  720,   0, 4, 60 },
	{ 1280,  720,   0, 4, 100 },
	/* VIC 71 */
	{ 1280,  720,   0, 4, 120 },
	{ 1920, 1080,   0, 4, 24 },
	{ 1920, 1080,   0, 4, 25 },
	{ 1920, 1080,   0, 4, 30 },
	{ 1920, 1080,   0, 4, 50 },
	{ 1920, 1080,   0, 4, 60 },
	{ 1920, 1080,   0, 4, 100 },
	{ 1920, 1080,   0, 4, 120 },
	{ 1680,  720,   0, 4, 24 },
	{ 1680,  720,   0, 4, 25 },
	/* VIC 81 */
	{ 1680,  720,   0, 4, 30 },
	{ 1680,  720,   0, 4, 50 },
	{ 1680,  720,   0, 4, 60 },
	{ 1680,  720,   0, 4, 100 },
	{ 1680,  720,   0, 4, 120 },
	{ 2560, 1080,   0, 4, 24 },
	{ 2560, 1080,   0, 4, 25 },
	{ 2560, 1080,   0, 4, 30 },
	{ 2560, 1080,   0, 4, 50 },
	{ 2560, 1080,   0, 4, 60 },
	/* VIC 91 */
	{ 2560, 1080,   0, 4, 100 },
	{ 2560, 1080,   0, 4, 120 },
	{ 3840, 2160,   0, 3, 24 },
	{ 3840, 2160,   0, 3, 25 },
	{ 3840, 2160,   0, 3, 30 },
	{ 3840, 2160,   0, 3, 50 },
	{ 3840, 2160,   0, 3, 60 },
	{ 4096, 2160,   0, 5, 24 },
	{ 4096, 2160,   0, 5, 25 },
	{ 4096, 2160,   0, 5, 30 },
	/* VIC 101 */
	{ 4096, 2160,   0, 5, 50 },
	{ 4096, 2160,   0, 5, 60 },
	{ 3840, 2160,   0, 4, 24 },
	{ 3840, 2160,   0, 4, 25 },
	{ 3840, 2160,   0, 4, 30 },
	{ 3840, 2160,   0, 4, 50 },
	{ 3840, 2160,   0, 4, 60 },
	{ 1280,  720,   0, 3, 48 },
	{ 1280,  720,   0, 4, 48 },
	{ 1680,  720,   0, 4, 48 },
	/* VIC 111 */
	{ 1920, 1080,   0, 3, 48 },
	{ 1920, 1080,   0, 4, 48 },
	{ 2560, 1080,   0, 4, 48 },
	{ 3840, 2160,   0, 3, 48 },
	{ 4096, 2160,   0, 5, 48 },
	{ 3840, 2160,   0, 4, 48 },
	{ 3840, 2160,   0, 3, 100 },
	{ 3840, 2160,   0, 3, 120 },
	{ 3840, 2160,   0, 4, 100 },
	{ 3840, 2160,   0, 4, 120 },
	/* VIC 121 */
	{ 5120, 2160,   0, 4, 24 },
	{ 5120, 2160,   0, 4, 25 },
	{ 5120, 2160,   0, 4, 30 },
	{ 5120, 2160,   0, 4, 48 },
	{ 5120, 2160,   0, 4, 50 },
	{ 5120, 2160,   0, 4, 60 },
	{ 5120, 2160,   0, 4, 100 },
};


const edid1_std_timing cta_mode2_timings[] = {
	/* VIC 193 */
	{ 5120, 2160,  0, 4, 120 },
	{ 7680, 4320,  0, 3, 24 },
	{ 7680, 4320,  0, 3, 25 },
	{ 7680, 4320,  0, 3, 30 },
	{ 7680, 4320,  0, 3, 48 },
	{ 7680, 4320,  0, 3, 50 },
	{ 7680, 4320,  0, 3, 60 },
	{ 7680, 4320,  0, 3, 100 },
	/* VIC 201 */
	{ 7680, 4320,  0, 3, 120 },
	{ 7680, 4320,  0, 4, 24 },
	{ 7680, 4320,  0, 4, 25 },
	{ 7680, 4320,  0, 4, 30 },
	{ 7680, 4320,  0, 4, 48 },
	{ 7680, 4320,  0, 4, 50 },
	{ 7680, 4320,  0, 4, 60 },
	{ 7680, 4320,  0, 4, 100 },
	{ 7680, 4320,  0, 4, 120 },
	{ 10240, 4320, 0, 4, 24 },
	/* VIC 211 */
	{ 10240, 4320, 0, 4, 25 },
	{ 10240, 4320, 0, 4, 30 },
	{ 10240, 4320, 0, 4, 48 },
	{ 10240, 4320, 0, 4, 50 },
	{ 10240, 4320, 0, 4, 60 },
	{ 10240, 4320, 0, 4, 100 },
	{ 10240, 4320, 0, 4, 120 },
	{ 4096, 2160,  0, 5, 100 },
	{ 4096, 2160,  0, 5, 120 },
};


static void
edid_timing_dump(edid1_detailed_timing *timing, bool warnInvalid)
{
	if (timing->h_active + timing->h_blank == 0
		|| timing->v_active + timing->v_blank == 0
		|| timing->pixel_clock == 0) {
		if (warnInvalid)
			dprintf("Invalid video mode (%dx%d)\n", timing->h_active, timing->v_active);
		return;
	}

	dprintf("Additional Video Mode (%dx%d@%dHz):\n",
		timing->h_active, timing->v_active,
		(timing->pixel_clock * 10000
			/ (timing->h_active + timing->h_blank)
			/ (timing->v_active + timing->v_blank)));
		// Refresh rate = pixel clock in MHz / Htotal / Vtotal

	dprintf("clock=%.3f MHz\n", timing->pixel_clock / 100.0);
	dprintf("h: (%d, %d, %d, %d)\n",
		timing->h_active, timing->h_active + timing->h_sync_off,
		timing->h_active + timing->h_sync_off
			+ timing->h_sync_width,
		timing->h_active + timing->h_blank);
	dprintf("v: (%d, %d, %d, %d)\n",
		timing->v_active, timing->v_active + timing->v_sync_off,
		timing->v_active + timing->v_sync_off
			+ timing->v_sync_width,
		timing->v_active + timing->v_blank);
	dprintf("size: %.1f cm x %.1f cm\n",
		timing->h_size / 10.0, timing->v_size / 10.0);
	dprintf("border: %.1f cm x %.1f cm\n",
		timing->h_border / 10.0, timing->v_border / 10.0);
}


static void
edid_cta_datablock_dump(cta_data_block *dataBlock)
{
	uint16 tagCode = dataBlock->tag_code;
	uint8 i, j;
	if (tagCode == 0x7)
		tagCode = 0x700 | dataBlock->extended_tag_code;
	switch (tagCode) {
		case 0x1:
			dprintf("Audio data block\n");
			for (i = 0; i * 3 < dataBlock->length; i++) {
				const char* formatName = NULL;
				audio_descr* descr = &dataBlock->audio.desc0;
				if (i > 0)
					descr = &dataBlock->audio.desc[i - 1];
				switch (descr->descr.format_code) {
					case 0x1: formatName = "Linear-PCM"; break;
					case 0x2: formatName = "AC-3"; break;
					case 0x3: formatName = "MPEG 1 (Layers 1 & 2)"; break;
					case 0x4: formatName = "MPEG 1 Layer 3 (MP3)"; break;
					case 0x5: formatName = "MPEG2 (multichannel)"; break;
					case 0x6: formatName = "AAC LC"; break;
					case 0x7: formatName = "DTS"; break;
					case 0x8: formatName = "ATRAC"; break;
					case 0x9: formatName = "One Bit Audio"; break;
					case 0xa: formatName = "Enhanced AC-3 (DD+)"; break;
					case 0xb: formatName = "DTS-HD"; break;
					case 0xc: formatName = "MAT (MLP)"; break;
					case 0xd: formatName = "DST"; break;
					case 0xe: formatName = "WMA Pro"; break;
					case 0xf: formatName = "unknown"; break;
				}
				dprintf("\t%s, max channels %d\n", formatName,
					descr->descr_2_8.max_channels + 1);
				dprintf("\t\tSupported sample rates (kHz): %s%s%s%s%s%s%s\n",
					descr->descr_2_8.can_192khz ? "192 " : "",
					descr->descr_2_8.can_176khz ? "176.4 " : "",
					descr->descr_2_8.can_96khz ? "96 " : "",
					descr->descr_2_8.can_88khz ? "88.2 " : "",
					descr->descr_2_8.can_48khz ? "48 " : "",
					descr->descr_2_8.can_44khz ? "44.1 " : "",
					descr->descr_2_8.can_32khz ? "32 " : ""
				);
				if (descr->descr.format_code >= 2 && descr->descr.format_code <= 0x8) {
					dprintf("\t\tMaximum bit rate: %u kHz\n",
						descr->descr_2_8.maximum_bitrate * 8);
				} else if (descr->descr.format_code == 0x1) {
					dprintf("\t\tSupported sample sizes (bits): %s%s%s\n",
						descr->descr_1.can_24bit ? "24 " : "",
						descr->descr_1.can_20bit ? "20 " : "",
						descr->descr_1.can_16bit ? "16 " : ""
					);
				}
			}
			break;
		case 0x2:
			dprintf("Video data block:\n");
			for (i = 0; i < dataBlock->length; i++) {
				uint8 vic = dataBlock->video.vic0;
				bool native = false;
				const edid1_std_timing *timing = NULL;
				if (i > 0)
					vic = dataBlock->video.vic[i - 1];
				if (((vic - 1) & 0x40) == 0) {
					native = (vic & 0x80) != 0;
					vic = vic & 0x7f;
				}
				if (vic > 0 && vic < B_COUNT_OF(cta_mode1_timings)) {
					timing = &cta_mode1_timings[vic - 1];
				} else if (vic >= 193 && vic < B_COUNT_OF(cta_mode2_timings) + 193) {
					timing = &cta_mode2_timings[vic - 193];
				}

				if (timing != NULL) {
					const char* ratioString = NULL;
					switch (timing->ratio) {
						case 1: ratioString = "4:3";
							break;
						case 2: ratioString = "5:4";
							break;
						case 3: ratioString = "16:9";
							break;
						case 4: ratioString = "64:27";
							break;
						case 5: ratioString = "256:135";
							break;
						default:
							break;
					}
					dprintf("\tVIC %3u %dx%d@%dHz %s%s\n", vic,
						timing->h_size, timing->v_size, timing->refresh, ratioString,
						native ? " (native)" : "");
				}
			}
			break;
		case 0x3:
		{
			uint32 ouinum = (dataBlock->vendor_specific.ouinum2 << 16)
				| (dataBlock->vendor_specific.ouinum1 << 8) | dataBlock->vendor_specific.ouinum0;
			const char* ouinumString = "unknown";
			if (ouinum == 0x000c03)
				ouinumString = "HDMI";
			if (ouinum == 0xc45dd8)
				ouinumString = "HDMI Forum";
			dprintf("Vendor specific data block, %06" B_PRIx32 " (%s)\n", ouinum, ouinumString);
			if (ouinum == 0x000c03) {
				dprintf("\tSource physical address: %u.%u.%u.%u\n",
					dataBlock->vendor_specific.hdmi.source_physical_address.a,
					dataBlock->vendor_specific.hdmi.source_physical_address.b,
					dataBlock->vendor_specific.hdmi.source_physical_address.c,
					dataBlock->vendor_specific.hdmi.source_physical_address.d);
#define PRINT_HDMI_FLAG(x, s)	if (dataBlock->vendor_specific.hdmi. x ) dprintf("\t" s "\n");
				PRINT_HDMI_FLAG(supports_ai, "Supports_AI");
				PRINT_HDMI_FLAG(dc_48bit, "DC_48bit");
				PRINT_HDMI_FLAG(dc_36bit, "DC_36bit");
				PRINT_HDMI_FLAG(dc_30bit, "DC_30bit");
				PRINT_HDMI_FLAG(dc_y444, "DC_Y444");
				PRINT_HDMI_FLAG(dvi_dual, "DVI_DUAL");
#undef PRINT_HDMI_FLAG
				dprintf("\tMaximum TMDS clock: %uMHz\n",
					dataBlock->vendor_specific.hdmi.max_tmds_clock * 5);
				if (dataBlock->vendor_specific.hdmi.vic_length > 0)
					dprintf("\tExtended HDMI video details:\n");
				for (i = 0; i < dataBlock->vendor_specific.hdmi.vic_length; i++) {
					const edid1_std_timing *timing = NULL;
					switch (dataBlock->vendor_specific.hdmi.vic[i]) {
						case 1: timing = &cta_mode1_timings[94]; break;
						case 2: timing = &cta_mode1_timings[93]; break;
						case 3: timing = &cta_mode1_timings[92]; break;
						case 4: timing = &cta_mode1_timings[97]; break;
					}
					if (timing != NULL) {
						const char* ratioString = NULL;
						switch (timing->ratio) {
							case 1: ratioString = "4:3";
								break;
							case 2: ratioString = "5:4";
								break;
							case 3: ratioString = "16:9";
								break;
							case 4: ratioString = "64:27";
								break;
							case 5: ratioString = "256:135";
								break;
							default:
								break;
						}
						dprintf("\tHDMI VIC %u %dx%d@%dHz %s\n", dataBlock->vendor_specific.hdmi.vic[i],
							timing->h_size, timing->v_size, timing->refresh, ratioString);
					}

				}
			} else if (ouinum == 0xc45dd8) {
				dprintf("\tVersion: %u\n", dataBlock->vendor_specific.hdmi_forum.version);
				dprintf("\tMaximum TMDS character rate: %uMHz\n",
					dataBlock->vendor_specific.hdmi_forum.max_tmds_rate * 5);
#define PRINT_HDMIFORUM_FLAG(x, s)	if (dataBlock->vendor_specific.hdmi_forum. x ) dprintf("\t" s "\n");
				PRINT_HDMIFORUM_FLAG(scdc_present, "SCDC Present");
				PRINT_HDMIFORUM_FLAG(scdc_read_request_capable, "SCDC Read Request Capable");
				PRINT_HDMIFORUM_FLAG(supports_cable_status, "Supports Cable Status");
				PRINT_HDMIFORUM_FLAG(supports_color_content_bits,
					"Supports Color Content Bits Per Component Indication");
				PRINT_HDMIFORUM_FLAG(supports_scrambling, "Supports scrambling for <= 340 Mcsc");
				PRINT_HDMIFORUM_FLAG(supports_3d_independent, "Supports 3D Independent View signaling");
				PRINT_HDMIFORUM_FLAG(supports_3d_dual_view, "Supports 3D Dual View signaling");
				PRINT_HDMIFORUM_FLAG(supports_3d_osd_disparity, "Supports 3D OSD Disparity signaling");
				PRINT_HDMIFORUM_FLAG(supports_uhd_vic, "Supports UHD VIC");
				PRINT_HDMIFORUM_FLAG(supports_16bit_deep_color_4_2_0,
					"Supports 16-bits/component Deep Color 4:2:0 Pixel Encoding");
				PRINT_HDMIFORUM_FLAG(supports_12bit_deep_color_4_2_0,
					"Supports 12-bits/component Deep Color 4:2:0 Pixel Encoding");
				PRINT_HDMIFORUM_FLAG(supports_10bit_deep_color_4_2_0,
					"Supports 10-bits/component Deep Color 4:2:0 Pixel Encoding");
#undef PRINT_HDMIFORUM_FLAG
			}
			break;
		}
		case 0x4:
			dprintf("Speaker allocation data block\n");
#define PRINT_SPEAKER_FLAG(x, s)	if (dataBlock->speaker_allocation_map. x ) dprintf("\t" s "\n");
			PRINT_SPEAKER_FLAG(FL_FR, "FL/FR - Front Left/Right");
			PRINT_SPEAKER_FLAG(LFE, "LFE1 - Low Frequency Effects 1");
			PRINT_SPEAKER_FLAG(FC, "FC - Front Center");
			PRINT_SPEAKER_FLAG(BL_BR, "BL/BR - Back Left/Right");
			PRINT_SPEAKER_FLAG(BC, "BC - Back Center");
			PRINT_SPEAKER_FLAG(FLC_FRC, "FLC/FRC - Front Left/Right of Center");
			PRINT_SPEAKER_FLAG(RLC_RRC, "RLC/RRC - Rear Left/Right of Center - deprecated");
			PRINT_SPEAKER_FLAG(FLW_FRW, "FLW/FRW - Front Left/Right Wide");
			PRINT_SPEAKER_FLAG(TpFL_TpFH, "TpFL/TpFR - Top Front Left/Right");
			PRINT_SPEAKER_FLAG(TpC, "TpC - Top Center");
			PRINT_SPEAKER_FLAG(TpFC, "TpFC - Top Front Center");
			PRINT_SPEAKER_FLAG(LS_RS, "LS/RS - Left/Right Surround");
			PRINT_SPEAKER_FLAG(LFE2, "LFE2 - Low Frequency Effects 2");
			PRINT_SPEAKER_FLAG(TpBC, "TpBC - Top Back Center");
			PRINT_SPEAKER_FLAG(SiL_SiR, "SiL/SiR - Side Left/Right");
			PRINT_SPEAKER_FLAG(TpSiL_TpSiR, "TpSiL/TpSiR - Top Side Left/Right");
			PRINT_SPEAKER_FLAG(TpBL_TpBR, "TpBL/TpBR - Top Back Left/Right");
			PRINT_SPEAKER_FLAG(BtFC, "BtFC - Bottom Front Center");
			PRINT_SPEAKER_FLAG(BtFL_BtFR, "BtFL/BtFR - Bottom Front Left/Right");
			PRINT_SPEAKER_FLAG(TpLS_TpRS, "TpLS/TpRS - Top Left/Right Surround");
			PRINT_SPEAKER_FLAG(LSd_RSd, "LSd/RSd - Left/Right Surround Direct");
#undef PRINT_SPEAKER_FLAG
			break;
		case 0x705:
			dprintf("Extended tag: Colorimetry data block\n");
#define PRINT_COLORIMETRY(x)	if (dataBlock->colorimetry. x ) dprintf("\t" #x );
			PRINT_COLORIMETRY(BT2020RGB);
			PRINT_COLORIMETRY(BT2020YCC);
			PRINT_COLORIMETRY(BT2020cYCC);
			PRINT_COLORIMETRY(opRGB);
			PRINT_COLORIMETRY(opYCC601);
			PRINT_COLORIMETRY(sYCC601);
			PRINT_COLORIMETRY(xvYCC709);
			PRINT_COLORIMETRY(xvYCC601);
			PRINT_COLORIMETRY(DCIP3);
#undef PRINT_COLORIMETRY
			dprintf("\n");
			break;
		case 0x700:
			dprintf("Extended tag: Video capability data block\n");
			dprintf("\tYCbCr quantization: %s\n",
				dataBlock->video_capability.QY ? "Selectable (via AVI YQ)" : "No Data");
			dprintf("\tRGB quantization: %s\n",
				dataBlock->video_capability.QS ? "Selectable (via AVI Q)" : "No Data");
			dprintf("\tPT scan behaviour: ");
			switch (dataBlock->video_capability.S_PT) {
				case 0: dprintf("No Data\n"); break;
				case 1: dprintf("Always Overscanned\n"); break;
				case 2: dprintf("Always Underscanned\n"); break;
				case 3: dprintf("Supports both over- and underscan\n"); break;
			}
			dprintf("\tIT scan behaviour: ");
			switch (dataBlock->video_capability.S_IT) {
				case 0: dprintf("IT Video Formats not supported\n"); break;
				case 1: dprintf("Always Overscanned\n"); break;
				case 2: dprintf("Always Underscanned\n"); break;
				case 3: dprintf("Supports both over- and underscan\n"); break;
			}
			dprintf("\tCE scan behaviour: ");
			switch (dataBlock->video_capability.S_CE) {
				case 0: dprintf("CE Video Formats not supported\n"); break;
				case 1: dprintf("Always Overscanned\n"); break;
				case 2: dprintf("Always Underscanned\n"); break;
				case 3: dprintf("Supports both over- and underscan\n"); break;
			}
		case 0x706:
			dprintf("Extended tag: HDR static metadata data block\n");
#define PRINT_SMD_FLAG(x, s)	if (dataBlock->hdr_static_metadata. x ) dprintf("\t\t" s "\n");
			dprintf("\tSupported Electro-Optical Transfer Functions:\n");
			PRINT_SMD_FLAG(ET_0, "Traditional gamma - SDR Luminance Range");
			PRINT_SMD_FLAG(ET_1, "Traditional gamma - HDR Luminance Range");
			PRINT_SMD_FLAG(ET_2, "SMPTE ST 2084");
			PRINT_SMD_FLAG(ET_3, "Hybrid Log-Gamma");
			dprintf("\tSupported Static Metadata Descriptors:\n");
			PRINT_SMD_FLAG(SM_0, "Static Metadata Type 1");
#undef PRINT_SMD_FLAG
			if (dataBlock->length >= 4) {
				dprintf("\tDesired content max luminance: %u\n",
					dataBlock->hdr_static_metadata.desired_content_max_luminance);
			}
			if (dataBlock->length >= 5) {
				dprintf("\tDesired content max frame-average luminance: %u\n",
					dataBlock->hdr_static_metadata.desired_content_max_frame_average_luminance);
			}
			if (dataBlock->length >= 6) {
				dprintf("\tDesired content min luminance: %u\n",
					dataBlock->hdr_static_metadata.desired_content_min_luminance);
			}
			break;
		case 0x707:
			dprintf("Extended tag: HDR dynamic metadata data block\n");
			break;
		case 0x708:
			dprintf("Extended tag: Native Video Resolution data block\n");
			break;
		case 0x711:
			dprintf("Vendor-Specific Audio Data Block\n");
		case 0x70f:
			dprintf("Extended tag: YCbCr 4:2:0 capability map data block\n");
			for (i = 0; i < dataBlock->length - 1; i++) {
				for (j = 0; j < 8; j++) {
					if ((dataBlock->YCbCr_4_2_0_capability_map.bitmap[i] & (1 << j)) != 0)
						dprintf("\tVSD Index %u\n", i * 8 + j);
				}
			}
			break;
		default:
			dprintf("unknown tag 0x%x\n", tagCode);
			break;

	}

}


void
edid_dump(edid1_info *edid)
{
	int i, j;

	dprintf("EDID info:\n");
	dprintf("  EDID version: %d.%d\n", edid->version.version, edid->version.revision);
	dprintf("  Vendor: %s Product ID: 0x%x\n", edid->vendor.manufacturer, edid->vendor.prod_id);
	dprintf("  Serial #: %" B_PRIu32 "\n", edid->vendor.serial);
	dprintf("  Produced in week/year: %u/%u\n", edid->vendor.week, edid->vendor.year);


	dprintf("  Type: %s\n", edid->display.input_type != 0 ? "Digital" : "Analog");
	if (edid->display.input_type != 0 && edid->version.revision >= 4) {
		dprintf("  Digital Bit Depth: %d\n", edid->display.digital_params.bit_depth);
		switch (edid->display.digital_params.interface) {
			case 0x1: dprintf("  DVI interface\n"); break;
			case 0x2: dprintf("  HDMIa interface\n"); break;
			case 0x3: dprintf("  HDMIb interface\n"); break;
			case 0x4: dprintf("  MDDI interface\n"); break;
			case 0x5: dprintf("  DisplayPort interface\n"); break;
		}
	}
	dprintf("  Size: %d cm x %d cm\n", edid->display.h_size,
		edid->display.v_size);
	dprintf("  Gamma=%.2f\n", (edid->display.gamma + 100) / 100.0);
	if (edid->display.gtf_supported) {
		if (edid->version.revision >= 4)
			dprintf("  Display supports continuous frequency\n");
		else
			dprintf("  GTF Timings supported\n");
	}
	dprintf("  Red (X,Y)=(%.4f,%.4f)\n", edid->display.red_x / 1024.0,
		edid->display.red_y / 1024.0);
	dprintf("  Green (X,Y)=(%.4f,%.4f)\n", edid->display.green_x / 1024.0,
		edid->display.green_y / 1024.0);
	dprintf("  Blue (X,Y)=(%.4f,%.4f)\n", edid->display.blue_x / 1024.0,
		edid->display.blue_y / 1024.0);
	dprintf("  White (X,Y)=(%.4f,%.4f)\n", edid->display.white_x / 1024.0,
		edid->display.white_y / 1024.0);

	dprintf("Supported Future Video Modes:\n");
	for (i = 0; i < EDID1_NUM_STD_TIMING; ++i) {
		if (edid->std_timing[i].h_size <= 256)
			continue;

		dprintf("%dx%d@%dHz (id=%d)\n",
			edid->std_timing[i].h_size, edid->std_timing[i].v_size,
			edid->std_timing[i].refresh, edid->std_timing[i].id);
	}

	dprintf("Supported VESA Video Modes:\n");
	if (edid->established_timing.res_720x400x70)
		dprintf("720x400@70Hz\n");
	if (edid->established_timing.res_720x400x88)
		dprintf("720x400@88Hz\n");
	if (edid->established_timing.res_640x480x60)
		dprintf("640x480@60Hz\n");
	if (edid->established_timing.res_640x480x67)
		dprintf("640x480@67Hz\n");
	if (edid->established_timing.res_640x480x72)
		dprintf("640x480@72Hz\n");
	if (edid->established_timing.res_640x480x75)
		dprintf("640x480@75Hz\n");
	if (edid->established_timing.res_800x600x56)
		dprintf("800x600@56Hz\n");
	if (edid->established_timing.res_800x600x60)
		dprintf("800x600@60Hz\n");

	if (edid->established_timing.res_800x600x72)
		dprintf("800x600@72Hz\n");
	if (edid->established_timing.res_800x600x75)
		dprintf("800x600@75Hz\n");
	if (edid->established_timing.res_832x624x75)
		dprintf("832x624@75Hz\n");
	if (edid->established_timing.res_1024x768x87i)
		dprintf("1024x768@87Hz interlaced\n");
	if (edid->established_timing.res_1024x768x60)
		dprintf("1024x768@60Hz\n");
	if (edid->established_timing.res_1024x768x70)
		dprintf("1024x768@70Hz\n");
	if (edid->established_timing.res_1024x768x75)
		dprintf("1024x768@75Hz\n");
	if (edid->established_timing.res_1280x1024x75)
		dprintf("1280x1024@75Hz\n");

	if (edid->established_timing.res_1152x870x75)
		dprintf("1152x870@75Hz\n");

	for (i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; ++i) {
		edid1_detailed_monitor *monitor = &edid->detailed_monitor[i];

		switch(monitor->monitor_desc_type) {
			case EDID1_SERIAL_NUMBER:
				dprintf("Serial Number: %s\n", monitor->data.serial_number);
				break;

			case EDID1_ASCII_DATA:
				dprintf("Ascii Data: %s\n", monitor->data.ascii_data);
				break;

			case EDID1_MONITOR_RANGES:
			{
				edid1_monitor_range monitor_range = monitor->data.monitor_range;

				dprintf("Horizontal frequency range = %d..%d kHz\n",
					monitor_range.min_h, monitor_range.max_h);
				dprintf("Vertical frequency range = %d..%d Hz\n",
					monitor_range.min_v, monitor_range.max_v);
				dprintf("Maximum pixel clock = %d MHz\n",
					(uint16)monitor_range.max_clock * 10);
				break;
			}

			case EDID1_MONITOR_NAME:
				dprintf("Monitor Name: %s\n", monitor->data.monitor_name);
				break;

			case EDID1_ADD_COLOUR_POINTER:
			{
				for (j = 0; j < EDID1_NUM_EXTRA_WHITEPOINTS; ++j) {
					edid1_whitepoint *whitepoint = &monitor->data.whitepoint[j];

					if (whitepoint->index == 0)
						continue;

					dprintf("Additional whitepoint: (X,Y)=(%f,%f) gamma=%f "
						"index=%i\n", whitepoint->white_x / 1024.0,
						whitepoint->white_y / 1024.0,
						(whitepoint->gamma + 100) / 100.0, whitepoint->index);
				}
				break;
			}

			case EDID1_ADD_STD_TIMING:
			{
				for (j = 0; j < EDID1_NUM_EXTRA_STD_TIMING; ++j) {
					edid1_std_timing *timing = &monitor->data.std_timing[j];

					if (timing->h_size <= 256)
						continue;

					dprintf("%dx%d@%dHz (id=%d)\n",
						timing->h_size, timing->v_size,
						timing->refresh, timing->id);
				}
				break;
			}

			case EDID1_IS_DETAILED_TIMING:
			{
				edid1_detailed_timing *timing = &monitor->data.detailed_timing;
				edid_timing_dump(timing, true);
				break;
			}
		}
	}

	if (edid->num_sections > 0)
		dprintf("Extension blocks: %u\n", edid->num_sections);
	if (edid->cta_block.tag == 0x2) {
		dprintf("CTA extension block\n");
		dprintf(" Extension version: %u\n", edid->cta_block.revision);
		dprintf(" Number of native detailed modes: %u\n", edid->cta_block.num_native_detailed);
		if (edid->cta_block.ycbcr422_supported)
			dprintf(" Supports YCbCr 4:2:2\n");
		if (edid->cta_block.ycbcr444_supported)
			dprintf(" Supports YCbCr 4:4:4\n");
		if (edid->cta_block.audio_supported)
			dprintf(" Basic audio support\n");
		if (edid->cta_block.underscan)
			dprintf(" Underscans PC formats by default\n");
		if (edid->cta_block.revision >= 3) {
			for (i = 0; i < edid->cta_block.num_data_blocks; i++) {
				edid_cta_datablock_dump(&edid->cta_block.data_blocks[i]);
			}
		}
		for (i = 0; i < 6; i++)
			edid_timing_dump(&edid->cta_block.detailed_timing[i], false);
	}
	if (edid->displayid_block.tag == 0x70) {
		dprintf("DisplayID extension block\n");
		dprintf(" Extension version: %u\n", edid->displayid_block.version);
		dprintf(" Extension count: %u\n", edid->displayid_block.extension_count);
	}
}
