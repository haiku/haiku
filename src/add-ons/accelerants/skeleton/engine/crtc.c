/* CTRC functionality */
/* Author:
   Rudolf Cornelissen 11/2002-9/2004
*/

#define MODULE_BIT 0x00040000

#include "std.h"

/*Adjust passed parameters to a valid mode line*/
status_t eng_crtc_validate_timing(
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
	if (*hd_e > ((0x01ff - 2) << 3)) *hd_e = ((0x01ff - 2) << 3);
	if (*hs_s > ((0x01ff - 1) << 3)) *hs_s = ((0x01ff - 1) << 3);
	if (*hs_e > ( 0x01ff      << 3)) *hs_e = ( 0x01ff      << 3);
	if (*ht   > ((0x01ff + 5) << 3)) *ht   = ((0x01ff + 5) << 3);

	/* NOTE: keep horizontal timing at multiples of 8! */
	/* confine to a reasonable width */
	if (*hd_e < 640) *hd_e = 640;
	if (si->ps.card_type > NV04)
	{
		if (*hd_e > 2048) *hd_e = 2048;
	}
	else
	{
		if (*hd_e > 1920) *hd_e = 1920;
	}

	/* if hor. total does not leave room for a sensible sync pulse, increase it! */
	if (*ht < (*hd_e + 80)) *ht = (*hd_e + 80);

	/* if hor. total does not adhere to max. blanking pulse width, decrease it! */
	if (*ht > (*hd_e + 0x3f8)) *ht = (*hd_e + 0x3f8);

	/* make sure sync pulse is not during display */
	if (*hs_e > (*ht - 8)) *hs_e = (*ht - 8);
	if (*hs_s < (*hd_e + 8)) *hs_s = (*hd_e + 8);

	/* correct sync pulse if it is too long:
	 * there are only 5 bits available to save this in the card registers! */
	if (*hs_e > (*hs_s + 0xf8)) *hs_e = (*hs_s + 0xf8);

/*vertical*/
	/* confine to required number of bits, taking logic into account */
	//fixme if needed: on GeForce cards there are 12 instead of 11 bits...
	if (*vd_e > (0x7ff - 2)) *vd_e = (0x7ff - 2);
	if (*vs_s > (0x7ff - 1)) *vs_s = (0x7ff - 1);
	if (*vs_e >  0x7ff     ) *vs_e =  0x7ff     ;
	if (*vt   > (0x7ff + 2)) *vt   = (0x7ff + 2);

	/* confine to a reasonable height */
	if (*vd_e < 480) *vd_e = 480;
	if (si->ps.card_type > NV04)
	{
		if (*vd_e > 1536) *vd_e = 1536;
	}
	else
	{
		if (*vd_e > 1440) *vd_e = 1440;
	}

	/*if vertical total does not leave room for a sync pulse, increase it!*/
	if (*vt < (*vd_e + 3)) *vt = (*vd_e + 3);

	/* if vert. total does not adhere to max. blanking pulse width, decrease it! */
	if (*vt > (*vd_e + 0xff)) *vt = (*vd_e + 0xff);

	/* make sure sync pulse is not during display */
	if (*vs_e > (*vt - 1)) *vs_e = (*vt - 1);
	if (*vs_s < (*vd_e + 1)) *vs_s = (*vd_e + 1);

	/* correct sync pulse if it is too long:
	 * there are only 4 bits available to save this in the card registers! */
	if (*vs_e > (*vs_s + 0x0f)) *vs_e = (*vs_s + 0x0f);

	return B_OK;
}

