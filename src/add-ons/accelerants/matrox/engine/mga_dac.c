/* program the DAC */
/* Authors:
   Mark Watson 2/2000,
   Apsed 2002,
   Rudolf Cornelissen 9/2002
*/

#define MODULE_BIT 0x00010000

#include "mga_std.h"

static status_t g100_g400max_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result);
static status_t g450_g550_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test);
static status_t g100_g400max_dac_sys_pll_find(
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

	LOG(4,("DAC:Setting screen mode %d brightness %f\n", mode, brightness));
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
		LOG(8,("DAC:Invalid bit depth requested\n"));
		return B_ERROR;
		break;
	}

	if (gx00_dac_palette(r,g,b)!=B_OK) return B_ERROR;

	/*set the mode - also sets VCLK dividor*/
	DXIW(MULCTRL,mode);
	DACW(PIXRDMSK,0xff); // apsed, palette addressing not masked

	LOG(2,("DAC: mulctrl=%x, pixrdmsk=%x\n",DXIR(MULCTRL), DACR(PIXRDMSK)));

	return B_OK;
}

/*program the DAC palette using the given r,g,b values*/
status_t gx00_dac_palette(uint8 r[256],uint8 g[256],uint8 b[256])
{
	int i;

	LOG(4,("DAC: setting palette\n"));

	/*clear palwtadd to start programming*/
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
if (0) {// apsed: reread LUT
	uint8 R, G, B;
	DACW(PALRDADD,0);
	for (i=0;i<256;i++)	{
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

status_t gx50_dac_check_pix_pll(uint8 m, uint8 n, uint8 p)
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

status_t gx50_dac_check_pix_pll_range(uint8 m, uint8 n, uint8 *p, uint8 *q)
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
		default:   return g100_g400max_dac_pix_pll_find(target, calc_pclk, m_result, n_result, p_result);
	}
	return B_ERROR;
}

/* find nearest valid pixel PLL setting: rewritten by rudolf */
static status_t g100_g400max_dac_pix_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result)
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
		req_pclk = (si->ps.min_pixel_vco / 8.0);
	/* upper limit is given by pins in combination with current active mode */
	if (req_pclk > max_pclk) req_pclk = max_pclk;

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
		req_pclk = (si->ps.min_pixel_vco / 16.0);
	/* upper limit is given by pins in combination with current active mode */
	if (req_pclk > max_pclk) req_pclk = max_pclk;

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
	n=best[1] - 1;
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
	f_vco = ((si->ps.f_ref * 2) / (m + 1)) * (n + 1);
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
		req_sclk = (si->ps.min_system_vco / 8.0);
	/* upper limit is max_system_vco */
	if (req_sclk > si->ps.max_system_vco) req_sclk = si->ps.max_system_vco;

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

