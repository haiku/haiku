/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Rudolf Cornelissen 8/2004-5/2005
*/

#include "nv_std.h"

int fd;
shared_info *si;
area_id shared_info_area;
area_id dma_cmd_buf_area;
vuint32 *regs;
area_id regs_area;
display_mode *my_mode_list;
area_id	my_mode_list_area;
int accelerantIsClone;

crtc_interrupt_enable	head1_interrupt_enable;
crtc_update_fifo		head1_update_fifo;
crtc_validate_timing 	head1_validate_timing;
crtc_set_timing 		head1_set_timing;
crtc_depth				head1_depth;
crtc_dpms				head1_dpms;
crtc_set_display_pitch	head1_set_display_pitch;
crtc_set_display_start	head1_set_display_start;
crtc_cursor_init		head1_cursor_init;
crtc_cursor_show		head1_cursor_show;
crtc_cursor_hide		head1_cursor_hide;
crtc_cursor_define		head1_cursor_define;
crtc_cursor_position	head1_cursor_position;
crtc_stop_tvout			head1_stop_tvout;
crtc_start_tvout		head1_start_tvout;

crtc_interrupt_enable	head2_interrupt_enable;
crtc_update_fifo		head2_update_fifo;
crtc_validate_timing	head2_validate_timing;
crtc_set_timing			head2_set_timing;
crtc_depth				head2_depth;
crtc_dpms				head2_dpms;
crtc_set_display_pitch	head2_set_display_pitch;
crtc_set_display_start	head2_set_display_start;
crtc_cursor_init		head2_cursor_init;
crtc_cursor_show		head2_cursor_show;
crtc_cursor_hide		head2_cursor_hide;
crtc_cursor_define		head2_cursor_define;
crtc_cursor_position	head2_cursor_position;
crtc_stop_tvout			head2_stop_tvout;
crtc_start_tvout		head2_start_tvout;

dac_mode				head1_mode;
dac_palette				head1_palette;
dac_set_pix_pll			head1_set_pix_pll;
dac_pix_pll_find		head1_pix_pll_find;

dac_mode				head2_mode;
dac_palette				head2_palette;
dac_set_pix_pll			head2_set_pix_pll;
dac_pix_pll_find		head2_pix_pll_find;

nv_get_set_pci nv_pci_access=
	{
		NV_PRIVATE_DATA_MAGIC,
		0,
		4,
		0
	};

nv_in_out_isa nv_isa_access=
	{
		NV_PRIVATE_DATA_MAGIC,
		0,
		1,
		0
	};
