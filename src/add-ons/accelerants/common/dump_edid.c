/*
 * Copyright 2003, Thomas Kurschel. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


/*!
	Part of DDC driver
	Dumps EDID content
*/


#include "edid.h"
#if !defined(_KERNEL_MODE) && !defined(_BOOT_MODE)
#	include "ddc_int.h"
#endif

#include <KernelExport.h>

#include <stdio.h>


void
edid_dump(edid1_info *edid)
{
	int i, j;

	dprintf("Vendor: %s\n", edid->vendor.manufacturer);
	dprintf("Product ID: %d\n", (int)edid->vendor.prod_id);
	dprintf("Serial #: %d\n", (int)edid->vendor.serial);
	dprintf("Produced in week/year: %d/%d\n", edid->vendor.week,
		edid->vendor.year);

	dprintf("EDID version: %d.%d\n", edid->version.version,
		edid->version.revision);

	dprintf("Type: %s\n", edid->display.input_type ? "Digital" : "Analog");
	dprintf("Size: %d cm x %d cm\n", edid->display.h_size,
		edid->display.v_size);
	dprintf("Gamma=%.3f\n", (edid->display.gamma + 100) / 100.0);
	dprintf("White (X,Y)=(%.3f,%.3f)\n", edid->display.white_x / 1024.0,
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
				if (timing->h_active + timing->h_blank == 0
					|| timing->v_active + timing->v_blank == 0) {
					dprintf("Invalid video mode (%dx%d)\n", timing->h_active,
						timing->v_active);
					continue;
				}
				dprintf("Additional Video Mode (%dx%d@%dHz):\n",
					timing->h_active, timing->v_active,
					(timing->pixel_clock * 10000
						/ (timing->h_active + timing->h_blank)
						/ (timing->v_active + timing->v_blank)));
					// Refresh rate = pixel clock in MHz / Htotal / Vtotal

				dprintf("clock=%f MHz\n", timing->pixel_clock / 100.0);
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
				break;
			}
		}
	}
}
