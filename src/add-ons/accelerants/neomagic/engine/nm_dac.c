/* program the DAC */
/* Author:
   Rudolf Cornelissen 4/2003-4/2004
*/

#define MODULE_BIT 0x00010000

#include "nm_std.h"

/*set the mode, brightness is a value from 0->2 (where 1 is equivalent to direct)*/
status_t nm_dac_mode(int mode,float brightness)
{
	uint8 *r, *g, *b, t[256];
	uint16 i;

	/*set colour arrays to point to space reserved in shared info*/
	r = si->color_data;
	g = r + 256;
	b = g + 256;

	LOG(4,("DAC: Setting screen mode %d brightness %f\n", mode, brightness));
	/*init a basic palette for brightness specified*/
	for (i = 0; i < 256; i++)
	{
		int ri = i * brightness;
		if (ri > 255) ri = 255;
		t[i] = ri;
	}

	/*modify the palette for the specified mode (&validate mode)*/
	/* Note: the Neomagic chips have a 6 bit wide Palette RAM! */
	switch(mode)
	{
	case BPP8:
		LOG(8,("DAC: 8bit mode is indexed by OS, aborting brightness setup.\n"));
		return B_OK;
		break;
	case BPP24:
		LOG(8,("DAC: 24bit mode is a direct mode, aborting brightness setup.\n"));
		return B_OK;
//fixme: unnessesary? direct mode (6bits PAL RAM is insufficient here!)
/*		for (i = 0; i < 256; i++)
		{
			b[i] = g[i] = r[i] = t[i];
		}
*/		break;
	case BPP16:
		for (i = 0; i < 32; i++)
		{
			/* blue and red have only the 5 most significant bits */
			b[i] = r[i] = t[i << 3];
		}
		for (i = 0; i < 64; i++)
		{
			/* the green component has 6 bits */
			g[i] = t[i << 2];
		}
		break;
	case BPP15:
		for (i = 0; i < 32; i++)
		{
			/* all color components have 5 bits */
			g[i] = r[i] = b[i] = t[i << 3];
		}
		break;
	default:
		LOG(8,("DAC: Invalid colordepth requested, aborting!\n"));
		return B_ERROR;
		break;
	}

	if (nm_dac_palette(r, g, b, i) != B_OK) return B_ERROR;

	/*set the mode - also sets VCLK dividor*/
//	DXIW(MULCTRL, mode);
//	LOG(2,("DAC: mulctrl 0x%02x\n", DXIR(MULCTRL)));

	/* disable palette RAM adressing mask */
	ISAWB(PALMASK, 0xff);
	LOG(2,("DAC: PAL pixrdmsk readback $%02x\n", ISARB(PALMASK)));

	return B_OK;
}

/* program the DAC palette using the given r,g,b values */
status_t nm_dac_palette(uint8 r[256],uint8 g[256],uint8 b[256], uint16 cnt)
{
	int i;

	LOG(4,("DAC: setting palette\n"));

	/* select first PAL adress before starting programming */
	ISAWB(PALINDW, 0x00);

	/* loop through all 256 to program DAC */
	for (i = 0; i < cnt; i++)
	{
		/* the 6 implemented bits are on b0-b5 of the bus */
		ISAWB(PALDATA, (r[i] >> 2));
		ISAWB(PALDATA, (g[i] >> 2));
		ISAWB(PALDATA, (b[i] >> 2));
	}
	if (ISARB(PALINDW) != (cnt & 0x00ff))
	{
		LOG(8,("DAC: PAL write index incorrect after programming\n"));
		return B_ERROR;
	}
if (1)
 {//reread LUT
	uint8 R, G, B;

	/* select first PAL adress to read (modulo 3 counter) */
	ISAWB(PALINDR, 0x00);
	for (i = 0; i < cnt; i++)
	{
		/* the 6 implemented bits are on b0-b5 of the bus */
		R = (ISARB(PALDATA) << 2);
		G = (ISARB(PALDATA) << 2);
		B = (ISARB(PALDATA) << 2);
		/* only compare the most significant 6 bits */
		if (((r[i] & 0xfc) != R) || ((g[i] & 0xfc) != G) || ((b[i] & 0xfc) != B)) 
			LOG(1,("DAC palette %d: w %x %x %x, r %x %x %x\n", i, r[i], g[i], b[i], R, G, B)); // apsed
	}
 }

	return B_OK;
}

/*program the pixpll - frequency in kHz*/
/* important note:
 * PIXPLLC is used - others should be kept as is
 */
