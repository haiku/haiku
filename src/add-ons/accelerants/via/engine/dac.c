/* program the DAC */
/* Author:
   Rudolf Cornelissen 12/2003-7/2005
*/

#define MODULE_BIT 0x00010000

#include "std.h"

static status_t cle266_km400_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test);

/* see if an analog VGA monitor is connected to connector #1 */
bool eng_dac_crt_connected(void)
{
	uint32 output, dac;
	bool present;

	/* save output connector setting */
	output = DACR(OUTPUT);
	/* save DAC state */
	dac = DACR(TSTCTRL);

	/* turn on DAC */
	DACW(TSTCTRL, (DACR(TSTCTRL) & 0xfffeffff));
	/* select primary head and turn off CRT (and DVI?) outputs */
	DACW(OUTPUT, (output & 0x0000feee));
	/* wait for signal lines to stabilize */
	snooze(1000);
	/* re-enable CRT output */
	DACW(OUTPUT, (DACR(OUTPUT) | 0x00000001));

	/* setup RGB test signal levels to approx 30% of DAC range and enable them */
	DACW(TSTDATA, ((0x2 << 30) | (0x140 << 20) | (0x140 << 10) | (0x140 << 0)));
	/* route test signals to output */
	DACW(TSTCTRL, (DACR(TSTCTRL) | 0x00001000));
	/* wait for signal lines to stabilize */
	snooze(1000);

	/* do actual detection: all signals paths high == CRT connected */
	if (DACR(TSTCTRL) & 0x10000000)
	{
		present = true;
		LOG(4,("DAC: CRT detected on connector #1\n"));
	}
	else
	{
		present = false;
		LOG(4,("DAC: no CRT detected on connector #1\n"));
	}

	/* kill test signal routing */
	DACW(TSTCTRL, (DACR(TSTCTRL) & 0xffffefff));

	/* restore output connector setting */
	DACW(OUTPUT, output);
	/* restore DAC state */
	DACW(TSTCTRL, dac);

	return present;
}

/*set the mode, brightness is a value from 0->2 (where 1 is equivalent to direct)*/
status_t eng_dac_mode(int mode,float brightness)
{
	uint8 *r,*g,*b;
	int i, ri;

	/* 8-bit mode uses the palette differently */
	if (mode == BPP8) return B_ERROR;

	/*set colour arrays to point to space reserved in shared info*/
	r = si->color_data;
	g = r + 256;
	b = g + 256;

	LOG(4,("DAC: Setting screen mode %d brightness %f\n", mode, brightness));
	/* init the palette for brightness specified */
	/* (Nvidia cards always use MSbits from screenbuffer as index for PAL) */
	for (i = 0; i < 256; i++)
	{
		ri = i * brightness;
		if (ri > 255) ri = 255;
		b[i] = g[i] = r[i] = ri;
	}

	if (eng_dac_palette(r,g,b) != B_OK) return B_ERROR;

	/* disable palette RAM adressing mask */
	ENG_REG8(RG8_PALMASK) = 0xff;
	LOG(2,("DAC: PAL pixrdmsk readback $%02x\n", ENG_REG8(RG8_PALMASK)));

	return B_OK;
}

/*program the DAC palette using the given r,g,b values*/
status_t eng_dac_palette(uint8 r[256],uint8 g[256],uint8 b[256])
{
	int i;
	LOG(4,("DAC: setting palette\n"));

	/* enable primary head palette access */
	SEQW(MMIO_EN, ((SEQR(MMIO_EN)) & 0xfe));
	/* ??? */
	SEQW(0x1b, ((SEQR(0x1b)) | 0x20));
	/* disable gamma correction HW mode */
	SEQW(FIFOWM, ((SEQR(FIFOWM)) & 0x7f));
	/* select first PAL adress before starting programming */
	ENG_REG8(RG8_PALINDW) = 0x00;

	/* loop through all 256 to program DAC */
	for (i = 0; i < 256; i++)
	{
		ENG_REG8(RG8_PALDATA) = r[i];
		ENG_REG8(RG8_PALDATA) = g[i];
		ENG_REG8(RG8_PALDATA) = b[i];
	}
	if (ENG_REG8(RG8_PALINDW) != 0x00)
	{
		LOG(8,("DAC: PAL write index incorrect after programming\n"));
		return B_ERROR;
	}
if (0)
 {//reread LUT
	uint8 R, G, B;

	/* select first PAL adress to read (modulo 3 counter) */
	ENG_REG8(RG8_PALINDR) = 0x00;
	for (i = 0; i < 256; i++)
	{
		R = ENG_REG8(RG8_PALDATA);
		G = ENG_REG8(RG8_PALDATA);
		B = ENG_REG8(RG8_PALDATA);
		if ((r[i] != R) || (g[i] != G) || (b[i] != B)) 
			LOG(1,("DAC palette %d: w %x %x %x, r %x %x %x\n", i, r[i], g[i], b[i], R, G, B)); // apsed
	}
 }

	return B_OK;
}

