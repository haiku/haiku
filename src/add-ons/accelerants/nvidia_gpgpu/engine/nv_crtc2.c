/* second CTRC functionality for GeForce cards */
/* Author:
   Rudolf Cornelissen 11/2002-6/2008
*/

#define MODULE_BIT 0x00020000

#include "nv_std.h"

/*
	Enable/Disable interrupts.  Just a wrapper around the
	ioctl() to the kernel driver.
*/
status_t nv_crtc2_interrupt_enable(bool flag)
{
	status_t result = B_OK;
	nv_set_vblank_int svi;

	if (si->ps.int_assigned)
	{
		/* set the magic number so the driver knows we're for real */
		svi.magic = NV_PRIVATE_DATA_MAGIC;
		svi.crtc = 1;
		svi.do_it = flag;
		/* contact driver and get a pointer to the registers and shared data */
		result = ioctl(fd, NV_RUN_INTERRUPTS, &svi, sizeof(svi));
	}

	return result;
}

status_t nv_crtc2_update_fifo()
{
	return B_OK;
}

/* Adjust passed parameters to a valid mode line */
status_t nv_crtc2_validate_timing(
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
	if (*hd_e > 2048) *hd_e = 2048;

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
	if (*vd_e > 1536) *vd_e = 1536;

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
status_t nv_crtc2_set_timing(display_mode target)
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

	LOG(4,("CRTC2: setting timing\n"));

	/* setup tuned internal modeline for flatpanel if connected and active */
	/* notes:
	 * - the CRTC modeline must end earlier than the panel modeline to keep correct
	 *   sync going;
	 * - if the CRTC modeline ends too soon, pixelnoise will occur in 8 (or so) pixel
	 *   wide horizontal stripes. This can be observed earliest on fullscreen overlay,
	 *   and if it gets worse, also normal desktop output will suffer. The stripes
	 *   are mainly visible at the left of the screen, over the entire screen height. */
	if (si->ps.tmds2_active)
	{
		LOG(2,("CRTC2: DFP active: tuning modeline\n"));

		/* horizontal timing */
		target.timing.h_sync_start =
			((uint16)((si->ps.p2_timing.h_sync_start / ((float)si->ps.p2_timing.h_display)) *
			target.timing.h_display)) & 0xfff8;

		target.timing.h_sync_end =
			((uint16)((si->ps.p2_timing.h_sync_end / ((float)si->ps.p2_timing.h_display)) *
			target.timing.h_display)) & 0xfff8;

		target.timing.h_total =
			(((uint16)((si->ps.p2_timing.h_total / ((float)si->ps.p2_timing.h_display)) *
			target.timing.h_display)) & 0xfff8) - 8;

		/* in native mode the CRTC needs some extra time to keep synced correctly;
		 * OTOH the overlay unit distorts if we reserve too much time! */
		if (target.timing.h_display == si->ps.p2_timing.h_display)
		{
			/* confirmed NV34 with 1680x1050 panel */
			target.timing.h_total -= 32;
		}

		if (target.timing.h_sync_start == target.timing.h_display)
			target.timing.h_sync_start += 8;
		if (target.timing.h_sync_end == target.timing.h_total)
			target.timing.h_sync_end -= 8;

		/* vertical timing */
		target.timing.v_sync_start =
			((uint16)((si->ps.p2_timing.v_sync_start / ((float)si->ps.p2_timing.v_display)) *
			target.timing.v_display));

		target.timing.v_sync_end =
			((uint16)((si->ps.p2_timing.v_sync_end / ((float)si->ps.p2_timing.v_display)) *
			target.timing.v_display));

		target.timing.v_total =
			((uint16)((si->ps.p2_timing.v_total / ((float)si->ps.p2_timing.v_display)) *
			target.timing.v_display)) - 1;

		if (target.timing.v_sync_start == target.timing.v_display)
			target.timing.v_sync_start += 1;
		if (target.timing.v_sync_end == target.timing.v_total)
			target.timing.v_sync_end -= 1;

		/* disable GPU scaling testmode so automatic scaling will be done */
		DAC2W(FP_DEBUG1, 0);
	}

	/* Modify parameters as required by standard VGA */
	htotal = ((target.timing.h_total >> 3) - 5);
	hdisp_e = ((target.timing.h_display >> 3) - 1);
	hblnk_s = hdisp_e;
	hblnk_e = (htotal + 4);
	hsync_s = (target.timing.h_sync_start >> 3);
	hsync_e = (target.timing.h_sync_end >> 3);

	vtotal = target.timing.v_total - 2;
	vdisp_e = target.timing.v_display - 1;
	vblnk_s = vdisp_e;
	vblnk_e = (vtotal + 1);
	vsync_s = target.timing.v_sync_start;
	vsync_e = target.timing.v_sync_end;

	/* prevent memory adress counter from being reset (linecomp may not occur) */
	linecomp = target.timing.v_display;

	/* enable access to secondary head */
	set_crtc_owner(1);

	/* Note for laptop and DVI flatpanels:
	 * CRTC timing has a seperate set of registers from flatpanel timing.
	 * The flatpanel timing registers have scaling registers that are used to match
	 * these two modelines. */
	{
		LOG(4,("CRTC2: Setting full timing...\n"));

		/* log the mode that will be set */
		LOG(2,("CRTC2:\n\tHTOT:%x\n\tHDISPEND:%x\n\tHBLNKS:%x\n\tHBLNKE:%x\n\tHSYNCS:%x\n\tHSYNCE:%x\n\t",htotal,hdisp_e,hblnk_s,hblnk_e,hsync_s,hsync_e));
		LOG(2,("VTOT:%x\n\tVDISPEND:%x\n\tVBLNKS:%x\n\tVBLNKE:%x\n\tVSYNCS:%x\n\tVSYNCE:%x\n",vtotal,vdisp_e,vblnk_s,vblnk_e,vsync_s,vsync_e));

		/* actually program the card! */
		/* unlock CRTC registers at index 0-7 */
		CRTC2W(VSYNCE, (CRTC2R(VSYNCE) & 0x7f));
		/* horizontal standard VGA regs */
		CRTC2W(HTOTAL, (htotal & 0xff));
		CRTC2W(HDISPE, (hdisp_e & 0xff));
		CRTC2W(HBLANKS, (hblnk_s & 0xff));
		/* also unlock vertical retrace registers in advance */
		CRTC2W(HBLANKE, ((hblnk_e & 0x1f) | 0x80));
		CRTC2W(HSYNCS, (hsync_s & 0xff));
		CRTC2W(HSYNCE, ((hsync_e & 0x1f) | ((hblnk_e & 0x20) << 2)));

		/* vertical standard VGA regs */
		CRTC2W(VTOTAL, (vtotal & 0xff));
		CRTC2W(OVERFLOW,
		(
			((vtotal & 0x100) >> (8 - 0)) | ((vtotal & 0x200) >> (9 - 5)) |
			((vdisp_e & 0x100) >> (8 - 1)) | ((vdisp_e & 0x200) >> (9 - 6)) |
			((vsync_s & 0x100) >> (8 - 2)) | ((vsync_s & 0x200) >> (9 - 7)) |
			((vblnk_s & 0x100) >> (8 - 3)) | ((linecomp & 0x100) >> (8 - 4))
		));
		CRTC2W(PRROWSCN, 0x00); /* not used */
		CRTC2W(MAXSCLIN, (((vblnk_s & 0x200) >> (9 - 5)) | ((linecomp & 0x200) >> (9 - 6))));
		CRTC2W(VSYNCS, (vsync_s & 0xff));
		CRTC2W(VSYNCE, ((CRTC2R(VSYNCE) & 0xf0) | (vsync_e & 0x0f)));
		CRTC2W(VDISPE, (vdisp_e & 0xff));
		CRTC2W(VBLANKS, (vblnk_s & 0xff));
		CRTC2W(VBLANKE, (vblnk_e & 0xff));
		CRTC2W(LINECOMP, (linecomp & 0xff));

		/* horizontal extended regs */
		//fixme: we reset bit4. is this correct??
		CRTC2W(HEB, (CRTC2R(HEB) & 0xe0) |
			(
		 	((htotal & 0x100) >> (8 - 0)) |
			((hdisp_e & 0x100) >> (8 - 1)) |
			((hblnk_s & 0x100) >> (8 - 2)) |
			((hsync_s & 0x100) >> (8 - 3))
			));

		/* (mostly) vertical extended regs */
		CRTC2W(LSR,
			(
		 	((vtotal & 0x400) >> (10 - 0)) |
			((vdisp_e & 0x400) >> (10 - 1)) |
			((vsync_s & 0x400) >> (10 - 2)) |
			((vblnk_s & 0x400) >> (10 - 3)) |
			((hblnk_e & 0x040) >> (6 - 4))
			//fixme: we still miss one linecomp bit!?! is this it??
			//| ((linecomp & 0x400) >> 3)	
			));

		/* more vertical extended regs */
		CRTC2W(EXTRA,
			(
		 	((vtotal & 0x800) >> (11 - 0)) |
			((vdisp_e & 0x800) >> (11 - 2)) |
			((vsync_s & 0x800) >> (11 - 4)) |
			((vblnk_s & 0x800) >> (11 - 6))
			//fixme: do we miss another linecomp bit!?!
			));

		/* setup 'large screen' mode */
		if (target.timing.h_display >= 1280)
			CRTC2W(REPAINT1, (CRTC2R(REPAINT1) & 0xfb));
		else
			CRTC2W(REPAINT1, (CRTC2R(REPAINT1) | 0x04));

		/* setup HSYNC & VSYNC polarity */
		LOG(2,("CRTC2: sync polarity: "));
		temp = NV_REG8(NV8_MISCR);
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
		NV_REG8(NV8_MISCW) = temp;

		LOG(2,(", MISC reg readback: $%02x\n", NV_REG8(NV8_MISCR)));
	}

	/* always disable interlaced operation */
	/* (interlace is supported on upto and including NV10, NV15, and NV30 and up) */
	CRTC2W(INTERLACE, 0xff);

	/* disable CRTC slaved mode unless a panel is in use */
	if (!si->ps.tmds2_active) CRTC2W(PIXEL, (CRTC2R(PIXEL) & 0x7f));

	/* setup flatpanel if connected and active */
	if (si->ps.tmds2_active)
	{
		uint32 iscale_x, iscale_y;

		/* calculate inverse scaling factors used by hardware in 20.12 format */
		iscale_x = (((1 << 12) * target.timing.h_display) / si->ps.p2_timing.h_display);
		iscale_y = (((1 << 12) * target.timing.v_display) / si->ps.p2_timing.v_display);

		/* unblock flatpanel timing programming (or something like that..) */
		CRTC2W(FP_HTIMING, 0);
		CRTC2W(FP_VTIMING, 0);
		LOG(2,("CRTC2: FP_HTIMING reg readback: $%02x\n", CRTC2R(FP_HTIMING)));
		LOG(2,("CRTC2: FP_VTIMING reg readback: $%02x\n", CRTC2R(FP_VTIMING)));

		/* enable full width visibility on flatpanel */
		DAC2W(FP_HVALID_S, 0);
		DAC2W(FP_HVALID_E, (si->ps.p2_timing.h_display - 1));
		/* enable full height visibility on flatpanel */
		DAC2W(FP_VVALID_S, 0);
		DAC2W(FP_VVALID_E, (si->ps.p2_timing.v_display - 1));

		/* nVidia cards support upscaling except on ??? */
		/* NV11 cards can upscale after all! */
		if (0)//si->ps.card_type == NV11)
		{
			/* disable last fetched line limiting */
			DAC2W(FP_DEBUG2, 0x00000000);
			/* inform panel to scale if needed */
			if ((iscale_x != (1 << 12)) || (iscale_y != (1 << 12)))
			{
				LOG(2,("CRTC2: DFP needs to do scaling\n"));
				DAC2W(FP_TG_CTRL, (DAC2R(FP_TG_CTRL) | 0x00000100));
			}
			else
			{
				LOG(2,("CRTC2: no scaling for DFP needed\n"));
				DAC2W(FP_TG_CTRL, (DAC2R(FP_TG_CTRL) & 0xfffffeff));
			}
		}
		else
		{
			float dm_aspect;

			LOG(2,("CRTC2: GPU scales for DFP if needed\n"));

			/* calculate display mode aspect */
			dm_aspect = (target.timing.h_display / ((float)target.timing.v_display));

			/* limit last fetched line if vertical scaling is done */
			if (iscale_y != (1 << 12))
				DAC2W(FP_DEBUG2, ((1 << 28) | ((target.timing.v_display - 1) << 16)));
			else
				DAC2W(FP_DEBUG2, 0x00000000);

			/* inform panel not to scale */
			DAC2W(FP_TG_CTRL, (DAC2R(FP_TG_CTRL) & 0xfffffeff));

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
			if ((iscale_x != (1 << 12)) && (si->ps.panel2_aspect > (dm_aspect + 0.10)))
			{
				uint16 diff;

				LOG(2,("CRTC2: (relative) widescreen panel: tuning horizontal scaling\n"));

				/* X-scaling should be the same as Y-scaling */
				iscale_x = iscale_y;
				/* enable testmode (b12) and program new X-scaling factor */
				DAC2W(FP_DEBUG1, (((iscale_x >> 1) & 0x00000fff) | (1 << 12)));
				/* center/cut-off left and right side of screen */
				diff = ((si->ps.p2_timing.h_display -
						((target.timing.h_display * (1 << 12)) / iscale_x))
						/ 2);
				DAC2W(FP_HVALID_S, diff);
				DAC2W(FP_HVALID_E, ((si->ps.p2_timing.h_display - diff) - 1));
			}
			/* correct for portrait panels... */
			/* NOTE:
			 * allow 0.10 difference so 1280x1024 panels will be used fullscreen! */
			if ((iscale_y != (1 << 12)) && (si->ps.panel2_aspect < (dm_aspect - 0.10)))
			{
				LOG(2,("CRTC2: (relative) portrait panel: should tune vertical scaling\n"));
				/* fixme: implement if this kind of portrait panels exist on nVidia... */
			}
		}

		/* do some logging.. */
		LOG(2,("CRTC2: FP_HVALID_S reg readback: $%08x\n", DAC2R(FP_HVALID_S)));
		LOG(2,("CRTC2: FP_HVALID_E reg readback: $%08x\n", DAC2R(FP_HVALID_E)));
		LOG(2,("CRTC2: FP_VVALID_S reg readback: $%08x\n", DAC2R(FP_VVALID_S)));
		LOG(2,("CRTC2: FP_VVALID_E reg readback: $%08x\n", DAC2R(FP_VVALID_E)));
		LOG(2,("CRTC2: FP_DEBUG0 reg readback: $%08x\n", DAC2R(FP_DEBUG0)));
		LOG(2,("CRTC2: FP_DEBUG1 reg readback: $%08x\n", DAC2R(FP_DEBUG1)));
		LOG(2,("CRTC2: FP_DEBUG2 reg readback: $%08x\n", DAC2R(FP_DEBUG2)));
		LOG(2,("CRTC2: FP_DEBUG3 reg readback: $%08x\n", DAC2R(FP_DEBUG3)));
		LOG(2,("CRTC2: FP_TG_CTRL reg readback: $%08x\n", DAC2R(FP_TG_CTRL)));
	}

	return B_OK;
}