/*set a mode line - inputs are in pixels*/
status_t eng_crtc_set_timing(display_mode target)
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

	/* setup tuned internal modeline for flatpanel if connected and active */
	/* notes:
	 * - the CRTC modeline must end earlier than the panel modeline to keep correct
	 *   sync going;
	 * - if the CRTC modeline ends too soon, pixelnoise will occur in 8 (or so) pixel
	 *   wide horizontal stripes. This can be observed earliest on fullscreen overlay,
	 *   and if it gets worse, also normal desktop output will suffer. The stripes
	 *   are mainly visible at the left of the screen, over the entire screen height. */
	if (si->ps.tmds1_active)
	{
		LOG(2,("CRTC: DFP active: tuning modeline\n"));

		/* horizontal timing */
		target.timing.h_sync_start =
			((uint16)((si->ps.p1_timing.h_sync_start / ((float)si->ps.p1_timing.h_display)) *
			target.timing.h_display)) & 0xfff8;

		target.timing.h_sync_end =
			((uint16)((si->ps.p1_timing.h_sync_end / ((float)si->ps.p1_timing.h_display)) *
			target.timing.h_display)) & 0xfff8;

		target.timing.h_total =
			(((uint16)((si->ps.p1_timing.h_total / ((float)si->ps.p1_timing.h_display)) *
			target.timing.h_display)) & 0xfff8) - 8;

		/* in native mode the CRTC needs some extra time to keep synced correctly;
		 * OTOH the overlay unit distorts if we reserve too much time! */
		if (target.timing.h_display == si->ps.p1_timing.h_display)
		{
			/* NV11 timing has different constraints than later cards */
			if (si->ps.card_type == NV11)
				target.timing.h_total -= 56;
			else
				/* confirmed NV34 with 1680x1050 panel */
				target.timing.h_total -= 32;
		}

		if (target.timing.h_sync_start == target.timing.h_display)
			target.timing.h_sync_start += 8;
		if (target.timing.h_sync_end == target.timing.h_total)
			target.timing.h_sync_end -= 8;

		/* vertical timing */
		target.timing.v_sync_start =
			((uint16)((si->ps.p1_timing.v_sync_start / ((float)si->ps.p1_timing.v_display)) *
			target.timing.v_display));

		target.timing.v_sync_end =
			((uint16)((si->ps.p1_timing.v_sync_end / ((float)si->ps.p1_timing.v_display)) *
			target.timing.v_display));

		target.timing.v_total =
			((uint16)((si->ps.p1_timing.v_total / ((float)si->ps.p1_timing.v_display)) *
			target.timing.v_display)) - 1;

		if (target.timing.v_sync_start == target.timing.v_display)
			target.timing.v_sync_start += 1;
		if (target.timing.v_sync_end == target.timing.v_total)
			target.timing.v_sync_end -= 1;

		/* disable GPU scaling testmode so automatic scaling will be done */
		DACW(FP_DEBUG1, 0);
	}

	/* Modify parameters as required by standard VGA */
	htotal = ((target.timing.h_total >> 3) - 5);
	hdisp_e = ((target.timing.h_display >> 3) - 1);
	hblnk_s = hdisp_e;
	hblnk_e = (htotal + 4);//0;
	hsync_s = (target.timing.h_sync_start >> 3);
	hsync_e = (target.timing.h_sync_end >> 3);

	vtotal = target.timing.v_total - 2;
	vdisp_e = target.timing.v_display - 1;
	vblnk_s = vdisp_e;
	vblnk_e = (vtotal + 1);
	vsync_s = target.timing.v_sync_start;//-1;
	vsync_e = target.timing.v_sync_end;//-1;

	/* prevent memory adress counter from being reset (linecomp may not occur) */
	linecomp = target.timing.v_display;

	/* enable access to primary head */
	set_crtc_owner(0);

	/* Note for laptop and DVI flatpanels:
	 * CRTC timing has a seperate set of registers from flatpanel timing.
	 * The flatpanel timing registers have scaling registers that are used to match
	 * these two modelines. */
	{
		LOG(4,("CRTC: Setting full timing...\n"));

		/* log the mode that will be set */
		LOG(2,("CRTC:\n\tHTOT:%x\n\tHDISPEND:%x\n\tHBLNKS:%x\n\tHBLNKE:%x\n\tHSYNCS:%x\n\tHSYNCE:%x\n\t",htotal,hdisp_e,hblnk_s,hblnk_e,hsync_s,hsync_e));
		LOG(2,("VTOT:%x\n\tVDISPEND:%x\n\tVBLNKS:%x\n\tVBLNKE:%x\n\tVSYNCS:%x\n\tVSYNCE:%x\n",vtotal,vdisp_e,vblnk_s,vblnk_e,vsync_s,vsync_e));

		/* actually program the card! */
		/* unlock CRTC registers at index 0-7 */
		CRTCW(VSYNCE, (CRTCR(VSYNCE) & 0x7f));
		/* horizontal standard VGA regs */
		CRTCW(HTOTAL, (htotal & 0xff));
		CRTCW(HDISPE, (hdisp_e & 0xff));
		CRTCW(HBLANKS, (hblnk_s & 0xff));
		/* also unlock vertical retrace registers in advance */
		CRTCW(HBLANKE, ((hblnk_e & 0x1f) | 0x80));
		CRTCW(HSYNCS, (hsync_s & 0xff));
		CRTCW(HSYNCE, ((hsync_e & 0x1f) | ((hblnk_e & 0x20) << 2)));

		/* vertical standard VGA regs */
		CRTCW(VTOTAL, (vtotal & 0xff));
		CRTCW(OVERFLOW,
		(
			((vtotal & 0x100) >> (8 - 0)) | ((vtotal & 0x200) >> (9 - 5)) |
			((vdisp_e & 0x100) >> (8 - 1)) | ((vdisp_e & 0x200) >> (9 - 6)) |
			((vsync_s & 0x100) >> (8 - 2)) | ((vsync_s & 0x200) >> (9 - 7)) |
			((vblnk_s & 0x100) >> (8 - 3)) | ((linecomp & 0x100) >> (8 - 4))
		));
		CRTCW(PRROWSCN, 0x00); /* not used */
		CRTCW(MAXSCLIN, (((vblnk_s & 0x200) >> (9 - 5)) | ((linecomp & 0x200) >> (9 - 6))));
		CRTCW(VSYNCS, (vsync_s & 0xff));
		CRTCW(VSYNCE, ((CRTCR(VSYNCE) & 0xf0) | (vsync_e & 0x0f)));
		CRTCW(VDISPE, (vdisp_e & 0xff));
		CRTCW(VBLANKS, (vblnk_s & 0xff));
		CRTCW(VBLANKE, (vblnk_e & 0xff));
		CRTCW(LINECOMP, (linecomp & 0xff));

		/* horizontal extended regs */
		//fixme: we reset bit4. is this correct??
		CRTCW(HEB, (CRTCR(HEB) & 0xe0) |
			(
		 	((htotal & 0x100) >> (8 - 0)) |
			((hdisp_e & 0x100) >> (8 - 1)) |
			((hblnk_s & 0x100) >> (8 - 2)) |
			((hsync_s & 0x100) >> (8 - 3))
			));

		/* (mostly) vertical extended regs */
		CRTCW(LSR,
			(
		 	((vtotal & 0x400) >> (10 - 0)) |
			((vdisp_e & 0x400) >> (10 - 1)) |
			((vsync_s & 0x400) >> (10 - 2)) |
			((vblnk_s & 0x400) >> (10 - 3)) |
			((hblnk_e & 0x040) >> (6 - 4))
			//fixme: we still miss one linecomp bit!?! is this it??
			//| ((linecomp & 0x400) >> 3)	
			));

		/* more vertical extended regs (on GeForce cards only) */
		if (si->ps.card_arch >= NV10A)
		{ 
			CRTCW(EXTRA,
				(
			 	((vtotal & 0x800) >> (11 - 0)) |
				((vdisp_e & 0x800) >> (11 - 2)) |
				((vsync_s & 0x800) >> (11 - 4)) |
				((vblnk_s & 0x800) >> (11 - 6))
				//fixme: do we miss another linecomp bit!?!
				));
		}

		/* setup 'large screen' mode */
		if (target.timing.h_display >= 1280)
			CRTCW(REPAINT1, (CRTCR(REPAINT1) & 0xfb));
		else
			CRTCW(REPAINT1, (CRTCR(REPAINT1) | 0x04));

		/* setup HSYNC & VSYNC polarity */
		LOG(2,("CRTC: sync polarity: "));
		temp = ENG_REG8(RG8_MISCR);
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
		ENG_REG8(RG8_MISCW) = temp;

		LOG(2,(", MISC reg readback: $%02x\n", ENG_REG8(RG8_MISCR)));
	}

	/* always disable interlaced operation */
	/* (interlace is supported on upto and including NV10, NV15, and NV30 and up) */
	CRTCW(INTERLACE, 0xff);

	/* disable CRTC slaved mode unless a panel is in use */
	// fixme: this kills TVout when it was in use...
	if (!si->ps.tmds1_active) CRTCW(PIXEL, (CRTCR(PIXEL) & 0x7f));

	/* setup flatpanel if connected and active */
	if (si->ps.tmds1_active)
	{
		uint32 iscale_x, iscale_y;

		/* calculate inverse scaling factors used by hardware in 20.12 format */
		iscale_x = (((1 << 12) * target.timing.h_display) / si->ps.p1_timing.h_display);
		iscale_y = (((1 << 12) * target.timing.v_display) / si->ps.p1_timing.v_display);

		/* unblock flatpanel timing programming (or something like that..) */
		CRTCW(FP_HTIMING, 0);
		CRTCW(FP_VTIMING, 0);
		LOG(2,("CRTC: FP_HTIMING reg readback: $%02x\n", CRTCR(FP_HTIMING)));
		LOG(2,("CRTC: FP_VTIMING reg readback: $%02x\n", CRTCR(FP_VTIMING)));

		/* enable full width visibility on flatpanel */
		DACW(FP_HVALID_S, 0);
		DACW(FP_HVALID_E, (si->ps.p1_timing.h_display - 1));
		/* enable full height visibility on flatpanel */
		DACW(FP_VVALID_S, 0);
		DACW(FP_VVALID_E, (si->ps.p1_timing.v_display - 1));

		/* nVidia cards support upscaling except on ??? */
		/* NV11 cards can upscale after all! */
		if (0)//si->ps.card_type == NV11)
		{
			/* disable last fetched line limiting */
			DACW(FP_DEBUG2, 0x00000000);
			/* inform panel to scale if needed */
			if ((iscale_x != (1 << 12)) || (iscale_y != (1 << 12)))
			{
				LOG(2,("CRTC: DFP needs to do scaling\n"));
				DACW(FP_TG_CTRL, (DACR(FP_TG_CTRL) | 0x00000100));
			}
			else
			{
				LOG(2,("CRTC: no scaling for DFP needed\n"));
				DACW(FP_TG_CTRL, (DACR(FP_TG_CTRL) & 0xfffffeff));
			}
		}
		else
		{
			float dm_aspect;

			LOG(2,("CRTC: GPU scales for DFP if needed\n"));

			/* calculate display mode aspect */
			dm_aspect = (target.timing.h_display / ((float)target.timing.v_display));

			/* limit last fetched line if vertical scaling is done */
			if (iscale_y != (1 << 12))
				DACW(FP_DEBUG2, ((1 << 28) | ((target.timing.v_display - 1) << 16)));
			else
				DACW(FP_DEBUG2, 0x00000000);

			/* inform panel not to scale */
			DACW(FP_TG_CTRL, (DACR(FP_TG_CTRL) & 0xfffffeff));

			/* GPU scaling is automatically setup by hardware, so only modify this
			 * scalingfactor for non 4:3 (1.33) aspect panels;
			 * let's consider 1280x1024 1:33 aspect (it's 1.25 aspect actually!) */

			/* correct for widescreen panels relative to mode...
			 * (so if panel is more widescreen than mode being set) */
			/* BTW: known widescreen panels:
			 * 1280 x  800 (1.60),
			 * 1440 x  900 (1.60),
			 * 1680 x 1050 (1.60),
			 * 1920 x 1200 (1.60). */
			/* known 4:3 aspect non-standard resolution panels:
			 * 1400 x 1050 (1.33). */
			/* NOTE:
			 * allow 0.10 difference so 1280x1024 panels will be used fullscreen! */
			if ((iscale_x != (1 << 12)) && (si->ps.panel1_aspect > (dm_aspect + 0.10)))
			{
				uint16 diff;

				LOG(2,("CRTC: (relative) widescreen panel: tuning horizontal scaling\n"));

				/* X-scaling should be the same as Y-scaling */
				iscale_x = iscale_y;
				/* enable testmode (b12) and program modified X-scaling factor */
				DACW(FP_DEBUG1, (((iscale_x >> 1) & 0x00000fff) | (1 << 12)));
				/* center/cut-off left and right side of screen */
				diff = ((si->ps.p1_timing.h_display -
						(target.timing.h_display * ((1 << 12) / ((float)iscale_x))))
						/ 2);
				DACW(FP_HVALID_S, diff);
				DACW(FP_HVALID_E, ((si->ps.p1_timing.h_display - diff) - 1));
			}
			/* correct for portrait panels... */
			/* NOTE:
			 * allow 0.10 difference so 1280x1024 panels will be used fullscreen! */
			if ((iscale_y != (1 << 12)) && (si->ps.panel1_aspect < (dm_aspect - 0.10)))
			{
				LOG(2,("CRTC: (relative) portrait panel: should tune vertical scaling\n"));
				/* fixme: implement if this kind of portrait panels exist on nVidia... */
			}
		}

		/* do some logging.. */
		LOG(2,("CRTC: FP_HVALID_S reg readback: $%08x\n", DACR(FP_HVALID_S)));
		LOG(2,("CRTC: FP_HVALID_E reg readback: $%08x\n", DACR(FP_HVALID_E)));
		LOG(2,("CRTC: FP_VVALID_S reg readback: $%08x\n", DACR(FP_VVALID_S)));
		LOG(2,("CRTC: FP_VVALID_E reg readback: $%08x\n", DACR(FP_VVALID_E)));
		LOG(2,("CRTC: FP_DEBUG0 reg readback: $%08x\n", DACR(FP_DEBUG0)));
		LOG(2,("CRTC: FP_DEBUG1 reg readback: $%08x\n", DACR(FP_DEBUG1)));
		LOG(2,("CRTC: FP_DEBUG2 reg readback: $%08x\n", DACR(FP_DEBUG2)));
		LOG(2,("CRTC: FP_DEBUG3 reg readback: $%08x\n", DACR(FP_DEBUG3)));
		LOG(2,("CRTC: FP_TG_CTRL reg readback: $%08x\n", DACR(FP_TG_CTRL)));
	}

	return B_OK;
}

