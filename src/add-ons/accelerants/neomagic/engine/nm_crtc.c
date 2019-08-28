/* CTRC functionality */
/* Author:
   Rudolf Cornelissen 4/2003-1/2006
*/

#define MODULE_BIT 0x00040000

#include "nm_std.h"

/* Adjust passed parameters to a valid mode line */
status_t nm_crtc_validate_timing(
	uint16 *hd_e,uint16 *hs_s,uint16 *hs_e,uint16 *ht,
	uint16 *vd_e,uint16 *vs_s,uint16 *vs_e,uint16 *vt
)
{
/* horizontal */
	/* make all parameters multiples of 8 */
	*hd_e &= 0xfff8;
	*hs_s &= 0xfff8;
	*hs_e &= 0xfff8;
	*ht   &= 0xfff8;

	/* confine to required number of bits, taking logic into account */
	if (*hd_e > ((0xff - 2) << 3)) *hd_e = ((0xff - 2) << 3);
	if (*hs_s > ((0xff - 1) << 3)) *hs_s = ((0xff - 1) << 3);
	if (*hs_e > ( 0xff      << 3)) *hs_e = ( 0xff      << 3);
	if (*ht   > ((0xff + 5) << 3)) *ht   = ((0xff + 5) << 3);

	/* NOTE: keep horizontal timing at multiples of 8! */
	/* confine to a reasonable width */
	if (*hd_e < 640) *hd_e = 640;
	if (*hd_e > si->ps.max_crtc_width) *hd_e = si->ps.max_crtc_width;

	/* if hor. total does not leave room for a sensible sync pulse,*/
	/* increase it! */
	if (*ht < (*hd_e + 80)) *ht = (*hd_e + 80);

	/* if hor. total does not adhere to max. blanking pulse width,*/
	/* decrease it! */
	if (*ht > (*hd_e + 0x1f8)) *ht = (*hd_e + 0x1f8);

	/* make sure sync pulse is not during display */
	if (*hs_e > (*ht - 8)) *hs_e = (*ht - 8);
	if (*hs_s < (*hd_e + 8)) *hs_s = (*hd_e + 8);

	/* correct sync pulse if it is too long:
	 * there are only 5 bits available to save this in the card registers! */
	if (*hs_e > (*hs_s + 0xf8)) *hs_e = (*hs_s + 0xf8);

/*vertical*/
	/* confine to required number of bits, taking logic into account */
	if (si->ps.card_type < NM2200)
	{
		if (*vd_e > (0x3ff - 2)) *vd_e = (0x3ff - 2);
		if (*vs_s > (0x3ff - 1)) *vs_s = (0x3ff - 1);
		if (*vs_e >  0x3ff     ) *vs_e =  0x3ff     ;
		if (*vt   > (0x3ff + 2)) *vt   = (0x3ff + 2);
	}
	else
	{
		if (*vd_e > (0x7ff - 2)) *vd_e = (0x7ff - 2);
		if (*vs_s > (0x7ff - 1)) *vs_s = (0x7ff - 1);
		if (*vs_e >  0x7ff     ) *vs_e =  0x7ff     ;
		if (*vt   > (0x7ff + 2)) *vt   = (0x7ff + 2);
	}

	/* confine to a reasonable height */
	if (*vd_e < 480) *vd_e = 480;
	if (*vd_e > si->ps.max_crtc_height) *vd_e = si->ps.max_crtc_height;

	/*if vertical total does not leave room for a sync pulse, increase it!*/
	if (*vt < (*vd_e + 3)) *vt = (*vd_e + 3);

	/* if vert. total does not adhere to max. blanking pulse width,*/
	/* decrease it! */
	if (*vt > (*vd_e + 0xff)) *vt = (*vd_e + 0xff);

	/* make sure sync pulse is not during display */
	if (*vs_e > (*vt - 1)) *vs_e = (*vt - 1);
	if (*vs_s < (*vd_e + 1)) *vs_s = (*vd_e + 1);

	/* correct sync pulse if it is too long:
	 * there are only 4 bits available to save this in the card registers! */
	if (*vs_e > (*vs_s + 0x0f)) *vs_e = (*vs_s + 0x0f);

	return B_OK;
}

