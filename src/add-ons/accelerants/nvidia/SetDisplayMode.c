
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Apsed,
	Rudolf Cornelissen 11/2002-1/2004
*/

#define MODULE_BIT 0x00200000

#include "acc_std.h"

/*
	Enable/Disable interrupts.  Just a wrapper around the
	ioctl() to the kernel driver.
*/
static void interrupt_enable(bool flag) {
	status_t result;
	nv_set_bool_state sbs;

	/* set the magic number so the driver knows we're for real */
	sbs.magic = NV_PRIVATE_DATA_MAGIC;
	sbs.do_it = flag;
	/* contact driver and get a pointer to the registers and shared data */
	result = ioctl(fd, NV_RUN_INTERRUPTS, &sbs, sizeof(sbs));
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
	status_t result;
	uint32 startadd,startadd_right;
	bool display, h, v;

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
	nv_crtc_dpms_fetch(&display, &h, &v);
	nv_crtc_dpms(false, false, false);
	if (si->ps.secondary_head) nv_crtc2_dpms(false, false, false);

	/*where in framebuffer the screen is (should this be dependant on previous MOVEDISPLAY?)*/
	startadd = (uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer;

	/* calculate and set new mode bytes_per_row */
	nv_general_validate_pic_size (&target, &si->fbc.bytes_per_row, &si->acc_mode);

	/*Perform the very long mode switch!*/
	if (target.flags & DUALHEAD_BITS) /*if some dualhead mode*/
	{
		uint8 colour_depth2 = colour_depth1;

		/* init display mode for secondary head */		
		display_mode target2 = target;

		LOG(1,("SETMODE: setting DUALHEAD mode\n"));

		/* validate flags for secondary TVout */
		if ((i2c_sec_tv_adapter() != B_OK) && (target2.flags & TV_BITS))
		{
			target.flags &= ~TV_BITS;//still needed for some routines...
			target2.flags &= ~TV_BITS;
			LOG(1,("SETMODE: blocking TVout: no TVout cable connected!\n"));
		}

		/* set the pixel clock PLL(s) */
		LOG(8,("SETMODE: target clock %dkHz\n",target.timing.pixel_clock));
		if (nv_dac_set_pix_pll(target) == B_ERROR)
			LOG(8,("SETMODE: error setting pixel clock (internal DAC)\n"));

		/* we do not need to set the pixelclock here for a head that's in TVout mode */
		if (!(target2.flags & TV_BITS))
		{
			LOG(8,("SETMODE: target2 clock %dkHz\n",target2.timing.pixel_clock));
			if (nv_dac2_set_pix_pll(target2) == B_ERROR)
				LOG(8,("SETMODE: error setting pixel clock (DAC2)\n"));
		}

		/*set the colour depth for CRTC1 and the DAC */
		switch(target.space)
		{
		case B_CMAP8:
			colour_depth1 =  8;
			nv_dac_mode(BPP8, 1.0);
			nv_crtc_depth(BPP8);
			break;
		case B_RGB15_LITTLE:
			colour_depth1 = 16;
			nv_dac_mode(BPP15, 1.0);
			nv_crtc_depth(BPP15);
			break;
		case B_RGB16_LITTLE:
			colour_depth1 = 16;
			nv_dac_mode(BPP16, 1.0);
			nv_crtc_depth(BPP16);
			break;
		case B_RGB32_LITTLE:
			colour_depth1 = 32;
			nv_dac_mode(BPP32, 1.0);
			nv_crtc_depth(BPP32);
			break;
		}
		/*set the colour depth for CRTC2 and DAC2 */
		switch(target2.space)
		{
		case B_CMAP8:
			colour_depth2 =  8;
			nv_dac2_mode(BPP8, 1.0);
			nv_crtc2_depth(BPP8);
			break;
		case B_RGB15_LITTLE:
			colour_depth2 = 16;
			nv_dac2_mode(BPP15, 1.0);
			nv_crtc2_depth(BPP15);
			break;
		case B_RGB16_LITTLE:
			colour_depth2 = 16;
			nv_dac2_mode(BPP16, 1.0);
			nv_crtc2_depth(BPP16);
			break;
		case B_RGB32_LITTLE:
			colour_depth2 = 32;
			nv_dac2_mode(BPP32, 1.0);
			nv_crtc2_depth(BPP32);
			break;
		}

		/* check if we are doing interlaced TVout mode */
		si->interlaced_tv_mode = false;
/*		if ((target2.flags & TV_BITS) && (si->ps.card_type >= G450))
			si->interlaced_tv_mode = true;
*/
		/*set the display(s) pitches*/
		nv_crtc_set_display_pitch ();
		//fixme: seperate for real dualhead modes:
		//we need a secondary si->fbc!
		nv_crtc2_set_display_pitch ();

		/*work out where the "right" screen starts*/
		startadd_right=startadd+(target.timing.h_display * (colour_depth1 >> 3));

		/* set the outputs */
		switch (si->ps.card_type)
		{
		//fixme..
		case G550:
			switch (target.flags & DUALHEAD_BITS)
			{
			case DUALHEAD_ON:
			case DUALHEAD_CLONE:
				//fixme: set output connectors only
				nv_general_dac_select(DS_CRTC1DAC_CRTC2MAVEN);
				break;
			case DUALHEAD_SWITCH:
				//fixme: set output connectors only
				nv_general_dac_select(DS_CRTC1MAVEN_CRTC2DAC);
				break;
			}
			break;
		default:
			break;
		}

		/*Tell card what memory to display*/
		switch (target.flags & DUALHEAD_BITS)
		{
		case DUALHEAD_ON:
		case DUALHEAD_SWITCH:
			nv_crtc_set_display_start(startadd,colour_depth1);
			nv_crtc2_set_display_start(startadd_right,colour_depth2);
			break;
		case DUALHEAD_CLONE:
			nv_crtc_set_display_start(startadd,colour_depth1);
			nv_crtc2_set_display_start(startadd,colour_depth2);
			break;
		}

		/* set the timing */
		nv_crtc_set_timing(target);
		/* we do not need to setup CRTC2 here for a head that's in TVout mode */
		if (!(target2.flags & TV_BITS))	result = nv_crtc2_set_timing(target2);

		/* TVout support: setup CRTC2 and it's pixelclock */
		if (si->ps.tvout && (target2.flags & TV_BITS)) maventv_init(target2);
	}
	else /* single head mode */
	{
		status_t status;
		int colour_mode = BPP32;
		
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
		status = nv_dac_set_pix_pll(target);

		if (status==B_ERROR)
			LOG(8,("CRTC: error setting pixel clock (internal DAC)\n"));

		/* set the colour depth for CRTC1 and the DAC */
		/* first set the colordepth */
		nv_crtc_depth(colour_mode);
		/* then(!) program the PAL (<8bit colordepth does not support 8bit PAL) */
		nv_dac_mode(colour_mode,1.0);

		/* set the display pitch */
		nv_crtc_set_display_pitch();

		/* tell the card what memory to display */
		nv_crtc_set_display_start(startadd,colour_depth1);

		/* enable primary analog output */
		switch (si->ps.card_type)
		{
		case NV11:
//			nv_general_dac_select(DS_CRTC1DAC_CRTC2MAVEN);
			break;
		case NV17: 
//			nv_general_dac_select(DS_CRTC1CON1_CRTC2CON2);
//			gx50_general_output_select();
			break;
		default:
			break;
		}
		
		/* set the timing */
		nv_crtc_set_timing(target);

		//fixme: shut-off the videoPLL if it exists...
	}

	/* update driver's mode store */
	si->dm = target;

	/* turn screen one on */
	nv_crtc_dpms(display, h, v);
	/* turn screen two on if a dualhead mode is active */
	if (target.flags & DUALHEAD_BITS) nv_crtc2_dpms(display,h,v);

	/* set up acceleration for this mode */
	nv_acc_init();
	/* set up overlay unit for this mode */
	nv_bes_init();

	LOG(1,("SETMODE: booted since %f mS\n", system_time()/1000.0));

	/* enable interrupts using the kernel driver */
	interrupt_enable(true);

	/* optimize memory-access if needed */
//	nv_crtc_mem_priority(colour_depth1);

	/* Tune RAM CAS-latency if needed. Must be done *here*! */
	nv_set_cas_latency();

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

	/* nVidia cards support pixelprecise panning on both heads in all modes:
	 * No stepping granularity needed! */

	/* determine bits used for the colordepth */
	switch(si->dm.space)
	{
	case B_CMAP8:
		colour_depth=8;
		break;
	case B_RGB15_LITTLE:
	case B_RGB16_LITTLE:
		colour_depth=16;
		break;
	case B_RGB24_LITTLE:
		colour_depth=24;
		break;
	case B_RGB32_LITTLE:
		colour_depth=32;
		break;
	default:
		return B_ERROR;
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
			nv_crtc_set_display_start(startadd,colour_depth);
			nv_crtc2_set_display_start(startadd_right,colour_depth);
			break;
		case DUALHEAD_OFF:
			nv_crtc_set_display_start(startadd,colour_depth);
			break;
		case DUALHEAD_CLONE:
			nv_crtc_set_display_start(startadd,colour_depth);
			nv_crtc2_set_display_start(startadd,colour_depth);
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
	nv_dac_palette(r,g,b);
	if (si->dm.flags & DUALHEAD_BITS) nv_dac2_palette(r,g,b);
}

/* Put the display into one of the Display Power Management modes. */
status_t SET_DPMS_MODE(uint32 dpms_flags) {
	interrupt_enable(false);

	LOG(4,("SET_DPMS_MODE: 0x%08x\n", dpms_flags));
	
	if (si->dm.flags & DUALHEAD_BITS) /*dualhead*/
	{
		switch(dpms_flags) 
		{
		case B_DPMS_ON:	/* H: on, V: on, display on */
			nv_crtc_dpms(true, true, true);
			if (si->ps.secondary_head) nv_crtc2_dpms(true, true, true);
			break;
		case B_DPMS_STAND_BY:
			nv_crtc_dpms(false, false, true);
			if (si->ps.secondary_head) nv_crtc2_dpms(false, false, true);
			break;
		case B_DPMS_SUSPEND:
			nv_crtc_dpms(false, true, false);
			if (si->ps.secondary_head) nv_crtc2_dpms(false, true, false);
			break;
		case B_DPMS_OFF: /* H: off, V: off, display off */
			nv_crtc_dpms(false, false, false);
			if (si->ps.secondary_head) nv_crtc2_dpms(false, false, false);
			break;
		default:
			LOG(8,("SET: Invalid DPMS settings (DH) 0x%08x\n", dpms_flags));
			interrupt_enable(true);
			return B_ERROR;
		}
	}
	else /* singlehead */
	{
		switch(dpms_flags) 
		{
		case B_DPMS_ON:	/* H: on, V: on, display on */
			nv_crtc_dpms(true, true, true);
			break;
		case B_DPMS_STAND_BY:
			nv_crtc_dpms(false, false, true);
			break;
		case B_DPMS_SUSPEND:
			nv_crtc_dpms(false, true, false);
			break;
		case B_DPMS_OFF: /* H: off, V: off, display off */
			nv_crtc_dpms(false, false, false);
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
	nv_crtc_dpms_fetch(&display, &h, &v);
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