status_t eng_crtc_depth(int mode)
{
	uint8 viddelay = 0;
	uint32 genctrl = 0;

	/* set VCLK scaling */
	switch(mode)
	{
	case BPP8:
		viddelay = 0x01;
		/* genctrl b4 & b5 reset: 'direct mode' */
		genctrl = 0x00101100;
		break;
	case BPP15:
		viddelay = 0x02;
		/* genctrl b4 & b5 set: 'indirect mode' (via colorpalette) */
		genctrl = 0x00100130;
		break;
	case BPP16:
		viddelay = 0x02;
		/* genctrl b4 & b5 set: 'indirect mode' (via colorpalette) */
		genctrl = 0x00101130;
		break;
	case BPP24:
		viddelay = 0x03;
		/* genctrl b4 & b5 set: 'indirect mode' (via colorpalette) */
		genctrl = 0x00100130;
		break;
	case BPP32:
		viddelay = 0x03;
		/* genctrl b4 & b5 set: 'indirect mode' (via colorpalette) */
		genctrl = 0x00101130;
		break;
	}
	/* enable access to primary head */
	set_crtc_owner(0);

	CRTCW(PIXEL, ((CRTCR(PIXEL) & 0xfc) | viddelay));
	DACW(GENCTRL, genctrl);

	return B_OK;
}

