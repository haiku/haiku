/* program the DAC */
/* Authors:
   Mark Watson 2/2000,
   Apsed 2002,
   Rudolf Cornelissen 9/2002-4/2003
*/

#define MODULE_BIT 0x00010000

#include "mga_std.h"

static status_t milx_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result);
static status_t g100_g400max_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test);
static status_t g450_g550_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test);
static status_t g100_g400max_dac_sys_pll_find(
	float req_sclk,float * calc_sclk,uint8 * m_result,uint8 * n_result,uint8 * p_result);
static status_t g450_g550_dac_sys_pll_find(
	float req_sclk,float * calc_sclk,uint8 * m_result,uint8 * n_result,uint8 * p_result);

/*set the mode, brightness is a value from 0->2 (where 1 is equivalent to direct)*/
status_t gx00_dac_mode(int mode,float brightness)
{
	uint8 *r,*g,*b,t[64];
	int i;

	/*set colour arrays to point to space reserved in shared info*/
	r=si->color_data;
	g=r+256;
	b=g+256;

	LOG(4,("DAC: Setting screen mode %d brightness %f\n", mode, brightness));
	/*init a basic palette for brightness specified*/
	for (i=0;i<256;i++)
	{
		int ri = i*brightness; // apsed
		if (ri > 255) ri = 255;
		r[i]=ri;
	}

	/*modify the palette for the specified mode (&validate mode)*/
	switch(mode)
	{
	case BPP8:
	case BPP24:case BPP32:
		for (i=0;i<256;i++)
		{
			b[i]=g[i]=r[i];
		}
		break;
	case BPP16:
		for (i=0;i<64;i++)
		{
			t[i]=r[i<<2];
		}
		for (i=0;i<64;i++)
		{
			g[i]=t[i];
		}
		for (i=0;i<32;i++)
		{
			b[i]=r[i]=t[i<<1];
		}
		break;
	case BPP15:
		for (i=0;i<32;i++)
		{
			t[i]=r[i<<3];
		}
		for (i=0;i<32;i++)
		{
			g[i]=r[i]=b[i]=t[i];
		}
		break;
	case BPP32DIR:
		break;
	default:
		LOG(8,("DAC: Invalid bit depth requested\n"));
		return B_ERROR;
		break;
	}

	if (gx00_dac_palette(r,g,b)!=B_OK) return B_ERROR;

	/*set the mode - also sets VCLK dividor*/
	if (si->ps.card_type >= G100)
	{
		DXIW(MULCTRL, mode);
		LOG(2,("DAC: mulctrl 0x%02x\n", DXIR(MULCTRL)));
	}
	else
	{
		/* MIL1/2 differs here (TVP3026DAC) */
		uint8  miscctrl = 0, latchctrl = 0;
		uint8  tcolctrl = 0, mulctrl   = 0;

		/* set the mode */
		switch (mode)
		{ 
		/* presetting mulctrl for DAC pixelbus_width of 32 */
		case BPP8:
			miscctrl=0x00; latchctrl=0x06; tcolctrl=0x80; mulctrl=0x4b;
			break;
		case BPP15:
		    miscctrl=0x20; latchctrl=0x06; tcolctrl=0x04; mulctrl=0x53;
		    break;
		case BPP16:
		    miscctrl=0x20; latchctrl=0x06; tcolctrl=0x05; mulctrl=0x53;
		    break;
		case BPP24:
		    miscctrl=0x20; latchctrl=0x06; tcolctrl=0x1f; mulctrl=0x5b;
		    break;
		case BPP32:
		    miscctrl=0x20; latchctrl=0x07; tcolctrl=0x06; mulctrl=0x5b;
		    break;
		case BPP32DIR:
			miscctrl=0x20; latchctrl=0x07; tcolctrl=0x06; mulctrl=0x5b;
			break;
		}
		
		/* modify mulctrl if DAC pixelbus_width is 64 */
		//fixme? do 32bit DACbus MIL 1/2 cards exist? if so, setup via si->ps...
		if (true) mulctrl += 1;

		DXIW(MISCCTRL, (DXIR(MISCCTRL) & 0x1d) | miscctrl); 
		DXIW(TVP_LATCHCTRL, latchctrl); 
		DXIW(TVP_TCOLCTRL, tcolctrl); 
		DXIW(MULCTRL, mulctrl);

		LOG(2,("DAC: TVP miscctrl 0x%02x, TVP latchctrl 0x%02x\n",
			DXIR(MISCCTRL), DXIR(TVP_LATCHCTRL)));
		LOG(2,("DAC: TVP tcolctrl 0x%02x, TVP mulctrl 0x%02x\n",
			DXIR(TVP_TCOLCTRL), DXIR(MULCTRL)));
	}

	/* disable palette RAM adressing mask */
	DACW(PIXRDMSK,0xff);
	LOG(2,("DAC: pixrdmsk 0x%02x\n", DACR(PIXRDMSK)));

	return B_OK;
}

