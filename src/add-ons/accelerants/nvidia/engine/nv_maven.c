/* program the MAVEN in monitor mode */

/* Authors:
   Mark Watson 6/2000,
   Rudolf Cornelissen 1/2003-4/2003

   Thanx to Petr Vandrovec for writing matroxfb.
*/

#define MODULE_BIT 0x00001000

#include "nv_std.h"

status_t g450_g550_maven_set_vid_pll(display_mode target);
status_t g100_g400max_maven_set_vid_pll(display_mode target);

status_t nv_maven_dpms(uint8 display,uint8 h,uint8 v)
{
	/* this function is nolonger needed on G450/G550 cards */
	if (si->ps.card_type > G550) return B_OK;

	if (display & h & v)
	{
		/* turn on screen */
		if (!(si->dm.flags & TV_BITS))
		{
			/* monitor mode */
			MAVW(MONEN, 0xb2);
			MAVW(MONSET, 0x20);		/* must be set to this in monitor mode */
			MAVW(OUTMODE, 0x03);	/* output: monitor mode */
			MAVW(STABLE, 0x22);		/* makes picture stable? */
			MAVW(TEST, 0x00);		/* turn off test signal */
		}
		else
		{
			/* TVout mode */
			MAVW(MONEN, 0xb3);
			MAVW(MONSET, 0x20);
			MAVW(OUTMODE, 0x08);	/* output: SVideo/Composite */
			MAVW(STABLE, 0x02);		/* makes picture stable? */
			//fixme? linux uses 0x14...
			MAVW(TEST, (MAVR(TEST) & 0x10));
		}
	}
	else
	{
		/* turn off screen using a few methods! */
		MAVW(STABLE, 0x6a);
//		MAVW(TEST, 0x03);
		MAVW(OUTMODE, 0x00);
	}

	return B_OK;
}

/*set a mode line - inputs are in pixels/scanlines*/
status_t nv_maven_set_timing(display_mode target)
{
	/* this function is nolonger needed on G450/G550 cards */
	if (si->ps.card_type > G550) return B_OK;

	LOG(4,("MAVEN: setting timing\n"));

	/*check horizontal timing parameters are to nearest 8 pixels*/
	if ((target.timing.h_display & 0x07)	|
		(target.timing.h_sync_start & 0x07)	|
		(target.timing.h_sync_end & 0x07)	|
		(target.timing.h_total & 0x07))
	{
		LOG(8,("MAVEN: Horizontal timing is not multiples of 8 pixels\n"));
		return B_ERROR;
	}

	/*program the MAVEN*/
	MAVWW(LASTLINEL, target.timing.h_total); 
	MAVWW(HSYNCLENL, (target.timing.h_sync_end - target.timing.h_sync_start));
	MAVWW(HSYNCSTRL, (target.timing.h_total - target.timing.h_sync_start));
	MAVWW(HDISPLAYL, ((target.timing.h_total - target.timing.h_sync_start) +
					   target.timing.h_display));
	MAVWW(HTOTALL, (target.timing.h_total + 1));

	MAVWW(VSYNCLENL, (target.timing.v_sync_end - target.timing.v_sync_start - 1));
	MAVWW(VSYNCSTRL, (target.timing.v_total - target.timing.v_sync_start));
	MAVWW(VDISPLAYL, (target.timing.v_total - 1));
	MAVWW(VTOTALL, (target.timing.v_total - 1));

	MAVWW(HVIDRSTL, (target.timing.h_total - si->crtc_delay));
	MAVWW(VVIDRSTL, (target.timing.v_total - 2));

	return B_OK;
}

/*set the mode, brightness is a value from 0->2 (where 1 is equivalent to direct)*/
status_t nv_maven_mode(int mode,float brightness)
{
	uint8 luma;

	/* this function is nolonger needed on G450/G550 cards */
	if (si->ps.card_type > G550) return B_OK;

	/*set luma to a suitable value for brightness*/
	/*assuming 1A is a sensible value*/
	luma = (uint8)(0x1a * brightness);
	MAVW(LUMA,luma);
	LOG(4,("MAVEN: LUMA setting - %x\n",luma));

	return B_OK;
}

