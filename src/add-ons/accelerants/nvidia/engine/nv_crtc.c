/* CTRC functionality */
/* Author:
   Rudolf Cornelissen 11/2002-3/2004
*/

#define MODULE_BIT 0x00040000

#include "nv_std.h"

/*Adjust passed parameters to a valid mode line*/
status_t nv_crtc_validate_timing(
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

	/* make sure sync pulse is not during display */
	if (*vs_e > (*vt - 1)) *vs_e = (*vt - 1);
	if (*vs_s < (*vd_e + 1)) *vs_s = (*vd_e + 1);

	/* correct sync pulse if it is too long:
	 * there are only 4 bits available to save this in the card registers! */
	if (*vs_e > (*vs_s + 0x0f)) *vs_e = (*vs_s + 0x0f);

	return B_OK;
}
	

/*set a mode line - inputs are in pixels*/
status_t nv_crtc_set_timing(display_mode target)
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

	/* enable access to CRTC1 on dualhead cards */
	if (si->ps.secondary_head) CRTCW(OWNER, 0x00);

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
	CRTCW(INTERLACE, 0xff);

	/* setup flatpanel if connected and active */
	if (si->ps.tmds1_active)
	{
		uint32 iscale_x, iscale_y;

		//fixme: checkout upscaling and aspect!!!
		/* calculate needed inverse scaling factors in 20.12 format */
		iscale_x = (((1 << 12) * target.timing.h_display) / si->ps.panel1_width);
		iscale_y = (((1 << 12) * target.timing.v_display) / si->ps.panel1_height);

		/* unblock flatpanel timing programming (or something like that..) */
		CRTCW(FP_HTIMING, 0);
		CRTCW(FP_VTIMING, 0);

		/* nVidia cards only support upscaling: NV11 can't scale at all */
		if ((si->ps.card_arch == NV11) ||
			(iscale_x > (1 << 12)) || (iscale_y > (1 << 12)))
		{
			LOG(2,("CRTC: DFP needs to do scaling\n"));

			/* disable last fetched line limiting */
			DACW(FP_DEBUG2, 0x00000000);
			/* inform panel to scale */
			DACW(FP_TG_CTRL, (DACR(FP_TG_CTRL) | 0x00000100));
		}
		else
		{
			LOG(2,("CRTC: GPU scales if needed\n"));

			/* GPU scaling is automatically setup by hardware */
//fixme: checkout non 4:3 aspect scaling.
//			DACW(FP_DEBUG3, (iscale_x & 0x00001fff) | ((iscale_y & 0x00001fff) << 16));
//			temp = (((iscale_x >> 1) & 0x00000fff) | (((iscale_y >> 1) & 0x00000fff) << 16));
//			DACW(FP_DEBUG1, (temp | (1 << 12) | (1 << 28)));

			/* limit last fetched line if vertical scaling is done */
			if (iscale_y != (1 << 12))
				DACW(FP_DEBUG2, ((1 << 28) | ((target.timing.v_display - 1) << 16)));
//not needed apparantly:
//								((1 << 12) | ((target.timing.h_display - 1) <<  0)));
			else
				DACW(FP_DEBUG2, 0x00000000);

			/* inform panel not to scale */
			DACW(FP_TG_CTRL, (DACR(FP_TG_CTRL) & 0xfffffeff));
		}

		/* do some logging.. */
		LOG(2,("CRTC: FP_DEBUG0 reg readback: $%08x\n", DACR(FP_DEBUG0)));
		LOG(2,("CRTC: FP_DEBUG1 reg readback: $%08x\n", DACR(FP_DEBUG1)));
		LOG(2,("CRTC: FP_DEBUG2 reg readback: $%08x\n", DACR(FP_DEBUG2)));
		LOG(2,("CRTC: FP_DEBUG3 reg readback: $%08x\n", DACR(FP_DEBUG3)));
		LOG(2,("CRTC: FP_TG_CTRL reg readback: $%08x\n", DACR(FP_TG_CTRL)));
	}

	return B_OK;
}

status_t nv_crtc_depth(int mode)
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
	/* enable access to CRTC1 on dualhead cards */
	if (si->ps.secondary_head) CRTCW(OWNER, 0x00);

	CRTCW(PIXEL, ((CRTCR(PIXEL) & 0xfc) | viddelay));
	DACW(GENCTRL, genctrl);

	return B_OK;
}