status_t eng_crtc_dpms(bool display, bool h, bool v)
{
	uint8 temp;

	LOG(4,("CRTC: setting DPMS: "));

	/* enable access to primary head */
	set_crtc_owner(0);

	/* start synchronous reset: required before turning screen off! */
	SEQW(RESET, 0x01);

	/* turn screen off */
	temp = SEQR(CLKMODE);
	if (display)
	{
		SEQW(CLKMODE, (temp & ~0x20));

		/* end synchronous reset if display should be enabled */
		SEQW(RESET, 0x03);

		//'safe mode' test! feedback needed with this 'setting'!
		if (0)//si->ps.tmds1_active)
		{
			/* powerup both LVDS (laptop panellink) and TMDS (DVI panellink)
			 * internal transmitters... */
			/* note:
			 * the powerbits in this register are hardwired to the DVI connectors,
			 * instead of to the DACs! (confirmed NV34) */
			//fixme...
			DACW(FP_DEBUG0, (DACR(FP_DEBUG0) & 0xcfffffff));
			/* ... and powerup external TMDS transmitter if it exists */
			/* (confirmed OK on NV28 and NV34) */
			CRTCW(0x59, (CRTCR(0x59) | 0x01));
		}

		LOG(4,("display on, "));
	}
	else
	{
		SEQW(CLKMODE, (temp | 0x20));

		//'safe mode' test! feedback needed with this 'setting'!
		if (0)//si->ps.tmds1_active)
		{
			/* powerdown both LVDS (laptop panellink) and TMDS (DVI panellink)
			 * internal transmitters... */
			/* note:
			 * the powerbits in this register are hardwired to the DVI connectors,
			 * instead of to the DACs! (confirmed NV34) */
			//fixme...
			DACW(FP_DEBUG0, (DACR(FP_DEBUG0) | 0x30000000));
			/* ... and powerdown external TMDS transmitter if it exists */
			/* (confirmed OK on NV28 and NV34) */
			CRTCW(0x59, (CRTCR(0x59) & 0xfe));
		}

		LOG(4,("display off, "));
	}

	if (h)
	{
		CRTCW(REPAINT1, (CRTCR(REPAINT1) & 0x7f));
		LOG(4,("hsync enabled, "));
	}
	else
	{
		CRTCW(REPAINT1, (CRTCR(REPAINT1) | 0x80));
		LOG(4,("hsync disabled, "));
	}
	if (v)
	{
		CRTCW(REPAINT1, (CRTCR(REPAINT1) & 0xbf));
		LOG(4,("vsync enabled\n"));
	}
	else
	{
		CRTCW(REPAINT1, (CRTCR(REPAINT1) | 0x40));
		LOG(4,("vsync disabled\n"));
	}

	return B_OK;
}

