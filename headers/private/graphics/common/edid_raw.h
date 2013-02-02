/*
 * Copyright 2003, Thomas Kurschel. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2006-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	Thomas Kurschel
 *	Bill Randle, billr@neocat.org
 */
#ifndef _EDID_RAW_H
#define _EDID_RAW_H


#include "bendian_bitfield.h"


/*!	Raw EDID data block.
	
	Raw data are packed in a really weird way. Never even
	think about using it directly, instead translate it via decode_edid()
	first.
*/

#define EDID1_NUM_DETAILED_MONITOR_DESC 4
#define EDID1_NUM_STD_TIMING 8
#define EDID1_NUM_EXTRA_STD_TIMING 6
#define EDID1_EXTRA_STRING_LEN 13
#define EDID1_NUM_EXTRA_WHITEPOINTS 2


// header 
typedef struct _PACKED {
	int8 pad[8];		// contains 0, -1, -1, -1, -1, -1, -1, 0
} edid1_header_raw;


// vendor info
typedef struct _PACKED {
	BBITFIELD8_3 (		// manufacturer
		pad : 1,
		c1 : 5,			// add '@' to get ascii
		c2_high : 2
	);
	BBITFIELD8_2 (
		c2_low : 3,
		c3 : 5
	);
	uint16 prod_id;
	uint32 serial;
	uint8 week;
	uint8 year;			// x+1990
} edid1_vendor_raw;


// version info
typedef struct _PACKED {
	uint8 version;
	uint8 revision;
} edid1_version_raw;


// display info
typedef struct _PACKED {
	BBITFIELD8_7 ( 
		input_type : 1,		// 1 : digital
		input_voltage : 2,	// 0=0.7V/0.3V, 1=0.714V/0.286, 
							// 2=1V/0.4V, 3=0.7V/0V
		setup : 1,			// true if voltage configurable
		sep_sync : 1,
		comp_sync : 1,
		sync_on_green : 1,
		sync_serr : 1
	);
	uint8 h_size;
	uint8 v_size;
	uint8 gamma;	// (x+100)/100
	BBITFIELD8_7 (
		dpms_standby : 1,
		dpms_suspend : 1,
		dpms_off : 1,
		display_type : 2,	// 0=mono, 1=rgb, 2=multicolour
		// since EDID version 1.1
		std_colour_space : 1,
		preferred_timing_mode : 1,
		gtf_supported : 1
	);
	BBITFIELD8_4 (		// low bits of red_x etc.
		red_x_low : 2,
		red_y_low : 2,
		green_x_low : 2,
		green_y_low : 2
	);
	BBITFIELD8_4 (
		blue_x_low : 2,
		blue_y_low : 2,
		white_x_low : 2,
		white_y_low : 2
	);
	uint8 red_x;		// all colours are 0.10 fixed point
	uint8 red_y;
	uint8 green_x;
	uint8 green_y;
	uint8 blue_x;
	uint8 blue_y;
	uint8 white_x;
	uint8 white_y;
} edid1_display_raw;


// raw standard timing data
typedef union _PACKED {
	struct _PACKED {
		uint8 h_size;		// (x+31)*8
		BBITFIELD8_2 (
			ratio : 2,		// 0=1:1, 1=3/4, 2=4/5, 3=9/16
			refresh : 6		// (x+60)
		);
	} timing;
	uint16 id;
} edid1_std_timing_raw;


// list of supported fixed timings
typedef struct _PACKED {
	BBITFIELD8_8 (
		res_720x400x70 : 1,
		res_720x400x88 : 1,
		res_640x480x60 : 1,
		res_640x480x67 : 1,
		res_640x480x72 : 1,
		res_640x480x75 : 1,
		res_800x600x56 : 1,
		res_800x600x60 : 1
	);
	BBITFIELD8_8 (
		res_800x600x72 : 1,
		res_800x600x75 : 1,
		res_832x624x75 : 1,
		res_1024x768x87i : 1,
		res_1024x768x60 : 1,
		res_1024x768x70 : 1,
		res_1024x768x75 : 1,
		res_1280x1024x75 : 1
	);
	BBITFIELD8_2 (
		res_1152x870x75 : 1,
		pad : 7
	);
} edid1_established_timing;