/* set a mode line */
status_t nm_crtc_set_timing(display_mode target, bool crt_only)
{
	uint8 temp;

	uint32 htotal;		/*total horizontal total VCLKs*/
	uint32 hdisp_e;            /*end of horizontal display (begins at 0)*/
	uint32 hsync_s;            /*begin of horizontal sync pulse*/
	uint32 hsync_e;            /*end of horizontal sync pulse*/
	uint32 hblnk_s;            /*begin horizontal blanking*/
	uint32 hblnk_e;            /*end horizontal blanking*/

	uint32 vtotal;		/*total vertical total scanlines*/
	uint32 vdisp_e;            /*end of vertical display*/
	uint32 vsync_s;            /*begin of vertical sync pulse*/
	uint32 vsync_e;            /*end of vertical sync pulse*/
	uint32 vblnk_s;            /*begin vertical blanking*/
	uint32 vblnk_e;            /*end vertical blanking*/

	uint32 linecomp;	/*split screen and vdisp_e interrupt*/

	LOG(4,("CRTC: setting timing\n"));

	/* modify visible sceensize if needed */
	/* (note that MOVE_CURSOR checks for a panning/scrolling mode itself,
	 *  the display_mode as placed in si->dm may _not_ be modified!) */
	if (!crt_only)
	{
		if (target.timing.h_display > si->ps.panel_width)
		{
			target.timing.h_display = si->ps.panel_width;
			LOG(4,
				("CRTC: req. width > panel width: setting panning mode\n"));
		}
		if (target.timing.v_display > si->ps.panel_height)
		{
			target.timing.v_display = si->ps.panel_height;
			LOG(4,
				("CRTC: req. height > panel height: setting scrolling "
				"mode\n"));
		}

		/* modify sync polarities
		 * (needed to maintain correct panel centering):
		 * both polarities must be negative (confirmed NM2160) */
		target.timing.flags &= ~(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC);
	}

	/* Modify parameters as required by standard VGA */
	htotal = ((target.timing.h_total >> 3) - 5);
	hdisp_e = ((target.timing.h_display >> 3) - 1);
	hblnk_s = hdisp_e;
	hblnk_e = (htotal + 0); /* this register differs from standard VGA! */
		/* (says + 4) */
	hsync_s = (target.timing.h_sync_start >> 3);
	hsync_e = (target.timing.h_sync_end >> 3);

	vtotal = target.timing.v_total - 2;
	vdisp_e = target.timing.v_display - 1;
	vblnk_s = vdisp_e;
	vblnk_e = (vtotal + 1);
	vsync_s = target.timing.v_sync_start;//-1;
	vsync_e = target.timing.v_sync_end;//-1;

	/* prevent memory adress counter from being reset */
	/* (linecomp may not occur) */
	linecomp = target.timing.v_display;

	if (crt_only)
	{
		LOG(4,("CRTC: CRT only mode, setting full timing...\n"));

		/* log the mode that will be set */
		LOG(2,("CRTC:\n\tHTOT:%x\n\tHDISPEND:%x\n\tHBLNKS:%x\n\tHBLNKE:%x\n\tHSYNCS:%x\n\tHSYNCE:%x\n\t",htotal,hdisp_e,hblnk_s,hblnk_e,hsync_s,hsync_e));
		LOG(2,("VTOT:%x\n\tVDISPEND:%x\n\tVBLNKS:%x\n\tVBLNKE:%x\n\tVSYNCS:%x\n\tVSYNCE:%x\n",vtotal,vdisp_e,vblnk_s,vblnk_e,vsync_s,vsync_e));

		/* actually program the card! */
		/* unlock CRTC registers at index 0-7 */
		temp = (ISACRTCR(VSYNCE) & 0x7f);
		/* we need to wait a bit or the card will mess-up it's */
		/* register values.. */
		snooze(10);
		ISACRTCW(VSYNCE, temp);
		/* horizontal standard VGA regs */
		ISACRTCW(HTOTAL, (htotal & 0xff));
		ISACRTCW(HDISPE, (hdisp_e & 0xff));
		ISACRTCW(HBLANKS, (hblnk_s & 0xff));
		/* also unlock vertical retrace registers in advance */
		ISACRTCW(HBLANKE, ((hblnk_e & 0x1f) | 0x80));
		ISACRTCW(HSYNCS, (hsync_s & 0xff));
		ISACRTCW(HSYNCE, ((hsync_e & 0x1f) | ((hblnk_e & 0x20) << 2)));

		/* vertical standard VGA regs */
		ISACRTCW(VTOTAL, (vtotal & 0xff));
		ISACRTCW(OVERFLOW,
		(
			((vtotal & 0x100) >> (8 - 0)) | ((vtotal & 0x200) >> (9 - 5)) |
			((vdisp_e & 0x100) >> (8 - 1)) | ((vdisp_e & 0x200) >> (9 - 6)) |
			((vsync_s & 0x100) >> (8 - 2)) | ((vsync_s & 0x200) >> (9 - 7)) |
			((vblnk_s & 0x100) >> (8 - 3)) | ((linecomp & 0x100) >> (8 - 4))
		));
		ISACRTCW(PRROWSCN, 0x00); /* not used */
		ISACRTCW(MAXSCLIN, (((vblnk_s & 0x200) >> (9 - 5))
			| ((linecomp & 0x200) >> (9 - 6))));
		ISACRTCW(VSYNCS, (vsync_s & 0xff));
		temp = (ISACRTCR(VSYNCE) & 0xf0);
		/* we need to wait a bit or the card will mess-up it's */
		/* register values.. */
		snooze(10);
		ISACRTCW(VSYNCE, (temp | (vsync_e & 0x0f)));
		ISACRTCW(VDISPE, (vdisp_e & 0xff));
		ISACRTCW(VBLANKS, (vblnk_s & 0xff));
		ISACRTCW(VBLANKE, (vblnk_e & 0xff));
//linux: (BIOSmode)
//	regp->CRTC[23] = 0xC3;
		ISACRTCW(LINECOMP, (linecomp & 0xff));

		/* horizontal - no extended regs available or needed on */
		/* NeoMagic chips */

		/* vertical - extended regs */
//fixme: checkout if b2 or 3 should be switched! (linux contains error here)
//fixme: linecomp should also have an extra bit... testable by setting linecomp
//to 100 for example and then try out writing an '1' to b2, b3(!) and the rest
//for screenorig reset visible on upper half of the screen or not at all..
		if (si->ps.card_type >= NM2200)
			ISACRTCW(VEXT,
			(
			 	((vtotal & 0x400) >> (10 - 0)) |
				((vdisp_e & 0x400) >> (10 - 1)) |
				((vblnk_s & 0x400) >> (10 - 2)) |
				((vsync_s & 0x400) >> (10 - 3))/*|
				((linecomp&0x400)>>3)*/
			));
	}
	else
	{
		LOG(4,
			("CRTC: internal flatpanel active, setting display region only\n"));

		/* actually program the card! */
		/* unlock CRTC registers at index 0-7 */
		temp = (ISACRTCR(VSYNCE) & 0x7f);
		/* we need to wait a bit or the card will mess-up it's */
		/* register values.. */
		snooze(10);
		ISACRTCW(VSYNCE, temp);
		/* horizontal standard VGA regs */
		ISACRTCW(HDISPE, (hdisp_e & 0xff));

		/* vertical standard VGA regs */
		temp = (ISACRTCR(OVERFLOW) & ~0x52);
		/* we need to wait a bit or the card will mess-up it's */
		/* register values.. */
		snooze(10);
		ISACRTCW(OVERFLOW,
		(
			temp |
			((vdisp_e & 0x100) >> (8 - 1)) |
			((vdisp_e & 0x200) >> (9 - 6)) |
			((linecomp & 0x100) >> (8 - 4))
		));

		ISACRTCW(PRROWSCN, 0x00); /* not used */

		temp = (ISACRTCR(MAXSCLIN) & ~0x40);
		/* we need to wait a bit or the card will mess-up it's */
		/* register values.. */
		snooze(10);
		ISACRTCW(MAXSCLIN, (temp | ((linecomp & 0x200) >> (9 - 6))));

		ISACRTCW(VDISPE, (vdisp_e & 0xff));
//linux:(BIOSmode)
//	regp->CRTC[23] = 0xC3;
		ISACRTCW(LINECOMP, (linecomp & 0xff));

		/* horizontal - no extended regs available or needed on */
		/* NeoMagic chips */

		/* vertical - extended regs */
//fixme: linecomp should have an extra bit... testable by setting linecomp
//to 100 for example and then try out writing an '1' to b2, b3(!) and the rest
//for screenorig reset visible on upper half of the screen or not at all..
		if (si->ps.card_type >= NM2200)
		{
			temp = (ISACRTCR(VEXT) & ~0x02);
			/* we need to wait a bit or the card will mess-up it's */
			/* register values.. */
			snooze(10);
			ISACRTCW(VEXT,
			(
				temp |
				((vdisp_e & 0x400) >> (10 - 1))/*|
				((linecomp&0x400)>>3)*/
			));
		}
	}

	/* setup HSYNC & VSYNC polarity */
	LOG(2,("CRTC: sync polarity: "));
	temp = ISARB(MISCR);
	if (target.timing.flags & B_POSITIVE_HSYNC)
	{
		LOG(2,("H:pos "));
		temp &= ~0x40;
	}
	else
	{
		LOG(2,("H:neg "));
		temp |= 0x40;
	}
	if (target.timing.flags & B_POSITIVE_VSYNC)
	{
		LOG(2,("V:pos "));
		temp &= ~0x80;
	}
	else
	{
		LOG(2,("V:neg "));
		temp |= 0x80;
	}
	/* we need to wait a bit or the card will mess-up it's register values.. */
	snooze(10);
	ISAWB(MISCW, temp);
	LOG(2,(", MISC reg readback: $%02x\n", ISARB(MISCR)));

	/* program 'fixed' mode if needed */
	if (si->ps.card_type != NM2070)
	{
		uint8 width;

		temp = ISAGRPHR(PANELCTRL1);
		/* we need to wait a bit or the card will mess-up it's register values.. */
		snooze(10);

		switch (target.timing.h_display)
		{
		case 1280:
			width = (3 << 5);
			break;
		case 1024:
			width = (2 << 5);
			break;
		case 800:
			width = (1 << 5);
			break;
		case 640:
		default: //fixme: non-std modes should be in between above modes?!?
			width = (0 << 5);
			break;
		}

		switch (si->ps.card_type)
		{
		case NM2090:
		case NM2093:
		case NM2097:
		case NM2160:
			//fixme: checkout b6????
			ISAGRPHW(PANELCTRL1, ((temp & ~0x20) | (width & 0x20)));
			break;
		default:
			/* NM2200 and later */
			ISAGRPHW(PANELCTRL1, ((temp & ~0x60) | (width & 0x60)));
			break;
		}
	}

	return B_OK;
}