status_t eng_crtc_dpms_fetch(bool *display, bool *h, bool *v)
{
	/* enable access to primary head */
	set_crtc_owner(0);

	*display = !(SEQR(CLKMODE) & 0x20);
	*h = !(CRTCR(REPAINT1) & 0x80);
	*v = !(CRTCR(REPAINT1) & 0x40);

	LOG(4,("CTRC: fetched DPMS state: "));
	if (*display) LOG(4,("display on, "));
	else LOG(4,("display off, "));
	if (*h) LOG(4,("hsync enabled, "));
	else LOG(4,("hsync disabled, "));
	if (*v) LOG(4,("vsync enabled\n"));
	else LOG(4,("vsync disabled\n"));

	return B_OK;
}

status_t eng_crtc_set_display_pitch() 
{
	uint32 offset;

	LOG(4,("CRTC: setting card pitch (offset between lines)\n"));

	/* figure out offset value hardware needs */
	offset = si->fbc.bytes_per_row / 8;

	LOG(2,("CRTC: offset register set to: $%04x\n", offset));

	/* enable access to primary head */
	set_crtc_owner(0);

	/* program the card */
	CRTCW(PITCHL, (offset & 0x00ff));
	CRTCW(REPAINT0, ((CRTCR(REPAINT0) & 0x1f) | ((offset & 0x0700) >> 3)));

	return B_OK;
}