status_t nv_crtc2_depth(int mode)
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
	/* enable access to secondary head */
	set_crtc_owner(1);

	CRTC2W(PIXEL, ((CRTC2R(PIXEL) & 0xfc) | viddelay));
	DAC2W(GENCTRL, genctrl);

	return B_OK;
}

status_t nv_crtc2_dpms(bool display, bool h, bool v, bool do_panel)
{
	uint8 temp;
	char msg[100];

	sprintf(msg, "CRTC2: setting DPMS: ");

	/* enable access to secondary head */
	set_crtc_owner(1);

	/* start synchronous reset: required before turning screen off! */
	SEQW(RESET, 0x01);

	temp = SEQR(CLKMODE);
	if (display)
	{
		/* turn screen on */
		SEQW(CLKMODE, (temp & ~0x20));

		/* end synchronous reset because display should be enabled */
		SEQW(RESET, 0x03);

		if (do_panel && si->ps.tmds2_active)
		{
			if (!si->ps.laptop)
			{
				/* restore original panelsync and panel-enable */
				uint32 panelsync = 0x00000000;
				if(si->ps.p2_timing.flags & B_POSITIVE_VSYNC) panelsync |= 0x00000001;
				if(si->ps.p2_timing.flags & B_POSITIVE_HSYNC) panelsync |= 0x00000010;
				/* display enable polarity (not an official flag) */
				if(si->ps.p2_timing.flags & B_BLANK_PEDESTAL) panelsync |= 0x10000000;
				DAC2W(FP_TG_CTRL, ((DAC2R(FP_TG_CTRL) & 0xcfffffcc) | panelsync));

				//fixme?: looks like we don't need this after all:
				/* powerup both LVDS (laptop panellink) and TMDS (DVI panellink)
				 * internal transmitters... */
				/* note:
				 * the powerbits in this register are hardwired to the DVI connectors,
				 * instead of to the DACs! (confirmed NV34) */
				//fixme...
				//DAC2W(FP_DEBUG0, (DAC2R(FP_DEBUG0) & 0xcfffffff));
				/* ... and powerup external TMDS transmitter if it exists */
				/* (confirmed OK on NV28 and NV34) */
				//CRTC2W(0x59, (CRTC2R(0x59) | 0x01));

				sprintf(msg, "%s(panel-)", msg);
			}
			else
			{
				//fixme: see if LVDS head can be determined with two panels there...
				if (!si->ps.tmds1_active)
				{
					/* b2 = 0 = enable laptop panel backlight */
					/* note: this seems to be a write-only register. */
					NV_REG32(NV32_LVDS_PWR) = 0x00000003;

					sprintf(msg, "%s(panel-)", msg);
				}
			}
		}

		sprintf(msg, "%sdisplay on, ", msg);
	}
	else
	{
		/* turn screen off */
		SEQW(CLKMODE, (temp | 0x20));

		if (do_panel && si->ps.tmds2_active)
		{
			if (!si->ps.laptop)
			{
				/* shutoff panelsync and disable panel */
				DAC2W(FP_TG_CTRL, ((DAC2R(FP_TG_CTRL) & 0xcfffffcc) | 0x20000022));

				//fixme?: looks like we don't need this after all:
				/* powerdown both LVDS (laptop panellink) and TMDS (DVI panellink)
				 * internal transmitters... */
				/* note:
				 * the powerbits in this register are hardwired to the DVI connectors,
				 * instead of to the DACs! (confirmed NV34) */
				//fixme...
				//DAC2W(FP_DEBUG0, (DAC2R(FP_DEBUG0) | 0x30000000));
				/* ... and powerdown external TMDS transmitter if it exists */
				/* (confirmed OK on NV28 and NV34) */
				//CRTC2W(0x59, (CRTC2R(0x59) & 0xfe));

				sprintf(msg, "%s(panel-)", msg);
			}
			else
			{
				//fixme: see if LVDS head can be determined with two panels there...
				if (!si->ps.tmds1_active)
				{
					/* b2 = 1 = disable laptop panel backlight */
					/* note: this seems to be a write-only register. */
					NV_REG32(NV32_LVDS_PWR) = 0x00000007;

					sprintf(msg, "%s(panel-)", msg);
				}
			}
		}

		sprintf(msg, "%sdisplay off, ", msg);
	}

	if (h)
	{
		CRTC2W(REPAINT1, (CRTC2R(REPAINT1) & 0x7f));
		sprintf(msg, "%shsync enabled, ", msg);
	}
	else
	{
		CRTC2W(REPAINT1, (CRTC2R(REPAINT1) | 0x80));
		sprintf(msg, "%shsync disabled, ", msg);
	}
	if (v)
	{
		CRTC2W(REPAINT1, (CRTC2R(REPAINT1) & 0xbf));
		sprintf(msg, "%svsync enabled\n", msg);
	}
	else
	{
		CRTC2W(REPAINT1, (CRTC2R(REPAINT1) | 0x40));
		sprintf(msg, "%svsync disabled\n", msg);
	}

	LOG(4, (msg));

	return B_OK;
}

