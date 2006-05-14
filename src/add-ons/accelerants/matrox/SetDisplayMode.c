/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Apsed,
	Rudolf Cornelissen 11/2002-5/2006
*/

#define MODULE_BIT 0x00200000

#include "acc_std.h"

/*
	Enable/Disable interrupts.  Just a wrapper around the
	ioctl() to the kernel driver.
*/
static void interrupt_enable(bool flag)
{
	status_t result;
	gx00_set_bool_state sbs;

	if (si->ps.int_assigned)
	{
		/* set the magic number so the driver knows we're for real */
		sbs.magic = GX00_PRIVATE_DATA_MAGIC;
		sbs.do_it = flag;
		/* contact driver and get a pointer to the registers and shared data */
		result = ioctl(fd, GX00_RUN_INTERRUPTS, &sbs, sizeof(sbs));
	}
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

	/* Adjust mode to valid one and fail if invalid */
	target /*= bounds*/ = *mode_to_set;
	/* show the mode bits */
	LOG(1, ("SETMODE: (ENTER) initial modeflags: $%08x\n", target.flags));
	LOG(1, ("SETMODE: requested target pixelclock %dkHz\n",  target.timing.pixel_clock));
	LOG(1, ("SETMODE: requested virtual_width %d, virtual_height %d\n",
										target.virtual_width, target.virtual_height));

	/* See BOUNDS WARNING above... */
	if (PROPOSE_DISPLAY_MODE(&target, &target, &target) == B_ERROR)	return B_ERROR;

	/* overlay engine, cursor and MOVE_DISPLAY need to know the status even when
	 * in singlehead mode */
	si->switched_crtcs = false;

	/* disable interrupts using the kernel driver */
	interrupt_enable(false);

	/* then turn off screen(s) */
	gx00_crtc_dpms(false, false, false);
	if (si->ps.secondary_head)
	{
		g400_crtc2_dpms(false, false, false);
	}
	else
	{
		/* on singlehead cards with TVout program the MAVEN as well */
		if (si->ps.tvout) gx00_maven_dpms(false, false, false);
	}

	/*where in framebuffer the screen is (should this be dependant on previous MOVEDISPLAY?)*/
	startadd = (uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer;

	/* calculate and set new mode bytes_per_row */
	gx00_general_validate_pic_size (&target, &si->fbc.bytes_per_row, &si->acc_mode);

	/*Perform the very long mode switch!*/
	if (target.flags & DUALHEAD_BITS) /*if some dualhead mode*/
	{
		uint8 colour_depth2 = colour_depth1;

		/* init display mode for secondary head */		
		display_mode target2 = target;

		LOG(1,("SETMODE: setting DUALHEAD mode\n"));

		/* set the pixel clock PLL(s) */
		LOG(8,("SETMODE: target clock %dkHz\n",target.timing.pixel_clock));
		if (gx00_dac_set_pix_pll(target) == B_ERROR)
			LOG(8,("SETMODE: error setting pixel clock (internal DAC)\n"));

		/* we do not need to set the pixelclock here for a head that's in TVout mode */
		if (!(target2.flags & TV_BITS))
		{
			LOG(8,("SETMODE: target2 clock %dkHz\n",target2.timing.pixel_clock));
			if (gx00_maven_set_vid_pll(target2) == B_ERROR)
				LOG(8,("SETMODE: error setting pixel clock (MAVEN)\n"));
		}

		/*set the colour depth for CRTC1 and the DAC */
		switch(target.space)
		{
		case B_RGB16_LITTLE:
			colour_depth1 = 16;
			gx00_dac_mode(BPP16, 1.0);
			gx00_crtc_depth(BPP16);
			break;
		case B_RGB32_LITTLE:
			colour_depth1 = 32;
			gx00_dac_mode(BPP32, 1.0);
			gx00_crtc_depth(BPP32);
			break;
		}
		/*set the colour depth for CRTC2 and the MAVEN */
		switch(target2.space)
		{
		case B_RGB16_LITTLE:
			colour_depth2 = 16;
			gx00_maven_mode(BPP16, 1.0);
			g400_crtc2_depth(BPP16);
			break;
		case B_RGB32_LITTLE:
			colour_depth2 = 32;
			gx00_maven_mode(BPP32DIR, 1.0);
			g400_crtc2_depth(BPP32DIR);
			break;
		}

		/* check if we are doing interlaced TVout mode */
		si->interlaced_tv_mode = false;
		if ((target2.flags & TV_BITS) && (si->ps.card_type >= G450))
			si->interlaced_tv_mode = true;

		/*set the display(s) pitches*/
		gx00_crtc_set_display_pitch ();
		//fixme: seperate for real dualhead modes:
		//we need a secondary si->fbc!
		g400_crtc2_set_display_pitch ();

		/*work out where the "right" screen starts*/
		startadd_right=startadd+(target.timing.h_display * (colour_depth1 >> 3));

		/* calculate needed MAVEN-CRTC delay: formula valid for straight-through CRTC's */
		si->crtc_delay = 44;
		if (colour_depth2 == 16) si->crtc_delay += 0;

		/* set the outputs */
		switch (si->ps.card_type)
		{
		case G400:
		case G400MAX:
			/* setup vertical timing adjust for crtc connected to the MAVEN:
			 * assuming connected straight through. */
			/* (extra "blanking" line for MAVEN hardware design fault) */
			target2.timing.v_display++;

			switch (target.flags & DUALHEAD_BITS)
			{
			case DUALHEAD_ON:
			case DUALHEAD_CLONE:
				gx00_general_dac_select(DS_CRTC1DAC_CRTC2MAVEN);
				si->switched_crtcs = false;
				break;
			case DUALHEAD_SWITCH:
				if (i2c_sec_tv_adapter() == B_OK)
				{
					/* Don't switch CRTC's because MAVEN YUV is impossible then, 
					 * and primary head output will be limited to 135Mhz pixelclock. */
					LOG(4,("SETMODE: secondary TV-adapter detected, switching buffers\n"));
					gx00_general_dac_select(DS_CRTC1DAC_CRTC2MAVEN);
					si->switched_crtcs = true;
				}
				else
				{
					/* This limits the pixelclocks on both heads to 135Mhz,
					 * but you can use overlay on the other output now. */
					LOG(4,("SETMODE: no secondary TV-adapter detected, switching CRTCs\n"));
					gx00_general_dac_select(DS_CRTC1MAVEN_CRTC2DAC);
					si->switched_crtcs = false;
					/* re-calculate MAVEN-CRTC delay: formula valid for crossed CRTC's */
					si->crtc_delay = 17;
					if (colour_depth1 == 16) si->crtc_delay += 4;
					/* re-setup vertical timing adjust for crtc connected to the MAVEN:
					 * cross connected. */
					/* (extra "blanking" line for MAVEN hardware design fault) */
					target.timing.v_display++;
					target2.timing.v_display--;
				}
				break;
			}
			break;
		case G450:
		case G550:
			if (!si->ps.primary_dvi)
			/* output connector use is always 'straight-through' */
			//fixme: re-evaluate when DVI is setup... 
			{
				switch (target.flags & DUALHEAD_BITS)
				{
				case DUALHEAD_ON:
				case DUALHEAD_CLONE:
					gx00_general_dac_select(DS_CRTC1CON1_CRTC2CON2);
					si->switched_crtcs = false;
					break;
				case DUALHEAD_SWITCH:
					if (i2c_sec_tv_adapter() == B_OK)
					{
						/* Don't switch CRTC's because MAVEN YUV and TVout is impossible then, 
						 * and primary head output will be limited to 235Mhz pixelclock. */
						LOG(4,("SETMODE: secondary TV-adapter detected, switching buffers\n"));
						gx00_general_dac_select(DS_CRTC1CON1_CRTC2CON2);
						si->switched_crtcs = true;
					}
					else
					{
						/* This limits the pixelclocks on both heads to 235Mhz,
						 * but you can use overlay on the other output now. */
						LOG(4,("SETMODE: no secondary TV-adapter detected, switching CRTCs\n"));
						gx00_general_dac_select(DS_CRTC1CON2_CRTC2CON1);
						si->switched_crtcs = false;
					}
					break;
				}
			}
			else
			/* output connector use is cross-linked if no TV cable connected! */
			//fixme: re-evaluate when DVI is setup... 
			{
				switch (target.flags & DUALHEAD_BITS)
				{
				case DUALHEAD_ON:
				case DUALHEAD_CLONE:
					if (i2c_sec_tv_adapter() == B_OK)
					{
						gx00_general_dac_select(DS_CRTC1CON1_CRTC2CON2);
						si->switched_crtcs = false;
					}
					else
					{
						/* This limits the pixelclocks on both heads to 235Mhz,
						 * but you can use overlay on the other output now. */
						gx00_general_dac_select(DS_CRTC1CON2_CRTC2CON1);
						si->switched_crtcs = false;
					}
					break;
				case DUALHEAD_SWITCH:
					if (i2c_sec_tv_adapter() == B_OK)
					{
						/* Don't switch CRTC's because MAVEN YUV and TVout is impossible then, 
						 * and primary head output will be limited to 235Mhz pixelclock. */
						LOG(4,("SETMODE: secondary TV-adapter detected, switching buffers\n"));
						gx00_general_dac_select(DS_CRTC1CON1_CRTC2CON2);
						si->switched_crtcs = true;
					}
					else
					{
						LOG(4,("SETMODE: no secondary TV-adapter detected, switching CRTCs\n"));
						gx00_general_dac_select(DS_CRTC1CON1_CRTC2CON2);
						si->switched_crtcs = false;
					}
					break;
				}
			}
			break;
		default:
			break;
		}

		if (si->switched_crtcs)
		{
				uint32 temp = startadd;
				startadd = startadd_right;
				startadd_right = temp;
		}

		/*Tell card what memory to display*/
		switch (target.flags & DUALHEAD_BITS)
		{
		case DUALHEAD_ON:
		case DUALHEAD_SWITCH:
			gx00_crtc_set_display_start(startadd,colour_depth1);
			g400_crtc2_set_display_start(startadd_right,colour_depth2);
			break;
		case DUALHEAD_CLONE:
			gx00_crtc_set_display_start(startadd,colour_depth1);
			g400_crtc2_set_display_start(startadd,colour_depth2);
			break;
		}

		/* set the timing */
		gx00_crtc_set_timing(target);
		/* we do not need to setup CRTC2 here for a head that's in TVout mode */
		if (!(target2.flags & TV_BITS))	result = g400_crtc2_set_timing(target2);

		/* TVout support: setup CRTC2 and it's pixelclock */
		if (si->ps.tvout && (target2.flags & TV_BITS)) maventv_init(target2);
	}
	else /* single head mode */
	{
		status_t status;
		int colour_mode = BPP32;
		
		switch(target.space)
		{
		case B_CMAP8:
			colour_depth1 =  8;
			colour_mode = BPP8;
			break;
		case B_RGB15_LITTLE:
			colour_depth1 = 16;
			colour_mode = BPP15;
			break;
		case B_RGB16_LITTLE:
			colour_depth1 = 16;
			colour_mode = BPP16;
			break;
		case B_RGB32_LITTLE:
			colour_depth1 = 32;
			colour_mode = BPP32;
			break;
		default:
			LOG(8,("SETMODE: Invalid singlehead colour depth 0x%08x\n", target.space));
			return B_ERROR;
		}

		/* set the pixel clock PLL */
		if (si->ps.card_type >= G100)
			//fixme: how about when TVout is enabled???
			status = gx00_dac_set_pix_pll(target);
		else
		{
			status = mil2_dac_set_pix_pll((target.timing.pixel_clock)/1000.0, colour_depth1);
		}
		if (status==B_ERROR)
			LOG(8,("CRTC: error setting pixel clock (internal DAC)\n"));

		/* set the colour depth for CRTC1 and the DAC */
		gx00_dac_mode(colour_mode,1.0);
		gx00_crtc_depth(colour_mode);

		/* if we do TVout mode, its non-interlaced (as we are on <= G400MAX for sure) */
		si->interlaced_tv_mode = false;

		/* set the display pitch */
		gx00_crtc_set_display_pitch();

		/* tell the card what memory to display */
		gx00_crtc_set_display_start(startadd,colour_depth1);

		/* enable primary analog output */
		switch (si->ps.card_type)
		{
		case G100:
		case G200: 
		case G400:
		case G400MAX: 
			if (!si->ps.secondary_head && si->ps.tvout && (target.flags & TV_BITS))
				gx00_general_dac_select(DS_CRTC1MAVEN);
			else
				gx00_general_dac_select(DS_CRTC1DAC);
			break;
		case G450:
		case G550: 
			gx00_general_dac_select(DS_CRTC1CON1_CRTC2CON2);
			gx50_general_output_select();
			break;
		default:
			break;
		}

		/* set the timing */
		if (!si->ps.secondary_head && si->ps.tvout && (target.flags & TV_BITS))
		{
			/* calculate MAVEN-CRTC delay: only used during TVout */
			si->crtc_delay = 17;
			if (colour_depth1 == 16) si->crtc_delay += 4;
			/* setup CRTC1 and it's pixelclock for TVout */
			maventv_init(target);
		}
		else
		{
			gx00_maven_shutoff();
			gx00_crtc_set_timing(target);
		}
	}

	/* update driver's mode store */
	si->dm = target;

	/* set up acceleration for this mode */
	gx00_acc_init();

	/* restore screen(s) output state(s) */
	SET_DPMS_MODE(si->dpms_flags);

	/* clear line at bottom of screen if dualhead mode:
	 * MAVEN hardware design fault 'fix'.
	 * Note:
	 * Not applicable for singlehead cards with a MAVEN, since it's only used
	 * for TVout there. */
	if ((target.flags & DUALHEAD_BITS) && (si->ps.card_type <= G400MAX))
		gx00_maven_clrline();

	LOG(1,("SETMODE: booted since %f mS\n", system_time()/1000.0));

	/* enable interrupts using the kernel driver */
	interrupt_enable(true);

	/* optimize memory-access if needed */
	gx00_crtc_mem_priority(colour_depth1);

	/* Tune RAM CAS-latency if needed. Must be done *here*! */
	mga_set_cas_latency();

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

	/* G400 CRTC1 handles multiples of 8 for 8bit, 4 for 16bit, 2 for 32 bit 
	   G400 CRTC2 handles multiples of 32 for 16-bit and 16 for 32-bit - must stoop to this in dualhead
     */

	/* reset lower bits, don't return an error! */
	if (si->dm.flags & DUALHEAD_BITS)
	{
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

	/* account for switched CRTC's */
	if (si->switched_crtcs)
	{
		uint32 temp = startadd;
		startadd = startadd_right;
		startadd_right = temp;
	}

	interrupt_enable(false);

	switch (si->dm.flags&DUALHEAD_BITS)
	{
		case DUALHEAD_ON:
		case DUALHEAD_SWITCH:
			gx00_crtc_set_display_start(startadd,colour_depth);
			g400_crtc2_set_display_start(startadd_right,colour_depth);
			break;
		case DUALHEAD_OFF:
			gx00_crtc_set_display_start(startadd,colour_depth);
			break;
		case DUALHEAD_CLONE:
			gx00_crtc_set_display_start(startadd,colour_depth);
			g400_crtc2_set_display_start(startadd,colour_depth);
			break;
	}

	interrupt_enable(true);
	return B_OK;
}

/*
	Set the indexed color palette.
*/
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
	gx00_dac_palette(r,g,b);
}

/* Put the display into one of the Display Power Management modes. */
status_t SET_DPMS_MODE(uint32 dpms_flags)
{
	interrupt_enable(false);

	LOG(4,("SET_DPMS_MODE: 0x%08x\n", dpms_flags));

	/* note current DPMS state for our reference */
	si->dpms_flags = dpms_flags;

	if (si->dm.flags & DUALHEAD_BITS) /* dualhead */
	{
		switch(dpms_flags) 
		{
		case B_DPMS_ON:	/* H: on, V: on */
			gx00_crtc_dpms(true, true, true);
			if (si->ps.secondary_head) g400_crtc2_dpms(true, true, true);
			break;
		case B_DPMS_STAND_BY:
			if (si->settings.greensync)
			{
				/* blank screen, but keep sync running */
				gx00_crtc_dpms(false, true, true);
			}
			else
			{
				gx00_crtc_dpms(false, false, true);
			}
			if (si->ps.secondary_head)
			{
				if ((si->dm.flags & TV_BITS) && (si->ps.card_type > G400MAX))
				{
					/* keep display enabled in TVout modes for G450 and G550! */
					g400_crtc2_dpms(true, false, true);
				}
				else
				{
					g400_crtc2_dpms(false, false, true);
				}
			}
			break;
		case B_DPMS_SUSPEND:
			if (si->settings.greensync)
			{
				/* blank screen, but keep sync running */
				gx00_crtc_dpms(false, true, true);
			}
			else
			{
				gx00_crtc_dpms(false, true, false);
			}
			if (si->ps.secondary_head)
			{
				if ((si->dm.flags & TV_BITS) && (si->ps.card_type > G400MAX))
				{
					/* keep display enabled in TVout modes for G450 and G550! */
					g400_crtc2_dpms(true, true, false);
				}
				else
				{
					g400_crtc2_dpms(false, true, false);
				}
			}
			break;
		case B_DPMS_OFF: /* H: off, V: off, display off */
			if (si->settings.greensync)
			{
				/* blank screen, but keep sync running */
				gx00_crtc_dpms(false, true, true);
			}
			else
			{
				gx00_crtc_dpms(false, false, false);
			}
			if (si->ps.secondary_head)
			{
				if ((si->dm.flags & TV_BITS) && (si->ps.card_type > G400MAX))
				{
					/* keep display enabled in TVout modes for G450 and G550! */
					g400_crtc2_dpms(true, false, false);
				}
				else
				{
					g400_crtc2_dpms(false, false, false);
				}
			}
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
		case B_DPMS_ON:	/* H: on, V: on */
			gx00_crtc_dpms(true, true, true);
			/* on singlehead cards with TVout program the MAVEN as well */
			if (si->dm.flags & TV_BITS) gx00_maven_dpms(true, true, true);
			break;
		case B_DPMS_STAND_BY:
			if (si->settings.greensync)
			{
				/* blank screen, but keep sync running */
				gx00_crtc_dpms(false, true, true);
			}
			else
			{
				gx00_crtc_dpms(false, false, true);
			}
			/* on singlehead cards with TVout program the MAVEN as well */
			if (si->dm.flags & TV_BITS) gx00_maven_dpms(false, false, true);
			break;
		case B_DPMS_SUSPEND:
			if (si->settings.greensync)
			{
				/* blank screen, but keep sync running */
				gx00_crtc_dpms(false, true, true);
			}
			else
			{
				gx00_crtc_dpms(false, true, false);
			}
			/* on singlehead cards with TVout program the MAVEN as well */
			if (si->dm.flags & TV_BITS) gx00_maven_dpms(false, true, false);
			break;
		case B_DPMS_OFF: /* H: off, V: off, display off */
			if (si->settings.greensync)
			{
				/* blank screen, but keep sync running */
				gx00_crtc_dpms(false, true, true);
			}
			else
			{
				gx00_crtc_dpms(false, false, false);
			}
			/* on singlehead cards with TVout program the MAVEN as well */
			if (si->dm.flags & TV_BITS) gx00_maven_dpms(false, false, false);
			break;
		default:
			LOG(8,("SET: Invalid DPMS settings (SH) 0x%08x\n", dpms_flags));
			interrupt_enable(true);
			return B_ERROR;
		}
	}
	interrupt_enable(true);
	return B_OK;
}

/* Report device DPMS capabilities. */
uint32 DPMS_CAPABILITIES(void)
{
	if (si->settings.greensync)
		/* we can blank the screen on CRTC1, G400 CRTC2 does not support intermediate
		 * modes anyway. */
		//fixme: G450/G550 support full DPMS on CRTC2...
		return 	B_DPMS_ON | B_DPMS_OFF;
	else
		/* normally CRTC1 supports full DPMS (and on G450/G550 CRTC2 also).. */
		return 	B_DPMS_ON | B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF;
}

/* Return the current DPMS mode. */
uint32 DPMS_MODE(void)
{
	return si->dpms_flags;
}