status_t eng_crtc_set_display_start(uint32 startadd,uint8 bpp) 
{
	uint8 temp;
	uint32 timeout = 0;

	LOG(4,("CRTC: setting card RAM to be displayed bpp %d\n", bpp));

	LOG(2,("CRTC: startadd: $%08x\n", startadd));
	LOG(2,("CRTC: frameRAM: $%08x\n", si->framebuffer));
	LOG(2,("CRTC: framebuffer: $%08x\n", si->fbc.frame_buffer));

	/* we might have no retraces during setmode! */
	/* wait 25mS max. for retrace to occur (refresh > 40Hz) */
	while (((ENG_RG32(RG32_RASTER) & 0x000007ff) < si->dm.timing.v_display) &&
			(timeout < (25000/10)))
	{
		/* don't snooze much longer or retrace might get missed! */
		snooze(10);
		timeout++;
	}

	/* enable access to primary head */
	set_crtc_owner(0);

	if (si->ps.card_arch == NV04A)
	{
		/* upto 32Mb RAM adressing: must be used this way on pre-NV10! */

		/* set standard registers */
		/* (NVidia: startadress in 32bit words (b2 - b17) */
		CRTCW(FBSTADDL, ((startadd & 0x000003fc) >> 2));
		CRTCW(FBSTADDH, ((startadd & 0x0003fc00) >> 10));

		/* set extended registers */
		/* NV4 extended bits: (b18-22) */
		temp = (CRTCR(REPAINT0) & 0xe0);
		CRTCW(REPAINT0, (temp | ((startadd & 0x007c0000) >> 18)));
		/* NV4 extended bits: (b23-24) */
		temp = (CRTCR(HEB) & 0x9f);
		CRTCW(HEB, (temp | ((startadd & 0x01800000) >> 18)));
	}
	else
	{
		/* upto 4Gb RAM adressing: must be used on NV10 and later! */
		/* NOTE:
		 * While this register also exists on pre-NV10 cards, it will
		 * wrap-around at 16Mb boundaries!! */

		/* 30bit adress in 32bit words */
		ENG_RG32(RG32_NV10FBSTADD32) = (startadd & 0xfffffffc);
	}

	/* set NV4/NV10 byte adress: (b0 - 1) */
	ATBW(HORPIXPAN, ((startadd & 0x00000003) << 1));

	return B_OK;
}

