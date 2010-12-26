/*
 * Copyright 2003, Thomas Kurschel. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


/*!
	Part of DDC driver

	EDID decoder.

	The EDID information is tightly packed; this file takes care of
	converting it to a usable structure.
*/


#include "edid.h"

#include <KernelExport.h>

#include <string.h>


//
// from hereon a bunch of decoders follow for each EDID section
//

static void
decode_vendor(edid1_vendor *vendor, const edid1_vendor_raw *raw)
{
	vendor->manufacturer[0] = raw->c1 + '@';
	vendor->manufacturer[1] = ((raw->c2_high << 3) | raw->c2_low) + '@';
	vendor->manufacturer[2] = raw->c3 + '@';
	vendor->manufacturer[3] = 0;
	vendor->prod_id = B_LENDIAN_TO_HOST_INT16(raw->prod_id);
	vendor->serial = B_LENDIAN_TO_HOST_INT32(raw->serial);
	vendor->week = raw->week;
	vendor->year = raw->year + 1990;
}


static void
decode_version(edid1_version *version, const edid1_version_raw *raw)
{
	version->version = raw->version;
	version->revision = raw->revision;	
}


static void
decode_display(edid1_display *display, const edid1_display_raw *raw)
{	
	display->input_type = raw->input_type;
	display->input_voltage = raw->input_voltage;
	display->setup = raw->setup;
	display->sep_sync = raw->sep_sync;
	display->comp_sync = raw->comp_sync;
	display->sync_on_green = raw->sync_on_green;
	display->sync_serr = raw->sync_serr;

	display->h_size = raw->h_size;
	display->v_size = raw->v_size;
	display->gamma = raw->gamma;

	display->dpms_standby = raw->dpms_standby;
	display->dpms_suspend = raw->dpms_suspend;
	display->dpms_off = raw->dpms_off;
	display->display_type = raw->display_type;
	display->std_colour_space = raw->std_colour_space;
	display->preferred_timing_mode = raw->preferred_timing_mode;
	display->gtf_supported = raw->gtf_supported;

	display->red_x = ((uint16)raw->red_x << 2) | raw->red_x_low;
	display->red_y = ((uint16)raw->red_y << 2) | raw->red_y_low;
	display->green_x = ((uint16)raw->green_x << 2) | raw->green_x_low;
	display->green_y = ((uint16)raw->green_y << 2) | raw->green_y_low;
	display->blue_x = ((uint16)raw->blue_x << 2) | raw->blue_x_low;
	display->blue_y = ((uint16)raw->blue_y << 2) | raw->blue_y_low;
	display->white_x = ((uint16)raw->white_x << 2) | raw->white_x_low;
	display->white_y = ((uint16)raw->white_y << 2) | raw->white_y_low;
}


static void
decode_std_timing(edid1_std_timing *timing, const edid1_std_timing_raw *raw)
{
	timing->h_size = (raw->timing.h_size + 31) * 8;
	timing->ratio = raw->timing.ratio;

	switch (raw->timing.ratio) {
		case 0:
			timing->v_size = timing->h_size;
			break;

		case 1:
			timing->v_size = timing->h_size * 3 / 4;
			break;

		case 2:
			timing->v_size = timing->h_size * 4 / 5;
			break;

		case 3:
			timing->v_size = timing->h_size * 9 / 16;
			break;
	}
	timing->refresh = raw->timing.refresh + 60;
	timing->id = raw->id;
}


static void
decode_whitepoint(edid1_whitepoint *whitepoint, const edid1_whitepoint_raw *raw)
{
	whitepoint[0].index = raw->index1;
	whitepoint[0].white_x = ((uint16)raw->white_x1 << 2) | raw->white_x1_low;
	whitepoint[0].white_y = ((uint16)raw->white_y1 << 2) | raw->white_y1_low;
	whitepoint[0].gamma = raw->gamma1;

	whitepoint[1].index = raw->index2;
	whitepoint[1].white_x = ((uint16)raw->white_x2 << 2) | raw->white_x2_low;
	whitepoint[1].white_y = ((uint16)raw->white_y2 << 2) | raw->white_y2_low;
	whitepoint[1].gamma = raw->gamma2;
}


