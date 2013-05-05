/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Rudolf Cornelissen 4/2003-1/2006
*/


#define MODULE_BIT 0x00200000

#include "acc_std.h"

/*
	Enable/Disable interrupts.  Just a wrapper around the
	ioctl() to the kernel driver.
*/
static void
interrupt_enable(bool flag)
{
	nm_set_bool_state sbs;

	if (si->ps.int_assigned) {
		/* set the magic number so the driver knows we're for real */
		sbs.magic = NM_PRIVATE_DATA_MAGIC;
		sbs.do_it = flag;
		/* contact driver and get a pointer to the registers and shared data */
		ioctl(fd, NM_RUN_INTERRUPTS, &sbs, sizeof(sbs));
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

	uint8 colour_depth = 24;
	uint32 startadd;

	/* if internal panel is active we don't touch the CRTC timing and the pixelPLL */
	bool crt_only = true;

	/* Adjust mode to valid one and fail if invalid */
	target /*= bounds*/ = *mode_to_set;
	/* show the mode bits */
	LOG(1, ("SETMODE: (ENTER) initial modeflags: $%08x\n", target.flags));
	LOG(1, ("SETMODE: requested target pixelclock %dkHz\n",  target.timing.pixel_clock));
	LOG(1, ("SETMODE: requested virtual_width %d, virtual_height %d\n",
										target.virtual_width, target.virtual_height));

	/* See BOUNDS WARNING above... */
	if (PROPOSE_DISPLAY_MODE(&target, &target, &target) == B_ERROR)	return B_ERROR;

	/* disable interrupts using the kernel driver */
	interrupt_enable(false);

	/* make sure the card's registers are unlocked (KB output select may lock!) */
	nm_unlock();

	/* if we have the flatpanel turned on modify visible part of mode if nessesary */
	if (nm_general_output_read() & 0x02)
	{
		LOG(4,("SETMODE: internal flatpanel enabled, skipping CRTC/pixelPLL setup\n"));
		crt_only = false;
	}

	/* turn off screen(s) */
	nm_crtc_dpms(false, false, false);

	/* where in framebuffer the screen is (should this be dependant on previous MOVEDISPLAY?) */
	startadd = (uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer;

	/* Perform the mode switch */
	{
		status_t status = B_OK;
		int colour_mode = BPP24;
		
		switch(target.space)
		{
		case B_CMAP8:        colour_depth =  8; colour_mode = BPP8;  break;
		case B_RGB15_LITTLE: colour_depth = 16; colour_mode = BPP15; break;
		case B_RGB16_LITTLE: colour_depth = 16; colour_mode = BPP16; break;
		case B_RGB24_LITTLE: colour_depth = 24; colour_mode = BPP24; break;
		default:
			LOG(8,("SETMODE: Invalid colorspace $%08x\n", target.space));
			return B_ERROR;
		}

		/* calculate and set new mode bytes_per_row */
		nm_general_validate_pic_size (&target, &si->fbc.bytes_per_row, &si->acc_mode);

		/* set the pixelclock PLL */
		if (crt_only)
		{
			status = nm_dac_set_pix_pll(target);
			if (status == B_ERROR)
				LOG(8,("CRTC: error setting pixelclock\n"));
		}

		/* set the colour depth for CRTC1 and the DAC */
		nm_dac_mode(colour_mode, 1.0);
		nm_crtc_depth(colour_mode);
		
		/* set the display pitch */
		nm_crtc_set_display_pitch();

		/* tell the card what memory to display */
		nm_crtc_set_display_start(startadd,colour_depth);

		/* enable primary analog output */
		//fixme: choose output connector(s)
		
		/* set the timing */
		nm_crtc_set_timing(target, crt_only);

		/* always setup centering so a KB BIOS switch to flatpanel will go OK... */
		nm_crtc_center(target, crt_only);
		/* program panel modeline if needed */
		if (!crt_only) nm_crtc_prg_panel();
	}

	/* update driver's mode store */
	si->dm = target;

	/* set up acceleration for this mode */
	nm_acc_init();

	/* restore screen(s) output state(s) */
	SET_DPMS_MODE(si->dpms_flags);

	/* log currently selected output */
	nm_general_output_select();

	LOG(1,("SETMODE: booted since %f mS\n", system_time()/1000.0));

	/* enable interrupts using the kernel driver */
	interrupt_enable(true);

	/* Tune RAM CAS-latency if needed. Must be done *here*! */
	nm_set_cas_latency();

	return B_OK;
}

/*
	Set which pixel of the virtual frame buffer will show up in the
	top left corner of the display device.  Used for page-flipping
	games and virtual desktops.
*/
status_t MOVE_DISPLAY(uint16 h_display_start, uint16 v_display_start)
{
	uint8 colour_depth;
	uint32 startadd;

	uint16 h_display = si->dm.timing.h_display; /* local copy needed for flatpanel */
	uint16 v_display = si->dm.timing.v_display; /* local copy needed for flatpanel */

	LOG(4,("MOVE_DISPLAY: h %d, v %d\n", h_display_start, v_display_start));

	/* Neomagic cards support pixelprecise panning in all modes:
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
	default:
		return B_ERROR;
	}

	/* if internal panel is active correct visible screensize! */
	if (nm_general_output_read() & 0x02)
	{
		if (h_display > si->ps.panel_width) h_display = si->ps.panel_width;
		if (v_display > si->ps.panel_height) v_display = si->ps.panel_height;
	}

	/* do not run past end of display */
	if ((h_display + h_display_start) > si->dm.virtual_width)
		return B_ERROR;
	if ((v_display + v_display_start) > si->dm.virtual_height)
		return B_ERROR;

	/* everybody remember where we parked... */
	si->dm.h_display_start = h_display_start;
	si->dm.v_display_start = v_display_start;

	/* actually set the registers */
	startadd = v_display_start * si->fbc.bytes_per_row;
	startadd += h_display_start * (colour_depth >> 3);
	startadd += (uint8*)si->fbc.frame_buffer - (uint8*)si->framebuffer;

	interrupt_enable(false);

	nm_crtc_set_display_start(startadd,colour_depth);

	interrupt_enable(true);
	return B_OK;
}

/* Set the indexed color palette. */
void SET_INDEXED_COLORS(uint count, uint8 first, uint8 *color_data, uint32 flags) {
	int i;
	uint8 *r,*g,*b;
	
	/* Protect gamma correction when not in CMAP8 */
	if (si->dm.space != B_CMAP8) return;

	r = si->color_data;
	g = r + 256;
	b = g + 256;

	i = first;
	while (count--)
	{
		r[i] = *color_data++;
		g[i] = *color_data++;
		b[i] = *color_data++;
		i++;	
	}
	nm_dac_palette(r,g,b, 256);
}

/* Put the display into one of the Display Power Management modes. */
status_t SET_DPMS_MODE(uint32 dpms_flags)
{
	interrupt_enable(false);

	LOG(4,("SET_DPMS_MODE: $%08x\n", dpms_flags));

	/* note current DPMS state for our reference */
	si->dpms_flags = dpms_flags;

	switch(dpms_flags) 
	{
	case B_DPMS_ON:	/* H: on, V: on */
		nm_crtc_dpms(true, true , true);
		break;
	case B_DPMS_STAND_BY:
		nm_crtc_dpms(false, false, true);
		break;
	case B_DPMS_SUSPEND:
		nm_crtc_dpms(false, true, false);
		break;
	case B_DPMS_OFF: /* H: off, V: off, display off */
		nm_crtc_dpms(false, false, false);
		break;
	default:
		LOG(8,("SET_DPMS_MODE: Invalid DPMS settings) $%08x\n", dpms_flags));
		interrupt_enable(true);
		return B_ERROR;
	}
	interrupt_enable(true);
	return B_OK;
}

/* Report device DPMS capabilities. */
uint32 DPMS_CAPABILITIES(void)
{
	if (si->ps.card_type < NM2200)
		/* MagicGraph cards don't have full DPMS support */
		return 	B_DPMS_ON | B_DPMS_OFF;
	else
		/* MagicMedia cards do have full DPMS support for external monitors */
		//fixme: checkout if this is true...
		return 	B_DPMS_ON | B_DPMS_STAND_BY | B_DPMS_SUSPEND | B_DPMS_OFF;
}

/* Return the current DPMS mode. */
uint32 DPMS_MODE(void)
{
	return si->dpms_flags;
}