status_t nm_crtc_depth(int mode)
{
	uint8 vid_delay = 0;

	LOG(4,("CRTC: setting colordepth to be displayed\n"));

	/* set VCLK scaling */
	switch(mode)
	{
	case BPP8:
		vid_delay = 0x01;
		break;
	case BPP15:
		vid_delay = 0x02;
		break;
	case BPP16:
		vid_delay = 0x03;
		break;
	case BPP24:
		vid_delay = 0x04;
		break;
	default:
		LOG(4,("CRTC: colordepth not supported, aborting!\n"));
		return B_ERROR;
		break;
	}

	switch (si->ps.card_type)
	{
	case NM2070:
		vid_delay |= (ISAGRPHR(COLDEPTH) & 0xf0);
		break;
	default:
		vid_delay |= (ISAGRPHR(COLDEPTH) & 0x70);
		break;
	}
	/* we need to wait a bit or the card will mess-up it's */
	/* register values.. (NM2160) */
	snooze(10);
	ISAGRPHW(COLDEPTH, vid_delay);

	snooze(10);
	LOG(4,("CRTC: colordepth register readback $%02x\n", (ISAGRPHR(COLDEPTH))));

	return B_OK;
}

status_t nm_crtc_dpms(bool display, bool h, bool v)
{
	char msg[100];
	const char* displayStatus;
	const char* horizontalSync;
	const char* verticalSync;

	uint8 temp, size_outputs;

	sprintf(msg, "CRTC: setting DPMS: ");

	/* start synchronous reset: required before turning screen off! */
	ISASEQW(RESET, 0x01);

	/* turn screen off */
	temp = ISASEQR(CLKMODE);
	/* we need to wait a bit or the card will mess-up it's */
	/* register values.. (NM2160) */
	snooze(10);

	if (display)
	{
		ISASEQW(CLKMODE, (temp & ~0x20));

		/* end synchronous reset if display should be enabled */
		ISASEQW(RESET, 0x03);
		displayStatus = "on";
	}
	else
	{
		ISASEQW(CLKMODE, (temp | 0x20));
		displayStatus = "off";
	}

	temp = 0x00;
	if (h)
	{
		horizontalSync = "enabled";
	}
	else
	{
		temp |= 0x10;
		horizontalSync = "disabled";
	}
	if (v)
	{
		verticalSync = "enabled";
	}
	else
	{
		temp |= 0x20;
		verticalSync = "disabled";
	}

	snprintf(msg, sizeof(msg),
	"CRTC: setting DPMS: display %s, hsync %s, vsync %s\n",
		displayStatus, horizontalSync, verticalSync);
	LOG(4, (msg));

	/* read panelsize and currently active outputs */
	size_outputs = nm_general_output_read();
	/* we need to wait a bit or the card will mess-up it's register */
	/* values.. (NM2160) */
	snooze(10);

	if (si->ps.card_type < NM2200)
	{
		/* no full DPMS support */
		if (temp)
		{
		    /* Turn panel plus backlight and external monitor's */
			/* sync signals off */
    		ISAGRPHW(PANELCTRL1, (size_outputs & 0xfc));
		}
		else
		{
		    /* restore 'previous' output device(s) */
    		ISAGRPHW(PANELCTRL1, size_outputs);
		}
	}
	else
	{
		if (temp)
		{
		    /* Turn panel plus backlight off */
   			ISAGRPHW(PANELCTRL1, (size_outputs & 0xfd));
		}
		else
		{
		    /* restore 'previous' panel situation */
   			ISAGRPHW(PANELCTRL1, size_outputs);
		}

		/* if external monitor is active, update it's DPMS state */
		if (size_outputs & 0x01)
		{
			/* we have full DPMS support for external monitors */
			//fixme: checkout if so...
			temp |= ((ISAGRPHR(ENSETRESET) & 0x0f) | 0x80);
			/* we need to wait a bit or the card will mess-up it's */
			/* register values.. (NM2160) */
			snooze(10);
			ISAGRPHW(ENSETRESET, temp);

			snooze(10);
			LOG(4,("CRTC: DPMS readback $%02x, programmed $%02x\n",
				ISAGRPHR(ENSETRESET), temp));
		}
	}

	return B_OK;
}