/*program the pixpll - frequency in kHz*/
status_t eng_dac_set_pix_pll(display_mode target)
{
	uint8 m=0,n=0,p=0;
//	uint time = 0;

	float pix_setting, req_pclk;
	status_t result;

	/* we offer this option because some panels have very tight restrictions,
	 * and there's no overlapping settings range that makes them all work.
	 * note:
	 * this assumes the cards BIOS correctly programmed the panel (is likely) */
	//fixme: when VESA DDC EDID stuff is implemented, this option can be deleted...
	if (0)//si->ps.tmds1_active && !si->settings.pgm_panel)
	{
		LOG(4,("DAC: Not programming DFP refresh (specified in via.settings)\n"));
		return B_OK;
	}

	/* fix a DVI or laptop flatpanel to 60Hz refresh! */
	/* Note:
	 * The pixelclock drives the flatpanel modeline, not the CRTC modeline. */
	if (0)//si->ps.tmds1_active)
	{
		LOG(4,("DAC: Fixing DFP refresh to 60Hz!\n"));

		/* use the panel's modeline to determine the needed pixelclock */
		target.timing.pixel_clock = si->ps.p1_timing.pixel_clock;
	}

	req_pclk = (target.timing.pixel_clock)/1000.0;
	LOG(4,("DAC: Setting PIX PLL for pixelclock %f\n", req_pclk));

	/* signal that we actually want to set the mode */
	result = eng_dac_pix_pll_find(target,&pix_setting,&m,&n,&p, 1);
	if (result != B_OK)
	{
		return result;
	}

	/* reset primary pixelPLL */
	SEQW(PLL_RESET, ((SEQR(PLL_RESET)) | 0x02));
	snooze(1000);
	SEQW(PLL_RESET, ((SEQR(PLL_RESET)) & ~0x02));

	/* program new frequency */
	if (si->ps.card_arch != K8M800)
	{
		/* fixme: b7 is a lock-indicator or a filter select: to be determined! */
		SEQW(PPLL_N_CLE, (n & 0x7f));
		SEQW(PPLL_MP_CLE, ((m & 0x3f) | ((p & 0x03) << 6)));
	}
	else
	{
		/* fixme: preliminary, still needs to be confirmed */
		SEQW(PPLL_N_OTH, (n & 0x7f));
		SEQW(PPLL_M_OTH, (m & 0x3f));
		SEQW(PPLL_P_OTH, (p & 0x03));
	}

	/* reset primary pixelPLL (playing it safe) */
	SEQW(PLL_RESET, ((SEQR(PLL_RESET)) | 0x02));
	snooze(1000);
	SEQW(PLL_RESET, ((SEQR(PLL_RESET)) & ~0x02));

	/* now select pixelclock source D (the above custom VIA programmable PLL) */
	snooze(1000);
	ENG_REG8(RG8_MISCW) = 0xcf;

	/* Wait for the PIXPLL frequency to lock until timeout occurs */
//fixme: do VIA cards have a LOCK indication bit??
/*	while((!(DXIR(PIXPLLSTAT)&0x40)) & (time <= 2000))
	{
		time++;
		snooze(1);
	}
	
	if (time > 2000)
		LOG(2,("DAC: PIX PLL frequency not locked!\n"));
	else
		LOG(2,("DAC: PIX PLL frequency locked\n"));
	DXIW(PIXCLKCTRL,DXIR(PIXCLKCTRL)&0x0B);         //enable the PIXPLL
*/

//for now:
	/* Give the PIXPLL frequency some time to lock... */
	snooze(1000);
	LOG(2,("DAC: PIX PLL frequency should be locked now...\n"));

	return B_OK;
}

