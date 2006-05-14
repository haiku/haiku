/* program the MAVEN in monitor mode */

/* Authors:
   Mark Watson 6/2000,
   Rudolf Cornelissen 1/2003-5/2006

   Thanx to Petr Vandrovec for writing matroxfb.
*/

#define MODULE_BIT 0x00001000

#include "mga_std.h"

status_t g450_g550_maven_set_vid_pll(display_mode target);
status_t g100_g400max_maven_set_vid_pll(display_mode target);

/* This routine blanks the first 'line' below the screen used if there's memory left
 * to place it in. This will prevent overscanning rubbish on the MAVEN DAC, but only
 * if no screen using virtual height is setup.
 * In the rare event the mode and overlay use up so much RAM there's not enough room
 * left for one blank line, you will get overscanning rubbish from the overlay as it
 * will overwrite the blank line placed here.
 * This 'rectangle fill' is done in software because not all modes are supported by
 * the acc engine.
 * This function exists to partly overcome a G400 MAVEN hardware design fault, which
 * influences output in both monitor and TVout modes. The fault is that the MAVEN
 * keeps repeatedly displaying the last 8 pixels fetched from memory until fetching
 * restarts after a vertical retrace.
 * In order to let the maven fetch black pixels, an extra line has to be added to its
 * CRTC timing. This line needs to be entirely filled black, so with zeros. This line
 * will be displayed as last visible line, and the last 8 pixels of it will be
 * repeated during vertical retrace.
 * Note that the overscanning rubbish can be suppressed in TVout modes by extending
 * the vertical sync pulse all the way 'to the end'. */
status_t gx00_maven_clrline()
{
	uint32 x, screensize, pointer_reservation;
	uint8* adr;

	/* this function is nolonger needed on G450/G550 cards */
	if (si->ps.card_type > G400MAX) return B_OK;

	/* checkout space needed for hardcursor (if any) */
	pointer_reservation = 0;
	/* MIL 1/2 cards have a seperate buffer for the cursorbitmap inside the DAC */
	if ((si->ps.card_type >= G100) && si->settings.hardcursor) pointer_reservation = 1024;

	/* calculate actual screensize */
	screensize = si->fbc.bytes_per_row * si->dm.virtual_height;

	/* check if there's room left for a full blank line following the actual screen */
	if ((screensize + si->fbc.bytes_per_row + pointer_reservation) <=
		(si->ps.memory_size * 1024 * 1024))
	{
		LOG(4,("MAVEN: clearing line directly below screen\n"));

		/* calculate first adress beyond actual screen */
		adr = (uint8*)si->fbc.frame_buffer;
		adr += screensize;
		/* clear the line */
		for (x = 0; x < si->fbc.bytes_per_row; x++)
		{
			*(adr + x) = 0x00;
		}
	}
	else
		LOG(4,("MAVEN: not clearing line directly below screen: no memory left\n"));

	return B_OK;
}

status_t gx00_maven_dpms(bool display, bool h, bool v)
{
	/* this function is nolonger needed on G450/G550 cards */
	if (si->ps.card_type > G400MAX) return B_OK;

	if (display && h && v)
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
		MAVW(OUTMODE, 0x00);
	}

	return B_OK;
}

/*set a mode line - inputs are in pixels/scanlines*/
status_t gx00_maven_set_timing(display_mode target)
{
	uint8 temp, cnt, offset, loop;

	/* this function is nolonger needed on G450/G550 cards */
	if (si->ps.card_type > G400MAX) return B_OK;

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

	/* setup HSYNC & VSYNC polarity */
	LOG(2,("MAVEN: sync polarity: "));
	temp = MAVR(LUMA);

	/* find out which offset from the 'reset position' we need */
	switch (((target.timing.flags & B_POSITIVE_HSYNC) >> (29 - 0)) |
			((target.timing.flags & B_POSITIVE_VSYNC) >> (30 - 1)))
	{
	case 0:
		/* H neg, V neg */
		LOG(2,("H:neg V:neg\n"));
		offset = 2;
		break;
	case 1:
		/* H pos, V neg */
		LOG(2,("H:pos V:neg\n"));
		offset = 3;
		break;
	case 2:
		/* H neg, V pos */
		LOG(2,("H:neg V:pos\n"));
		offset = 1;
		break;
	case 3:
	default:
		/* H pos, V pos */
		LOG(2,("H:pos V:pos\n"));
		offset = 0;
		break;
	}
	/* calculate the number of steps we need to make from the current 'position' */
	cnt = 0;
	if ((offset - ((int)si->maven_syncpol_offset)) < 0) cnt = 4;
	cnt += offset - si->maven_syncpol_offset;
	/* note the offset from 'reset position' we will have now */
	si->maven_syncpol_offset = offset;

	/* program the maven: */
	/* make sure pulse bit is reset */
	temp &= ~0x20;
	MAVW(LUMA, temp);
	snooze(5);
	/* enable maven sync polarity programming */
	temp |= 0x10;
	MAVW(LUMA, temp);
	snooze(5);
	/* pulse sync programming bit until we have the polarities we want */
	for (loop = 0; loop < cnt; loop++)
	{
		MAVW(LUMA, (temp | 0x20));
		snooze(5);
		MAVW(LUMA, (temp & ~0x20));
		snooze(5);
	}
	/* disable maven sync polarity programming and reset pulse bit */
	MAVW(LUMA, (temp & ~0x30));

	return B_OK;
}