status_t nm_crtc_set_display_pitch()
{
	uint32 offset;

	LOG(4,("CRTC: setting card pitch (offset between lines)\n"));

	/* figure out offset value hardware needs: same for all Neomagic cards */
	offset = si->fbc.bytes_per_row / 8;

	LOG(2,("CRTC: offset register set to: $%04x\n", offset));

	/* program the card */
	ISACRTCW(PITCHL, (offset & 0xff));
	//fixme: test for max supported pitch if possible,
	//not all bits below will be implemented.
	//NM2160: confirmed b0 and b1 in register below to exist and work.
	if (si->ps.card_type != NM2070)
		ISAGRPHW(CRTC_PITCHE, ((offset & 0xff00) >> 8));

	return B_OK;
}

status_t nm_crtc_set_display_start(uint32 startadd,uint8 bpp)
{
	uint8 val;
	uint32 timeout = 0;

	LOG(2,("CRTC: relative startadd: $%06x\n",startadd));
	LOG(2,("CRTC: frameRAM: $%08x\n",si->framebuffer));
	LOG(2,("CRTC: framebuffer: $%08x\n",si->fbc.frame_buffer));

	/* make sure we _just_ left retrace, because otherwise distortions */
	/* might occur during our reprogramming (no double buffering) */
	/* (verified on NM2160) */

	/* we might have no retraces during setmode! So:
	 * wait 25mS max. for retrace to occur (refresh > 40Hz) */
	//fixme? move this function to the kernel driver... is much 'faster'.
	while ((!(ISARB(INSTAT1) & 0x08)) && (timeout < (25000/4)))
	{
		snooze(4);
		timeout++;
	}
	/* now wait until retrace ends (with timeout) */
	timeout = 0;
	while ((ISARB(INSTAT1) & 0x08) && (timeout < (25000/4)))
	{
		snooze(4);
		timeout++;
	}

	/* set standard VGA registers */
	/* (startadress in 32bit words (b2 - b17) */
    ISACRTCW(FBSTADDH, ((startadd & 0x03fc00) >> 10));
    ISACRTCW(FBSTADDL, ((startadd & 0x0003fc) >> 2));

	/* set NM extended register */
	//fixme: NM2380 _must_ have one more bit (has >4Mb RAM)!!
	//this is testable via virtualscreen in 640x480x8 mode...
	//(b4 is >256Kb adresswrap bit, so that's already occupied)
	val = ISAGRPHR(FBSTADDE);
	/* we need to wait a bit or the card will mess-up it's */
	/* register values.. (NM2160) */
	snooze(10);
	if (si->ps.card_type < NM2200)
		/* extended bits: (b18-20) */
		ISAGRPHW(FBSTADDE,(((startadd >> 18) & 0x07) | (val & 0xf8)));
	else
		/* extended bits: (b18-21) */
		ISAGRPHW(FBSTADDE,(((startadd >> 18) & 0x0f) | (val & 0xf0)));

	/* set byte adress: (b0 - 1):
	 * Neomagic cards work with _pixel_ offset here. */
	switch(bpp)
	{
	case 8:
		ISAATBW(HORPIXPAN, (startadd & 0x00000003));
		break;
	case 15:
	case 16:
		ISAATBW(HORPIXPAN, ((startadd & 0x00000002) >> 1));
		break;
	case 24:
		ISAATBW(HORPIXPAN, ((4 - (startadd & 0x00000003)) & 0x03));
		break;
	}

	return B_OK;
}