/*set up system pll - NB mclk is memory clock, */ 
status_t g100_dac_set_sys_pll()
{
	/* values for DAC sys pll registers */
	uint8 m, n, p;
	uint time = 0;
	uint32 temp;
	float calc_sclk;

	LOG(1,("DAC: Setting up G100 system clock\n"));
	//zodra gclk, mclk en fmclk DIV ingesteld is via PINS en hieronder:
	g100_g400max_dac_sys_pll_find(si->ps.std_engine_clock, &calc_sclk, &m, &n, &p);

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

/*set up system pll - NB mclk is memory clock, */ 
status_t g200_dac_set_sys_pll()
{
	uint8 m, n, p;/*values for DAC sys pll registers*/
	uint time = 0;
	float calc_sclk;

	LOG(1,("DAC: Setting up G200 system clock\n"));
	//zodra gclk, mclk en fmclk DIV ingesteld is via PINS !EN OPTION2! en hieronder:
	//g100_g400max_dac_sys_pll_find(si->ps.std_engine_clock, &calc_sclk, &m, &n, &p);
	//voor nu:
	g100_g400max_dac_sys_pll_find(124.2, &calc_sclk, &m, &n, &p);

	/*reprogram the clock - set PCI/AGP, program, set to programmed*/
	//fixme: PINS..
	CFGW(OPTION2,0x8000);				/*no memory clock divider (pinced from win)*/

	CFGW(OPTION,CFGR(OPTION)|0x04);                 /*disable the SYSPLL*/
	CFGW(OPTION,CFGR(OPTION)&0xFFFFFFFC);           /*select the PCI/AGP clock*/
	CFGW(OPTION,CFGR(OPTION)&0xFFFFFFFB);           /*enable the SYSPLL*/

	DXIW(SYSPLLM,m);
	DXIW(SYSPLLN,n);
	DXIW(SYSPLLP,p);

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

	CFGW(OPTION,CFGR(OPTION)|0x04);                 /*disable the SYSPLL*/
	//fixme: PINS..
	CFGW(OPTION,(CFGR(OPTION)&0xFFFFFF27)|0x1);           /*select the SYSPLLs chosen*/
	CFGW(OPTION,(CFGR(OPTION)&0xFFFFFFFB)|0x20);           /*enable the SYSPLL*/

	return B_OK;
}

/*set up system pll - NB mclk is memory clock, */ 
status_t g400_dac_set_sys_pll()

{
	uint32 scalers; /*value for option 3*/
	uint8 m, n, p;/*values for DAC sys pll registers*/
	uint8 mclk_duty,oclk_duty;

	uint time = 0;
	float mclk,oclk;
	float temp_f;
	float calc_sclk;
	
	/* fixme? get from PINS */
	int mclk_div = 4;
	int oclk_div = 3;

	float div[8]={0.3333,0.4,0.4444,0.5,0.6666,0,0,0};

	LOG(1,("DAC: Setting up G400/G400MAX system clock\n"));
	g100_g400max_dac_sys_pll_find(si->ps.std_engine_clock, &calc_sclk, &m, &n, &p);

	/* calculate the real clock speeds derivated from SYSPLL */
	mclk = div[mclk_div] * calc_sclk;
	oclk = div[oclk_div] * calc_sclk;

	/*work out the duty cycle correction*/
	temp_f=1/(float)oclk;
	temp_f*=1000;
	LOG(2,("DAC:oclk correction ns: %f\n",temp_f));
	temp_f-=2.25;
	temp_f/=0.5;
	oclk_duty=(uint8)temp_f;

	temp_f=1/(float)mclk;
	temp_f*=1000;
	LOG(2,("DAC:mclk correction ns: %f\n",temp_f));
	temp_f-=2.25;
	temp_f/=0.5;
	mclk_duty=(uint8)temp_f;

	/*calculate OPTION3*/
	scalers=(0x1<<0)|(0x1<<10)|(0x1<<20);
	scalers|=(oclk_div<<3)|(mclk_div<<13)|(oclk_div<<23);
	scalers|=(oclk_duty<<6)|(mclk_duty<<16)|(oclk_duty<<26);

	/*print out the results*/
	LOG(2,("DAC: MCLK:%f\tOCLK:%f\n",mclk,oclk));
	LOG(2,("DAC: mclk_div:0x%x mclk_duty:0x%x\noclk_div:0x%x oclk_duty:0x%x\nOPTION3: 0x%x\n",
		mclk_div,mclk_duty,oclk_div,oclk_duty,scalers));

	/*reprogram the clock - set PCI/AGP, program, set to programmed*/
	CFGW(OPTION2,0);								/*clear so don't o/clock add ons*/
	CFGW(OPTION,CFGR(OPTION)|0x04);                 /*disable the SYSPLL*/
	CFGW(OPTION3,0);                                /*select the PCI/AGP clock*/
	CFGW(OPTION,CFGR(OPTION)&0xFFFFFFFB);           /*enable the SYSPLL*/

	DXIW(SYSPLLM,m);
	DXIW(SYSPLLN,n);
	DXIW(SYSPLLP,p);

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

	CFGW(OPTION,CFGR(OPTION)|0x04);                 /*disable the SYSPLL*/
	CFGW(OPTION3,scalers);                          /*select the SYSPLLs chosen*/
	CFGW(OPTION,CFGR(OPTION)&0xFFFFFFFB);           /*enable the SYSPLL*/

	return B_OK;
}

/*set up system pll - NB mclk is memory clock, */ 
//fixme: implement this routine for coldstart:
//status_t g450_dac_set_sys_pll(int m,int n,int mclk_div,int oclk_div)