/*set the mode, brightness is a value from 0->2 (where 1 is equivalent to direct)*/
status_t gx00_maven_mode(int mode,float brightness)
{
	uint8 luma;

	/* this function is nolonger needed on G450/G550 cards */
	if (si->ps.card_type > G400MAX) return B_OK;

	/* set luma to a suitable value for brightness */
	/* assuming 0x0a is a sensible value */
	/* fixme:
	 * it looks like b6 and/or b7 determine the luma: just two values possible. */
	/* NOTE: b4 and b5 have another function, don't set! (sync polarity programming) */
	luma = (uint8)(0x0a * brightness);
	if (luma > 0x0f) luma = 0x0f;

	MAVW(LUMA, luma);
	LOG(4,("MAVEN: LUMA setting - %x\n", luma));

	return B_OK;
}

void gx00_maven_shutoff()
{
	switch (si->ps.card_type)
	{
		case G100:
		case G200:
		//fixme: see if this works on G400 too:
		//case G400:
		//case G400MAX:
			/* prevents distortions on CRTC1... */
			MAVW(TEST, 0x03);
			MAVW(MONEN, 0x00);
			MAVW(MONSET, 0x00);
			break;
		default:
			break;
	}
}

status_t gx00_maven_set_vid_pll(display_mode target)
{
	switch (si->ps.card_type)
	{
		case G450:
		case G550:
			return g450_g550_maven_set_vid_pll(target);
			break;
		default:
			return g100_g400max_maven_set_vid_pll(target);
			break;
	}
	return B_ERROR;
}
	
status_t g450_g550_maven_set_vid_pll(display_mode target)
{
	uint8 m=0,n=0,p=0;
	uint time = 0;

	float pix_setting, req_pclk;
	status_t result;

	req_pclk = (target.timing.pixel_clock)/1000.0;
	LOG(4,("MAVEN: Setting VID PLL for pixelclock %f\n", req_pclk));

	result = g450_g550_maven_vid_pll_find(target,&pix_setting,&m,&n,&p, 1);
	if (result != B_OK)
	{
		return result;
	}

	/*reprogram (disable,select,wait for stability,enable)*/
	CR2W(CTL, (CR2R(CTL) | 0x08)); 					/* disable the VIDPLL */
	CR2W(CTL, (CR2R(CTL) | 0x06)); 					/* select the VIDPLL */
	DXIW(VIDPLLM,(m));								/* set m value */
	DXIW(VIDPLLN,(n));								/* set n value */
	DXIW(VIDPLLP,(p));								/* set p value */

	/* Wait for the VIDPLL frequency to lock until timeout occurs */
	while((!(DXIR(VIDPLLSTAT) & 0x40)) & (time <= 2000))
	{
		time++;
		snooze(1);
	}

	if (time > 2000)
		LOG(2,("MAVEN: VID PLL frequency not locked!\n"));
	else
		LOG(2,("MAVEN: VID PLL frequency locked\n"));
	CR2W(CTL, (CR2R(CTL) & ~0x08)); 				/* enable the VIDPLL */

	return B_OK;
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
	case G100:
		LOG(4,("MAVEN: G100 restrictions apply\n"));
		m_max = 32;
		break;
	case G200:
		LOG(4,("MAVEN: G200 restrictions apply\n"));
		m_max = 32;
		break;
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
	LOG(2,("MAVEN: vid PLL check: req. %fMHz got %fMHz, mnp 0x%02x 0x%02x 0x%02x\n",
		req_pclk, *calc_pclk, *m_result, *n_result, *p_result));

	return B_OK;
}

static status_t gx50_maven_check_vid_pll(uint8 m, uint8 n, uint8 p)
{
	uint time = 0, count = 0;

	/* reprogram (disable,select,wait for stability,enable) */
	CR2W(CTL, (CR2R(CTL) | 0x06)); 					/* select the VIDPLL */
	DXIW(VIDPLLM,(m));								/* set m value */
	DXIW(VIDPLLN,(n));								/* set n value */
	DXIW(VIDPLLP,(p));								/* set p value */

	/* give the PLL 1mS at least to get a lock */
	time = 0;
	while((!(DXIR(VIDPLLSTAT) & 0x40)) & (time <= 1000))
	{
		time++;
		snooze(1);
	}
	
	/* no lock aquired, not useable */
	if (time > 1000) return B_ERROR;

	/* check if lock holds for at least 90% of the time */
	for (time = 0, count = 0; time <= 1000; time++)
	{
		if(DXIR(VIDPLLSTAT) & 0x40) count++;
		snooze(1);		
	}
	/* we have a winner */
	if (count >= 900) return B_OK;

	/* nogo, the PLL does not stabilize */
	return B_ERROR;
}