static void
decode_detailed_timing(edid1_detailed_timing *timing,
	const edid1_detailed_timing_raw *raw)
{
	timing->pixel_clock = raw->pixel_clock;
	timing->h_active = ((uint16)raw->h_active_high << 8) | raw->h_active;
	timing->h_blank = ((uint16)raw->h_blank_high << 8) | raw->h_blank;
	timing->v_active = ((uint16)raw->v_active_high << 8) | raw->v_active;
	timing->v_blank = ((uint16)raw->v_blank_high << 8) | raw->v_blank;
	timing->h_sync_off = ((uint16)raw->h_sync_off_high << 8) | raw->h_sync_off;
	timing->h_sync_width = ((uint16)raw->h_sync_width_high << 8) | raw->h_sync_width;
	timing->v_sync_off = ((uint16)raw->v_sync_off_high << 4) | raw->v_sync_off;
	timing->v_sync_width = ((uint16)raw->v_sync_width_high << 4) | raw->v_sync_width;
	timing->h_size = ((uint16)raw->h_size_high << 8) | raw->h_size;
	timing->v_size = ((uint16)raw->v_size_high << 8) | raw->v_size;
	timing->h_border = raw->h_border;
	timing->v_border = raw->v_border;
	timing->interlaced = raw->interlaced;
	timing->stereo = raw->stereo;
	timing->sync = raw->sync;
	timing->misc = raw->misc;
}


//! copy string until 0xa, removing trailing spaces
static void
copy_str(char *dest, const uint8 *src, size_t len)
{
	uint32 i;

	// copy until 0xa
	for (i = 0; i < len; i++) {
		if (*src == 0xa)
			break;

		*dest++ = *src++;
	}

	// remove trailing spaces
	while (i-- > 0) {
		if (*(dest - 1) != ' ')
			break;

		dest--;
	}

	*dest = '\0';
}


static void
decode_detailed_monitor(edid1_detailed_monitor *monitor,
	const edid1_detailed_monitor_raw *raw, bool enableExtra)
{
	int i, j;

	for (i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; ++i, ++monitor, ++raw) {

		// workaround: normally, all four bytes must be zero for detailed
		// description, but at least some Formac monitors violate that:
		// they have some additional info that start at zero_4(!),
		// so even if only the first two _or_ the other two bytes are
		// zero, we accept it as a monitor description block
		if (enableExtra
			&& ((raw->extra.zero_0[0] == 0 && raw->extra.zero_0[1] == 0)
				|| (raw->extra.zero_0[2] == 0 && raw->extra.zero_4 == 0))) {
			monitor->monitor_desc_type = raw->extra.monitor_desc_type;

			switch (raw->extra.monitor_desc_type) {
				case EDID1_SERIAL_NUMBER:
					copy_str(monitor->data.serial_number, 
						raw->extra.data.serial_number, EDID1_EXTRA_STRING_LEN);
					break;

				case EDID1_ASCII_DATA:
					copy_str(monitor->data.ascii_data, 
						raw->extra.data.ascii_data, EDID1_EXTRA_STRING_LEN);
					break;

				case EDID1_MONITOR_RANGES:
					monitor->data.monitor_range = raw->extra.data.monitor_range;
					break;

				case EDID1_MONITOR_NAME:
					copy_str(monitor->data.monitor_name, 
						raw->extra.data.monitor_name, EDID1_EXTRA_STRING_LEN);
					break;

				case EDID1_ADD_COLOUR_POINTER:
					decode_whitepoint(monitor->data.whitepoint, 
						&raw->extra.data.whitepoint);
					break;

				case EDID1_ADD_STD_TIMING:
					for (j = 0; j < EDID1_NUM_EXTRA_STD_TIMING; ++j) {
						decode_std_timing(&monitor->data.std_timing[j],
							&raw->extra.data.std_timing[j]);
					}
					break;
			}
		} else if (raw->detailed_timing.pixel_clock > 0) {
			monitor->monitor_desc_type = EDID1_IS_DETAILED_TIMING;
			decode_detailed_timing(&monitor->data.detailed_timing,
				&raw->detailed_timing);
		}
	}
}


//	#pragma mark -


//!	Main function to decode edid data
void
edid_decode(edid1_info *edid, const edid1_raw *raw)
{
	int i;
	memset(edid, 0, sizeof(edid1_info));

	decode_vendor(&edid->vendor, &raw->vendor);
	decode_version(&edid->version, &raw->version);
	decode_display(&edid->display, &raw->display);	

	edid->established_timing = raw->established_timing;

	for (i = 0; i < EDID1_NUM_STD_TIMING; ++i) {
		decode_std_timing(&edid->std_timing[i], &raw->std_timing[i]);
	}

	decode_detailed_monitor(edid->detailed_monitor, raw->detailed_monitor,
		edid->version.version == 1 && edid->version.revision >= 1);
}