/* setup centering mode for current internal or simultaneous  */
/* flatpanel mode */
status_t nm_crtc_center(display_mode target, bool crt_only)
{
	/* note:
	 * NM2070 apparantly doesn't support horizontal */
	/* centering this way... */

	uint8 vcent1, vcent2, vcent3, vcent4, vcent5;
	uint8 hcent1, hcent2, hcent3, hcent4, hcent5;
	uint8 ctrl2, ctrl3;

	/* preset no centering */
	uint16 hoffset = 0;
	uint16 voffset = 0;
	vcent1 = vcent2 = vcent3 = vcent4 = vcent5 = 0x00;
	hcent1 = hcent2 = hcent3 = hcent4 = hcent5 = 0x00;
	ctrl2 = ctrl3 = 0x00;

	/* calculate offsets for centering if prudent */
	if (!crt_only)
	{
		if (target.timing.h_display < si->ps.panel_width)
		{
			hoffset = (si->ps.panel_width - target.timing.h_display);
			/* adjust for register contraints:
			 * horizontal center granularity is 16 pixels */
			hoffset = ((hoffset >> 4) - 1);
			/* turn on horizontal centering? */
			ctrl3 = 0x10;
		}

		if (target.timing.v_display < si->ps.panel_height)
		{
			voffset = (si->ps.panel_height - target.timing.v_display);
			/* adjust for register contraints:
			 * vertical center granularity is 2 pixels */
			voffset = ((voffset >> 1) - 2);
			/* turn on vertical centering? */
			ctrl2 = 0x01;
		}

		switch(target.timing.h_display)
		{
		case 640:
			hcent1 = hoffset;
			vcent3 = voffset;
			break;
		case 800:
			hcent2 = hoffset;
			switch(target.timing.v_display)
			{
			case 480:
				//Linux fixme: check this out...
				vcent3 = voffset;
				break;
			case 600:
				vcent4 = voffset;
				break;
			}
			break;
		case 1024:
			hcent5 = hoffset;
			vcent5 = voffset;
			break;
		case 1280:
			/* this mode equals the largest possible panel on the *
			 * newest chip: so no centering needed here. */
			break;
		default:
			//fixme?: block non-standard modes? for now: not centered.
			break;
		}
	}

	/* now program the card's registers */
	ISAGRPHW(PANELVCENT1, vcent1);
	ISAGRPHW(PANELVCENT2, vcent2);
	ISAGRPHW(PANELVCENT3, vcent3);
	if (si->ps.card_type > NM2070)
	{
		ISAGRPHW(PANELVCENT4, vcent4);
		ISAGRPHW(PANELHCENT1, hcent1);
		ISAGRPHW(PANELHCENT2, hcent2);
		ISAGRPHW(PANELHCENT3, hcent3);
	}
	if (si->ps.card_type >= NM2160)
	{
		ISAGRPHW(PANELHCENT4, hcent4);
	}
	if (si->ps.card_type >= NM2200)
	{
		ISAGRPHW(PANELVCENT5, vcent5);
		ISAGRPHW(PANELHCENT5, hcent5);
	}

	/* program panel control register 2: don't touch bit 3-5 */
	ctrl2 |= (ISAGRPHR(PANELCTRL2) & 0x38);
	/* we need to wait a bit or the card will mess-up it's register */
	/* values.. (NM2160) */
	snooze(10);
	ISAGRPHW(PANELCTRL2, ctrl2);

	if (si->ps.card_type > NM2070)
	{
		/* program panel control register 3: don't touch bit 7-5 */
		/* and bit 3-0 */
		ctrl3 |= (ISAGRPHR(PANELCTRL3) & 0xef);
		/* we need to wait a bit or the card will mess-up it's  */
		/* register values.. (NM2160) */
		snooze(10);
		ISAGRPHW(PANELCTRL3, ctrl3);
	}

	return B_OK;
}