static status_t gx50_maven_check_vid_pll_range(uint8 m, uint8 n, uint8 *p, uint8 *q)
{
	uint8 s=0, p_backup = *p;

	/* preset no candidate, non working setting */
	*q = 0;
	/* preset lowest range filter */
	*p &= 0x47;

	/* iterate through all possible filtersettings */
	CR2W(CTL, (CR2R(CTL) | 0x08)); /* disable the VIDPLL */

	for (s = 0; s < 8 ;s++)
	{
		if (gx50_maven_check_vid_pll(m, n, *p)== B_OK)
		{
			/* now check 3 closest lower and higher settings */
			if ((gx50_maven_check_vid_pll(m, n - 3, *p)== B_OK) &&
				(gx50_maven_check_vid_pll(m, n - 2, *p)== B_OK) &&
				(gx50_maven_check_vid_pll(m, n - 1, *p)== B_OK) &&
				(gx50_maven_check_vid_pll(m, n + 1, *p)== B_OK) &&
				(gx50_maven_check_vid_pll(m, n + 2, *p)== B_OK) &&
				(gx50_maven_check_vid_pll(m, n + 3, *p)== B_OK))
			{
				LOG(2,("MAVEN: found optimal working VCO filter: #%d\n",s));
				/* preset first choice setting found */
				*q = 1;
				/* we are done */
				CR2W(CTL, (CR2R(CTL) & ~0x08)); /* enable the VIDPLL */
				return B_OK;
			}
			else
			{
				LOG(2,("MAVEN: found critical but working VCO filter: #%d\n",s));
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
	CR2W(CTL, (CR2R(CTL) & ~0x08)); /* enable the VIDPLL */
	/* we found only a non-optimal value */
	if (*q == 2) return B_OK;

	/* nothing worked at all */
	LOG(2,("MAVEN: no working VCO filter found!\n"));
	return B_ERROR;
}

/* find nearest valid video PLL setting */
status_t g450_g550_maven_vid_pll_find
	(display_mode target,float * calc_pclk,uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test)
{
	int m = 0, n = 0;
	uint8 p = 0, q = 0;
	float error, error_best = 999999999;
	int best[3]; 
	float f_vco, max_pclk;
	float req_pclk = target.timing.pixel_clock/1000.0;

	LOG(4,("MAVEN: G450/G550 restrictions apply\n"));

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
	/* lower limit is min_pixel_vco divided by highest postscaler-factor */
	if (req_pclk < (si->ps.min_video_vco / 16.0))
	{
		LOG(4,("MAVEN: clamping vidclock: requested %fMHz, set to %fMHz\n",
										req_pclk, (float)(si->ps.min_video_vco / 16.0)));
		req_pclk = (si->ps.min_video_vco / 16.0);
	}
	/* upper limit is given by pins in combination with current active mode */
	if (req_pclk > max_pclk)
	{
		LOG(4,("MAVEN: clamping vidclock: requested %fMHz, set to %fMHz\n",
														req_pclk, (float)max_pclk));
		req_pclk = max_pclk;
	}

	/* iterate through all valid PLL postscaler settings */
	for (p=0x01; p < 0x20; p = p<<1)
	{
		/* calculate the needed VCO frequency for this postscaler setting */
		f_vco = req_pclk * p;

		/* check if this is within range of the VCO specs */
		if ((f_vco >= si->ps.min_video_vco) && (f_vco <= si->ps.max_video_vco))
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
	LOG(2,("MAVEN: vid VCO frequency found %fMhz\n", f_vco));

	/* now find the filtersetting that matches best with this frequency by testing.
	 * for now we assume this routine succeeds to get us a stable setting */
	if (test)
		gx50_maven_check_vid_pll_range(m, n, &p, &q);
	else 
		LOG(2,("MAVEN: Not testing G450/G550 VCO feedback filters\n"));

	/* return the results */
	*calc_pclk = f_vco / best[2];
	*m_result = m;
	*n_result = n;
	*p_result = p;

	/* display the found pixelclock values */
	LOG(2,("MAVEN: vid PLL check: req. %fMHz got %fMHz, mnp 0x%02x 0x%02x 0x%02x\n",
		req_pclk, *calc_pclk, *m_result, *n_result, *p_result));

	return B_OK;
}