status_t nv_crtc_dpms(bool display, bool h, bool v)
{
	uint8 temp;

	LOG(4,("CRTC: setting DPMS: "));

	/* enable access to CRTC1 (and SEQUENCER1) on dualhead cards */
	if (si->ps.secondary_head) CRTCW(OWNER, 0x00);

	/* start synchronous reset: required before turning screen off! */
	SEQW(RESET, 0x01);

	/* turn screen off */
	temp = SEQR(CLKMODE);
	if (display)
	{
		SEQW(CLKMODE, (temp & ~0x20));

		/* end synchronous reset if display should be enabled */
		SEQW(RESET, 0x03);

		LOG(4,("display on, "));
	}
	else
	{
		SEQW(CLKMODE, (temp | 0x20));

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

status_t nv_crtc_dpms_fetch(bool *display, bool *h, bool *v)
{
	/* enable access to CRTC1 (and SEQUENCER1) on dualhead cards */
	if (si->ps.secondary_head) CRTCW(OWNER, 0x00);

	*display = !(SEQR(CLKMODE) & 0x20);
	*h = !(CRTCR(REPAINT1) & 0x80);
	*v = !(CRTCR(REPAINT1) & 0x40);

	LOG(4,("CTRC: fetched DPMS state:"));
	if (display) LOG(4,("display on, "));
	else LOG(4,("display off, "));
	if (h) LOG(4,("hsync enabled, "));
	else LOG(4,("hsync disabled, "));
	if (v) LOG(4,("vsync enabled\n"));
	else LOG(4,("vsync disabled\n"));

	return B_OK;
}

status_t nv_crtc_set_display_pitch() 
{
	uint32 offset;

	LOG(4,("CRTC: setting card pitch (offset between lines)\n"));

	/* figure out offset value hardware needs */
	offset = si->fbc.bytes_per_row / 8;

	LOG(2,("CRTC: offset register set to: $%04x\n", offset));

	/* enable access to CRTC1 on dualhead cards */
	if (si->ps.secondary_head) CRTCW(OWNER, 0x00);

	/* program the card */
	CRTCW(PITCHL, (offset & 0x00ff));
	CRTCW(REPAINT0, ((CRTCR(REPAINT0) & 0x1f) | ((offset & 0x0700) >> 3)));

	return B_OK;
}

status_t nv_crtc_set_display_start(uint32 startadd,uint8 bpp) 
{
	uint8 temp;
	uint32 timeout = 0;

	LOG(4,("CRTC: setting card RAM to be displayed bpp %d\n", bpp));

	LOG(2,("CRTC: startadd: $%08x\n", startadd));
	LOG(2,("CRTC: frameRAM: $%08x\n", si->framebuffer));
	LOG(2,("CRTC: framebuffer: $%08x\n", si->fbc.frame_buffer));

	/* we might have no retraces during setmode! */
	/* wait 25mS max. for retrace to occur (refresh > 40Hz) */
	while (((NV_REG32(NV32_RASTER) & 0x000007ff) < si->dm.timing.v_display) &&
			(timeout < (25000/10)))
	{
		/* don't snooze much longer or retrace might get missed! */
		snooze(10);
		timeout++;
	}

	/* enable access to CRTC1 on dualhead cards */
	if (si->ps.secondary_head) CRTCW(OWNER, 0x00);

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
		NV_REG32(NV32_NV10FBSTADD32) = (startadd & 0xfffffffc);
	}

	/* set NV4/NV10 byte adress: (b0 - 1) */
	ATBW(HORPIXPAN, ((startadd & 0x00000003) << 1));

	return B_OK;
}

status_t nv_crtc_cursor_init()
{
	int i;
	uint32 * fb;
	/* cursor bitmap will be stored at the start of the framebuffer */
	const uint32 curadd = 0;

	/* enable access to CRTC1 on dualhead cards */
	if (si->ps.secondary_head) CRTCW(OWNER, 0x00);

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
		NV_REG32(NV32_NV10CURADD32) = (curadd & 0xfffff800);
	}

	/* set cursor colour: not needed because of direct nature of cursor bitmap. */

	/*clear cursor*/
	fb = (uint32 *) si->framebuffer + curadd;
	for (i=0;i<(2048/4);i++)
	{
		fb[i]=0;
	}

	/* select 32x32 pixel, 16bit color cursorbitmap, no doublescan */
	NV_REG32(NV32_CURCONF) = 0x02000100;

	/* activate hardware cursor */
	nv_crtc_cursor_show();

	return B_OK;
}

status_t nv_crtc_cursor_show()
{
	LOG(4,("CRTC: enabling cursor\n"));

	/* enable access to CRTC1 on dualhead cards */
	if (si->ps.secondary_head) CRTCW(OWNER, 0x00);

	/* b0 = 1 enables cursor */
	CRTCW(CURCTL0, (CRTCR(CURCTL0) | 0x01));

	return B_OK;
}

status_t nv_crtc_cursor_hide()
{
	LOG(4,("CRTC: disabling cursor\n"));

	/* enable access to CRTC1 on dualhead cards */
	if (si->ps.secondary_head) CRTCW(OWNER, 0x00);

	/* b0 = 0 disables cursor */
	CRTCW(CURCTL0, (CRTCR(CURCTL0) & 0xfe));

	return B_OK;
}

/*set up cursor shape*/
status_t nv_crtc_cursor_define(uint8* andMask,uint8* xorMask)
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
status_t nv_crtc_cursor_position(uint16 x, uint16 y)
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
		while (((uint16)(NV_REG32(NV32_RASTER) & 0x000007ff)) < (yhigh + 16))
		{
			snooze(10);
		}
	}
	else
	{
		/* no room to spare, just wait for retrace (is relatively slow) */
		while ((NV_REG32(NV32_RASTER) & 0x000007ff) < si->dm.timing.v_display)
		{
			/* don't snooze much longer or retrace might get missed! */
			snooze(10);
		}
	}

	/* update cursorposition */
	DACW(CURPOS, ((x & 0x0fff) | ((y & 0x0fff) << 16)));

	return B_OK;
}