status_t eng_crtc_cursor_init()
{
	int i;
	uint32 * fb;
	/* cursor bitmap will be stored at the start of the framebuffer */
	const uint32 curadd = 0;

	/* enable access to primary head */
	set_crtc_owner(0);

	/* set cursor bitmap adress ... */
	if ((si->ps.card_arch == NV04A) || (si->ps.laptop))
	{
		/* must be used this way on pre-NV10 and on all 'Go' cards! */

		/* cursorbitmap must start on 2Kbyte boundary: */
		/* set adress bit11-16, and set 'no doublescan' (registerbit 1 = 0) */
		CRTCW(CURCTL0, ((curadd & 0x0001f800) >> 9));
		/* set adress bit17-23, and set graphics mode cursor(?) (registerbit 7 = 1) */
		CRTCW(CURCTL1, (((curadd & 0x00fe0000) >> 17) | 0x80));
		/* set adress bit24-31 */
		CRTCW(CURCTL2, ((curadd & 0xff000000) >> 24));
	}
	else
	{
		/* upto 4Gb RAM adressing:
		 * can be used on NV10 and later (except for 'Go' cards)! */
		/* NOTE:
		 * This register does not exist on pre-NV10 and 'Go' cards. */

		/* cursorbitmap must still start on 2Kbyte boundary: */
		ENG_RG32(RG32_NV10CURADD32) = (curadd & 0xfffff800);
	}

	/* set cursor colour: not needed because of direct nature of cursor bitmap. */

	/*clear cursor*/
	fb = (uint32 *) si->framebuffer + curadd;
	for (i=0;i<(2048/4);i++)
	{
		fb[i]=0;
	}

	/* select 32x32 pixel, 16bit color cursorbitmap, no doublescan */
	ENG_RG32(RG32_CURCONF) = 0x02000100;

	/* activate hardware cursor */
	eng_crtc_cursor_show();

	return B_OK;
}