status_t nv_crtc2_set_display_pitch() 
{
	uint32 offset;

	LOG(4,("CRTC2: setting card pitch (offset between lines)\n"));

	/* figure out offset value hardware needs */
	offset = si->fbc.bytes_per_row / 8;

	LOG(2,("CRTC2: offset register set to: $%04x\n", offset));

	/* enable access to secondary head */
	set_crtc_owner(1);

	/* program the card */
	CRTC2W(PITCHL, (offset & 0x00ff));
	CRTC2W(REPAINT0, ((CRTC2R(REPAINT0) & 0x1f) | ((offset & 0x0700) >> 3)));

	return B_OK;
}

status_t nv_crtc2_set_display_start(uint32 startadd,uint8 bpp) 
{
	uint32 timeout = 0;

	LOG(4,("CRTC2: setting card RAM to be displayed bpp %d\n", bpp));

	LOG(2,("CRTC2: startadd: $%08x\n", startadd));
	LOG(2,("CRTC2: frameRAM: $%08x\n", si->framebuffer));
	LOG(2,("CRTC2: framebuffer: $%08x\n", si->fbc.frame_buffer));

	/* we might have no retraces during setmode! */
	/* wait 25mS max. for retrace to occur (refresh > 40Hz) */
	while (((NV_REG32(NV32_RASTER2) & 0x000007ff) < si->dm.timing.v_display) &&
			(timeout < (25000/10)))
	{
		/* don't snooze much longer or retrace might get missed! */
		snooze(10);
		timeout++;
	}

	/* enable access to secondary head */
	set_crtc_owner(1);

	/* upto 4Gb RAM adressing: must be used on NV10 and later! */
	/* NOTE:
	 * While this register also exists on pre-NV10 cards, it will
	 * wrap-around at 16Mb boundaries!! */

	/* 30bit adress in 32bit words */
	NV_REG32(NV32_NV10FB2STADD32) = (startadd & 0xfffffffc);

	/* set byte adress: (b0 - 1) */
	ATB2W(HORPIXPAN, ((startadd & 0x00000003) << 1));

	return B_OK;
}

