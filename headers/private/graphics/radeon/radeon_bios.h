/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon kernel driver
		
	BIOS data structures
*/

#ifndef _RADEON_BIOS_H
#define _RADEON_BIOS_H

typedef struct {
	uint8 clock_chip_type;
	uint8 struct_size;
	uint8 accelerator_entry;
	uint8 VGA_entry;
	uint16 VGA_table_offset;
	uint16 POST_table_offset;
	uint16 XCLK;
	uint16 MCLK;
	uint8 num_PLL_blocks;
	uint8 size_PLL_blocks;
	uint16 PCLK_ref_freq;
	uint16 PCLK_ref_divider;
	uint32 PCLK_min_freq;
	uint32 PCLK_max_freq;
	uint16 MCLK_ref_freq;
	uint16 MCLK_ref_divider;
	uint32 MCLK_min_freq;
	uint32 MCLK_max_freq;
	uint16 XCLK_ref_freq;
	uint16 XCLK_ref_divider;
	uint32 XCLK_min_freq;
	uint32 XCLK_max_freq;
} __attribute__ ((packed)) PLL_BLOCK;

typedef struct {
	uint8 dummy0;
	char name[24];				// 1
	uint16 panel_xres;			// 25
	uint16 panel_yres;			// 27
	
	uint8 dummy[15];
	
	uint16 panel_pwr_delay;		// 44
	uint16 ref_div;				// 46
	uint8 post_div;				// 48
	uint8 feedback_div;			// 49
	uint8 dummy2[14];
	
	uint16 fpi_timing_ofs[20];	// 64
} __attribute__ ((packed)) FPI_BLOCK;

typedef struct {
	uint16 panel_xres;			// 0
	uint16 panel_yres;			// 2
	uint8 dummy4[5];
	
	uint16 dot_clock;			// 9
	uint8 dummy11[6];

	uint16 h_total;				// 17
	uint16 h_display;			// 19
	uint16 h_sync_start;		// 21
	uint8 h_sync_width;			// 23
	
	uint16 v_total;				// 24
	uint16 v_display;			// 26
	uint16 v_sync;				// 28
} __attribute__ ((packed)) FPI_TIMING_BLOCK;

#endif
