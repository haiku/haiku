/* program the DAC */
/* Author:
   Rudolf Cornelissen 12/2003-5/2004
*/

#define MODULE_BIT 0x00010000

#include "nv_std.h"

static status_t nv4_nv10_nv20_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test);
static status_t g100_g400max_dac_sys_pll_find(
	float req_sclk,float * calc_sclk,uint8 * m_result,uint8 * n_result,uint8 * p_result);

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
//	uint time = 0;

	float pix_setting, req_pclk;
	status_t result;

	/* fix a DVI or laptop flatpanel to 62Hz refresh!
	 * (we can't risk getting below 60.0Hz as some panels shut-off then!) */
	/* Note:
	 * The pixelclock drives the flatpanel modeline, not the CRTC modeline. */
	if (si->ps.tmds1_active)
	{
		LOG(4,("DAC: Fixing DFP refresh to 62Hz!\n"));

		/* readout the panel's modeline to determine the needed pixelclock */
		target.timing.pixel_clock = (
			((DACR(FP_HTOTAL) & 0x0000ffff) + 1) *
			((DACR(FP_VTOTAL) & 0x0000ffff) + 1) *
			62) / 1000;
	}

	req_pclk = (target.timing.pixel_clock)/1000.0;
	LOG(4,("DAC: Setting PIX PLL for pixelclock %f\n", req_pclk));

	/* signal that we actually want to set the mode */
	result = nv_dac_pix_pll_find(target,&pix_setting,&m,&n,&p, 1);
	if (result != B_OK)
	{
		return result;
	}

	/*reprogram (disable,select,wait for stability,enable)*/
//	DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0x0F)|0x04);  /*disable the PIXPLL*/
//	DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0x0C)|0x01);  /*select the PIXPLL*/

	/* program new frequency */
	DACW(PIXPLLC, ((p << 16) | (n << 8) | m));

	/* program 2nd set N and M scalers if they exist (b31=1 enables them) */
	if ((si->ps.card_type == NV31) || (si->ps.card_type == NV36))
		DACW(PIXPLLC2, 0x80000401);

	/* Wait for the PIXPLL frequency to lock until timeout occurs */