/* program panel modeline if needed */
status_t nm_crtc_prg_panel()
{
status_t stat = B_ERROR;

	/* only NM2070 requires this apparantly */
	/* (because it's BIOS doesn't do it OK) */
	if (si->ps.card_type > NM2070) return B_OK;

	switch(si->ps.panel_width)
	{
	case 640:
		/* 640x480 panels are only used on NM2070 */
		ISACRTCW(PANEL_0x40, 0x5f);
		ISACRTCW(PANEL_0x41, 0x50);
		ISACRTCW(PANEL_0x42, 0x02);
		ISACRTCW(PANEL_0x43, 0x55);
		ISACRTCW(PANEL_0x44, 0x81);
		ISACRTCW(PANEL_0x45, 0x0b);
		ISACRTCW(PANEL_0x46, 0x2e);
		ISACRTCW(PANEL_0x47, 0xea);
		ISACRTCW(PANEL_0x48, 0x0c);
		ISACRTCW(PANEL_0x49, 0xe7);
		ISACRTCW(PANEL_0x4a, 0x04);
		ISACRTCW(PANEL_0x4b, 0x2d);
		ISACRTCW(PANEL_0x4c, 0x28);
		ISACRTCW(PANEL_0x4d, 0x90);
		ISACRTCW(PANEL_0x4e, 0x2b);
		ISACRTCW(PANEL_0x4f, 0xa0);
		stat = B_OK;
		break;
	case 800:
		switch(si->ps.panel_height)
		{
		case 600:
			/* 800x600 panels are used on all cards... */
			ISACRTCW(PANEL_0x40, 0x7f);
			ISACRTCW(PANEL_0x41, 0x63);
			ISACRTCW(PANEL_0x42, 0x02);
			ISACRTCW(PANEL_0x43, 0x6c);
			ISACRTCW(PANEL_0x44, 0x1c);
			ISACRTCW(PANEL_0x45, 0x72);
			ISACRTCW(PANEL_0x46, 0xe0);
			ISACRTCW(PANEL_0x47, 0x58);
			ISACRTCW(PANEL_0x48, 0x0c);
			ISACRTCW(PANEL_0x49, 0x57);
			ISACRTCW(PANEL_0x4a, 0x73);
			ISACRTCW(PANEL_0x4b, 0x3d);
			ISACRTCW(PANEL_0x4c, 0x31);
			ISACRTCW(PANEL_0x4d, 0x01);
			ISACRTCW(PANEL_0x4e, 0x36);
			ISACRTCW(PANEL_0x4f, 0x1e);
			if (si->ps.card_type > NM2070)
			{
				ISACRTCW(PANEL_0x50, 0x6b);
				ISACRTCW(PANEL_0x51, 0x4f);
				ISACRTCW(PANEL_0x52, 0x0e);
				ISACRTCW(PANEL_0x53, 0x58);
				ISACRTCW(PANEL_0x54, 0x88);
				ISACRTCW(PANEL_0x55, 0x33);
				ISACRTCW(PANEL_0x56, 0x27);
				ISACRTCW(PANEL_0x57, 0x16);
				ISACRTCW(PANEL_0x58, 0x2c);
				ISACRTCW(PANEL_0x59, 0x94);
			}
			stat = B_OK;
			break;
		case 480:
			/* ...while 800x480 widescreen panels are not used on NM2070. */
			ISACRTCW(PANEL_0x40, 0x7f);
			ISACRTCW(PANEL_0x41, 0x63);
			ISACRTCW(PANEL_0x42, 0x02);
			ISACRTCW(PANEL_0x43, 0x6b);
			ISACRTCW(PANEL_0x44, 0x1b);
			ISACRTCW(PANEL_0x45, 0x72);
			ISACRTCW(PANEL_0x46, 0xe0);
			ISACRTCW(PANEL_0x47, 0x1c);
			ISACRTCW(PANEL_0x48, 0x00);
			ISACRTCW(PANEL_0x49, 0x57);
			ISACRTCW(PANEL_0x4a, 0x73);
			ISACRTCW(PANEL_0x4b, 0x3e);
			ISACRTCW(PANEL_0x4c, 0x31);
			ISACRTCW(PANEL_0x4d, 0x01);
			ISACRTCW(PANEL_0x4e, 0x36);
			ISACRTCW(PANEL_0x4f, 0x1e);
			ISACRTCW(PANEL_0x50, 0x6b);
			ISACRTCW(PANEL_0x51, 0x4f);
			ISACRTCW(PANEL_0x52, 0x0e);
			ISACRTCW(PANEL_0x53, 0x57);
			ISACRTCW(PANEL_0x54, 0x87);
			ISACRTCW(PANEL_0x55, 0x33);
			ISACRTCW(PANEL_0x56, 0x27);
			ISACRTCW(PANEL_0x57, 0x16);
			ISACRTCW(PANEL_0x58, 0x2c);
			ISACRTCW(PANEL_0x59, 0x94);
			stat = B_OK;
			break;
		}
		break;
	case 1024:
		switch(si->ps.panel_height)
		{
		case 768:
			/* 1024x768 panels are only used on later cards
			 * (NM2097 and later ?) */
			ISACRTCW(PANEL_0x40, 0xa3);
			ISACRTCW(PANEL_0x41, 0x7f);
			ISACRTCW(PANEL_0x42, 0x06);
			ISACRTCW(PANEL_0x43, 0x85);
			ISACRTCW(PANEL_0x44, 0x96);
			ISACRTCW(PANEL_0x45, 0x24);
			ISACRTCW(PANEL_0x46, 0xe5);
			ISACRTCW(PANEL_0x47, 0x02);
			ISACRTCW(PANEL_0x48, 0x08);
			ISACRTCW(PANEL_0x49, 0xff);
			ISACRTCW(PANEL_0x4a, 0x25);
			ISACRTCW(PANEL_0x4b, 0x4f);
			ISACRTCW(PANEL_0x4c, 0x40);
			ISACRTCW(PANEL_0x4d, 0x00);
			ISACRTCW(PANEL_0x4e, 0x44);
			ISACRTCW(PANEL_0x4f, 0x0c);
			ISACRTCW(PANEL_0x50, 0x7a);
			ISACRTCW(PANEL_0x51, 0x56);
			ISACRTCW(PANEL_0x52, 0x00);
			ISACRTCW(PANEL_0x53, 0x5d);
			ISACRTCW(PANEL_0x54, 0x0e);
			ISACRTCW(PANEL_0x55, 0x3b);
			ISACRTCW(PANEL_0x56, 0x2b);
			ISACRTCW(PANEL_0x57, 0x00);
			ISACRTCW(PANEL_0x58, 0x2f);
			ISACRTCW(PANEL_0x59, 0x18);
			ISACRTCW(PANEL_0x60, 0x88);
			ISACRTCW(PANEL_0x61, 0x63);
			ISACRTCW(PANEL_0x62, 0x0b);
			ISACRTCW(PANEL_0x63, 0x69);
			ISACRTCW(PANEL_0x64, 0x1a);
			stat = B_OK;
			break;
	    case 480:
			/* 1024x480 widescreen panels are only used on later cards
			 * (NM2097 and later ?) */
			ISACRTCW(PANEL_0x40, 0xa3);
			ISACRTCW(PANEL_0x41, 0x7f);
			ISACRTCW(PANEL_0x42, 0x1b);
			ISACRTCW(PANEL_0x43, 0x89);
			ISACRTCW(PANEL_0x44, 0x16);
			ISACRTCW(PANEL_0x45, 0x0b);
			ISACRTCW(PANEL_0x46, 0x2c);
			ISACRTCW(PANEL_0x47, 0xe8);
			ISACRTCW(PANEL_0x48, 0x0c);
			ISACRTCW(PANEL_0x49, 0xe7);
			ISACRTCW(PANEL_0x4a, 0x09);
			ISACRTCW(PANEL_0x4b, 0x4f);
			ISACRTCW(PANEL_0x4c, 0x40);
			ISACRTCW(PANEL_0x4d, 0x00);
			ISACRTCW(PANEL_0x4e, 0x44);
			ISACRTCW(PANEL_0x4f, 0x0c);
			ISACRTCW(PANEL_0x50, 0x7a);
			ISACRTCW(PANEL_0x51, 0x56);
			ISACRTCW(PANEL_0x52, 0x00);
			ISACRTCW(PANEL_0x53, 0x5d);
			ISACRTCW(PANEL_0x54, 0x0e);
			ISACRTCW(PANEL_0x55, 0x3b);
			ISACRTCW(PANEL_0x56, 0x2a);
			ISACRTCW(PANEL_0x57, 0x00);
			ISACRTCW(PANEL_0x58, 0x2f);
			ISACRTCW(PANEL_0x59, 0x18);
			ISACRTCW(PANEL_0x60, 0x88);
			ISACRTCW(PANEL_0x61, 0x63);
			ISACRTCW(PANEL_0x62, 0x0b);
			ISACRTCW(PANEL_0x63, 0x69);
			ISACRTCW(PANEL_0x64, 0x1a);
			stat = B_OK;
			break;
		}
		break;
	case 1280:
		/* no info available */
		break;
	}

	if (stat != B_OK)
		LOG(2,("CRTC: unable to program panel: unknown modeline needed.\n"));

	return stat;
}