status_t nm_dac_set_pix_pll(display_mode target)
{
	uint8 m=0,n=0,p=0;
	uint8 temp;
//	uint time = 0;

	float pix_setting, req_pclk;
	status_t result;

	req_pclk = (target.timing.pixel_clock)/1000.0;
	LOG(4,("DAC: Setting PIX PLL for pixelclock %f\n", req_pclk));

	result = nm_dac_pix_pll_find(target,&pix_setting,&m,&n,&p);
	if (result != B_OK)
	{
		return result;
	}
	
	/*reprogram (disable,select,wait for stability,enable)*/
//unknown on Neomagic?:
//	DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0x0F)|0x04);  /*disable the PIXPLL*/
//	DXIW(PIXCLKCTRL,(DXIR(PIXCLKCTRL)&0x0C)|0x01);  /*select the PIXPLL*/

	/* select PixelPLL registerset C */
	temp = (ISARB(MISCR) | 0x0c);
	/* we need to wait a bit or the card will mess-up it's register values.. */
	snooze(10);
	ISAWB(MISCW, temp);

	/*set VCO divider (lsb) */
	ISAGRPHW(PLLC_NL, n);
	/*set VCO divider (msb) if it exists */
	if (si->ps.card_type >= NM2200)
	{
		temp = (ISAGRPHR(PLLC_NH) & 0x0f);
		/* we need to wait a bit or the card will mess-up it's register values.. */
		snooze(10);
		ISAGRPHW(PLLC_NH, (temp | (p & 0xf0)));
	}
	/*set main reference frequency divider */
	ISAGRPHW(PLLC_M, m);

	/* Wait for the PIXPLL frequency to lock until timeout occurs */
//unknown on Neomagic?:
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
	LOG(4,("DAC: PLLSEL $%02x\n", ISARB(MISCR)));
	LOG(4,("DAC: PLLN $%02x\n", ISAGRPHR(PLLC_NL)));
	LOG(4,("DAC: PLLM $%02x\n", ISAGRPHR(PLLC_M)));

	LOG(2,("DAC: PIX PLL frequency should be locked now...\n"));

	return B_OK;
}

/* find nearest valid pix pll */
status_t nm_dac_pix_pll_find
	(display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result)
{
	int m = 0, n = 0, p = 0, n_max, m_max;
	float error, error_best = 999999999;
	int best[2]; 
	float f_vco, max_pclk;
	float req_pclk = target.timing.pixel_clock/1000.0;

	/* determine the max. scaler settings for the current card */
	switch (si->ps.card_type)
	{
	case NM2070:
		LOG(4,("DAC: NM2070 restrictions apply\n"));
		m_max = 32;
		n_max = 128;
		break;
	case NM2090:
	case NM2093:
	case NM2097:
	case NM2160:
		LOG(4,("DAC: NM2090/93/97/NM2160 restrictions apply\n"));
		m_max = 64;
		n_max = 128;
		break;
	default:
		LOG(4,("DAC: NM22xx/NM23xx restrictions apply\n"));
		m_max = 64;
		n_max = 2048;
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
		default:
			/* use fail-safe value */
			max_pclk = si->ps.max_dac1_clock_24;
			break;
	}

	/* Make sure the requested pixelclock is within the PLL's operational limits */
	/* lower limit is min_pixel_vco (PLL postscaler does not exist in NM cards) */
	if (req_pclk < si->ps.min_pixel_vco)
	{
		LOG(4,("DAC: clamping pixclock: requested %fMHz, set to %fMHz\n",
										req_pclk, (float)si->ps.min_pixel_vco));
		req_pclk = si->ps.min_pixel_vco;
	}
	/* upper limit is given by pins in combination with current active mode */
	if (req_pclk > max_pclk)
	{
		LOG(4,("DAC: clamping pixclock: requested %fMHz, set to %fMHz\n",
														req_pclk, (float)max_pclk));
		req_pclk = max_pclk;
	}

	/* calculate the needed VCO frequency for this postscaler setting
	 * (NM cards have no PLL postscaler) */
	f_vco = req_pclk;

	/* iterate trough all valid reference-frequency postscaler settings */
	for (m = 1; m <= m_max; m++)
	{
		/* only even reference postscaler settings are supported beyond half the range */
		if ((m > (m_max / 2)) && ((m / 2.0) != 0.0)) continue;
		/* calculate VCO postscaler setting for current setup.. */
		n = (int)(((f_vco * m) / si->ps.f_ref) + 0.5);
		/* ..and check for validity */
		if ((n < 1) || (n > n_max))	continue;

		/* find error in frequency this setting gives */
		error = fabs(req_pclk - ((si->ps.f_ref / m) * n));

		/* note the setting if best yet */
		if (error < error_best)
		{
			error_best = error;
			best[0] = m;
			best[1] = n;
		}
	}

	/* setup the scalers programming values for found optimum setting */
	n = best[1] - 1;
	/* the reference frequency postscaler are actually two postscalers:
	 * p can divide by 1 or 2, and m can divide by 1-32 (1-16 for NM2070). */
	if (best[0] <= (m_max / 2))
	{
		m = best[0] - 1;
		p = (0 << 7);
	}
	else
	{
		m = (best[0] / 2) - 1;
		p = (1 << 7);
	}

	/* display and return the results */
	*calc_pclk = (si->ps.f_ref / best[0]) * best[1];
	*m_result = m;
	if (si->ps.card_type < NM2200)
	{
		*n_result = ((n & 0x07f) | p);
		*p_result = 0;
		LOG(2,("DAC: pix PLL check: requested %fMHz got %fMHz, nm $%02x $%02x\n",
			req_pclk, *calc_pclk, *n_result, *m_result));
	}
	else
	{
		*n_result = (n & 0x0ff);
		*p_result = (((n & 0x700) >> 4) | p);
		LOG(2,("DAC: pix PLL check: requested %fMHz got %fMHz, pnm $%02x $%02x $%02x\n",
			req_pclk, *calc_pclk, *p_result, *n_result, *m_result));
	}

	return B_OK;
}
