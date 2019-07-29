
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Apsed,
	Rudolf Cornelissen 11/2002-9/2005
*/

#define MODULE_BIT 0x00200000

#include "acc_std.h"

/*
	Enable/Disable interrupts.  Just a wrapper around the
	ioctl() to the kernel driver.
*/
static void interrupt_enable(bool flag) {
	status_t result;
	eng_set_bool_state sbs;

	/* set the magic number so the driver knows we're for real */
	sbs.magic = VIA_PRIVATE_DATA_MAGIC;
	sbs.do_it = flag;
	/* contact driver and get a pointer to the registers and shared data */
	result = ioctl(fd, ENG_RUN_INTERRUPTS, &sbs, sizeof(sbs));
}

/* First validate the mode, then call lots of bit banging stuff to set the mode(s)! */
status_t SET_DISPLAY_MODE(display_mode *mode_to_set) 
{
	/* BOUNDS WARNING:
	 * It's impossible to deviate whatever small amount in a display_mode if the lower
	 * and upper limits are the same!
	 * Besides:
	 * BeOS (tested R5.0.3PE) is failing BWindowScreen::SetFrameBuffer() if PROPOSEMODE
	 * returns B_BAD_VALUE!
	 * Which means PROPOSEMODE should not return that on anything except on
	 * deviations for:
	 * display_mode.virtual_width;
	 * display_mode.virtual_height;
	 * display_mode.timing.h_display;
	 * display_mode.timing.v_display;
	 * So:
	 * We don't use bounds here by making sure bounds and target are the same struct!
	 * (See the call to PROPOSE_DISPLAY_MODE below) */
	display_mode /*bounds,*/ target;

	uint8 colour_depth1 = 32;
//	status_t result;
	uint32 startadd,startadd_right;
	bool display, h, v;
//	bool crt1, crt2, cross;

	/* Adjust mode to valid one and fail if invalid */
	target /*= bounds*/ = *mode_to_set;
	/* show the mode bits */
	LOG(1, ("SETMODE: (ENTER) initial modeflags: $%08x\n", target.flags));
	LOG(1, ("SETMODE: requested target pixelclock %dkHz\n",  target.timing.pixel_clock));
	LOG(1, ("SETMODE: requested virtual_width %d, virtual_height %d\n",
										target.virtual_width, target.virtual_height));

	/* See BOUNDS WARNING above... */
	if (PROPOSE_DISPLAY_MODE(&target, &target, &target) == B_ERROR)	return B_ERROR;

	/* if not dualhead capable card clear dualhead flags */
	if (!(target.flags & DUALHEAD_CAPABLE))
	{
		target.flags &= ~DUALHEAD_BITS;
	}
	/* if not TVout capable card clear TVout flags */
	if (!(target.flags & TV_CAPABLE))
	{
		target.flags &= ~TV_BITS;
	}
	LOG(1, ("SETMODE: (CONT.) validated command modeflags: $%08x\n", target.flags));

	/* disable interrupts using the kernel driver */
	interrupt_enable(false);

	/* find current DPMS state, then turn off screen(s) */
	head1_dpms_fetch(&display, &h, &v);
	head1_dpms(false, false, false);
//	if (si->ps.secondary_head) head2_dpms(false, false, false);

	/*where in framebuffer the screen is (should this be dependant on previous MOVEDISPLAY?)*/
	startadd = (uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer;

	/* calculate and set new mode bytes_per_row */
	eng_general_validate_pic_size (&target, &si->fbc.bytes_per_row, &si->acc_mode);

	/*Perform the very long mode switch!*/
	if (target.flags & DUALHEAD_BITS) /*if some dualhead mode*/
	{
		uint8 colour_depth2 = colour_depth1;

		/* init display mode for secondary head */		
		display_mode target2 = target;

		LOG(1,("SETMODE: setting DUALHEAD mode\n"));

		/* validate flags for secondary TVout */
//		if ((i2c_sec_tv_adapter() != B_OK) && (target2.flags & TV_BITS))
//		{
//			target.flags &= ~TV_BITS;//still needed for some routines...
//			target2.flags &= ~TV_BITS;
//			LOG(1,("SETMODE: blocking TVout: no TVout cable connected!\n"));
//		}

		/* detect which connectors have a CRT connected */
		//fixme: 'hot-plugging' for analog monitors removed: remove code as well;
		//or make it work with digital panels connected as well.
//		crt1 = eng_dac_crt_connected();
//		crt2 = eng_dac2_crt_connected();
		/* connect outputs 'straight-through' */
//		if (crt1)
//		{
			/* connector1 is used as primary output */
//			cross = false;
//		}
//		else
//		{
//			if (crt2)
				/* connector2 is used as primary output */
//				cross = true;
//			else
				/* no CRT detected: assume connector1 is used as primary output */
//				cross = false;
//		}
		/* set output connectors assignment if possible */
//		if ((target.flags & DUALHEAD_BITS) == DUALHEAD_SWITCH)
			/* invert output assignment in switch mode */
//			eng_general_head_select(true);
//		else
//			eng_general_head_select(false);

		/* set the pixel clock PLL(s) */
		LOG(8,("SETMODE: target clock %dkHz\n",target.timing.pixel_clock));
		if (head1_set_pix_pll(target) == B_ERROR)
			LOG(8,("SETMODE: error setting pixel clock (internal DAC)\n"));

		/* we do not need to set the pixelclock here for a head that's in TVout mode */
//		if (!(target2.flags & TV_BITS))
//		{
//			LOG(8,("SETMODE: target2 clock %dkHz\n",target2.timing.pixel_clock));
//			if (head2_set_pix_pll(target2) == B_ERROR)
//				LOG(8,("SETMODE: error setting pixel clock (DAC2)\n"));
//		}

		/*set the colour depth for CRTC1 and the DAC */
		switch(target.space)
		{
		case B_CMAP8:
			colour_depth1 =  8;
			head1_mode(BPP8, 1.0);
			head1_depth(BPP8);
			break;
		case B_RGB15_LITTLE:
			colour_depth1 = 16;
			head1_mode(BPP15, 1.0);
			head1_depth(BPP15);
			break;
		case B_RGB16_LITTLE:
			colour_depth1 = 16;
			head1_mode(BPP16, 1.0);
			head1_depth(BPP16);
			break;
		case B_RGB32_LITTLE:
			colour_depth1 = 32;
			head1_mode(BPP32, 1.0);
			head1_depth(BPP32);
			break;
		}
		/*set the colour depth for CRTC2 and DAC2 */
		switch(target2.space)
		{
		case B_CMAP8:
			colour_depth2 =  8;
//			head2_mode(BPP8, 1.0);
//			head2_depth(BPP8);
			break;
		case B_RGB15_LITTLE:
			colour_depth2 = 16;
//			head2_mode(BPP15, 1.0);
//			head2_depth(BPP15);
			break;
		case B_RGB16_LITTLE:
			colour_depth2 = 16;
//			head2_mode(BPP16, 1.0);
//			head2_depth(BPP16);
			break;
		case B_RGB32_LITTLE:
			colour_depth2 = 32;
//			head2_mode(BPP32, 1.0);
//			head2_depth(BPP32);
			break;
		}

		/* check if we are doing interlaced TVout mode */
		si->interlaced_tv_mode = false;
/*		if ((target2.flags & TV_BITS) && (si->ps.card_type >= G450))
			si->interlaced_tv_mode = true;
*/
		/*set the display(s) pitches*/
		head1_set_display_pitch ();
		//fixme: seperate for real dualhead modes:
		//we need a secondary si->fbc!
//		head2_set_display_pitch ();

		/*work out where the "right" screen starts*/
		startadd_right = startadd + (target.timing.h_display * (colour_depth1 >> 3));

		/* Tell card what memory to display */
		switch (target.flags & DUALHEAD_BITS)
		{
		case DUALHEAD_ON:
		case DUALHEAD_SWITCH:
			head1_set_display_start(startadd,colour_depth1);
//			head2_set_display_start(startadd_right,colour_depth2);
			break;
		case DUALHEAD_CLONE:
			head1_set_display_start(startadd,colour_depth1);
//			head2_set_display_start(startadd,colour_depth2);
			break;
		}

		/* set the timing */
		head1_set_timing(target);
		/* we do not need to setup CRTC2 here for a head that's in TVout mode */
//		if (!(target2.flags & TV_BITS))	result = head2_set_timing(target2);

		/* TVout support: setup CRTC2 and it's pixelclock */
//		if (si->ps.tvout && (target2.flags & TV_BITS)) maventv_init(target2);
	}
	else /* single head mode */
	{
		status_t status;
		int colour_mode = BPP32;

		/* connect output */
		if (si->ps.secondary_head)
		{
			/* detect which connectors have a CRT connected */
			//fixme: 'hot-plugging' for analog monitors removed: remove code as well;
			//or make it work with digital panels connected as well.
//			crt1 = eng_dac_crt_connected();
//			crt2 = eng_dac2_crt_connected();
			/* connect outputs 'straight-through' */
//			if (crt1)
//			{
				/* connector1 is used as primary output */
//				cross = false;
//			}
//			else
//			{
//				if (crt2)
					/* connector2 is used as primary output */
//					cross = true;
//				else
					/* no CRT detected: assume connector1 is used as primary output */
//					cross = false;
//			}
			/* set output connectors assignment if possible */
			eng_general_head_select(false);
		}

		switch(target.space)
		{
		case B_CMAP8:        colour_depth1 =  8; colour_mode = BPP8;  break;
		case B_RGB15_LITTLE: colour_depth1 = 16; colour_mode = BPP15; break;
		case B_RGB16_LITTLE: colour_depth1 = 16; colour_mode = BPP16; break;
		case B_RGB32_LITTLE: colour_depth1 = 32; colour_mode = BPP32; break;
		default:
			LOG(8,("SETMODE: Invalid singlehead colour depth 0x%08x\n", target.space));
			return B_ERROR;
		}

		/* set the pixel clock PLL */
		status = head1_set_pix_pll(target);

		if (status==B_ERROR)
			LOG(8,("CRTC: error setting pixel clock (internal DAC)\n"));

		/* set the colour depth for CRTC1 and the DAC */
		/* first set the colordepth */
		head1_depth(colour_mode);
		/* then(!) program the PAL (<8bit colordepth does not support 8bit PAL) */
		head1_mode(colour_mode,1.0);

		/* set the display pitch */
		head1_set_display_pitch();

		/* tell the card what memory to display */
		head1_set_display_start(startadd,colour_depth1);

		/* set the timing */
		head1_set_timing(target);

		//fixme: shut-off the videoPLL if it exists...
	}

	/* update driver's mode store */
	si->dm = target;

	/* turn screen one on */
	head1_dpms(display, h, v);
	/* turn screen two on if a dualhead mode is active */
//	if (target.flags & DUALHEAD_BITS) head2_dpms(display,h,v);

	/* set up acceleration for this mode */
//	eng_acc_init();
	/* set up overlay unit for this mode */
	eng_bes_init();

	LOG(1,("SETMODE: booted since %f mS\n", system_time()/1000.0));

	/* enable interrupts using the kernel driver */
	interrupt_enable(true);

	/* optimize memory-access if needed */
//	head1_mem_priority(colour_depth1);

	/* Tune RAM CAS-latency if needed. Must be done *here*! */
	eng_set_cas_latency();

	return B_OK;
}

/*
	Set which pixel of the virtual frame buffer will show up in the
	top left corner of the display device.  Used for page-flipping
	games and virtual desktops.
*/
status_t MOVE_DISPLAY(uint16 h_display_start, uint16 v_display_start) {
	uint8 colour_depth;
	uint32 startadd,startadd_right;

	LOG(4,("MOVE_DISPLAY: h %d, v %d\n", h_display_start, v_display_start));

	/* VIA CRTC1 handles multiples of 8 for 8bit, 4 for 16bit, 2 for 32 bit 
	   VIA CRTC2 is yet unknown...
     */

	/* reset lower bits, don't return an error! */
	if (si->dm.flags & DUALHEAD_BITS)
	{
		//fixme for VIA...
		switch(si->dm.space)
		{
		case B_RGB16_LITTLE:
			colour_depth=16;
			h_display_start &= ~0x1f;
			break;
		case B_RGB32_LITTLE:
			colour_depth=32;
			h_display_start &= ~0x0f;
			break;
		default:
			LOG(8,("SET:Invalid DH colour depth 0x%08x, should never happen\n", si->dm.space));
			return B_ERROR;
		}
	}
	else
	{
		switch(si->dm.space)
		{
		case B_CMAP8:
			colour_depth=8;
			h_display_start &= ~0x07;
			break;
		case B_RGB15_LITTLE: case B_RGB16_LITTLE:
			colour_depth=16;
			h_display_start &= ~0x03;
			break;
		case B_RGB32_LITTLE:
			colour_depth=32;
			h_display_start &= ~0x01;
			break;
		default:
			return B_ERROR;
		}
	}

	/* do not run past end of display */
	switch (si->dm.flags & DUALHEAD_BITS)
	{
	case DUALHEAD_ON:
	case DUALHEAD_SWITCH:
		if (((si->dm.timing.h_display * 2) + h_display_start) > si->dm.virtual_width)
			return B_ERROR;
		break;
	default:
		if ((si->dm.timing.h_display + h_display_start) > si->dm.virtual_width)
			return B_ERROR;
		break;
	}
	if ((si->dm.timing.v_display + v_display_start) > si->dm.virtual_height)
		return B_ERROR;

	/* everybody remember where we parked... */
	si->dm.h_display_start = h_display_start;
	si->dm.v_display_start = v_display_start;

	/* actually set the registers */
	//fixme: seperate both heads: we need a secondary si->fbc!
	startadd = v_display_start * si->fbc.bytes_per_row;
	startadd += h_display_start * (colour_depth >> 3);
	startadd += (uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer;
	startadd_right = startadd + si->dm.timing.h_display * (colour_depth >> 3);

	interrupt_enable(false);

	switch (si->dm.flags & DUALHEAD_BITS)
	{
		case DUALHEAD_ON:
		case DUALHEAD_SWITCH:
			head1_set_display_start(startadd,colour_depth);
//			head2_set_display_start(startadd_right,colour_depth);
			break;
		case DUALHEAD_OFF:
			head1_set_display_start(startadd,colour_depth);
			break;
		case DUALHEAD_CLONE:
			head1_set_display_start(startadd,colour_depth);
//			head2_set_display_start(startadd,colour_depth);
			break;
	}

	interrupt_enable(true);
	return B_OK;
}

/* Set the indexed color palette */
void SET_INDEXED_COLORS(uint count, uint8 first, uint8 *color_data, uint32 flags) {
	int i;
	uint8 *r,*g,*b;
	
	/* Protect gamma correction when not in CMAP8 */
	if (si->dm.space != B_CMAP8) return;

	r=si->color_data;
	g=r+256;
	b=g+256;

	i=first;
	while (count--)
	{
		r[i]=*color_data++;
		g[i]=*color_data++;
		b[i]=*color_data++;
		i++;	
	}
	head1_palette(r,g,b);
//	if (si->dm.flags & DUALHEAD_BITS) head2_palette(r,g,b);
}

/* Put the display into one of the Display Power Management modes. */
status_t SET_DPMS_MODE(uint32 dpms_flags) {
	interrupt_enable(false);

	LOG(4,("SET_DPMS_MODE: 0x%08x\n", dpms_flags));
	
#if 0
	if (si->dm.flags & DUALHEAD_BITS) /*dualhead*/
	{
		switch(dpms_flags) 
		{
		case B_DPMS_ON:	/* H: on, V: on, display on */
			head1_dpms(true, true, true);
			if (si->ps.secondary_head) head2_dpms(true, true, true);
			break;
		case B_DPMS_STAND_BY:
			head1_dpms(false, false, true);
			if (si->ps.secondary_head) head2_dpms(false, false, true);
			break;
		case B_DPMS_SUSPEND:
			head1_dpms(false, true, false);
			if (si->ps.secondary_head) head2_dpms(false, true, false);
			break;
		case B_DPMS_OFF: /* H: off, V: off, display off */
			head1_dpms(false, false, false);
			if (si->ps.secondary_head) head2_dpms(false, false, false);
			break;
		default:
			LOG(8,("SET: Invalid DPMS settings (DH) 0x%08x\n", dpms_flags));
			interrupt_enable(true);
			return B_ERROR;
		}
	} else /* singlehead */
#endif
	{
		switch(dpms_flags) 
		{
		case B_DPMS_ON:	/* H: on, V: on, display on */
			head1_dpms(true, true, true);
			break;
		case B_DPMS_STAND_BY:
			head1_dpms(false, false, true);
			break;
		case B_DPMS_SUSPEND:
			head1_dpms(false, true, false);
			break;
		case B_DPMS_OFF: /* H: off, V: off, display off */
			head1_dpms(false, false, false);
			break;
		default:
			LOG(8,("SET: Invalid DPMS settings (DH) 0x%08x\n", dpms_flags));
			interrupt_enable(true);
			return B_ERROR;
		}
	}
	interrupt_enable(true);
	return B_OK;
}

/* Report device DPMS capabilities */
uint32 DPMS_CAPABILITIES(void) {
	return 	(B_DPMS_ON | B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF);
}

/* Return the current DPMS mode */
uint32 DPMS_MODE(void) {
	bool display, h, v;
	
	interrupt_enable(false);
	head1_dpms_fetch(&display, &h, &v);

	interrupt_enable(true);

	if (display && h && v)
		return B_DPMS_ON;
	else if(v)
		return B_DPMS_STAND_BY;
	else if(h)
		return B_DPMS_SUSPEND;
	else
		return B_DPMS_OFF;
}