status_t nm_crtc_cursor_init()
{
	int i;
	vuint32 * fb;
	uint32 curadd, curreg;

	/* the cursor is at the end of cardRAM */
	curadd = ((si->ps.memory_size * 1024) - si->ps.curmem_size);

	/* set cursor bitmap adress on a 1kb boundary, and move the bits around
	 * so they get placed at the correct registerbits */
	curreg = (((curadd >> 10) & 0x000f) << 8);
	curreg |= (((curadd >> 10) & 0x0ff0) >> 4);
	/* NM2380 must have an extra bit for > 4Mb: assuming it to be on b12... */
	curreg |= ((curadd >> 10) & 0x1000);

	if (si->ps.card_type < NM2200)
		CR1W(CURADDRESS, curreg);
	else
		CR1W(22CURADDRESS, curreg);

	/*set cursor colour*/
	if (si->ps.card_type < NM2200)
	{
		/* background is black */
		CR1W(CURBGCOLOR, 0x00000000);
		/* foreground is white */
		CR1W(CURFGCOLOR, 0x00ffffff);
	}
	else
	{
		/* background is black */
		CR1W(22CURBGCOLOR, 0x00000000);
		/* foreground is white */
		CR1W(22CURFGCOLOR, 0x00ffffff);
	}

	/* we must set a valid colordepth to get full RAM access on Neomagic cards:
	 * in old pre 8-bit color VGA modes some planemask is in effect apparantly,
	 * allowing access only to every 7th and 8th RAM byte across the
	 * entire RAM. */
	nm_crtc_depth(BPP8);

	/* clear cursor: so we need full RAM access! */
	fb = ((vuint32 *)(((uintptr_t)si->framebuffer) + curadd));
	for (i = 0; i < (1024/4); i++)
	{
		fb[i] = 0;
	}

	/* activate hardware cursor */
	nm_crtc_cursor_show();

	return B_OK;
}