/*program the DAC palette using the given r,g,b values*/
status_t gx00_dac_palette(uint8 r[256],uint8 g[256],uint8 b[256])
{
	int i;

	LOG(4,("DAC: setting palette\n"));

	/* clear palwtadd before starting programming (LUT index) */
	DACW(PALWTADD,0);

	/*loop through all 256 to program DAC*/
	for (i=0;i<256;i++)
	{
		DACW(PALDATA,r[i]);
		DACW(PALDATA,g[i]);
		DACW(PALDATA,b[i]);
	}
	if (DACR(PALWTADD)!=0)
	{
		LOG(8,("DAC: PALWTADD is not 0 after programming\n"));
		return B_ERROR;
	}
if (0)
 {// apsed: reread LUT
	uint8 R, G, B;

	/* clear LUT color (modulo 3 counter) */
	DACW(PALRDADD,0);
	for (i=0;i<256;i++)
	{
		R = DACR(PALDATA);
		G = DACR(PALDATA);
		B = DACR(PALDATA);
		if ((r[i] != R) || (g[i] != G) || (b[i] != B)) 
			LOG(1,("DAC palette %d: w %x %x %x, r %x %x %x\n", i, r[i], g[i], b[i], R, G, B)); // apsed
	}
 }

	return B_OK;
}

/*program the pixpll - frequency in kHz*/
/*important notes:
 * MISC(clksel) = select A,B,C PIXPLL (25,28,none)
 * PIXPLLC is used - others should be kept as is
 * VCLK is quadword clock (max is PIXPLL/2) - set according to DEPTH
 * BESCLK,CRTC2 are not touched 
 */
status_t gx00_dac_set_pix_pll(display_mode target)
{
	uint8 m=0,n=0,p=0;
	uint time = 0;

	float pix_setting, req_pclk;
	status_t result;

	req_pclk = (target.timing.pixel_clock)/1000.0;
	LOG(4,("DAC: Setting PIX PLL for pixelclock %f\n", req_pclk));

	/* signal that we actually want to set the mode */
	result = gx00_dac_pix_pll_find(target,&pix_setting,&m,&n,&p, 1);
	if (result != B_OK)
	{
		return result;
	}
	
	/*reprogram (disable,select,wait for stability,enable)*/
	DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0x0F)|0x04);  /*disable the PIXPLL*/
	DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0x0C)|0x01);  /*select the PIXPLL*/
	VGAW(MISCW,((VGAR(MISCR)&0xF3)|0x8));           /*select PIXPLLC*/
	DXIW(PIXPLLCM,(m));								/*set m value*/
	DXIW(PIXPLLCN,(n));								/*set n value*/
	DXIW(PIXPLLCP,(p));                             /*set p value*/

	/* Wait for the PIXPLL frequency to lock until timeout occurs */
	while((!(DXIR(PIXPLLSTAT)&0x40)) & (time <= 2000))
	{
		time++;
		snooze(1);
	}
	
	if (time > 2000)
		LOG(2,("DAC: PIX PLL frequency not locked!\n"));
	else
		LOG(2,("DAC: PIX PLL frequency locked\n"));
	DXIW(PIXCLKCTRL,DXIR(PIXCLKCTRL)&0x0B);         /*enable the PIXPLL*/

	return B_OK;
}

static status_t gx50_dac_check_pix_pll(uint8 m, uint8 n, uint8 p)
{
	uint time = 0, count = 0;

	/*reprogram (disable,select,wait for stability,enable)*/
	DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0x0C)|0x01);  /*select the PIXPLL*/
	VGAW(MISCW,((VGAR(MISCR)&0xF3)|0x8));           /*select PIXPLLC*/
	DXIW(PIXPLLCM,(m));								/*set m value*/
	DXIW(PIXPLLCN,(n));								/*set n value*/
	DXIW(PIXPLLCP,(p));								/*set p value*/

	/* give the PLL 1mS at least to get a lock */
	time = 0;
	while((!(DXIR(PIXPLLSTAT)&0x40)) & (time <= 1000))
	{
		time++;
		snooze(1);
	}
	
	/* no lock aquired, not useable */
	if (time > 1000) return B_ERROR;

	/* check if lock holds for at least 90% of the time */
	for (time = 0, count = 0; time <= 1000; time++)
	{
		if(DXIR(PIXPLLSTAT)&0x40) count++;
		snooze(1);		
	}	
	/* we have a winner */
	if (count >= 900) return B_OK;

	/* nogo, the PLL does not stabilize */
	return B_ERROR;
}