status_t nv_crtc2_cursor_init()
{
	int i;
	vuint32 * fb;
	/* cursor bitmap will be stored at the start of the framebuffer */
	const uint32 curadd = 0;

	/* enable access to secondary head */
	set_crtc_owner(1);

	/* set cursor bitmap adress ... */
	if (si->ps.laptop)
	{
		/* must be used this way on pre-NV10 and on all 'Go' cards! */

		/* cursorbitmap must start on 2Kbyte boundary: */
		/* set adress bit11-16, and set 'no doublescan' (registerbit 1 = 0) */
		CRTC2W(CURCTL0, ((curadd & 0x0001f800) >> 9));
		/* set adress bit17-23, and set graphics mode cursor(?) (registerbit 7 = 1) */
		CRTC2W(CURCTL1, (((curadd & 0x00fe0000) >> 17) | 0x80));
		/* set adress bit24-31 */
		CRTC2W(CURCTL2, ((curadd & 0xff000000) >> 24));
	}
	else
	{
		/* upto 4Gb RAM adressing:
		 * can be used on NV10 and later (except for 'Go' cards)! */
		/* NOTE:
		 * This register does not exist on pre-NV10 and 'Go' cards. */

		/* cursorbitmap must still start on 2Kbyte boundary: */
		NV_REG32(NV32_NV10CUR2ADD32) = (curadd & 0xfffff800);
	}

	/* set cursor colour: not needed because of direct nature of cursor bitmap. */

	/*clear cursor*/
	fb = (vuint32 *) si->framebuffer + curadd;
	for (i=0;i<(2048/4);i++)
	{
		fb[i]=0;
	}

	/* select 32x32 pixel, 16bit color cursorbitmap, no doublescan */
	NV_REG32(NV32_2CURCONF) = 0x02000100;

	/* activate hardware-sync between cursor updates and vertical retrace */
	DAC2W(NV10_CURSYNC, (DAC2R(NV10_CURSYNC) | 0x02000000));

	/* activate hardware cursor */
	nv_crtc2_cursor_show();

	return B_OK;
}

