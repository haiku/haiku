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


// analog input parameters
typedef struct _PACKED {
	BBITFIELD8_7 (
		input_type : 1,		// 0 : analog, 1 : digital
		input_voltage : 2,	// 0=0.7V/0.3V, 1=0.714V/0.286,
							// 2=1V/0.4V, 3=0.7V/0V
		setup : 1,			// true if voltage configurable
		sep_sync : 1,
		comp_sync : 1,
		sync_on_green : 1,
		sync_serr : 1
	);
} edid1_analog_params_raw;


// digital input parameters
typedef struct _PACKED {
	BBITFIELD8_3 (
		input_type : 1,	// 0 : analog, 1 : digital
		bit_depth : 3,	// 0=undefined, 1=6,2=8,3=10,4=12,5=14,6=16,7=reserved
		interface : 4	// 0=undefined, 1=DVI, 2=HDMIa, 3=HDMIb
						// 4=MDDI, 5=DisplayPort
	);
} edid1_digital_params_raw;


// display info
typedef struct _PACKED {
	union {
		edid1_analog_params_raw	analog_params;
		edid1_digital_params_raw digital_params;
	};
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

typedef union _PACKED {
	struct _PACKED {
		BBITFIELD8_3 (
			reserved : 1,
			format_code : 4,
			reserved2 : 3
		);
	} descr;
	struct _PACKED {
		BBITFIELD8_3 (
			reserved : 1,
			format_code : 4,
			max_channels : 3
		);
		BBITFIELD8_8 (
			reserved2 : 1,
			can_192khz : 1,
			can_176khz : 1,
			can_96khz : 1,
			can_88khz : 1,
			can_48khz : 1,
			can_44khz : 1,
			can_32khz : 1
		);
		BBITFIELD8_4 (
			reserved3 : 5,
			can_24bit : 1,
			can_20bit : 1,
			can_16bit : 1
		);
	} descr_1;
	struct _PACKED {
		BBITFIELD8_3 (
			reserved : 1,
			format_code : 4,
			max_channels : 3
		);
		BBITFIELD8_8 (
			reserved2 : 1,
			can_192khz : 1,
			can_176khz : 1,
			can_96khz : 1,
			can_88khz : 1,
			can_48khz : 1,
			can_44khz : 1,
			can_32khz : 1
		);
		uint8 maximum_bitrate;
	} descr_2_8;

} audio_descr;

#if !defined(__GNUC__) || __GNUC__ < 3
#define FLEXIBLE_ARRAY_LENGTH 0
#else
#define FLEXIBLE_ARRAY_LENGTH
#endif
typedef struct _PACKED {
	BBITFIELD8_2 (
		tag_code : 3,
		length : 5
	);
	union _PACKED {
		uint8 extended_tag_code;

		struct _PACKED {
			uint8 vic0;
			uint8 vic[FLEXIBLE_ARRAY_LENGTH];
		} video;

		struct _PACKED {
			audio_descr desc0;
			audio_descr desc[FLEXIBLE_ARRAY_LENGTH];
		} audio;

		struct _PACKED {
			uint8 ouinum0;
			uint8 ouinum1;
			uint8 ouinum2;
			union _PACKED {
				struct _PACKED {
					struct _PACKED {
						BBITFIELD8_2 (
							a : 4,
							b : 4
						);
						BBITFIELD8_2 (
							c : 4,
							d : 4
						);
					} source_physical_address;
					BBITFIELD8_7 (
						supports_ai : 1,
						dc_48bit : 1,
						dc_36bit : 1,
						dc_30bit : 1,
						dc_y444 : 1,
						reserved : 2,
						dvi_dual : 1
					);
					uint8 max_tmds_clock;
					uint8 reserved2[2];
					BBITFIELD8_2 (
						vic_length : 3,
						length_3d : 5
					);
					uint8 vic[FLEXIBLE_ARRAY_LENGTH];
				} hdmi;
				struct _PACKED {
					uint8 version;
					uint8 max_tmds_rate;
					BBITFIELD8_8 (
						scdc_present : 1,
						scdc_read_request_capable : 1,
						supports_cable_status : 1,
						supports_color_content_bits : 1,
						supports_scrambling : 1,
						supports_3d_independent : 1,
						supports_3d_dual_view : 1,
						supports_3d_osd_disparity : 1
					);
					BBITFIELD8_5 (
						max_frl_rate : 4,
						supports_uhd_vic : 1,
						supports_16bit_deep_color_4_2_0 : 1,
						supports_12bit_deep_color_4_2_0 : 1,
						supports_10bit_deep_color_4_2_0 : 1
					);
				} hdmi_forum;
			};
		} vendor_specific;

		struct _PACKED {
			BBITFIELD8_8 (
				FLW_FRW : 1,
				RLC_RRC : 1,
				FLC_FRC : 1,
				BC : 1,
				BL_BR : 1,
				FC : 1,
				LFE : 1,
				FL_FR : 1
			);
			BBITFIELD8_8 (
				TpSiL_TpSiR : 1,
				SiL_SiR : 1,
				TpBC : 1,
				LFE2 : 1,
				LS_RS : 1,
				TpFC : 1,
				TpC : 1,
				TpFL_TpFH : 1
			);
			BBITFIELD8_6 (
				reserved: 3,
				LSd_RSd : 1,
				TpLS_TpRS : 1,
				BtFL_BtFR : 1,
				BtFC : 1,
				TpBL_TpBR : 1
			);
		} speaker_allocation_map;

		struct _PACKED {
			uint8 extended_tag_code;
			BBITFIELD8_8 (
				BT2020RGB : 1,
				BT2020YCC : 1,
				BT2020cYCC : 1,
				opRGB : 1,
				opYCC601 : 1,
				sYCC601 : 1,
				xvYCC709 : 1,
				xvYCC601 : 1
			);
			BBITFIELD8_2 (
				DCIP3 : 1,
				reserved : 7
			);
		} colorimetry;

		struct _PACKED {
			uint8 extended_tag_code;
			uint8 bitmap[FLEXIBLE_ARRAY_LENGTH];
		} YCbCr_4_2_0_capability_map;

		struct _PACKED {
			uint8 extended_tag_code;
			BBITFIELD8_5 (
				QY : 1,
				QS : 1,
				S_PT : 2,
				S_IT : 2,
				S_CE : 2
			);
		} video_capability;

		struct _PACKED {
			uint8 extended_tag_code;
			BBITFIELD8_7 (
				reserved : 2,
				ET_5 : 1,
				ET_4 : 1,
				ET_3 : 1,
				ET_2 : 1,
				ET_1 : 1,
				ET_0 : 1
			);
			BBITFIELD8_8 (
				SM_7 : 1,
				SM_6 : 1,
				SM_5 : 1,
				SM_4 : 1,
				SM_3 : 1,
				SM_2 : 1,
				SM_1 : 1,
				SM_0 : 1
			);
			uint8 desired_content_max_luminance;
			uint8 desired_content_max_frame_average_luminance;
			uint8 desired_content_min_luminance;
		} hdr_static_metadata;

		struct _PACKED {
			uint8 extended_tag_code;
			// TODO extend
		} hdr_dyn_metadata;

		uint8 buffer[127];
	};
} cta_data_block;

typedef struct _PACKED {
	uint8 tag;
	uint8 revision;
	uint8 offset;
	BBITFIELD8_5 (
		underscan : 1,
		audio : 1,
		ycbcr444 : 1,
		ycbcr422 : 1,
		num_native_detailed : 4
	);
	cta_data_block data_blocks[0];

	uint8 reserved[123];
	uint8 check_sum;								// 1 byte
} cta_raw;											// total: 128 bytes

typedef struct _PACKED {
	uint8 tag;
	uint8 version;
	uint8 length;
	uint8 reserved;
	uint8 extension_count;
	uint8 reserved2[123];
} displayid_raw;									// total: 128 bytes

#endif
