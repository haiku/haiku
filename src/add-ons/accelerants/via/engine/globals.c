/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Rudolf Cornelissen 8/2004
*/

#include "std.h"

int fd;
shared_info *si;
area_id shared_info_area;
vuint32 *regs;
area_id regs_area;
display_mode *my_mode_list;
area_id	my_mode_list_area;
int accelerantIsClone;

crtc_validate_timing 	head1_validate_timing;
crtc_set_timing 		head1_set_timing;
crtc_depth				head1_depth;
crtc_dpms				head1_dpms;
crtc_dpms_fetch			head1_dpms_fetch;
crtc_set_display_pitch	head1_set_display_pitch;
crtc_set_display_start	head1_set_display_start;
crtc_cursor_init		head1_cursor_init;
crtc_cursor_show		head1_cursor_show;
crtc_cursor_hide		head1_cursor_hide;
crtc_cursor_define		head1_cursor_define;
crtc_cursor_position	head1_cursor_position;

crtc_validate_timing	head2_validate_timing;
crtc_set_timing			head2_set_timing;
crtc_depth				head2_depth;
crtc_dpms				head2_dpms;
crtc_dpms_fetch			head2_dpms_fetch;
crtc_set_display_pitch	head2_set_display_pitch;
crtc_set_display_start	head2_set_display_start;
crtc_cursor_init		head2_cursor_init;
crtc_cursor_show		head2_cursor_show;
crtc_cursor_hide		head2_cursor_hide;
crtc_cursor_define		head2_cursor_define;
crtc_cursor_position	head2_cursor_position;

dac_mode				head1_mode;
dac_palette				head1_palette;
dac_set_pix_pll			head1_set_pix_pll;
dac_pix_pll_find		head1_pix_pll_find;

dac_mode				head2_mode;
dac_palette				head2_palette;
dac_set_pix_pll			head2_set_pix_pll;
dac_pix_pll_find		head2_pix_pll_find;

eng_get_set_pci eng_pci_access=
	{
		VIA_PRIVATE_DATA_MAGIC,
		0,
		4,
		0
	};

eng_in_out_isa eng_isa_access=
	{
		VIA_PRIVATE_DATA_MAGIC,
		0,
		1,
		0
	};