static status_t gx50_dac_check_pix_pll_range(uint8 m, uint8 n, uint8 *p, uint8 *q)
{
	uint8 s=0, p_backup = *p;

	/* preset no candidate, non working setting */
	*q = 0;
	/* preset lowest range filter */
	*p &= 0x47;

	/* iterate through all possible filtersettings */
	DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0x0F)|0x04);  /*disable the PIXPLL*/

	for (s = 0; s < 8 ;s++)
	{
		if (gx50_dac_check_pix_pll(m, n, *p)== B_OK)
		{
			/* now check 3 closest lower and higher settings */
			if ((gx50_dac_check_pix_pll(m, n - 3, *p)== B_OK) &&
				(gx50_dac_check_pix_pll(m, n - 2, *p)== B_OK) &&
				(gx50_dac_check_pix_pll(m, n - 1, *p)== B_OK) &&
				(gx50_dac_check_pix_pll(m, n + 1, *p)== B_OK) &&
				(gx50_dac_check_pix_pll(m, n + 2, *p)== B_OK) &&
				(gx50_dac_check_pix_pll(m, n + 3, *p)== B_OK))
			{
				LOG(2,("DAC: found optimal working VCO filter: #%d\n",s));
				/* preset first choice setting found */
				*q = 1;
				/* we are done */
				DXIW(PIXCLKCTRL,DXIR(PIXCLKCTRL)&0x0B);     /*enable the PIXPLL*/
				return B_OK;
			}
			else
			{
				LOG(2,("DAC: found critical but working VCO filter: #%d\n",s));
				/* preset backup setting found */
				*q = 2;
				/* remember this setting */
				p_backup = *p;
				/* let's continue to see if a better filter exists */
			}
		}
	/* new filtersetting to try */
	*p += (1 << 3);
	}

	/* return the (last found) backup result, or the original p value */
	*p = p_backup;	
	DXIW(PIXCLKCTRL,DXIR(PIXCLKCTRL)&0x0B);     /*enable the PIXPLL*/
	/* we found only a non-optimal value */
	if (*q == 2) return B_OK;

	/* nothing worked at all */
	LOG(2,("DAC: no working VCO filter found!\n"));
	return B_ERROR;
}

/* find nearest valid pix pll */
status_t gx00_dac_pix_pll_find
	(display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test)
{
	switch (si->ps.card_type) {
		case G550:
		case G450: return g450_g550_dac_pix_pll_find(target, calc_pclk, m_result, n_result, p_result, test);
		case MIL2:
		case MIL1: return milx_dac_pix_pll_find(target, calc_pclk, m_result, n_result, p_result);
		default:   return g100_g400max_dac_pix_pll_find(target, calc_pclk, m_result, n_result, p_result, test);
	}
	return B_ERROR;
}

