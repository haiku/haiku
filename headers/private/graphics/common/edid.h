/*
 * Copyright 2003, Thomas Kurschel. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _EDID_H
#define _EDID_H

//!	Extended Display Identification Data (EDID)

#include "edid_raw.h"


// vendor info
typedef struct {
	char manufacturer[4];
	uint16 prod_id;
	uint32 serial;
	uint8 week;
	uint16 year;
} edid1_vendor;

// version info
typedef struct {
	uint8 version;
	uint8 revision;
} edid1_version;

// display info
typedef struct {
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
	uint16 red_x;		// all colours are 0.10 fixed point
	uint16 red_y;
	uint16 green_x;
	uint16 green_y;
	uint16 blue_x;
	uint16 blue_y;
	uint16 white_x;
	uint16 white_y;
} edid1_display;

// standard timing data
typedef struct {
	uint16 h_size;
	uint16 v_size;
	uint16 id;
	uint8 ratio;
	uint8 refresh;
} edid1_std_timing;

// additional whitepoint
typedef struct {
	uint8 index;
	uint16 white_x;
	uint16 white_y;
	uint8 gamma;	// (x+100)/100
} edid1_whitepoint;

// detailed timing description
typedef struct {
	uint16 pixel_clock; // in 10 kHz
	uint16 h_active;
	uint16 h_blank;
	uint16 v_active;
	uint16 v_blank;
	uint16 h_sync_off;
	uint16 h_sync_width;
	uint16 v_sync_off;
	uint16 v_sync_width;
	uint16 h_size;
	uint16 v_size;
	uint16 h_border;
	uint16 v_border;
	BBITFIELD8_4 (
		interlaced : 1,
		stereo : 2,		// upper bit set - left on sync
						// lower bit set - right on sync
		sync : 2,
		misc : 2
	);
} edid1_detailed_timing;

// detailed monitor description
typedef struct {
	uint8 monitor_desc_type;
	union {
		char serial_number[EDID1_EXTRA_STRING_LEN];
		char ascii_data[EDID1_EXTRA_STRING_LEN];
		edid1_monitor_range monitor_range;
		char monitor_name[EDID1_EXTRA_STRING_LEN];
		edid1_whitepoint whitepoint[EDID1_NUM_EXTRA_WHITEPOINTS];
		edid1_std_timing std_timing[EDID1_NUM_EXTRA_STD_TIMING];
		edid1_detailed_timing detailed_timing;
	} data;
} edid1_detailed_monitor;

// EDID data block
typedef struct edid1_info {  
	edid1_vendor vendor;
	edid1_version version;
	edid1_display display;	
	edid1_established_timing established_timing;
	edid1_std_timing std_timing[EDID1_NUM_STD_TIMING];

	// since EDID version 1.2
	edid1_detailed_monitor detailed_monitor[EDID1_NUM_DETAILED_MONITOR_DESC];

	uint8 num_sections;
} edid1_info;

#define EDID_VERSION_1 1

#ifdef __cplusplus
extern "C" {
#endif

void edid_decode(edid1_info *edid, const edid1_raw *raw);
void edid_dump(edid1_info *edid);

#ifdef __cplusplus
}
#endif

#endif	// _EDID_H