status_t nm_crtc_cursor_show()
{
	if (si->ps.card_type < NM2200)
	{
		CR1W(CURCTRL, 0x00000001);
	}
	else
	{
		CR1W(22CURCTRL, 0x00000001);
	}
	return B_OK;
}

status_t nm_crtc_cursor_hide()
{
//linux fixme: using this kills PCI(?) access sometimes,
//so use ISA access as below...
/*
	if (si->ps.card_type < NM2200)
	{
		CR1W(CURCTRL, 0x00000000);
	}
	else
	{
		CR1W(22CURCTRL, 0x00000000);
	}
*/
	/* disable cursor */
	ISAGRPHW(CURCTRL,0x00);

	return B_OK;
}

/*set up cursor shape*/
status_t nm_crtc_cursor_define(uint8* andMask,uint8* xorMask)
{
	uint8 y;
	vuint8 * cursor;

	/* get a pointer to the cursor: it's at the end of cardRAM */
	cursor = (vuint8*) si->framebuffer;
	cursor += ((si->ps.memory_size * 1024) - si->ps.curmem_size);

	/*draw the cursor*/
	for(y=0;y<16;y++)
	{
		cursor[y*16+8]=~*andMask++;
		cursor[y*16+0]=*xorMask++;
		cursor[y*16+9]=~*andMask++;
		cursor[y*16+1]=*xorMask++;
	}

	//test.. only valid for <NM2200!!
/*	{
		float pclk;
		uint8 n,m,x = 1;
		n = ISAGRPHR(PLLC_NL);
		m = ISAGRPHR(PLLC_M);
		LOG(4,("CRTC: PLLSEL $%02x\n", ISARB(MISCR)));
		LOG(4,("CRTC: PLLN $%02x\n", n));
		LOG(4,("CRTC: PLLM $%02x\n", m));

		if (n & 0x80) x = 2;
		n &= 0x7f;
		pclk = ((si->ps.f_ref * (n + 1)) / ((m + 1) * x));
		LOG(2,("CRTC: Pixelclock is %fMHz\n", pclk));
		nm_general_output_select();
	}
*/
	return B_OK;
}

/*position the cursor*/
status_t nm_crtc_cursor_position(uint16 x ,uint16 y)
{
//NM2160 is ok without this, still verify the rest..:
	/* make sure we are not in retrace, because the register(s) might get copied
	 * during our reprogramming them (double buffering feature) */
/*	fixme!?
	while (ACCR(STATUS) & 0x08)
	{
		snooze(4);
	}
*/
	if (si->ps.card_type < NM2200)
	{
		CR1W(CURX, (uint32)x);
		CR1W(CURY, (uint32)y);
	}
	else
	{
		CR1W(22CURX, (uint32)x);
		CR1W(22CURY, (uint32)y);
	}

	return B_OK;
}