status_t nv_crtc2_cursor_show()
{
	LOG(4,("CRTC2: enabling cursor\n"));

	/* enable access to secondary head */
	set_crtc_owner(1);

	/* b0 = 1 enables cursor */
	CRTC2W(CURCTL0, (CRTC2R(CURCTL0) | 0x01));

	/* workaround for hardware bug confirmed existing on NV43:
	 * Cursor visibility is not updated without a position update if its hardware
	 * retrace sync is enabled. */
	if (si->ps.card_arch == NV40A) DAC2W(CURPOS, (DAC2R(CURPOS)));

	return B_OK;
}

status_t nv_crtc2_cursor_hide()
{
	LOG(4,("CRTC2: disabling cursor\n"));

	/* enable access to secondary head */
	set_crtc_owner(1);

	/* b0 = 0 disables cursor */
	CRTC2W(CURCTL0, (CRTC2R(CURCTL0) & 0xfe));

	/* workaround for hardware bug confirmed existing on NV43:
	 * Cursor visibility is not updated without a position update if its hardware
	 * retrace sync is enabled. */
	if (si->ps.card_arch == NV40A) DAC2W(CURPOS, (DAC2R(CURPOS)));

	return B_OK;
}

/*set up cursor shape*/
status_t nv_crtc2_cursor_define(uint8* andMask,uint8* xorMask)
{
	int x, y;
	uint8 b;
	vuint16 *cursor;
	uint16 pixel;

	/* get a pointer to the cursor */
	cursor = (vuint16*) si->framebuffer;

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
status_t nv_crtc2_cursor_position(uint16 x, uint16 y)
{
	/* the cursor position is updated during retrace by card hardware */

	/* update cursorposition */
	DAC2W(CURPOS, ((x & 0x0fff) | ((y & 0x0fff) << 16)));

	return B_OK;
}
