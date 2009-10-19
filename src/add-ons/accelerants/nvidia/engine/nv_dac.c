/* program the DAC */
/* Author:
   Rudolf Cornelissen 12/2003-10/2009
*/

#define MODULE_BIT 0x00010000

#include "nv_std.h"

static void nv_dac_dump_pix_pll(void);
static status_t nv4_nv10_nv20_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test);

/* see if an analog VGA monitor is connected to connector #1 */
bool nv_dac_crt_connected(void)
{
	uint32 output, dac;
	bool present;

	/* save output connector setting */
	output = DACR(OUTPUT);
	/* save DAC state */
	dac = DACR(TSTCTRL);

	/* turn on DAC */
	DACW(TSTCTRL, (DACR(TSTCTRL) & 0xfffeffff));
	if (si->ps.secondary_head)
	{
		/* select primary CRTC (head) and turn off CRT (and DVI?) outputs */
		DACW(OUTPUT, (output & 0x0000feee));
	}
	else
	{
		/* turn off CRT (and DVI?) outputs */
		/* note:
		 * Don't touch the CRTC (head) assignment bit, as that would have undefined
		 * results. Confirmed NV15 cards getting into lasting RAM access trouble
		 * otherwise!! (goes for both system gfx RAM access and CRTC/DAC RAM access.) */
		DACW(OUTPUT, (output & 0x0000ffee));
	}
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
status_t nv_dac_mode(int mode,float brightness)
{
	uint8 *r,*g,*b;
	int i, ri;

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

	if (nv_dac_palette(r,g,b) != B_OK) return B_ERROR;

	/* disable palette RAM adressing mask */
	NV_REG8(NV8_PALMASK) = 0xff;
	LOG(2,("DAC: PAL pixrdmsk readback $%02x\n", NV_REG8(NV8_PALMASK)));

	return B_OK;
}

/* enable/disable dithering */
status_t nv_dac_dither(bool dither)
{
	/* older cards can't do dithering */
	if ((si->ps.card_type != NV11) && !si->ps.secondary_head) return B_ERROR;

	if (dither) {
		LOG(4,("DAC: enabling dithering\n"));

		if (si->ps.card_type == NV11) {
			/* NV11 apparantly has a fixed dithering pattern */

			/* enable dithering */
			DACW(NV11_DITHER, (DACR(NV11_DITHER) | 0x00010000));
		} else {
			/* setup dithering pattern */
			DACW(FP_DITH_PATT1, 0xe4e4e4e4);
			DACW(FP_DITH_PATT2, 0xe4e4e4e4);
			DACW(FP_DITH_PATT3, 0xe4e4e4e4);
			DACW(FP_DITH_PATT4, 0x44444444);
			DACW(FP_DITH_PATT5, 0x44444444);
			DACW(FP_DITH_PATT6, 0x44444444);

			/* enable dithering */
			DACW(FP_DITHER, (DACR(FP_DITHER) | 0x00000001));
		}
	} else {
		LOG(4,("DAC: disabling dithering\n"));

		if (si->ps.card_type == NV11) {
			/* disable dithering */
			DACW(NV11_DITHER, (DACR(NV11_DITHER) & ~0x00010000));
		} else {
			/* disable dithering */
			DACW(FP_DITHER, (DACR(FP_DITHER) & ~0x00000001));
		}
	}

	return B_OK;
}

/*program the DAC palette using the given r,g,b values*/
status_t nv_dac_palette(uint8 r[256],uint8 g[256],uint8 b[256])
{
	int i;

	LOG(4,("DAC: setting palette\n"));

	/* select first PAL adress before starting programming */
	NV_REG8(NV8_PALINDW) = 0x00;

	/* loop through all 256 to program DAC */
	for (i = 0; i < 256; i++)
	{
		/* the 6 implemented bits are on b0-b5 of the bus */
		NV_REG8(NV8_PALDATA) = r[i];
		NV_REG8(NV8_PALDATA) = g[i];
		NV_REG8(NV8_PALDATA) = b[i];
	}
	if (NV_REG8(NV8_PALINDW) != 0x00)
	{
		LOG(8,("DAC: PAL write index incorrect after programming\n"));
		return B_ERROR;
	}
if (1)
 {//reread LUT
	uint8 R, G, B;

	/* select first PAL adress to read (modulo 3 counter) */
	NV_REG8(NV8_PALINDR) = 0x00;
	for (i = 0; i < 256; i++)
	{
		R = NV_REG8(NV8_PALDATA);
		G = NV_REG8(NV8_PALDATA);
		B = NV_REG8(NV8_PALDATA);
		if ((r[i] != R) || (g[i] != G) || (b[i] != B)) 
			LOG(1,("DAC palette %d: w %x %x %x, r %x %x %x\n", i, r[i], g[i], b[i], R, G, B)); // apsed
	}
 }

	return B_OK;
}

/*program the pixpll - frequency in kHz*/
status_t nv_dac_set_pix_pll(display_mode target)
{
	uint8 m=0,n=0,p=0;

	float pix_setting, req_pclk;
	status_t result;

	/* fix a DVI or laptop flatpanel to 60Hz refresh! */
	/* Note:
	 * The pixelclock drives the flatpanel modeline, not the CRTC modeline. */
	if (si->ps.monitors & CRTC1_TMDS)
	{
		LOG(4,("DAC: Fixing DFP refresh to 60Hz!\n"));

		/* use the panel's modeline to determine the needed pixelclock */
		target.timing.pixel_clock = si->ps.p1_timing.pixel_clock;
	}

	req_pclk = (target.timing.pixel_clock)/1000.0;

	/* signal that we actually want to set the mode */
	result = nv_dac_pix_pll_find(target,&pix_setting,&m,&n,&p, 1);
	if (result != B_OK) return result;

	/* dump old setup for learning purposes */
	nv_dac_dump_pix_pll();

	/* some logging for learning purposes */
	LOG(4,("DAC: current NV30_PLLSETUP settings: $%08x\n", DACR(NV30_PLLSETUP)));
	/* this register seems to (dis)connect functions blocks and PLLs:
	 * there seem to be two PLL types per function block (on some cards),
	 * b16-17 DAC1clk, b18-19 DAC2clk, b20-21 GPUclk, b22-23 MEMclk. */
	LOG(4,("DAC: current (0x0000c040) settings: $%08x\n", NV_REG32(0x0000c040)));

	/* disable spread spectrum modes for the pixelPLLs _first_ */
	/* spread spectrum: b0,1 = GPUclk, b2,3 = MEMclk, b4,5 = DAC1clk, b6,7 = DAC2clk;
	 * b16-19 influence clock routing to digital outputs (internal/external LVDS transmitters?) */
	if (si->ps.card_arch >= NV30A)
		DACW(NV30_PLLSETUP, (DACR(NV30_PLLSETUP) & ~0x000000f0));

	/* we offer this option because some panels have very tight restrictions,
	 * and there's no overlapping settings range that makes them all work.
	 * note:
	 * this assumes the cards BIOS correctly programmed the panel (is likely) */
	//fixme: when VESA DDC EDID stuff is implemented, this option can be deleted...
	if ((si->ps.monitors & CRTC1_TMDS) && !si->settings.pgm_panel) {
		LOG(4,("DAC: Not programming DFP refresh (specified in nvidia.settings)\n"));
	} else {
		LOG(4,("DAC: Setting PIX PLL for pixelclock %f\n", req_pclk));

		/* program new frequency */
		DACW(PIXPLLC, ((p << 16) | (n << 8) | m));

		/* program 2nd set N and M scalers if they exist (b31=1 enables them) */
		if (si->ps.ext_pll) DACW(PIXPLLC2, 0x80000401);

		/* Give the PIXPLL frequency some time to lock... (there's no indication bit available) */
		snooze(1000);

		LOG(2,("DAC: PIX PLL frequency should be locked now...\n"));
	}

	/* enable programmable PLLs */
	/* (confirmed PLLSEL to be a write-only register on NV04 and NV11!) */
	/* note:
	 * setup PLL assignment _after_ programming PLL */
	if (si->ps.secondary_head) {
		if (si->ps.card_arch < NV40A) {
			DACW(PLLSEL, 0x30000f00);
		} else {
			DACW(NV40_PLLSEL2, (DACR(NV40_PLLSEL2) & ~0x10000100));
			DACW(PLLSEL, 0x30000f04);
		}
	} else {
		DACW(PLLSEL, 0x10000700);
	}

	return B_OK;
}

static void nv_dac_dump_pix_pll(void)
{
	uint32 dividers1, dividers2;
	uint8 m1, n1, p1;
	uint8 m2 = 1, n2 = 1;
	float f_vco, f_phase, f_pixel;

	LOG(2,("DAC: dumping current pixelPLL settings:\n"));

	dividers1 = DACR(PIXPLLC);
	m1 = (dividers1 & 0x000000ff);
	n1 = (dividers1 & 0x0000ff00) >> 8;
	p1 = 0x01 << ((dividers1 & 0x00070000) >> 16);
	LOG(2,("DAC: divider1 settings ($%08x): M1=%d, N1=%d, P1=%d\n", dividers1, m1, n1, p1));

	if (si->ps.ext_pll) {
		dividers2 = DACR(PIXPLLC2);
		if (dividers2 & 0x80000000) {
			/* the extended PLL part is enabled */
			m2 = (dividers2 & 0x000000ff);
			n2 = (dividers2 & 0x0000ff00) >> 8;
			LOG(2,("DAC: divider2 is enabled, settings ($%08x): M2=%d, N2=%d\n", dividers2, m2, n2));
		} else {
			LOG(2,("DAC: divider2 is disabled ($%08x)\n", dividers2));
		}
	}

	/* log the frequencies found */
	f_phase = si->ps.f_ref / (m1 * m2);
	f_vco = (f_phase * n1 * n2);
	f_pixel = f_vco / p1;

	LOG(2,("DAC: phase discriminator frequency is %fMhz\n", f_phase));
	LOG(2,("DAC: VCO frequency is %fMhz\n", f_vco));
	LOG(2,("DAC: pixelclock is %fMhz\n", f_pixel));
	LOG(2,("DAC: end of dump.\n"));

	/* apparantly if a VESA modecall during boot fails we need to explicitly select the PLL's
	 * again (was already done during driver init) if we readout the current PLL setting.. */
	if (si->ps.secondary_head)
		DACW(PLLSEL, 0x30000f00);
	else
		DACW(PLLSEL, 0x10000700);
}

/* find nearest valid pix pll */
status_t nv_dac_pix_pll_find
	(display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test)
{
	switch (si->ps.card_type) {
		default:   return nv4_nv10_nv20_dac_pix_pll_find(target, calc_pclk, m_result, n_result, p_result, test);
	}
	return B_ERROR;
}

/* find nearest valid pixel PLL setting */
static status_t nv4_nv10_nv20_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test)
{
	int m = 0, n = 0, p = 0/*, m_max*/;
	float error, error_best = 999999999;
	int best[3]; 
	float f_vco, max_pclk;
	float req_pclk = target.timing.pixel_clock/1000.0;

	LOG(4,("DAC: NV4/NV10/NV20 restrictions apply\n"));

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
	if (req_pclk < (si->ps.min_pixel_vco / 16.0))
	{
		LOG(4,("DAC: clamping pixclock: requested %fMHz, set to %fMHz\n",
										req_pclk, (float)(si->ps.min_pixel_vco / 16.0)));
		req_pclk = (si->ps.min_pixel_vco / 16.0);
	}
	/* upper limit is given by pins in combination with current active mode */
	if (req_pclk > max_pclk)
	{
		LOG(4,("DAC: clamping pixclock: requested %fMHz, set to %fMHz\n",
														req_pclk, (float)max_pclk));
		req_pclk = max_pclk;
	}

	/* iterate through all valid PLL postscaler settings */
	for (p=0x01; p < 0x20; p = p<<1)
	{
		/* calculate the needed VCO frequency for this postscaler setting */
		f_vco = req_pclk * p;

		/* check if this is within range of the VCO specs */
		if ((f_vco >= si->ps.min_pixel_vco) && (f_vco <= si->ps.max_pixel_vco))
		{
			/* FX5600 and FX5700 tweak for 2nd set N and M scalers */
			if (si->ps.ext_pll) f_vco /= 4;

			/* iterate trough all valid reference-frequency postscaler settings */
			for (m = 7; m <= 14; m++)
			{
				/* check if phase-discriminator will be within operational limits */
				//fixme: PLL calcs will be resetup/splitup/updated...
				if (si->ps.card_type == NV36)
				{
					if (((si->ps.f_ref / m) < 3.2) || ((si->ps.f_ref / m) > 6.4)) continue;
				}
				else
				{
					if (((si->ps.f_ref / m) < 1.0) || ((si->ps.f_ref / m) > 2.0)) continue;
				}

				/* calculate VCO postscaler setting for current setup.. */
				n = (int)(((f_vco * m) / si->ps.f_ref) + 0.5);

				/* ..and check for validity */
				if ((n < 1) || (n > 255))	continue;

				/* find error in frequency this setting gives */
				if (si->ps.ext_pll)
				{
					/* FX5600 and FX5700 tweak for 2nd set N and M scalers */
					error = fabs((req_pclk / 4) - (((si->ps.f_ref / m) * n) / p));
				}
				else
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
	/* FX5600 and FX5700 tweak for 2nd set N and M scalers */
	if (si->ps.ext_pll) f_vco *= 4;

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
	case 16:
		p = 0x04;
		break;
	}
	*p_result = p;

	/* display the found pixelclock values */
	LOG(2,("DAC: pix PLL check: requested %fMHz got %fMHz, mnp 0x%02x 0x%02x 0x%02x\n",
		req_pclk, *calc_pclk, *m_result, *n_result, *p_result));

	return B_OK;
}

/* find nearest valid system PLL setting */
status_t nv_dac_sys_pll_find(
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