//fixme: do NV cards have a LOCK indication bit??
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
			if ((si->ps.card_type == NV31) || (si->ps.card_type == NV36)) f_vco /= 4;

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
				if ((si->ps.card_type == NV31) || (si->ps.card_type == NV36))
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
	if ((si->ps.card_type == NV31) || (si->ps.card_type == NV36)) f_vco *= 4;

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
static status_t g100_g400max_dac_sys_pll_find(
	float req_sclk,float * calc_sclk,uint8 * m_result,uint8 * n_result,uint8 * p_result)
{
	int m = 0, n = 0, p = 0, m_max;
	float error, error_best = 999999999;
	int best[3]; 
	float f_vco;

	/* determine the max. reference-frequency postscaler setting for the 
	 * current card (see G100, G200 and G400 specs). */
	switch(si->ps.card_type)
	{
/*	case G100:
		LOG(4,("DAC: G100 restrictions apply\n"));
		m_max = 7;
		break;
	case G200:
		LOG(4,("DAC: G200 restrictions apply\n"));
		m_max = 7;
		break;
*/	default:
		LOG(4,("DAC: G400/G400MAX restrictions apply\n"));
		m_max = 32;
		break;
	}

	/* Make sure the requested systemclock is within the PLL's operational limits */
	/* lower limit is min_system_vco divided by highest postscaler-factor */
	if (req_sclk < (si->ps.min_system_vco / 8.0))
	{
		LOG(4,("DAC: clamping sysclock: requested %fMHz, set to %fMHz\n",
										req_sclk, (float)(si->ps.min_system_vco / 8.0)));
		req_sclk = (si->ps.min_system_vco / 8.0);
	}
	/* upper limit is max_system_vco */
	if (req_sclk > si->ps.max_system_vco)
	{
		LOG(4,("DAC: clamping sysclock: requested %fMHz, set to %fMHz\n",
										req_sclk, (float)si->ps.max_system_vco));
		req_sclk = si->ps.max_system_vco;
	}

	/* iterate through all valid PLL postscaler settings */
	for (p=0x01; p < 0x10; p = p<<1)
	{
		/* calculate the needed VCO frequency for this postscaler setting */
		f_vco = req_sclk * p;

		/* check if this is within range of the VCO specs */
		if ((f_vco >= si->ps.min_system_vco) && (f_vco <= si->ps.max_system_vco))
		{
			/* iterate trough all valid reference-frequency postscaler settings */
			for (m = 2; m <= m_max; m++)
			{
				/* calculate VCO postscaler setting for current setup.. */
				n = (int)(((f_vco * m) / si->ps.f_ref) + 0.5);
				/* ..and check for validity */
				if ((n < 8) || (n > 128))	continue;

				/* find error in frequency this setting gives */
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
	m=best[0] - 1;
	n=best[1] - 1;
	p=best[2] - 1;

	/* calc the needed PLL loopbackfilter setting belonging to current VCO speed, 
	 * for the current card (see G100, G200 and G400 specs). */
	f_vco = (si->ps.f_ref / (m + 1)) * (n + 1);
	LOG(2,("DAC: sys VCO frequency found %fMhz\n", f_vco));

	switch(si->ps.card_type)
	{
	default:
		for(;;)
		{
			if (f_vco >= 240) {p |= (0x03 << 3); break;};
			if (f_vco >= 170) {p |= (0x02 << 3); break;};
			if (f_vco >= 110) {p |= (0x01 << 3); break;};
			break;
		}
		break;
	}

	/* return the results */
	*calc_sclk = f_vco / ((p & 0x07) + 1);
	*m_result = m;
	*n_result = n;
	*p_result = p;

	/* display the found pixelclock values */
	LOG(2,("DAC: sys PLL check: requested %fMHz got %fMHz, mnp 0x%02x 0x%02x 0x%02x\n",
		req_sclk, *calc_sclk, *m_result, *n_result, *p_result));

	return B_OK;
}

/*set up system pll - NB mclk is memory clock */ 
status_t g400_dac_set_sys_pll()
{
	/* values for DAC sys pll registers */
	uint8 m, n, p;
//	uint time = 0;
	float calc_sclk;

	LOG(1,("DAC: Setting up G400/G400MAX system clock\n"));
	g100_g400max_dac_sys_pll_find((float)si->ps.std_engine_clock, &calc_sclk, &m, &n, &p);

	/* reprogram the clock - set PCI/AGP, program, set to programmed */
	/* clear, so don't o/clock addons */
//	CFGW(OPTION2, 0);
	/* disable the SYSPLL */
//	CFGW(OPTION, CFGR(OPTION) | 0x04);
	/* select the PCI/AGP clock */
//	CFGW(OPTION3, 0);
	/* enable the SYSPLL */
//	CFGW(OPTION, CFGR(OPTION) & 0xfffffffb);

	/* program the new clock */
//	DXIW(SYSPLLM, m);
//	DXIW(SYSPLLN, n);
//	DXIW(SYSPLLP, p);

	/* Wait for the SYSPLL frequency to lock until timeout occurs */
/*	while((!(DXIR(SYSPLLSTAT)&0x40)) & (time <= 2000))
	{
		time++;
		snooze(1);
	}
	
	if (time > 2000)
		LOG(2,("DAC: sys PLL frequency not locked!\n"));
	else
		LOG(2,("DAC: sys PLL frequency locked\n"));
*/
	/* disable the SYSPLL */
//	CFGW(OPTION, CFGR(OPTION) | 0x04);
	/* setup Gclk, Mclk and Wclk divs via PINS and select SYSPLL as system clock source */
//	CFGW(OPTION3, si->ps.option3_reg);
	/* make sure the PLLs are not swapped (set default config) */
//	CFGW(OPTION, CFGR(OPTION) & 0xffffffbf);
	/* enable the SYSPLL (and make sure the SYSPLL is indeed powered up) */
//	CFGW(OPTION, (CFGR(OPTION) & 0xfffffffb) | 0x20);

	return B_OK;
}