status_t nv_maven_set_vid_pll(display_mode target)
{
	switch (si->ps.card_type)
	{
		default:
			return g100_g400max_maven_set_vid_pll(target);
			break;
	}
	return B_ERROR;
}
	
/* program the video PLL in the MAVEN */
status_t g100_g400max_maven_set_vid_pll(display_mode target)
{
	uint8 m=0,n=0,p=0;

	float pix_setting, req_pclk;
	status_t result;

	req_pclk = (target.timing.pixel_clock)/1000.0;
	LOG(4,("MAVEN: Setting VID PLL for pixelclock %f\n", req_pclk));

	result = g100_g400max_maven_vid_pll_find(target,&pix_setting,&m,&n,&p);
	if (result != B_OK)
	{
		return result;
	}

	/*reprogram (select,wait for stability)*/
	MAVW(PIXPLLM,(m));								/* set m value */
	MAVW(PIXPLLN,(n));								/* set n value */
	MAVW(PIXPLLP,(p | 0x80));						/* set p value enabling PLL */

	/* Wait for the VIDPLL frequency to lock: detection is not possible it seems */
	snooze(2000);

	LOG(2,("MAVEN: VID PLL frequency should be locked now...\n"));

	return B_OK;
}

/* find nearest valid video PLL setting */
status_t g100_g400max_maven_vid_pll_find(
	display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result)
{
	int m = 0, n = 0, p = 0, m_max;
	float error, error_best = 999999999;
	int best[3]; 
	float f_vco, max_pclk;
	float req_pclk = target.timing.pixel_clock/1000.0;

	/* determine the max. reference-frequency postscaler setting for the current card */
	//fixme: check G100 and G200 m_max if possible...
	switch(si->ps.card_type)
	{
	default:
		LOG(4,("MAVEN: G400/G400MAX restrictions apply\n"));
		m_max = 32;
		break;
	}

	/* determine the max. pixelclock for the current videomode */
	switch (target.space)
	{
		case B_RGB16_LITTLE:
			max_pclk = si->ps.max_dac2_clock_16;
			break;
		case B_RGB32_LITTLE:
			max_pclk = si->ps.max_dac2_clock_32;
			break;
		default:
			/* use fail-safe value */
			max_pclk = si->ps.max_dac2_clock_32;
			break;
	}
	/* if some dualhead mode is active, an extra restriction might apply */
	if ((target.flags & DUALHEAD_BITS) && (target.space == B_RGB32_LITTLE))
		max_pclk = si->ps.max_dac2_clock_32dh;

	/* Make sure the requested pixelclock is within the PLL's operational limits */
	/* lower limit is min_video_vco divided by highest postscaler-factor */
	if (req_pclk < (si->ps.min_video_vco / 8.0))
	{
		LOG(4,("MAVEN: clamping vidclock: requested %fMHz, set to %fMHz\n",
										req_pclk, (float)(si->ps.min_video_vco / 8.0)));
		req_pclk = (si->ps.min_video_vco / 8.0);
	}
	/* upper limit is given by pins in combination with current active mode */
	if (req_pclk > max_pclk)
	{
		LOG(4,("MAVEN: clamping vidclock: requested %fMHz, set to %fMHz\n",
														req_pclk, (float)max_pclk));
		req_pclk = max_pclk;
	}

	/* iterate through all valid PLL postscaler settings */
	for (p=0x01; p < 0x10; p = p<<1)
	{
		/* calculate the needed VCO frequency for this postscaler setting */
		f_vco = req_pclk * p;

		/* check if this is within range of the VCO specs */
		if ((f_vco >= si->ps.min_video_vco) && (f_vco <= si->ps.max_video_vco))
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

	/* calc the needed PLL loopbackfilter setting belonging to current VCO speed */
	f_vco = (si->ps.f_ref / (m + 1)) * (n + 1);
	LOG(2,("MAVEN: vid VCO frequency found %fMhz\n", f_vco));

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
	*calc_pclk = f_vco / ((p & 0x07) + 1);
	*m_result = m;
	*n_result = n;
	*p_result = p;

	/* display the found pixelclock values */
	LOG(2,("MAVEN: vid PLL check: req. %fMHz got %fMHz, mnp 0x%02x 0x%02x 0x%02x\n",
		req_pclk, *calc_pclk, *m_result, *n_result, *p_result));

	return B_OK;
}