/* find nearest valid pix pll */
status_t eng_dac_pix_pll_find
	(display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test)
{
	//fixme: add K8M800 calcs if needed..
	switch (si->ps.card_type) {
		default:   return cle266_km400_dac_pix_pll_find(target, calc_pclk, m_result, n_result, p_result, test);
	}
	return B_ERROR;
}

/* find nearest valid pixel PLL setting */
static status_t cle266_km400_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test)
{
	int m = 0, n = 0, p = 0/*, m_max*/;
	float error, error_best = 999999999;
	int best[3]; 
	float f_vco, max_pclk;
	float req_pclk = target.timing.pixel_clock/1000.0;

	/* determine the max. reference-frequency postscaler setting for the 
	 * current card (see G100, G200 and G400 specs). */
/*	switch(si->ps.card_type)
	{
	case G100:
		LOG(4,("DAC: G100 restrictions apply\n"));
		m_max = 7;
		break;
	case G200:
		LOG(4,("DAC: G200 restrictions apply\n"));
		m_max = 7;
		break;
	default:
		LOG(4,("DAC: G400/G400MAX restrictions apply\n"));
		m_max = 32;
		break;
	}
*/
	LOG(4,("DAC: CLE266/KM400 restrictions apply\n"));

	/* determine the max. pixelclock for the current videomode */
	switch (target.space)
	{
		case B_CMAP8:
			max_pclk = si->ps.max_dac1_clock_8;
			break;
		case B_RGB15_LITTLE:
		case B_RGB16_LITTLE:
			max_pclk = si->ps.max_dac1_clock_16;
			break;
		case B_RGB24_LITTLE:
			max_pclk = si->ps.max_dac1_clock_24;
			break;
		case B_RGB32_LITTLE:
			max_pclk = si->ps.max_dac1_clock_32;
			break;
		default:
			/* use fail-safe value */
			max_pclk = si->ps.max_dac1_clock_32;
			break;
	}
	/* if some dualhead mode is active, an extra restriction might apply */
	if ((target.flags & DUALHEAD_BITS) && (target.space == B_RGB32_LITTLE))
		max_pclk = si->ps.max_dac1_clock_32dh;

	/* Make sure the requested pixelclock is within the PLL's operational limits */
	/* lower limit is min_pixel_vco divided by highest postscaler-factor */
	if (req_pclk < (si->ps.min_pixel_vco / 8.0))
	{
		LOG(4,("DAC: clamping pixclock: requested %fMHz, set to %fMHz\n",
										req_pclk, (float)(si->ps.min_pixel_vco / 8.0)));
		req_pclk = (si->ps.min_pixel_vco / 8.0);
	}
	/* upper limit is given by pins in combination with current active mode */
	if (req_pclk > max_pclk)
	{
		LOG(4,("DAC: clamping pixclock: requested %fMHz, set to %fMHz\n",
														req_pclk, (float)max_pclk));
		req_pclk = max_pclk;
	}

	/* iterate through all valid PLL postscaler settings */
	for (p = 0x01; p < 0x10; p = p << 1)
	{
		/* calculate the needed VCO frequency for this postscaler setting */
		f_vco = req_pclk * p;

		/* check if this is within range of the VCO specs */
		if ((f_vco >= si->ps.min_pixel_vco) && (f_vco <= si->ps.max_pixel_vco))
		{
			/* iterate trough all valid reference-frequency postscaler settings */
			for (m = 1; m <= 63; m++)
			{
				/* check if phase-discriminator will be within operational limits */
				/* (range as used by VESA BIOS on CLE266, verified.) */
				if (((si->ps.f_ref / m) < 2.0) || ((si->ps.f_ref / m) > 3.6)) continue;

				/* calculate VCO postscaler setting for current setup.. */
				n = (int)(((f_vco * m) / si->ps.f_ref) + 0.5);

				/* ..and check for validity */
				/* (checked VESA BIOS on CLE266, b7 is NOT part of divider.) */
				if ((n < 1) || (n > 127)) continue;

				/* find error in frequency this setting gives */
				error = fabs(req_pclk - (((si->ps.f_ref / m) * n) / p));

				/* note the setting if best yet */
				if (error < error_best)
				{
					error_best = error;
					best[0]=m;
					best[1]=n;
					best[2]=p;
				}
			}
		}
	}

	/* setup the scalers programming values for found optimum setting */
	m = best[0];
	n = best[1];
	p = best[2];

	/* log the VCO frequency found */
	f_vco = ((si->ps.f_ref / m) * n);

	LOG(2,("DAC: pix VCO frequency found %fMhz\n", f_vco));

	/* return the results */
	*calc_pclk = (f_vco / p);
	*m_result = m;
	*n_result = n;
	switch(p)
	{
	case 1:
		p = 0x00;
		break;
	case 2:
		p = 0x01;
		break;
	case 4:
		p = 0x02;
		break;
	case 8:
		p = 0x03;
		break;
	}
	*p_result = p;

	/* display the found pixelclock values */
	LOG(2,("DAC: pix PLL check: requested %fMHz got %fMHz, mnp 0x%02x 0x%02x 0x%02x\n",
		req_pclk, *calc_pclk, *m_result, *n_result, *p_result));

	return B_OK;
}

