/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Mark Watson,
	Apsed,
	Rudolf Cornelissen 10/2002
*/

#define MODULE_BIT 0x00200000

#include "acc_std.h"

#include "matroxfb_maven_hack.h" //a port of the matroxfb code to do TVout - used with permission

/*
	Enable/Disable interrupts.  Just a wrapper around the
	ioctl() to the kernel driver.
*/
static void interrupt_enable(bool flag) {
	status_t result;
	gx00_set_bool_state sbs;

	/* set the magic number so the driver knows we're for real */
	sbs.magic = GX00_PRIVATE_DATA_MAGIC;
	sbs.do_it = flag;
	/* contact driver and get a pointer to the registers and shared data */
	result = ioctl(fd, GX00_RUN_INTERRUPTS, &sbs, sizeof(sbs));
}

//	       apsed TODO gx00_crtc_mem_priority() ?? 
//	/*calculate if high priority request are needed and how many*/
//	Tpix = 1000000.0/(dm->timing.pixel_clock);
//	Tmclk = si->ps.mem_clk_period;
//	temp = (128/colour_depth);
//	HIPRILVL = 64*Tmclk + (1-46*(temp))*Tpix;
//	HIPRILVL/= -8*Tpix*temp;
//	gx00_crtc_mem_priority((uint8)HIPRILVL);
//	/*XXX - memory priority*/

