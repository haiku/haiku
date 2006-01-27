/* CTRC functionality */
/* Authors:
   Mark Watson 2/2000,
   Apsed,
   Rudolf Cornelissen 11/2002-11/2005
*/

#define MODULE_BIT 0x00040000

#include "mga_std.h"

/*Adjust passed parameters to a valid mode line*/
status_t gx00_crtc_validate_timing(
	uint16 *hd_e,uint16 *hs_s,uint16 *hs_e,uint16 *ht,
	uint16 *vd_e,uint16 *vs_s,uint16 *vs_e,uint16 *vt
)
{
/* horizontal */
	/* make all parameters multiples of 8 */
	*hd_e &= 0x0ff8; /* 2048 is a valid value for this item! */
	*hs_s &= 0x0ff8;
	*hs_e &= 0x0ff8;
	*ht   &= 0x0ff8;

	/* confine to required number of bits, taking logic into account */
	if (*hd_e > ((0x00ff + 1) << 3)) *hd_e = ((0x00ff + 1) << 3);
	if (*hs_s > ((0x01ff - 1) << 3)) *hs_s = ((0x01ff - 1) << 3);
	if (*hs_e > ( 0x01ff      << 3)) *hs_e = ( 0x01ff      << 3);
	if (*ht   > ((0x01ff + 5) << 3)) *ht   = ((0x01ff + 5) << 3);

	/* NOTE: keep horizontal timing at multiples of 8! */
	/* confine to a reasonable width */
	if (*hd_e < 640) *hd_e = 640;
	switch (si->ps.card_type)
	{
	case MIL1:
	case MYST: /* fixme MYST220 has MIL2 range.. */
		if (*hd_e > 1600) *hd_e = 1600;
		break;
	case MIL2:
	case G100:
	case G200:
		if (*hd_e > 1920) *hd_e = 1920;
		break;
	default: /* G400 and up */
		if (*hd_e > 2048) *hd_e = 2048;
		break;
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
	if (*vd_e >  0x7ff     ) *vd_e =  0x7ff     ; /* linecomp max value = 0x7ff! */
	if (*vs_s > (0xfff - 1)) *vs_s = (0xfff - 1);
	if (*vs_e >  0xfff     ) *vs_e =  0xfff     ;
	if (*vt   > (0xfff + 2)) *vt   = (0xfff + 2);

	/* confine to a reasonable height */
	if (*vd_e < 480) *vd_e = 480;
	switch (si->ps.card_type)
	{
	case MIL1:
	case MYST: /* fixme MYST220 has MIL2 range.. */
		if (*vd_e > 1200) *vd_e = 1200;
		break;
	case MIL2:
	case G100:
	case G200:
		if (*vd_e > 1440) *vd_e = 1440;
		break;
	default: /* G400 and up */
		if (*vd_e > 1536) *vd_e = 1536;
		break;
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

/* set a mode line - inputs are in pixels */
status_t gx00_crtc_set_timing(display_mode target)
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
	hblnk_e = (htotal + 4);
	hsync_s = (target.timing.h_sync_start >> 3);
	hsync_e = (target.timing.h_sync_end >> 3);

	vtotal = target.timing.v_total - 2;
	vdisp_e = target.timing.v_display - 1;
	vblnk_s = vdisp_e;
	vblnk_e = (vtotal + 1);
	vsync_s = target.timing.v_sync_start - 1; /* Matrox */
	vsync_e = target.timing.v_sync_end - 1; /* Matrox */

	/* We use the Matrox linecomp INT function to detect the
	 * vertical retrace at the earliest possible moment.. */
	linecomp = target.timing.v_display;

	/*log the mode I am setting*/
	LOG(2,("CRTC:\n\tHTOT:%x\n\tHDISPEND:%x\n\tHBLNKS:%x\n\tHBLNKE:%x\n\tHSYNCS:%x\n\tHSYNCE:%x\n\t",htotal,hdisp_e,hblnk_s,hblnk_e,hsync_s,hsync_e));
	LOG(2,("VTOT:%x\n\tVDISPEND:%x\n\tVBLNKS:%x\n\tVBLNKE:%x\n\tVSYNCS:%x\n\tVSYNCE:%x\n",vtotal,vdisp_e,vblnk_s,vblnk_e,vsync_s,vsync_e));

	/*actually program the card! Note linecomp is programmed to vblnk_s for VBI*/
	/*horizontal - VGA regs*/

	VGAW_I(CRTC, 0x00, (htotal & 0x0ff));
	VGAW_I(CRTC, 0x01, (hdisp_e & 0x0ff));
	VGAW_I(CRTC, 0x02, (hblnk_s & 0x0ff));
	/* b7 should be set for compatibility reasons */
	VGAW_I(CRTC, 0x03, ((hblnk_e & 0x01f) | 0x80));
	VGAW_I(CRTC, 0x04, (hsync_s & 0x0ff));
	VGAW_I(CRTC, 0x05, (hsync_e & 0x01f) | ((hblnk_e & 0x020) << 2));
	
	/*vertical - VGA regs*/
	VGAW_I(CRTC, 0x06, (vtotal & 0x0ff));
	VGAW_I(CRTC, 0x07,
	(
		((vtotal & 0x100) >> (8 - 0)) | ((vtotal & 0x200) >> (9 - 5)) |
		((vdisp_e & 0x100) >> (8 - 1)) | ((vdisp_e & 0x200) >> (9 - 6)) |
		((vsync_s & 0x100) >> (8 - 2)) | ((vsync_s & 0x200) >> (9 - 7)) |
		((vblnk_s & 0x100) >> (8 - 3)) | ((linecomp & 0x100) >> (8 - 4))
	));
	VGAW_I(CRTC, 0x08, 0x00);
	VGAW_I(CRTC, 0x09, ((vblnk_s & 0x200) >> (9 - 5)) | ((linecomp & 0x200) >> (9 - 6)));
	VGAW_I(CRTC, 0x10, (vsync_s & 0x0ff));
	VGAW_I(CRTC, 0x11, (((VGAR_I(CRTC, 0x11)) & 0xf0) | (vsync_e & 0x00f)));
	VGAW_I(CRTC, 0x12, (vdisp_e & 0x0ff));
	VGAW_I(CRTC, 0x15, (vblnk_s & 0x0ff));
	VGAW_I(CRTC, 0x16, (vblnk_e & 0x0ff));
	VGAW_I(CRTC, 0x18, (linecomp & 0x0ff));

	/* horizontal - extended regs */
	/* do not touch external sync reset inputs: used for TVout */
	VGAW_I(CRTCEXT, 1,
	(
		((htotal & 0x100) >> (8 - 0)) |
		((hblnk_s & 0x100) >> (8 - 1)) |
		((hsync_s & 0x100) >> (8 - 2)) |
		((hblnk_e & 0x040) >> (6 - 6)) |
		(VGAR_I(CRTCEXT, 1) & 0xb8)
	));

	/*vertical - extended regs*/
	VGAW_I(CRTCEXT, 2,
	(
	 	((vtotal & 0xc00) >> (10 - 0)) |
		((vdisp_e & 0x400) >> (10 - 2)) |
		((vblnk_s & 0xc00) >> (10 - 3)) |
		((vsync_s & 0xc00) >> (10 - 5)) |
		((linecomp & 0x400) >> (10 - 7))
	));

	/* setup HSYNC & VSYNC polarity */
	LOG(2,("CRTC: sync polarity: "));
	temp = VGAR(MISCR);
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
	VGAW(MISCW, temp);

	LOG(2,(", MISC reg readback: $%02x\n", VGAR(MISCR)));

	return B_OK;
}

status_t gx00_crtc_depth(int mode)
{
	uint8 viddelay = 0; // in CRTCEXT3, reserved if >= G100

	if (si->ps.card_type < G100) do { // apsed TODO in caller
		if (si->ps.memory_size <= 2) { viddelay = 1<<3; break;}
		if (si->ps.memory_size <= 4) { viddelay = 0<<3; break;}
		viddelay = 2<<3; // for 8 to 16Mb of memory
	} while (0);

	/* setup green_sync if requested */
	if (si->settings.greensync)
	{
		/* enable sync_on_green: ctrl bit polarity was reversed for Gxxx cards! */
		if (si->ps.card_type <= MIL2)
			DXIW(GENCTRL, (DXIR(GENCTRL) | 0x20));
		else
			DXIW(GENCTRL, (DXIR(GENCTRL) & ~0x20));
		/* select horizontal _and_ vertical sync */
		viddelay |= 0x40;

		LOG(4,("CRTC: sync_on_green enabled\n"));
	}
	else
	{
		/* disable sync_on_green: ctrl bit polarity was reversed for Gxxx cards! */
		if (si->ps.card_type <= MIL2)
			DXIW(GENCTRL, (DXIR(GENCTRL) & ~0x20));
		else
			DXIW(GENCTRL, (DXIR(GENCTRL) | 0x20));

		LOG(4,("CRTC: sync_on_green disabled\n"));
	}

	/*set VCLK scaling*/
	switch(mode)
	{
	case BPP8:
		VGAW_I(CRTCEXT,3,viddelay|0x80);
		break;
	case BPP15:case BPP16:
		VGAW_I(CRTCEXT,3,viddelay|0x81);
		break;
	case BPP24:
		VGAW_I(CRTCEXT,3,viddelay|0x82);
		break;
	case BPP32:case BPP32DIR:
		VGAW_I(CRTCEXT,3,viddelay|0x83);
		break;
	}
	return B_OK;
}

status_t gx00_crtc_dpms(bool display, bool h, bool v) // MIL2
{
	char msg[100];

	sprintf(msg, "CRTC: setting DPMS: ");

	if (display)
	{
		VGAW_I(SEQ,1, 0x00);
		sprintf(msg, "%sdisplay on, ", msg);
	}
	else
	{
		VGAW_I(SEQ,1, 0x20);
		sprintf(msg, "%sdisplay off, ", msg);
	}
	if (h)
	{
		VGAW_I(CRTCEXT, 1, (VGAR_I(CRTCEXT, 1) & 0xef));
		sprintf(msg, "%shsync enabled, ", msg);
	}
	else
	{
		VGAW_I(CRTCEXT, 1, (VGAR_I(CRTCEXT, 1) | 0x10));
		sprintf(msg, "%shsync disabled, ", msg);
	}
	if (v)
	{
		VGAW_I(CRTCEXT, 1, (VGAR_I(CRTCEXT, 1) & 0xdf));
		sprintf(msg, "%svsync enabled\n", msg);
	}
	else
	{
		VGAW_I(CRTCEXT, 1, (VGAR_I(CRTCEXT, 1) | 0x20));
		sprintf(msg, "%svsync disabled\n", msg);
	}

	LOG(4, (msg));

	/* set some required fixed values for proper MGA mode initialisation */
	VGAW_I(CRTC,0x17,0xC3);
	VGAW_I(CRTC,0x14,0x00);

	/* make sure CRTC1 sync is patched through on connector on G450/G550! */
	if (si->ps.card_type >= G450)
	{
		if (si->crossed_conns)
		{
			/* patch through HD15 hsync and vsync unmodified */
			DXIW(SYNCCTRL, (DXIR(SYNCCTRL) & 0x0f));
		}
		else
		{
			/* patch through DVI-A hsync and vsync unmodified */
			DXIW(SYNCCTRL, (DXIR(SYNCCTRL) & 0xf0));
		}
	}

	return B_OK;
}

status_t gx00_crtc_set_display_pitch() 
{
	uint32 offset;

	LOG(4,("CRTC: setting card pitch (offset between lines)\n"));

	/* figure out offset value hardware needs:
	 * same for MIL1-G550 cards assuming MIL1/2 uses the TVP3026 64-bits DAC etc. */
	offset = si->fbc.bytes_per_row / 16;

	LOG(2,("CRTC: offset register: 0x%04x\n",offset));
		
	/*program the card!*/
	VGAW_I(CRTC,0x13,(offset&0xFF));
	VGAW_I(CRTCEXT,0,(VGAR_I(CRTCEXT,0)&0xCF)|((offset&0x300)>>4));
	return B_OK;
}

status_t gx00_crtc_set_display_start(uint32 startadd,uint8 bpp) 
{
	uint32 ext0;

	LOG(4,("CRTC: setting card RAM to be displayed bpp %d\n", bpp));

	/* Matrox docs are false/incomplete, always program qword adress. */
	startadd >>= 3;

	LOG(2,("CRTC: startadd: %x\n",startadd));
	LOG(2,("CRTC: frameRAM: %x\n",si->framebuffer));
	LOG(2,("CRTC: framebuffer: %x\n",si->fbc.frame_buffer));

	/* make sure we are in retrace on MIL cards (if possible), because otherwise
	 * distortions might occur during our reprogramming them (no double buffering) */
	if (si->ps.card_type < G100)
	{
		/* we might have no retraces during setmode! */
		uint32 timeout = 0;
		/* wait 25mS max. for retrace to occur (refresh > 40Hz) */
		while ((!(ACCR(STATUS) & 0x08)) && (timeout < (25000/4)))
		{
			snooze(4);
			timeout++;
		}
	}

	/*set standard registers*/
	VGAW_I(CRTC,0xD,startadd&0xFF);
	VGAW_I(CRTC,0xC,(startadd&0xFF00)>>8);

	//calculate extra bits that are standard over Gx00 series
	ext0 = VGAR_I(CRTCEXT,0)&0xB0;
	ext0|= (startadd&0xF0000)>>16;

	//if card is a G200 or G400 then do first extension bit
	if (si->ps.card_type>=G200)
		ext0|=(startadd&0x100000)>>14;
	
	//if card is a G400 then do write to its extension register
	if (si->ps.card_type>=G400)
		VGAW_I(CRTCEXT,8,((startadd&0x200000)>>21));

	//write the extension bits
	VGAW_I(CRTCEXT,0,ext0);

	return B_OK;
}

status_t gx00_crtc_mem_priority(uint8 colordepth)
{	
	float tpixclk, tmclk, refresh, temp;
	uint8 mp, vc, hiprilvl, maxhipri, prioctl;

	/* we can only do this if card pins is read OK *and* card is coldstarted! */
	if (si->settings.usebios || (si->ps.pins_status != B_OK))
	{
		LOG(4,("CRTC: Card not coldstarted, skipping memory priority level setup\n"));
		return B_OK;
	}

	/* only on G200 the mem_priority register should be programmed with this formula */
	if (si->ps.card_type != G200)
	{
		LOG(4,("CRTC: Memory priority level setup not needed, skipping\n"));
		return B_OK;
	}

	/* make sure the G200 is running at peak performance, so for instance high-res
	 * overlay distortions due to bandwidth limitations are minimal.
	 * Note please that later cards have plenty of bandwidth to cope by default.
	 * Note also that the formula needed is entirely cardtype-dependant! */
	LOG(4,("CRTC: Setting G200 memory priority level\n"));

	/* set memory controller pipe depth, assuming no codec or Vin operating */
	switch ((si->ps.memrdbk_reg & 0x00c00000) >> 22)
	{
	case 0:
		mp = 52;
		break;
	case 1:
		mp = 41;
		break;
	case 2:
		mp = 32;
		break;
	default:
		mp = 52;
		LOG(8,("CRTC: Streamer flowcontrol violation in PINS, defaulting to %%00\n"));
		break;
	}

	/* calculate number of videoclocks needed per 8 pixels */
	vc = (8 * colordepth) / 64;
	
	/* calculate pixelclock period (nS) */
	tpixclk = 1000000 / si->dm.timing.pixel_clock;

	/* calculate memoryclock period (nS) */
	if (si->ps.v3_option2_reg & 0x08)
	{
		tmclk = 1000.0 / si->ps.std_engine_clock;
	}
	else
	{
		if (si->ps.v3_clk_div & 0x02)
			tmclk = 3000.0 / si->ps.std_engine_clock;
		else
			tmclk = 2000.0 / si->ps.std_engine_clock;
	}

	/* calculate refreshrate of current displaymode */
	refresh = ((si->dm.timing.pixel_clock * 1000) /
		((uint32)si->dm.timing.h_total * (uint32)si->dm.timing.v_total));

	/* calculate high priority request level, but stay on the 'crtc-safe' side:
	 * hence 'formula + 1.0' instead of 'formula + 0.5' */
	temp = (((((mp * tmclk) + (11 * vc * tpixclk)) / tpixclk) - (vc - 1)) / (8 * vc)) + 1.0;
	if (temp > 7.0) temp = 7.0;
	if (temp < 0.0) temp = 0.0;
	hiprilvl = 7 - ((uint8) temp);
	/* limit non-crtc priority so crtc always stays 'just' OK */
	if (hiprilvl > 4) hiprilvl = 4;
	if ((si->dm.timing.v_display > 768) && (hiprilvl > 3)) hiprilvl = 3;
	if ((si->dm.timing.v_display > 864) && (hiprilvl > 2) && (refresh >= 76.0)) hiprilvl = 2; 
	if ((si->dm.timing.v_display > 1024) && (hiprilvl > 2)) hiprilvl = 2;

	/* calculate maximum high priority requests */
	temp = (vc * (tmclk / tpixclk)) + 0.5;
	if (temp > (float)hiprilvl) temp = (float)hiprilvl;
	if (temp < 0.0) temp = 0.0;
	maxhipri = ((uint8) temp);

	/* program the card */
	prioctl = ((hiprilvl & 0x07) | ((maxhipri & 0x07) << 4));	
	VGAW_I(CRTCEXT, 6, prioctl);

	/* log results */
	LOG(4,("CRTC: Vclks/char is %d, pixClk period %02.2fnS, memClk period %02.2fnS\n",
		vc, tpixclk, tmclk));
	LOG(4,("CRTC: memory priority control register is set to $%02x\n", prioctl));

	return B_OK;
}

status_t gx00_crtc_cursor_init()
{
	int i;

	if (si->ps.card_type >= G100)
	{
		vuint32 * fb;
		/* cursor bitmap will be stored at the start of the framebuffer on >= G100 */
		const uint32 curadd = 0;

		/* set cursor bitmap adress ... */
		DXIW(CURADDL,curadd >> 10);
		DXIW(CURADDH,curadd >> 18);
		/* ... and repeat that: G100 requires other programming order than later cards!?! */
		DXIW(CURADDL,curadd >> 10);
		DXIW(CURADDH,curadd >> 18);

		/*set cursor colour*/
		DXIW(CURCOL0RED,0XFF);
		DXIW(CURCOL0GREEN,0xFF);
		DXIW(CURCOL0BLUE,0xFF);
		DXIW(CURCOL1RED,0);
		DXIW(CURCOL1GREEN,0);
		DXIW(CURCOL1BLUE,0);
		DXIW(CURCOL2RED,0);
		DXIW(CURCOL2GREEN,0);
		DXIW(CURCOL2BLUE,0);

		/*clear cursor*/
		fb = (vuint32 *) si->framebuffer + curadd;
		for (i=0;i<(1024/4);i++)
		{
			fb[i]=0;
		}
	}
	else
	/* <= G100 cards have serial cursor color registers,
	 * and dedicated cursor bitmap RAM (in TVP3026 DAC)
	 */
	{
		/* select first colorRAM adress */
		DACW(TVP_CUROVRWTADD,0x00);
		/* overscan/border color is black, order of colors set is R,G,B */
		DACW(TVP_CUROVRDATA,0xff);
		DACW(TVP_CUROVRDATA,0xff);
		DACW(TVP_CUROVRDATA,0xff);
		/* set sursor color 0 */
		DACW(TVP_CUROVRDATA,0xff);
		DACW(TVP_CUROVRDATA,0xff);
		DACW(TVP_CUROVRDATA,0xff);
		/* set sursor color 1 */
		DACW(TVP_CUROVRDATA,0x00);
		DACW(TVP_CUROVRDATA,0x00);
		DACW(TVP_CUROVRDATA,0x00);
		/* set sursor color 2 */
		DACW(TVP_CUROVRDATA,0x00);
		DACW(TVP_CUROVRDATA,0x00);
		DACW(TVP_CUROVRDATA,0x00);

		/* select first cursor pattern DAC-internal RAM adress, and
		 * make sure indirect cursor control register is selected as active register */
		DXIW(CURCTRL,(DXIR(CURCTRL) & 0x73));
		DACW(PALWTADD,0x00);
		/* now clear it, auto-incrementing the adress */
		for(i=0;i<1024;i++)
		{
			DACW(TVP_CURRAMDATA,0x00);
		}
	}

	/* activate hardware cursor */
	gx00_crtc_cursor_show();

	return B_OK;
}

status_t gx00_crtc_cursor_show()
{
	if ((si->ps.card_type < G100) && (si->dm.timing.h_total > 2048))
	{
		/* MIL1/2 DAC needs to be told if h_total for the active mode gets above 2048 */
		DXIW(CURCTRL, 0x11);
	}
	else
	{
		DXIW(CURCTRL, 0x01);
	}

	return B_OK;
}

status_t gx00_crtc_cursor_hide()
{
	DXIW(CURCTRL,0);
	return B_OK;
}

/*set up cursor shape*/
status_t gx00_crtc_cursor_define(uint8* andMask,uint8* xorMask)
{
	int y;

	if(si->ps.card_type >= G100)
	{
		vuint8 * cursor;

		/*get a pointer to the cursor*/
		cursor = (vuint8*) si->framebuffer;

		/*draw the cursor*/
		for(y=0;y<16;y++)
		{
			cursor[y*16+7]=~*andMask++;
			cursor[y*16+15]=*xorMask++;
			cursor[y*16+6]=~*andMask++;
			cursor[y*16+14]=*xorMask++;
		}
	}
	else
	/* <= G100 cards have dedicated cursor bitmap RAM (in TVP3026 DAC) */
	{
		uint8 curctrl;

		/* disable the cursor to prevent distortions in screen output */
		curctrl = (DXIR(CURCTRL));
		DXIW(CURCTRL, (curctrl & 0xfc));
		/* select first cursor pattern DAC-internal RAM adress for plane 0 */
		DXIW(CURCTRL, (DXIR(CURCTRL) & ~0x0c));
		DACW(PALWTADD, 0x00);
		/* now fill it, partly auto-incrementing the adress */
		for(y = 0; y < 16; y++)
		{
			DACW(PALWTADD, (y * 8));
			DACW(TVP_CURRAMDATA, ~*andMask++);
			DACW(TVP_CURRAMDATA, ~*andMask++);
		}
		/* select first cursor pattern DAC-internal RAM adress for plane 1 */
		DXIW(CURCTRL, (DXIR(CURCTRL) | 0x08));
		DACW(PALWTADD, 0x00);
		/* now fill it, partly auto-incrementing the adress */
		for(y = 0; y < 16; y++)
		{
			DACW(PALWTADD, y*8);
			DACW(TVP_CURRAMDATA, *xorMask++);
			DACW(TVP_CURRAMDATA, *xorMask++);
		}
		/* delay restoring the cursor to prevent distortions in screen output */
		snooze(5);
		/* restore the cursor */
		DXIW(CURCTRL, curctrl);
	}

	return B_OK;
}

/*position the cursor*/
status_t gx00_crtc_cursor_position(uint16 x ,uint16 y)
{
	int i=64;

	x+=i;
	y+=i;

	/* make sure we are not in retrace, because the register(s) might get copied
	 * during our reprogramming them (double buffering feature) */
	while (ACCR(STATUS) & 0x08)
	{
		snooze(4);
	}

	DACW(CURSPOSXL,x&0xFF);
	DACW(CURSPOSXH,x>>8);
	DACW(CURSPOSYL,y&0xFF);
	DACW(CURSPOSYH,y>>8);

	return B_OK;
}