/* find nearest valid system PLL setting */
status_t eng_dac_sys_pll_find(
	float req_sclk, float* calc_sclk, uint8* m_result, uint8* n_result, uint8* p_result, uint8 test)
{
	int m = 0, n = 0, p = 0, m_max, p_max;
	float error, error_best = 999999999;
	int best[3];
	float f_vco, discr_low, discr_high;

	/* determine the max. reference-frequency postscaler setting for the 
	 * current requested clock */
	switch (si->ps.card_arch)
	{
	case NV04A:
		LOG(4,("DAC: NV04 restrictions apply\n"));
		/* set phase-discriminator frequency range (Mhz) (verified) */
		discr_low = 1.0;
		discr_high = 2.0;
		/* set max. useable reference frequency postscaler divider factor */
		m_max = 14;
		/* set max. useable VCO output postscaler divider factor */
		p_max = 16;
		break;
	default:
		switch (si->ps.card_type)
		{
		case NV28:
			//fixme: how about some other cards???
			LOG(4,("DAC: NV28 restrictions apply\n"));
			/* set max. useable reference frequency postscaler divider factor;
			 * apparantly we would get distortions on high PLL output frequencies if
			 * we use the phase-discriminator at low frequencies */
			if (req_sclk > 340.0) m_max = 2;			/* Fpll > 340Mhz */
			else if (req_sclk > 200.0) m_max = 4;		/* 200Mhz < Fpll <= 340Mhz */
				else if (req_sclk > 150.0) m_max = 6;	/* 150Mhz < Fpll <= 200Mhz */
					else m_max = 14;					/* Fpll < 150Mhz */

			/* set max. useable VCO output postscaler divider factor */
			p_max = 32;
			/* set phase-discriminator frequency range (Mhz) (verified) */
			discr_low = 1.0;
			discr_high = 27.0;
			break;
		default:
			LOG(4,("DAC: NV10/NV20/NV30 restrictions apply\n"));
			/* set max. useable reference frequency postscaler divider factor;
			 * apparantly we would get distortions on high PLL output frequencies if
			 * we use the phase-discriminator at low frequencies */
			if (req_sclk > 340.0) m_max = 2;		/* Fpll > 340Mhz */
			else if (req_sclk > 250.0) m_max = 6;	/* 250Mhz < Fpll <= 340Mhz */
				else m_max = 14;					/* Fpll < 250Mhz */

			/* set max. useable VCO output postscaler divider factor */
			p_max = 16;
			/* set phase-discriminator frequency range (Mhz) (verified) */
			if (si->ps.card_type == NV36) discr_low = 3.2;
			else discr_low = 1.0;
			/* (high discriminator spec is failsafe) */
			discr_high = 14.0;
			break;
		}
		break;
	}

	LOG(4,("DAC: PLL reference frequency postscaler divider range is 1 - %d\n", m_max));
	LOG(4,("DAC: PLL VCO output postscaler divider range is 1 - %d\n", p_max));
	LOG(4,("DAC: PLL discriminator input frequency range is %2.2fMhz - %2.2fMhz\n",
		discr_low, discr_high));

	/* Make sure the requested clock is within the PLL's operational limits */
	/* lower limit is min_system_vco divided by highest postscaler-factor */
	if (req_sclk < (si->ps.min_system_vco / ((float)p_max)))
	{
		LOG(4,("DAC: clamping sysclock: requested %fMHz, set to %fMHz\n",
			req_sclk, (si->ps.min_system_vco / ((float)p_max))));
		req_sclk = (si->ps.min_system_vco / ((float)p_max));
	}
	/* upper limit is given by pins */
	if (req_sclk > si->ps.max_system_vco)
	{
		LOG(4,("DAC: clamping sysclock: requested %fMHz, set to %fMHz\n",
			req_sclk, (float)si->ps.max_system_vco));
		req_sclk = si->ps.max_system_vco;
	}

	/* iterate through all valid PLL postscaler settings */
	for (p=0x01; p <= p_max; p = p<<1)
	{
		/* calculate the needed VCO frequency for this postscaler setting */
		f_vco = req_sclk * p;

		/* check if this is within range of the VCO specs */
		if ((f_vco >= si->ps.min_system_vco) && (f_vco <= si->ps.max_system_vco))
		{
			/* FX5600 and FX5700 tweak for 2nd set N and M scalers */
			if (si->ps.ext_pll) f_vco /= 4;

			/* iterate trough all valid reference-frequency postscaler settings */
			for (m = 1; m <= m_max; m++)
			{
				/* check if phase-discriminator will be within operational limits */
				if (((si->ps.f_ref / m) < discr_low) || ((si->ps.f_ref / m) > discr_high))
					continue;

				/* calculate VCO postscaler setting for current setup.. */
				n = (int)(((f_vco * m) / si->ps.f_ref) + 0.5);

				/* ..and check for validity */
				if ((n < 1) || (n > 255)) continue;

				/* find error in frequency this setting gives */
				if (si->ps.ext_pll)
				{
					/* FX5600 and FX5700 tweak for 2nd set N and M scalers */
					error = fabs((req_sclk / 4) - (((si->ps.f_ref / m) * n) / p));
				}
				else
					error = fabs(req_sclk - (((si->ps.f_ref / m) * n) / p));

				/* note the setting if best yet */
				if (error < error_best)
				{
					error_best = error;
					best[0]=m;
					best[1]=n;
					best[2]=p;
				}
			}
		}
	}

	/* setup the scalers programming values for found optimum setting */
	m = best[0];
	n = best[1];
	p = best[2];

	/* log the VCO frequency found */
	f_vco = ((si->ps.f_ref / m) * n);
	/* FX5600 and FX5700 tweak for 2nd set N and M scalers */
	if (si->ps.ext_pll) f_vco *= 4;

	LOG(2,("DAC: sys VCO frequency found %fMhz\n", f_vco));

	/* return the results */
	*calc_sclk = (f_vco / p);
	*m_result = m;
	*n_result = n;
	switch(p)
	{
	case 1:
		p = 0x00;
		break;
	case 2:
		p = 0x01;
		break;
	case 4:
		p = 0x02;
		break;
	case 8:
		p = 0x03;
		break;
	case 16:
		p = 0x04;
		break;
	case 32:
		p = 0x05;
		break;
	}
	*p_result = p;

	/* display the found pixelclock values */
	LOG(2,("DAC: sys PLL check: requested %fMHz got %fMHz, mnp 0x%02x 0x%02x 0x%02x\n",
		req_sclk, *calc_sclk, *m_result, *n_result, *p_result));

	return B_OK;
}