status_t eng_crtc_cursor_show()
{
	LOG(4,("CRTC: enabling cursor\n"));

	/* enable access to CRTC1 on dualhead cards */
	set_crtc_owner(0);

	/* b0 = 1 enables cursor */
	CRTCW(CURCTL0, (CRTCR(CURCTL0) | 0x01));

	return B_OK;
}

status_t eng_crtc_cursor_hide()
{
	LOG(4,("CRTC: disabling cursor\n"));

	/* enable access to primary head */
	set_crtc_owner(0);

	/* b0 = 0 disables cursor */
	CRTCW(CURCTL0, (CRTCR(CURCTL0) & 0xfe));

	return B_OK;
}

/*set up cursor shape*/
status_t eng_crtc_cursor_define(uint8* andMask,uint8* xorMask)
{
	int x, y;
	uint8 b;
	uint16 *cursor;
	uint16 pixel;

	/* get a pointer to the cursor */
	cursor = (uint16*) si->framebuffer;

	/* draw the cursor */
	/* (Nvidia cards have a RGB15 direct color cursor bitmap, bit #16 is transparancy) */
	for (y = 0; y < 16; y++)
	{
		b = 0x80;
		for (x = 0; x < 8; x++)
		{
			/* preset transparant */
			pixel = 0x0000;
			/* set white if requested */
			if ((!(*andMask & b)) && (!(*xorMask & b))) pixel = 0xffff;
			/* set black if requested */
			if ((!(*andMask & b)) &&   (*xorMask & b))  pixel = 0x8000;
			/* set invert if requested */
			if (  (*andMask & b)  &&   (*xorMask & b))  pixel = 0x7fff;
			/* place the pixel in the bitmap */
			cursor[x + (y * 32)] = pixel;
			b >>= 1;
		}
		xorMask++;
		andMask++;
		b = 0x80;
		for (; x < 16; x++)
		{
			/* preset transparant */
			pixel = 0x0000;
			/* set white if requested */
			if ((!(*andMask & b)) && (!(*xorMask & b))) pixel = 0xffff;
			/* set black if requested */
			if ((!(*andMask & b)) &&   (*xorMask & b))  pixel = 0x8000;
			/* set invert if requested */
			if (  (*andMask & b)  &&   (*xorMask & b))  pixel = 0x7fff;
			/* place the pixel in the bitmap */
			cursor[x + (y * 32)] = pixel;
			b >>= 1;
		}
		xorMask++;
		andMask++;
	}

	return B_OK;
}

/* position the cursor */
status_t eng_crtc_cursor_position(uint16 x, uint16 y)
{
	uint16 yhigh;

	/* make sure we are beyond the first line of the cursorbitmap being drawn during
	 * updating the position to prevent distortions: no double buffering feature */
	/* Note:
	 * we need to return as quick as possible or some apps will exhibit lagging.. */

	/* read the old cursor Y position */
	yhigh = ((DACR(CURPOS) & 0x0fff0000) >> 16); 
	/* make sure we will wait until we are below both the old and new Y position:
	 * visible cursorbitmap drawing needs to be done at least... */
	if (y > yhigh) yhigh = y;

	if (yhigh < (si->dm.timing.v_display - 16))
	{
		/* we have vertical lines below old and new cursorposition to spare. So we
		 * update the cursor postion 'mid-screen', but below that area. */
		while (((uint16)(ENG_RG32(RG32_RASTER) & 0x000007ff)) < (yhigh + 16))
		{
			snooze(10);
		}
	}
	else
	{
		/* no room to spare, just wait for retrace (is relatively slow) */
		while ((ENG_RG32(RG32_RASTER) & 0x000007ff) < si->dm.timing.v_display)
		{
			/* don't snooze much longer or retrace might get missed! */
			snooze(10);
		}
	}

	/* update cursorposition */
	DACW(CURPOS, ((x & 0x0fff) | ((y & 0x0fff) << 16)));

	return B_OK;
}