// types of detailed monitor description
enum {
	EDID1_SERIAL_NUMBER = 0xff,
	EDID1_ASCII_DATA = 0xfe,
	EDID1_MONITOR_RANGES = 0xfd,
	EDID1_MONITOR_NAME = 0xfc,
	EDID1_ADD_COLOUR_POINTER = 0xfb,
	EDID1_ADD_STD_TIMING = 0xfa,
	EDID1_IS_DETAILED_TIMING = 1
};


// monitor frequency range
typedef struct _PACKED {
	uint8 min_v;
	uint8 max_v;
	uint8 min_h;
	uint8 max_h;
	uint8 max_clock;	// in 10 MHz (!)
} edid1_monitor_range;


// additional whitepoint
typedef struct _PACKED {
	uint8 index1;
	BBITFIELD8_3 (
		pad1 : 4,
		white_x1_low : 2,
		white_y1_low : 2
	);
	uint8 white_x1;
	uint8 white_y1;
	uint8 gamma1;	// (x+100)/100
	uint8 index2;
	BBITFIELD8_3 (
		pad2 : 4,
		white_x2_low : 2,
		white_y2_low : 2
	);
	uint8 white_x2;
	uint8 white_y2;
	uint8 gamma2;	// (x+100)/100
} edid1_whitepoint_raw;


// detailed timing description
typedef struct _PACKED {
	uint16 pixel_clock; // in 10 kHz (!)
	uint8 h_active;
	uint8 h_blank;
	BBITFIELD8_2 (
		h_active_high : 4,
		h_blank_high : 4
	);
	uint8 v_active;
	uint8 v_blank;
	BBITFIELD8_2 (
		v_active_high : 4,
		v_blank_high : 4
	);
	uint8 h_sync_off;
	uint8 h_sync_width;
	BBITFIELD8_2 (
		v_sync_off : 4,
		v_sync_width : 4
	);
	BBITFIELD8_4 (
		h_sync_off_high : 2,
		h_sync_width_high : 2,
		v_sync_off_high : 2,
		v_sync_width_high : 2
	);
	uint8 h_size;
	uint8 v_size;
	BBITFIELD8_2 (
		h_size_high : 4,
		v_size_high : 4
	);
	uint8 h_border;
	uint8 v_border;
	BBITFIELD8_5 (
		interlaced : 1,
		stereo : 2,		// upper bit set - left on sync
						// lower bit set - right on sync
		sync : 2,
		misc : 2,
		stereo_il : 1
	);
} edid1_detailed_timing_raw;


// detailed monitor description
typedef union _PACKED {
	edid1_detailed_timing_raw detailed_timing;
	struct _PACKED {
		uint8 zero_0[3];
		uint8 monitor_desc_type;
		uint8 zero_4;
		union _PACKED {
			uint8 serial_number[EDID1_EXTRA_STRING_LEN];
			uint8 ascii_data[EDID1_EXTRA_STRING_LEN];
			uint8 monitor_name[EDID1_EXTRA_STRING_LEN];
			edid1_monitor_range monitor_range;
			edid1_whitepoint_raw whitepoint;
			edid1_std_timing_raw std_timing[EDID1_NUM_EXTRA_STD_TIMING];
		} data;
	} extra;
} edid1_detailed_monitor_raw;


// raw EDID data
// everything is packed data, mixture of little endian and big endian
// and a bit brain dead overall - nothing your dad would be proud of
typedef struct _PACKED { 
	edid1_header_raw header; 						// 8 bytes
	edid1_vendor_raw vendor;						// 10 bytes
	edid1_version_raw version;						// 2 bytes
	edid1_display_raw display;						// 15 bytes
	edid1_established_timing established_timing;	// 3 bytes
	edid1_std_timing_raw std_timing[EDID1_NUM_STD_TIMING];
													// 8 a 2 bytes -> 16 bytes

	// since EDID version 1.2
	edid1_detailed_monitor_raw detailed_monitor[EDID1_NUM_DETAILED_MONITOR_DESC];	
													// 4 a 18 bytes -> 72 bytes

	uint8 num_sections; 							// 1 byte
	uint8 check_sum;								// 1 byte
} edid1_raw;										// total: 128 bytes

#endif