/* First validate the mode, then call lots of bit banging stuff to set the mode(s)! */
status_t SET_DISPLAY_MODE(display_mode *mode_to_set) 
{
	display_mode bounds, target;

	uint8 colour_depth=32;
	status_t result;
	uint32 startadd,startadd_right;
//	double HIPRILVL;
//	double Tmclk,Tpix,temp;	
// apsed TODO startadd is 19 bits if < g200

	uint8 display,h,v;

	struct my_timming tv_timing;
	struct mavenregs tv_regs;

	bool switched_crtcs = false;

	/* Adjust mode to valid one and fail if invalid */
	target = bounds = *mode_to_set;
	/* show the mode bits */
	LOG(1, ("SetDisplayMode - initial flags: %x\n", target.flags));
	LOG(1, ("SetDisplayMode - %20s %08x %d\n", "timing.pixel_clock", mode_to_set->timing.pixel_clock, mode_to_set->timing.pixel_clock));
	LOG(1, ("SetDisplayMode - %20s %08x %d\n", "virtual_width", mode_to_set->virtual_width, mode_to_set->virtual_width));
	LOG(1, ("SetDisplayMode - %20s %08x %d\n", "virtual_height", mode_to_set->virtual_height, mode_to_set->virtual_height));

	if (PROPOSE_DISPLAY_MODE(&target, &bounds, &bounds) == B_ERROR)
		return B_ERROR;

	/* if not dualhead capable be sure mode is set to single head */
	if ((!si->ps.secondary_head) || (!(target.flags&DUALHEAD_CAPABLE)))
	{
		target.flags&=~DUALHEAD_BITS;
		target.flags&=~TV_BITS;
	}
	else if (!(target.flags&TV_CAPABLE))
	{
		target.flags&=~TV_BITS;
	}
	
	LOG(1, ("SetDisplayMode - validated flags: %x\n", target.flags));

	/* disable interrupts using the kernel driver */
	interrupt_enable(false);

	/*find current DPMS state, then turn off screen*/
	gx00_crtc_dpms_fetch(&display,&h,&v);
	gx00_crtc_dpms(0,0,0);
	if (si->ps.card_type >= G400) // apsed TODO when g200 pixrdmsk is broken
		g400_crtc2_dpms(0,0,0);
	if (si->ps.secondary_head)
		gx00_maven_dpms(0,0,0);

	/*where in framebuffer the screen is (should this be dependant on previous MOVEDISPLAY?)*/
	startadd=(si->fbc.frame_buffer)-(si->framebuffer);

	/*Perform the very long mode switch!*/
	LOG(1,("DUALHEAD: %d\n",target.flags&DUALHEAD_BITS));
	if ((target.flags&DUALHEAD_BITS)) /*if some dualhead mode*/
	{
		
		/*set the pixel clock PLL(s)*/
		if (gx00_dac_set_pix_pll(target)==B_ERROR)
			LOG(8,("SET: error setting pixel clock (internal DAC)\n"));
		if (si->ps.secondary_head)
		{
			if (gx00_maven_set_pix_pll((target.timing.pixel_clock)/1000.0)==B_ERROR)
				LOG(8,("SET: error setting pixel clock (MAVEN)\n"));
		}
		else
		{
			LOG(8,("SET: not setting maven clock (G450?)\n"));
		}

		/*set the colour depth for CRTC1, CRTC2 and the DAC*/
		switch(target.space)
		{
		case B_RGB16_LITTLE:
			colour_depth=16;
			gx00_dac_mode(BPP16,1.0);
			gx00_crtc_depth(BPP16);
			g400_crtc2_depth(BPP16);
			break;
		case B_RGB32_LITTLE:
			colour_depth=32;
			gx00_dac_mode(BPP32,1.0);
			gx00_crtc_depth(BPP32);
			g400_crtc2_depth(BPP32DIR);
			break;
		default:
			LOG(8,("SET: Invalid dualhead colour depth 0x%08x - should never happen!\n", target.space));
		}

		/*set the display(s) pitches*/
		gx00_crtc_set_display_pitch (target.virtual_width, colour_depth);
		g400_crtc2_set_display_pitch (target.virtual_width, colour_depth);

		/*work out where the "right" screen starts*/
		startadd_right=startadd+(target.timing.h_display*(colour_depth>>3));

		/*set the output DAC*/
		switch (si->ps.card_type)
		{
		case G450:
			gx00_general_dac_select(DS_CRTCDAC_CRTC2DAC2);
			switched_crtcs = false;
			break;
		case G400:case G400MAX:
			gx00_general_dac_select(DS_CRTCMAVEN_CRTC2DAC);
			switched_crtcs = false;
			break;
		default:
			break;
		}

		if (switched_crtcs)
		{
				int temp = startadd;
				startadd = startadd_right;
				startadd_right = temp;
		}

		/*Tell card what memory to display*/
		si->crtc_delay=17+4*(colour_depth==16);
		switch (target.flags&DUALHEAD_BITS)
		{
		case DUALHEAD_ON:
			gx00_crtc_set_display_start(startadd_right,colour_depth);
			g400_crtc2_set_display_start(startadd,colour_depth);
			break;
		case DUALHEAD_CLONE:
			gx00_crtc_set_display_start(startadd,colour_depth);
			g400_crtc2_set_display_start(startadd,colour_depth);
			break;
		case DUALHEAD_SWITCH:
			gx00_crtc_set_display_start(startadd,colour_depth);
			g400_crtc2_set_display_start(startadd_right,colour_depth);
			break;
		}

		/*set the timing*/
		result = gx00_crtc_set_timing /*crtc1*/
		(
			target.timing.h_display,
			target.timing.h_sync_start,
			target.timing.h_sync_end,
			target.timing.h_total,
			(target.timing.v_display+1), /*extra "blanking" line for MAVEN*/
			target.timing.v_sync_start,
			target.timing.v_sync_end,
			target.timing.v_total,
			target.timing.flags&B_POSITIVE_HSYNC,
			target.timing.flags&B_POSITIVE_VSYNC
		);

		result = g400_crtc2_set_timing 
		(
			target.timing.h_display,
			target.timing.h_sync_start,
			target.timing.h_sync_end,
			target.timing.h_total,
			(target.timing.v_display),
			target.timing.v_sync_start,
			target.timing.v_sync_end,
			target.timing.v_total,
			target.timing.flags&B_POSITIVE_HSYNC,
			target.timing.flags&B_POSITIVE_VSYNC
		);
		if (si->ps.secondary_head)
		{
			result = gx00_maven_set_timing /*maven*/
			(
				target.timing.h_display,
				target.timing.h_sync_start,
				target.timing.h_sync_end,
				target.timing.h_total,
				(target.timing.v_display+1), /*extra "blanking" line*/
				target.timing.v_sync_start,
				target.timing.v_sync_end,
				target.timing.v_total,
				target.timing.flags&B_POSITIVE_HSYNC,
				target.timing.flags&B_POSITIVE_VSYNC
			);
		}

		/*turn screen one on and screen two on*/
		gx00_crtc_dpms(display,h,v);
		g400_crtc2_dpms(display,h,v);
		if (si->ps.secondary_head)
			gx00_maven_dpms(display,h,v);
		
		/*TVout support*/
		if (si->ps.secondary_tvout && (target.flags&TV_BITS))
		{
			si->crtc_delay+=5;

			/*create a my tim(m)ing structure... for tvout*/
			tv_timing.pixclock = target.timing.pixel_clock;
			tv_timing.HDisplay = target.timing.h_display;
			tv_timing.HSyncStart = target.timing.h_sync_start;
			tv_timing.HSyncEnd = target.timing.h_sync_end;
			tv_timing.HTotal = target.timing.h_total;
			tv_timing.VDisplay = target.timing.v_display;
			tv_timing.VSyncStart = target.timing.v_sync_start;
			tv_timing.VSyncEnd = target.timing.v_sync_end;
			tv_timing.VTotal = target.timing.v_total;
			tv_timing.delay=si->crtc_delay;

			if (target.flags&TV_PAL)
			{
				LOG(2, ("OUTMODE: PAL\n"));
				maven_set_mode(2);
			}
			else
			{
				LOG(2, ("OUTMODE: NTSC\n"));
				maven_set_mode(1);
			}

			maven_out_compute(&tv_timing, &tv_regs);
			maven_out_program(&tv_regs);
			maven_out_start();
		}
	}
	else /*single head mode*/
	{
		status_t status;
		int      colour_mode = BPP32;
		
		switch(target.space)
		{
		case B_CMAP8:        colour_depth =  8; colour_mode = BPP8;  break;
		case B_RGB15_LITTLE: colour_depth = 16; colour_mode = BPP15; break;
		case B_RGB16_LITTLE: colour_depth = 16; colour_mode = BPP16; break;
		case B_RGB32_LITTLE: colour_depth = 32; colour_mode = BPP32; break;
		default:
			LOG(8,("SET: Invalid singlehead colour depth 0x%08x\n", target.space));
			return B_ERROR;
		}

		/*set the pixel clock PLL*/
		if (si->ps.card_type >= G100) 
			status = gx00_dac_set_pix_pll(target);
		else status = mil2_dac_set_pix_pll((target.timing.pixel_clock)/1000.0, colour_depth);
		if (status==B_ERROR)
			LOG(8,("CRTC: error setting pixel clock (internal DAC)\n"));

		/*set the colour depth for CRTC1 and the DAC*/
		if (si->ps.card_type >= G100) gx00_dac_mode(colour_mode,1.0);
		else mil2_dac_mode(colour_mode,1.0, 
			target.timing.flags&B_POSITIVE_HSYNC,
			target.timing.flags&B_POSITIVE_VSYNC,
			target.timing.flags&B_SYNC_ON_GREEN);

		gx00_crtc_depth(colour_mode);
		
		/*set the display pitch*/
		gx00_crtc_set_display_pitch (target.virtual_width, colour_depth);

		/*tell the card what memory to display*/
		gx00_crtc_set_display_start(startadd,colour_depth);
		if (si->ps.card_type >= G100) 
			gx00_general_dac_select(DS_CRTCDAC_CRTC2MAVEN);
		
		/*set the timing*/
		result = gx00_crtc_set_timing /*crtc1*/
		(
			target.timing.h_display,
			target.timing.h_sync_start,
			target.timing.h_sync_end,
			target.timing.h_total,
			target.timing.v_display,
			target.timing.v_sync_start,
			target.timing.v_sync_end,
			target.timing.v_total,
			target.timing.flags&B_POSITIVE_HSYNC,
			target.timing.flags&B_POSITIVE_VSYNC
		);

		/*turn screen one on and screen two off*/
		gx00_crtc_dpms(display,h,v);
		if (si->ps.card_type >= G400) // apsed TODO when g200 pixrdmsk is broken
			g400_crtc2_dpms(0,0,0);
		if (si->ps.secondary_head)
			gx00_maven_dpms(0,0,0);
	}
	
	/*update driver's mode store*/
	si->dm=target;
	si->fbc.bytes_per_row=target.virtual_width*(colour_depth>>3);

	/*set up acceleration for this mode*/
	si->dm.virtual_height+=1;//for clipping!
	gx00_acc_init();
	si->dm.virtual_height-=1;

	/*clear line at bottom of screen (For maven) if dualhead mode*/
	gx00_acc_rectangle(0,si->dm.virtual_width+1,si->dm.virtual_height,1,0);

	MSG(("INIT_ACCELERANT: booted since %f ms\n", system_time()/1000.0));

	/* enable interrupts using the kernel driver */
	interrupt_enable(true);

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
	if (si->dm.flags&DUALHEAD_BITS)
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
	if ((si->dm.timing.h_display + h_display_start) > si->dm.virtual_width)
		return B_ERROR;
	if ((si->dm.timing.v_display + v_display_start) > si->dm.virtual_height)
		return B_ERROR;

	/* everybody remember where we parked... */
	si->dm.h_display_start = h_display_start;
	si->dm.v_display_start = v_display_start;

	/* actually set the registers */
	startadd=v_display_start*(si->dm.virtual_width*colour_depth)>>3;
	startadd+=h_display_start;
	startadd+=(si->fbc.frame_buffer)-(si->framebuffer);
	startadd_right=startadd+si->dm.timing.h_display*(colour_depth>>3);

	interrupt_enable(false);

	switch (si->dm.flags&DUALHEAD_BITS)
	{
		case DUALHEAD_ON:
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
		case DUALHEAD_SWITCH:
			g400_crtc2_set_display_start(startadd,colour_depth);
			gx00_crtc_set_display_start(startadd_right,colour_depth);
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
	if (si->ps.card_type >= G100) gx00_dac_palette(r,g,b);
	else mil2_dac_palette(r,g,b);
}


/* masks for DPMS control bits */
enum {
	H_SYNC_OFF = 0x01,
	V_SYNC_OFF = 0x02,
	DISPLAY_OFF = 0x04,
	BITSMASK = H_SYNC_OFF | V_SYNC_OFF | DISPLAY_OFF
};

/*
	Put the display into one of the Display Power Management modes.
*/
status_t SET_DPMS_MODE(uint32 dpms_flags) {
	interrupt_enable(false);

	LOG(4,("SET_DPMS_MODE: 0x%08x\n", dpms_flags));
	
	if (si->dm.flags&DUALHEAD_BITS) /*dualhead*/
	{
		switch(dpms_flags) 
		{
		case B_DPMS_ON:	/* H: on, V: on */
			gx00_crtc_dpms(1,1,1);
			g400_crtc2_dpms(1,1,1);
			if (si->ps.secondary_head)
				gx00_maven_dpms(1,1,1);
			break;
		case B_DPMS_STAND_BY:
			gx00_crtc_dpms(0,0,1);
			g400_crtc2_dpms(0,0,1);
			if (si->ps.secondary_head)
				gx00_maven_dpms(0,0,1);
			break;
		case B_DPMS_SUSPEND:
			gx00_crtc_dpms(0,1,0);
			g400_crtc2_dpms(0,1,0);
			if (si->ps.secondary_head)
				gx00_maven_dpms(0,1,0);
			break;
		case B_DPMS_OFF: /* H: off, V: off, display off */
			gx00_crtc_dpms(0,0,0);
			g400_crtc2_dpms(0,0,0);
			if (si->ps.secondary_head)
				gx00_maven_dpms(0,0,0);
			break;
		default:
			LOG(8,("SET: Invalid DPMS settings (DH) 0x%08x\n", dpms_flags));
			interrupt_enable(true);
			return B_ERROR;
		}
	}
	else /*singlehead*/
	{
		switch(dpms_flags) 
		{
		case B_DPMS_ON:	/* H: on, V: on */
			gx00_crtc_dpms(1,1,1);
			break;
		case B_DPMS_STAND_BY:
			gx00_crtc_dpms(0,0,1);
			break;
		case B_DPMS_SUSPEND:
			gx00_crtc_dpms(0,1,0);
			break;
		case B_DPMS_OFF: /* H: off, V: off, display off */
			gx00_crtc_dpms(0,0,0);
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

/*
	Report device DPMS capabilities.
*/
uint32 DPMS_CAPABILITIES(void) {
	return 	B_DPMS_ON | B_DPMS_STAND_BY | B_DPMS_SUSPEND|B_DPMS_OFF;
}


/*
	Return the current DPMS mode.
*/
uint32 DPMS_MODE(void) {
	uint8 display,h,v;
	
	interrupt_enable(false);
	gx00_crtc_dpms_fetch(&display,&h,&v);
	interrupt_enable(true);

	if (display&h&v)
		return B_DPMS_ON;
	else if(v)
		return B_DPMS_STAND_BY;
	else if(h)
		return B_DPMS_SUSPEND;
	else
		return B_DPMS_OFF;
}