/* find nearest valid pixel PLL setting: rewritten by rudolf */
static status_t milx_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result)
{
	int m = 0, n = 0, p = 0;
	float error, error_best = 999999999;
	int best[3]; 
	float f_vco, max_pclk;
	float req_pclk = target.timing.pixel_clock/1000.0;

	LOG(4,("DAC: MIL1/MIL2 TVP restrictions apply\n"));

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

	/* Make sure the requested pixelclock is within the PLL's operational limits */
	/* lower limit is min_pixel_vco divided by highest postscaler-factor */
	if (req_pclk < (si->ps.min_pixel_vco / 8.0))
	{
		LOG(4,("DAC: TVP clamping pixclock: requested %fMHz, set to %fMHz\n",
										req_pclk, (float)(si->ps.min_pixel_vco / 8.0)));
		req_pclk = (si->ps.min_pixel_vco / 8.0);
	}
	/* upper limit is given by pins in combination with current active mode */
	if (req_pclk > max_pclk)
	{
		LOG(4,("DAC: TVP clamping pixclock: requested %fMHz, set to %fMHz\n",
														req_pclk, (float)max_pclk));
		req_pclk = max_pclk;
	}

	/* iterate through all valid PLL postscaler settings */
	for (p=0x01; p < 0x10; p = p<<1)
	{
		/* calculate the needed VCO frequency for this postscaler setting */
		f_vco = req_pclk * p;

		/* check if this is within range of the VCO specs */
		if ((f_vco >= si->ps.min_pixel_vco) && (f_vco <= si->ps.max_pixel_vco))
		{
			/* iterate trough all valid reference-frequency postscaler settings */
			for (n = 3; n <= 25; n++)
			{
				/* calculate VCO postscaler setting for current setup.. */
				m = (int)(((f_vco * n) / (8 * si->ps.f_ref)) + 0.5);
				/* ..and check for validity */
				if ((m < 3) || (m > 64))	continue;

				/* find error in frequency this setting gives */
				error = fabs(req_pclk - ((((8 * si->ps.f_ref) / n) * m) / p));

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

	m = best[0];
	n = best[1];
	p = best[2];

	f_vco = (((8 * si->ps.f_ref) / n) * m);
	LOG(2,("DAC: TVP pix VCO frequency found %fMhz\n", f_vco));

	/* setup the scalers programming values for found optimum setting */
	*calc_pclk = (f_vco / p);
	*m_result = (65 - m);
	*n_result = (65 - n);

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
	LOG(2,("DAC: TVP pix PLL check: requested %fMHz got %fMHz, mnp 0x%02x 0x%02x 0x%02x\n",
		req_pclk, *calc_pclk, *m_result, *n_result, *p_result));

	return B_OK;
}

/* find nearest valid pixel PLL setting: rewritten by rudolf */
static status_t g100_g400max_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test)
{
	int m = 0, n = 0, p = 0, m_max;
	float error, error_best = 999999999;
	int best[3]; 
	float f_vco, max_pclk;
	float req_pclk = target.timing.pixel_clock/1000.0;

	/* determine the max. reference-frequency postscaler setting for the 
	 * current card (see G100, G200 and G400 specs). */
	switch(si->ps.card_type)
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

	/* make sure the pixelPLL and the videoPLL have a little different settings to
	 * minimize distortions in the outputs due to crosstalk:
	 * do *not* change the videoPLL setting because it must be exact if TVout is enabled! */
	/* Note:
	 * only modify the clock if we are actually going to set the mode */
	if ((target.flags & DUALHEAD_BITS) && test)
	{
		LOG(4,("DAC: dualhead mode active: modified requested pixelclock +1.5%%\n"));
		req_pclk *= 1.015;
	}

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
	for (p=0x01; p < 0x10; p = p<<1)
	{
		/* calculate the needed VCO frequency for this postscaler setting */
		f_vco = req_pclk * p;

		/* check if this is within range of the VCO specs */
		if ((f_vco >= si->ps.min_pixel_vco) && (f_vco <= si->ps.max_pixel_vco))
		{
			/* iterate trough all valid reference-frequency postscaler settings */
			for (m = 2; m <= m_max; m++)
			{
				/* calculate VCO postscaler setting for current setup.. */
				n = (int)(((f_vco * m) / si->ps.f_ref) + 0.5);
				/* ..and check for validity */
				if ((n < 8) || (n > 128))	continue;

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
	m=best[0] - 1;
	n=best[1] - 1;
	p=best[2] - 1;

	/* calc the needed PLL loopbackfilter setting belonging to current VCO speed, 
	 * for the current card (see G100, G200 and G400 specs). */
	f_vco = (si->ps.f_ref / (m + 1)) * (n + 1);
	LOG(2,("DAC: pix VCO frequency found %fMhz\n", f_vco));

	switch(si->ps.card_type)
	{
	case G100:
	case G200:
		for(;;)
		{
			if (f_vco >= 180) {p |= (0x03 << 3); break;};
			if (f_vco >= 140) {p |= (0x02 << 3); break;};
			if (f_vco >= 100) {p |= (0x01 << 3); break;};
			break;
		}
		break;
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
	*calc_pclk = f_vco / ((p & 0x07) + 1);
	*m_result = m;
	*n_result = n;
	*p_result = p;

	/* display the found pixelclock values */
	LOG(2,("DAC: pix PLL check: requested %fMHz got %fMHz, mnp 0x%02x 0x%02x 0x%02x\n",
		req_pclk, *calc_pclk, *m_result, *n_result, *p_result));

	return B_OK;
}

/* find nearest valid pixel PLL setting: rewritten by rudolf */
static status_t g450_g550_dac_pix_pll_find
	(display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test)
{
	int m = 0, n = 0;
	uint8 p = 0, q = 0;
	float error, error_best = 999999999;
	int best[3]; 
	float f_vco, max_pclk;
	float req_pclk = target.timing.pixel_clock/1000.0;

	LOG(4,("DAC: G450/G550 restrictions apply\n"));

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
			/* iterate trough all valid reference-frequency postscaler settings */
			for (m = 2; m <= 32; m++)
			{
				/* calculate VCO postscaler setting for current setup.. */
				n = (int)(((f_vco * m) / (si->ps.f_ref * 2)) + 0.5);
				/* ..and check for validity, BUT:
				 * Keep in mind that we need to be able to test n-3 ... n+3! */
				if ((n < (8 + 3)) || (n > (128 - 3)))	continue;

				/* find error in frequency this setting gives */
				error = fabs(req_pclk - ((((si->ps.f_ref * 2)/ m) * n) / p));

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
	n=best[1] - 2;
	switch(best[2])
	{
	case 1:
		p = 0x40;
		break;
	case 2:
		p = 0x00;
		break;
	case 4:
		p = 0x01;
		break;
	case 8:
		p = 0x02;
		break;
	case 16:
		p = 0x03;
		break;
	}

	/* log the closest VCO speed found */
	f_vco = ((si->ps.f_ref * 2) / (m + 1)) * (n + 2);
	LOG(2,("DAC: pix VCO frequency found %fMhz\n", f_vco));

	/* now find the filtersetting that matches best with this frequency by testing.
	 * for now we assume this routine succeeds to get us a stable setting */
	if (test)
		gx50_dac_check_pix_pll_range(m, n, &p, &q);
	else 
		LOG(2,("DAC: Not testing G450/G550 VCO feedback filters\n"));

	/* return the results */
	*calc_pclk = f_vco / best[2];
	*m_result = m;
	*n_result = n;
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
	case G100:
	case G200:
		for(;;)
		{
			if (f_vco >= 180) {p |= (0x03 << 3); break;};
			if (f_vco >= 140) {p |= (0x02 << 3); break;};
			if (f_vco >= 100) {p |= (0x01 << 3); break;};
			break;
		}
		break;
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

static status_t gx50_dac_check_sys_pll(uint8 m, uint8 n, uint8 p)
{
	uint time = 0, count = 0;

	/* program the new clock */
	DXIW(SYSPLLM, m);
	DXIW(SYSPLLN, n);
	DXIW(SYSPLLP, p);

	/* Wait for the SYSPLL frequency to lock until timeout occurs */
	time = 0;
	while((!(DXIR(SYSPLLSTAT)&0x40)) & (time <= 1000))
	{
		time++;
		snooze(1);
	}
	
	/* no lock aquired, not useable */
	if (time > 1000) return B_ERROR;

	/* check if lock holds for at least 90% of the time */
	for (time = 0, count = 0; time <= 1000; time++)
	{
		if(DXIR(SYSPLLSTAT)&0x40) count++;
		snooze(1);		
	}	
	/* we have a winner */
	if (count >= 900) return B_OK;

	/* nogo, the PLL does not stabilize */
	return B_ERROR;
}

static status_t gx50_dac_check_sys_pll_range(uint8 m, uint8 n, uint8 *p, uint8 *q)
{
	uint8 s=0, p_backup = *p;

	/* preset no candidate, non working setting */
	*q = 0;
	/* preset lowest range filter */
	*p &= 0x47;

	/* iterate through all possible filtersettings */
	for (s = 0; s < 8 ;s++)
	{
		if (gx50_dac_check_sys_pll(m, n, *p)== B_OK)
		{
			/* now check 3 closest lower and higher settings */
			if ((gx50_dac_check_sys_pll(m, n - 3, *p)== B_OK) &&
				(gx50_dac_check_sys_pll(m, n - 2, *p)== B_OK) &&
				(gx50_dac_check_sys_pll(m, n - 1, *p)== B_OK) &&
				(gx50_dac_check_sys_pll(m, n + 1, *p)== B_OK) &&
				(gx50_dac_check_sys_pll(m, n + 2, *p)== B_OK) &&
				(gx50_dac_check_sys_pll(m, n + 3, *p)== B_OK))
			{
				LOG(2,("DAC: found optimal working VCO filter: #%d\n",s));
				/* preset first choice setting found */
				*q = 1;
				/* we are done */
				return B_OK;
			}
			else
			{
				LOG(2,("DAC: found critical but working VCO filter: #%d\n",s));
				/* preset backup setting found */
				*q = 2;
				/* remember this setting */
				p_backup = *p;
				/* let's continue to see if a better filter exists */
			}
		}
	/* new filtersetting to try */
	*p += (1 << 3);
	}

	/* return the (last found) backup result, or the original p value */
	*p = p_backup;	
	/* we found only a non-optimal value */
	if (*q == 2) return B_OK;

	/* nothing worked at all */
	LOG(2,("DAC: no working VCO filter found!\n"));
	return B_ERROR;
}

/* find nearest valid system PLL setting */
static status_t g450_g550_dac_sys_pll_find(
	float req_sclk,float * calc_sclk,uint8 * m_result,uint8 * n_result,uint8 * p_result)
{
	int m = 0, n = 0;
	uint8 p = 0, q = 0;
	float error, error_best = 999999999;
	int best[3]; 
	float f_vco;

	LOG(4,("DAC: G450/G550 restrictions apply\n"));

	/* Make sure the requested pixelclock is within the PLL's operational limits */
	/* lower limit is min_system_vco divided by highest postscaler-factor */
	if (req_sclk < (si->ps.min_system_vco / 16.0))
	{
		LOG(4,("DAC: clamping sysclock: requested %fMHz, set to %fMHz\n",
										req_sclk, (float)(si->ps.min_system_vco / 16.0)));
		req_sclk = (si->ps.min_system_vco / 16.0);
	}
	/* upper limit is max_system_vco */
	if (req_sclk > si->ps.max_system_vco)
	{
		LOG(4,("DAC: clamping sysclock: requested %fMHz, set to %fMHz\n",
										req_sclk, (float)si->ps.max_system_vco));
		req_sclk = si->ps.max_system_vco;
	}

	/* iterate through all valid PLL postscaler settings */
	for (p=0x01; p < 0x20; p = p<<1)
	{
		/* calculate the needed VCO frequency for this postscaler setting */
		f_vco = req_sclk * p;

		/* check if this is within range of the VCO specs */
		if ((f_vco >= si->ps.min_system_vco) && (f_vco <= si->ps.max_system_vco))
		{
			/* iterate trough all valid reference-frequency postscaler settings */
			for (m = 2; m <= 32; m++)
			{
				/* calculate VCO postscaler setting for current setup.. */
				n = (int)(((f_vco * m) / (si->ps.f_ref * 2)) + 0.5);
				/* ..and check for validity, BUT:
				 * Keep in mind that we need to be able to test n-3 ... n+3! */
				if ((n < (8 + 3)) || (n > (128 - 3)))	continue;

				/* find error in frequency this setting gives */
				error = fabs(req_sclk - ((((si->ps.f_ref * 2)/ m) * n) / p));

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
	n=best[1] - 2;
	switch(best[2])
	{
	case 1:
		p = 0x40;
		break;
	case 2:
		p = 0x00;
		break;
	case 4:
		p = 0x01;
		break;
	case 8:
		p = 0x02;
		break;
	case 16:
		p = 0x03;
		break;
	}

	/* log the closest VCO speed found */
	f_vco = ((si->ps.f_ref * 2) / (m + 1)) * (n + 2);
	LOG(2,("DAC: sys VCO frequency found %fMhz\n", f_vco));

	/* now find the filtersetting that matches best with this frequency by testing.
	 * for now we assume this routine succeeds to get us a stable setting */
	gx50_dac_check_sys_pll_range(m, n, &p, &q);

	/* return the results */
	*calc_sclk = f_vco / best[2];
	*m_result = m;
	*n_result = n;
	*p_result = p;

	/* display the found pixelclock values */
	LOG(2,("DAC: sys PLL check: requested %fMHz got %fMHz, mnp 0x%02x 0x%02x 0x%02x\n",
		req_sclk, *calc_sclk, *m_result, *n_result, *p_result));

	return B_OK;
}

/*set up system pll - NB mclk is memory clock */ 
status_t g100_dac_set_sys_pll()
{
	/* values for DAC sys pll registers */
	uint8 m, n, p;
	uint time = 0;
	uint32 temp;
	float calc_sclk;

	LOG(1,("DAC: Setting up G100 system clock\n"));
	g100_g400max_dac_sys_pll_find((float)si->ps.std_engine_clock, &calc_sclk, &m, &n, &p);

	/* reprogram the clock - set PCI/AGP, program, set to programmed */
	/* disable the SYSPLL */
	CFGW(OPTION, CFGR(OPTION) | 0x04);
	/* select the PCI/AGP clock */
	CFGW(OPTION, CFGR(OPTION) & 0xfffffffc);
	/* enable the SYSPLL */
	CFGW(OPTION, CFGR(OPTION) & 0xfffffffb);

	/* program the new clock */
	DXIW(SYSPLLM, m);
	DXIW(SYSPLLN, n);
	DXIW(SYSPLLP, p);

	/* Wait for the SYSPLL frequency to lock until timeout occurs */
	while((!(DXIR(SYSPLLSTAT)&0x40)) & (time <= 2000))
	{
		time++;
		snooze(1);
	}
	
	if (time > 2000)
		LOG(2,("DAC: sys PLL frequency not locked!\n"));
	else
		LOG(2,("DAC: sys PLL frequency locked\n"));

	/* disable the SYSPLL */
	CFGW(OPTION, CFGR(OPTION) | 0x04);
	/* setup Gclk, Mclk and FMclk divisors according to PINS */
	temp = (CFGR(OPTION) & 0xffffff27);
	if (si->ps.v3_clk_div & 0x01) temp |= 0x08;
	if (si->ps.v3_clk_div & 0x02) temp |= 0x10;
	if (si->ps.v3_clk_div & 0x04) temp |= 0x80;
	/* fixme: swapPLL can only be done when the rest of the driver respects this also! */
	//never used AFAIK:
	//if (si->ps.v3_clk_div & 0x08) temp |= 0x40;
	/* select the SYSPLL as system clock source */
	temp |= 0x01;
	CFGW(OPTION, temp);
	/* enable the SYSPLL (and make sure the SYSPLL is indeed powered up) */
	CFGW(OPTION, (CFGR(OPTION) & 0xfffffffb) | 0x20);

	return B_OK;
}

/*set up system pll - NB mclk is memory clock */ 
status_t g200_dac_set_sys_pll()
{
	/* values for DAC sys pll registers */
	uint8 m, n, p;
	uint time = 0;
	uint32 temp;
	float calc_sclk;

	LOG(1,("DAC: Setting up G200 system clock\n"));
	g100_g400max_dac_sys_pll_find((float)si->ps.std_engine_clock, &calc_sclk, &m, &n, &p);

	/* reprogram the clock - set PCI/AGP, program, set to programmed */
	/* disable the SYSPLL */
	CFGW(OPTION, CFGR(OPTION) | 0x04);
	/* select the PCI/AGP clock */
	CFGW(OPTION, CFGR(OPTION) & 0xfffffffc);
	/* enable the SYSPLL */
	CFGW(OPTION, CFGR(OPTION) & 0xfffffffb);

	/* program the new clock */
	DXIW(SYSPLLM, m);
	DXIW(SYSPLLN, n);
	DXIW(SYSPLLP, p);

	/* Wait for the SYSPLL frequency to lock until timeout occurs */
	while((!(DXIR(SYSPLLSTAT)&0x40)) & (time <= 2000))
	{
		time++;
		snooze(1);
	}
	
	if (time > 2000)
		LOG(2,("DAC: sys PLL frequency not locked!\n"));
	else
		LOG(2,("DAC: sys PLL frequency locked\n"));

	/* disable the SYSPLL */
	CFGW(OPTION, CFGR(OPTION) | 0x04);
	/* setup Wclk divisor and enable/disable Wclk, Gclk and Mclk divisors 
	 * according to PINS */
	temp = (CFGR(OPTION2) & 0x00383000);
	if (si->ps.v3_option2_reg & 0x04) temp |= 0x00004000;
	if (si->ps.v3_option2_reg & 0x08) temp |= 0x00008000;
	if (si->ps.v3_option2_reg & 0x10) temp |= 0x00010000;
	if (si->ps.v3_option2_reg & 0x20) temp |= 0x00020000;
	CFGW(OPTION2, temp);
	/* setup Gclk and Mclk divisors according to PINS */
	temp = (CFGR(OPTION) & 0xffffff27);
	if (si->ps.v3_clk_div & 0x01) temp |= 0x08;
	if (si->ps.v3_clk_div & 0x02) temp |= 0x10;
	/* fixme: swapPLL can only be done when the rest of the driver respects this also! */
	//never used AFAIK:
	//if (si->ps.v3_clk_div & 0x08) temp |= 0x40;
	/* select the SYSPLL as system clock source */
	temp |= 0x01;
	CFGW(OPTION, temp);
	/* enable the SYSPLL (and make sure the SYSPLL is indeed powered up) */
	CFGW(OPTION, (CFGR(OPTION) & 0xfffffffb) | 0x20);

	return B_OK;
}

/*set up system pll - NB mclk is memory clock */ 
status_t g400_dac_set_sys_pll()
{
	/* values for DAC sys pll registers */
	uint8 m, n, p;
	uint time = 0;
	float calc_sclk;

	LOG(1,("DAC: Setting up G400/G400MAX system clock\n"));
	g100_g400max_dac_sys_pll_find((float)si->ps.std_engine_clock, &calc_sclk, &m, &n, &p);

	/* reprogram the clock - set PCI/AGP, program, set to programmed */
	/* clear, so don't o/clock addons */
	CFGW(OPTION2, 0);
	/* disable the SYSPLL */
	CFGW(OPTION, CFGR(OPTION) | 0x04);
	/* select the PCI/AGP clock */
	CFGW(OPTION3, 0);
	/* enable the SYSPLL */
	CFGW(OPTION, CFGR(OPTION) & 0xfffffffb);

	/* program the new clock */
	DXIW(SYSPLLM, m);
	DXIW(SYSPLLN, n);
	DXIW(SYSPLLP, p);

	/* Wait for the SYSPLL frequency to lock until timeout occurs */
	while((!(DXIR(SYSPLLSTAT)&0x40)) & (time <= 2000))
	{
		time++;
		snooze(1);
	}
	
	if (time > 2000)
		LOG(2,("DAC: sys PLL frequency not locked!\n"));
	else
		LOG(2,("DAC: sys PLL frequency locked\n"));

	/* disable the SYSPLL */
	CFGW(OPTION, CFGR(OPTION) | 0x04);
	/* setup Gclk, Mclk and Wclk divs via PINS and select SYSPLL as system clock source */
	CFGW(OPTION3, si->ps.option3_reg);
	/* make sure the PLLs are not swapped (set default config) */
	CFGW(OPTION, CFGR(OPTION) & 0xffffffbf);
	/* enable the SYSPLL (and make sure the SYSPLL is indeed powered up) */
	CFGW(OPTION, (CFGR(OPTION) & 0xfffffffb) | 0x20);

	return B_OK;
}

/*set up system pll - NB mclk is memory clock */ 
status_t g450_dac_set_sys_pll()
{
	/* values for DAC sys pll registers */
	uint8 m, n, p;
	uint time = 0;
	float calc_sclk;

	LOG(1,("DAC: Setting up G450/G550 system clock\n"));
	/* reprogram the clock - set PCI/AGP, program, set to programmed */
	/* clear, so don't o/clock addons */
	CFGW(OPTION2, 0);
	/* setup OPTION via pins */
	CFGW(OPTION, si->ps.option_reg);
	/* disable the SYSPLL */
	CFGW(OPTION, CFGR(OPTION) | 0x04);
	/* select the PCI/AGP clock */
	CFGW(OPTION3, 0);
	/* enable the SYSPLL */
	CFGW(OPTION, CFGR(OPTION) & 0xfffffffb);

	/* this routine also tests the filters, so it actually programs the clock already */
	g450_g550_dac_sys_pll_find((float)si->ps.std_engine_clock, &calc_sclk, &m, &n, &p);

	/* program the new clock */
	DXIW(SYSPLLM, m);
	DXIW(SYSPLLN, n);
	DXIW(SYSPLLP, p);

	/* Wait for the SYSPLL frequency to lock until timeout occurs */
	while((!(DXIR(SYSPLLSTAT)&0x40)) & (time <= 2000))
	{
		time++;
		snooze(1);
	}
	
	if (time > 2000)
		LOG(2,("DAC: sys PLL frequency not locked!\n"));
	else
		LOG(2,("DAC: sys PLL frequency locked\n"));

	/* disable the SYSPLL */
	CFGW(OPTION, CFGR(OPTION) | 0x04);
	/* setup Gclk, Mclk and Wclk divs via PINS and select SYSPLL as system clock source */
	CFGW(OPTION3, si->ps.option3_reg);
	/* setup option2 via pins */
	CFGW(OPTION2, si->ps.option2_reg);
	/* make sure the PLLs are not swapped (set default config) */
	/* fixme: swapPLL can only be done when the rest of the driver respects this also!
	 * (never used AFAIK) */
	CFGW(OPTION, CFGR(OPTION) & 0xffffffbf);
	/* enable the SYSPLL (and make sure the SYSPLL is indeed powered up) */
	CFGW(OPTION, (CFGR(OPTION) & 0xfffffffb) | 0x20);

	return B_OK;
}
