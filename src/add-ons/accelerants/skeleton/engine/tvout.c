/* Authors:
   Mark Watson 2000,
   Rudolf Cornelissen 1/2003-12/2003

   Thanx to Petr Vandrovec for writing matroxfb.
*/

#define MODULE_BIT 0x00100000

#include "std.h"

typedef struct {
	uint32 h_total;
	uint32 h_display;
	uint32 h_sync_length;
	uint32 front_porch;
	uint32 back_porch;
	uint32 color_burst;
	uint32 v_total;
	float chroma_subcarrier;
} gx50_maven_timing;

void gxx0_maventv_PAL_init(uint8* buffer);
void gxx0_maventv_NTSC_init(uint8* buffer);
void gx50_maventv_PAL_timing(gx50_maven_timing *m_timing);
void gx50_maventv_NTSC_timing(gx50_maven_timing *m_timing);

//fixme: setup fixed CRTC2 modes for all modes and block other modes:
// 		- 640x480, 800x600, 1024x768 NTSC and PAL overscan compensated modes (desktop)
// 		- 640x480, 720x480 NTSC and 768x576, 720x576 non-overscan compensated modes (video)
//fixme: try to implement 'fast' and 'slow' settings for all modes,
//       so buffer duplication or skipping won't be neccesary for realtime video.
//fixme: try to setup the CRTC2 in interlaced mode for the video modes on <= G400MAX cards.

/* find 'exact' valid video PLL setting */
status_t g100_g400max_maventv_vid_pll_find(
	display_mode target, unsigned int * ht_new, unsigned int * ht_last_line,
	uint8 * m_result, uint8 * n_result, uint8 * p_result)
{
	int m = 0, n = 0, p = 0, m_max;
	float diff, diff_smallest = 999999999;
	int best[5], h_total_mod; 
	float fields_sec, f_vco;
	/* We need to be exact, so work with clockperiods per field instead of with frequency.
	 * Make sure however we truncate these clocks to be integers!
	 * (The NTSC field frequency would otherwise prevent the 'whole number of clocks per field'
	 *  check done in this routine later on...) */
	uint32 vco_clks_field, max_pclks_field, req_pclks_field;
	/* We need this variable to be a float, because we need to be able to verify if this 
	 * represents a whole number of clocks per field later on! */
	float calc_pclks_field;

	LOG(2,("MAVENTV: searching for EXACT videoclock match\n"));

	/* determine the max. reference-frequency postscaler setting for the current card */
	//fixme: check G100 and G200 m_max if exist and possible...
	switch(si->ps.card_type)
	{
/*	case G100:
		LOG(2,("MAVENTV: G100 restrictions apply\n"));
		m_max = 32;
		break;
	case G200:
		LOG(2,("MAVENTV: G200 restrictions apply\n"));
		m_max = 32;
		break;
*/	default:
		LOG(2,("MAVENTV: G400/G400MAX restrictions apply\n"));
		m_max = 32;
		break;
	}

	/* set number of fields per second to generate */
	if ((target.flags & TV_BITS) == TV_PAL)
		fields_sec = 50.0;
	else
		fields_sec = 59.94;

	/* determine the max. pixelclock for the current videomode */
	switch (target.space)
	{
		case B_RGB16_LITTLE:
			max_pclks_field = (si->ps.max_dac2_clock_16 * 1000000) / fields_sec;
			break;
		case B_RGB32_LITTLE:
			max_pclks_field = (si->ps.max_dac2_clock_32 * 1000000) / fields_sec;
			break;
		default:
			/* use fail-safe value */
			max_pclks_field = (si->ps.max_dac2_clock_32 * 1000000) / fields_sec;
			break;
	}
	/* if some dualhead mode is active, an extra restriction might apply */
	if ((target.flags & DUALHEAD_BITS) && (target.space == B_RGB32_LITTLE))
		max_pclks_field = (si->ps.max_dac2_clock_32dh * 1000000) / fields_sec;

	/* Checkout all possible Htotal settings within the current granularity step
	 * of CRTC2 to get a real close videoclock match!
	 * (The MAVEN apparantly has a granularity of 1 pixel, while CRTC2 has 8 pixels) */
	for (h_total_mod = 0; h_total_mod < 8; h_total_mod++)
	{
		LOG(2,("MAVENTV: trying h_total modification of +%d...\n", h_total_mod));

		/* Calculate videoclock to be a bit to high so we can compensate for an exact
		 * match via h_total_lastline.. */
		*ht_new = target.timing.h_total + h_total_mod + 2;

		/* Make sure the requested pixelclock is within the PLL's operational limits */
		/* lower limit is min_video_vco divided by highest postscaler-factor */
		req_pclks_field = *ht_new * target.timing.v_total;
		if (req_pclks_field < (((si->ps.min_video_vco * 1000000) / fields_sec) / 8.0))
		{
			req_pclks_field = (((si->ps.min_video_vco * 1000000) / fields_sec) / 8.0);
			LOG(4,("MAVENTV: WARNING, clamping at lowest possible videoclock\n"));
		}
		/* upper limit is given by pins in combination with current active mode */
		if (req_pclks_field > max_pclks_field)
		{
			req_pclks_field = max_pclks_field;
			LOG(4,("MAVENTV: WARNING, clamping at highest possible videoclock\n"));
		}

		/* iterate through all valid PLL postscaler settings */
		for (p=0x01; p < 0x10; p = p<<1)
		{
			/* calc the needed number of VCO clocks per field for this postscaler setting */
			vco_clks_field = req_pclks_field * p;

			/* check if this is within range of the VCO specs */
			if ((vco_clks_field >= ((si->ps.min_video_vco * 1000000) / fields_sec)) &&
				(vco_clks_field <= ((si->ps.max_video_vco * 1000000) / fields_sec)))
			{
				/* iterate trough all valid reference-frequency postscaler settings */
				for (m = 2; m <= m_max; m++)
				{
					/* calculate VCO postscaler setting for current setup.. */
					n = (int)(((vco_clks_field * m) / ((si->ps.f_ref * 1000000) / fields_sec)) + 0.5);
					/* ..and check for validity */
					if ((n < 8) || (n > 128))	continue;

					/* special TVmode stuff starts here (rest is in fact standard): */
					/* calculate number of videoclocks per field */
					calc_pclks_field =
						(((uint32)((si->ps.f_ref * 1000000) / fields_sec)) * n) / ((float)(m * p));

					/* we need a whole number of clocks per field, otherwise it won't work correctly.
					 * (TVout will flicker, green fields will occur) */
					if (calc_pclks_field != (uint32)calc_pclks_field) continue; 

					/* check if we have the min. needed number of clocks per field for a sync lock */
					if (calc_pclks_field < ((*ht_new * (target.timing.v_total - 1)) + 2)) continue;

					/* calc number of clocks we have for the last field line */
					*ht_last_line = calc_pclks_field - (*ht_new * (target.timing.v_total - 1));

					/* check if we haven't got too much clocks in the last field line for a sync lock */
					if (*ht_last_line > *ht_new) continue;
			
					/* we have a match! */
					/* calculate the difference between a full line and the last line */
					diff = *ht_new - *ht_last_line;

					/* if this last_line comes closer to a full line than earlier 'hits' then use it */
					if (diff < diff_smallest)
					{
						/* log results */
						if (diff_smallest == 999999999)
							LOG(2,("MAVENTV: MATCH, "));
						else
							LOG(2,("MAVENTV: better MATCH,"));
						f_vco = (si->ps.f_ref / m) * n;
						LOG(2,("found vid VCO freq %fMhz, pixclk %fMhz\n", f_vco, (f_vco / p)));
						LOG(2,("MAVENTV: mnp(ex. filter) 0x%02x 0x%02x 0x%02x, h_total %d, ht_lastline %d\n",
							(m - 1), (n - 1), (p - 1), (*ht_new - 2), (*ht_last_line - 2)));
						
						/* remember this best match */
						diff_smallest = diff;
						best[0] = m;
						best[1] = n;
						best[2] = p;
						/* h_total to use for this setting:
						 * exclude the 'calculate clock a bit too high' trick */
						best[3] = *ht_new - 2;
						/* ht_last_line to use for this setting:
						 * exclude the 'calculate clock a bit too high' trick */
						best[4] = *ht_last_line - 2;
					}
				}
			}
		}
	}
	LOG(2,("MAVENTV: search completed.\n"));

	/* setup the scalers programming values for found optimum setting */
	m = best[0] - 1;
	n = best[1] - 1;
	p = best[2] - 1;

	/* if no match was found set fixed PLL frequency so we have something valid at least */
	if (diff_smallest == 999999999)
	{
		LOG(4,("MAVENTV: WARNING, no MATCH found!\n"));

		if (si->ps.f_ref == 27.000)
		{
			/* set 13.5Mhz */
			m = 0x03;
			n = 0x07;
			p = 0x03;
		}
		else
		{
			/* set 14.31818Mhz */
			m = 0x01;
			n = 0x07;
			p = 0x03;
		}
		best[3] = target.timing.h_total;
		best[4] = target.timing.h_total;
	}

	/* calc the needed PLL loopbackfilter setting belonging to current VCO speed */
	f_vco = (si->ps.f_ref / (m + 1)) * (n + 1);
	LOG(2,("MAVENTV: using vid VCO frequency %fMhz\n", f_vco));

	switch(si->ps.card_type)
	{
/*	case G100:
	case G200:
		for(;;)
		{
			if (f_vco >= 180) {p |= (0x03 << 3); break;};
			if (f_vco >= 140) {p |= (0x02 << 3); break;};
			if (f_vco >= 100) {p |= (0x01 << 3); break;};
			break;
		}
		break;
*/	default:
		for(;;)
		{
			if (f_vco >= 240) {p |= (0x03 << 3); break;};
			if (f_vco >= 170) {p |= (0x02 << 3); break;};
			if (f_vco >= 110) {p |= (0x01 << 3); break;};
			break;
		}
		break;
	}

	/* return results */
	*m_result = m;
	*n_result = n;
	*p_result = p;
	*ht_new = best[3];
	*ht_last_line = best[4];

	/* display the found pixelclock values */
	LOG(2,("MAVENTV: vid PLL check: got %fMHz, mnp 0x%02x 0x%02x 0x%02x\n",
		(f_vco / ((p & 0x07) + 1)), m, n, p));
	LOG(2,("MAVENTV: new h_total %d, ht_lastline %d\n", *ht_new, *ht_last_line));

	/* return status */
	if (diff_smallest == 999999999) return B_ERROR;
	return B_OK;
}

	/* Notes about timing:
	 * Note:
	 * all horizontal timing is measured in pixelclock periods;
	 * all? vertical timing is measured in field? lines.  */

	/* Note:
	 * <= G400MAX cards have a fixed 27Mhz(?) clock for TV timing register values,
	 * while on G450/G550 these need to be calculated based on the video pixelclock. */


	/* Notes about signal strengths:
	 * Note:
	 * G400 and earlier cards have a fixed reference voltage of +2.6 Volt;
	 * G450 and G550 cards MAVEN DACs have a switchable ref voltage of +1.5/+2.0 Volt.
	 *
	 * This voltage is used to feed the videosignals:
	 * - Hsync pulse level;
	 * - Lowest active video output level;
	 * - Highest active video output level.
	 * These actual voltages are set via 10bit DACs.
	 *
	 * G450/G550:
	 * The color burst amplitude videosignal is fed by 80% of the above mentioned
	 * ref. voltage, and is set via an 8bit DAC.
	 * On G400 and earlier cards the ref. voltage is different, and also differs
	 * for PAL and NTSC mode. */

	/* Note:
	 * Increasing the distance between the highest and lowest active video output
	 * level increases contrast; decreasing it decreases contrast. */

	/* Note:
	 * Increasing both the highest and lowest active video output level with the
	 * same amount increases brightness; decreasing it decreases brightness. */

	/* Note:
	 * Increasing the Hsync pulse level increases the black level, so decreases
	 * brightness and contrast. */

/* Preset maven PAL output (625lines, 50Hz mode) */
void gxx0_maventv_PAL_init(uint8* buffer) 
{
	uint16 value;

	/* Chroma subcarrier divider */
	buffer[0x00] = 0x2A;
	buffer[0x01] = 0x09;
	buffer[0x02] = 0x8A;
	buffer[0x03] = 0xCB; 

	buffer[0x04] = 0x00;
	buffer[0x05] = 0x00;
	buffer[0x06] = 0xF9;
	buffer[0x07] = 0x00;
	/* Hsync pulse length */
	buffer[0x08] = 0x7E;
	/* color burst length */
	buffer[0x09] = 0x44;
	/* back porch length */
	buffer[0x0a] = 0x9C;

	/* color burst amplitude */
/*	if (si->ps.card_type <= G400MAX)
	{
		buffer[0x0b] = 0x3e;
	}
	else
	{
*/		buffer[0x0b] = 0x48;
//	}

	buffer[0x0c] = 0x21;
	buffer[0x0d] = 0x00;

//	if (si->ps.card_type <= G400MAX)
//	{
		/* Lowest active video output level.
		 * Warning: make sure this stays above (or equals) the sync pulse level! */
//		value = 0x0ea;
//		buffer[0x0e] = ((value >> 2) & 0xff);
//		buffer[0x0f] = (value & 0x03);
		/* horizontal sync pulse level */
//		buffer[0x10] = ((value >> 2) & 0xff);
//		buffer[0x11] = (value & 0x03);
//	}
//	else
	{
		/* Lowest active video output level.
		 * Warning: make sure this stays above (or equals) the sync pulse level! */
		value = 0x130;
		buffer[0x0e] = ((value >> 2) & 0xff);
		buffer[0x0f] = (value & 0x03);
		/* horizontal sync pulse level */
		buffer[0x10] = ((value >> 2) & 0xff);
		buffer[0x11] = (value & 0x03);
	}

	buffer[0x12] = 0x1A;
	buffer[0x13] = 0x2A;
	
	/* functional unit */
	buffer[0x14] = 0x1C;
	buffer[0x15] = 0x3D;
	buffer[0x16] = 0x14;
	
	/* vertical total */ //(=625)
	/* b9-2 */
	buffer[0x17] = 0x9C;
	/* b1-0 in b1-0 */
	buffer[0x18] = 0x01;

	buffer[0x19] = 0x00;
	buffer[0x1a] = 0xFE;
	buffer[0x1b] = 0x7E;
	buffer[0x1c] = 0x60;
	buffer[0x1d] = 0x05;

	/* Highest active video output level.
	 * Warning: make sure this stays above the lowest active video output level! */
/*	if (si->ps.card_type <= G400MAX)
	{
		value = 0x24f;
		buffer[0x1e] = ((value >> 2) & 0xff);
		buffer[0x1f] = (value & 0x03);
	}
	else
*/	{
		value = 0x300;
		buffer[0x1e] = ((value >> 2) & 0xff);
		buffer[0x1f] = (value & 0x03);
	}

	/* saturation (field?) #1 */
//	if (si->ps.card_type <= G400MAX)
//		buffer[0x20] = 0x72;
//	else
		buffer[0x20] = 0xA5;

	buffer[0x21] = 0x07;

	/* saturation (field?) #2 */
//	if (si->ps.card_type <= G400MAX)
//		buffer[0x22] = 0x72;
//	else
		buffer[0x22] = 0xA5;

	buffer[0x23] = 0x00;
	buffer[0x24] = 0x00;
	/* hue? */
	buffer[0x25] = 0x00;

	buffer[0x26] = 0x08;
	buffer[0x27] = 0x04;
	buffer[0x28] = 0x00;
	buffer[0x29] = 0x1A;

	/* functional unit */
	buffer[0x2a] = 0x55;
	buffer[0x2b] = 0x01;

	/* front porch length */
	buffer[0x2c] = 0x26;

	/* functional unit */
	buffer[0x2d] = 0x07;
	buffer[0x2e] = 0x7E;

	/* functional unit */
	buffer[0x2f] = 0x02;
	buffer[0x30] = 0x54;

	/* horizontal visible */
	value = 0x580;
	buffer[0x31] = ((value >> 3) & 0xff);
	buffer[0x32] = (value & 0x07);

	/* upper blanking (in field lines) */
	buffer[0x33] = 0x14; //=((v_total - v_sync_end)/2) -1

	buffer[0x34] = 0x49;
	buffer[0x35] = 0x00;
	buffer[0x36] = 0x00;
	buffer[0x37] = 0xA3;
	buffer[0x38] = 0xC8;
	buffer[0x39] = 0x22;
	buffer[0x3a] = 0x02;
	buffer[0x3b] = 0x22;

	/* functional unit */
	buffer[0x3c] = 0x3F;
	buffer[0x3d] = 0x03;
}

/* Preset maven NTSC output (525lines, 59.94Hz mode) */
void gxx0_maventv_NTSC_init(uint8* buffer) 
{
	uint16 value;

	/* Chroma subcarrier frequency */
	buffer[0x00] = 0x21;
	buffer[0x01] = 0xF0;
	buffer[0x02] = 0x7C;
	buffer[0x03] = 0x1F;

	buffer[0x04] = 0x00;
	buffer[0x05] = 0x00;//b1 = ON enables colorbar testimage
	buffer[0x06] = 0xF9;//b0 = ON enables MAVEN TV output
	buffer[0x07] = 0x00;//influences the colorburst signal amplitude somehow

	/* Hsync pulse length */
	buffer[0x08] = 0x7E;
	/* color burst length */
	buffer[0x09] = 0x43;
	/* back porch length */
	buffer[0x0a] = 0x7E;

	/* color burst amplitude */
//	if (si->ps.card_type <= G400MAX)
//	{
//		buffer[0x0b] = 0x46;
//	}
//	else
	{
		buffer[0x0b] = 0x48;
	}

	buffer[0x0c] = 0x00;
	buffer[0x0d] = 0x00;

//	if (si->ps.card_type <= G400MAX)
//	{
		/* Lowest active video output level.
		 * Warning: make sure this stays above (or equals) the sync pulse level! */
//		value = 0x0ea;
//		buffer[0x0e] = ((value >> 2) & 0xff);
//		buffer[0x0f] = (value & 0x03);
		/* horizontal sync pulse level */
//		buffer[0x10] = ((value >> 2) & 0xff);
//		buffer[0x11] = (value & 0x03);
//	}
//	else
	{
		/* Lowest active video output level.
		 * Warning: make sure this stays above (or equals) the sync pulse level! */
		value = 0x130;
		buffer[0x0e] = ((value >> 2) & 0xff);
		buffer[0x0f] = (value & 0x03);
		/* horizontal sync pulse level */
		buffer[0x10] = ((value >> 2) & 0xff);
		buffer[0x11] = (value & 0x03);
	}

	buffer[0x12] = 0x17;
	buffer[0x13] = 0x21;

	/* functional unit */
	buffer[0x14] = 0x1B;
	buffer[0x15] = 0x1B;
	buffer[0x16] = 0x24;

	/* vertical total */
	/* b9-2 */
	buffer[0x17] = 0x83;
	/* b1-0 in b1-0 */
	buffer[0x18] = 0x01;

	buffer[0x19] = 0x00;//mv register?
	buffer[0x1a] = 0x0F;
	buffer[0x1b] = 0x0F;
	buffer[0x1c] = 0x60;
	buffer[0x1d] = 0x05;

	/* Highest active video output level.
	 * Warning: make sure this stays above the lowest active video output level! */
/*	if (si->ps.card_type <= G400MAX)
	{
		value = 0x24f;
		buffer[0x1e] = ((value >> 2) & 0xff);
		buffer[0x1f] = (value & 0x03);
	}
	else
*/	{
		value = 0x300;
		buffer[0x1e] = ((value >> 2) & 0xff);
		buffer[0x1f] = (value & 0x03);
	}

	/* color saturation #1 (Y-B ?) */
//	if (si->ps.card_type <= G400MAX)
//		buffer[0x20] = 0x5F;
//	else
		buffer[0x20] = 0x9C;

	buffer[0x21] = 0x04;

	/* color saturation #2 (Y-R ?) */
//	if (si->ps.card_type <= G400MAX)
//		buffer[0x22] = 0x5F;
//	else
		buffer[0x22] = 0x9C;

	buffer[0x23] = 0x01;
	buffer[0x24] = 0x02;

	/* hue: preset at 0 degrees */
	buffer[0x25] = 0x00;

	buffer[0x26] = 0x0A;
	buffer[0x27] = 0x05;//sync stuff
	buffer[0x28] = 0x00;
	buffer[0x29] = 0x10;//field line-length stuff

	/* functional unit */
	buffer[0x2a] = 0xFF;
	buffer[0x2b] = 0x03;

	/* front porch length */
	buffer[0x2c] = 0x24;

	/* functional unit */
	buffer[0x2d] = 0x0F;
	buffer[0x2e] = 0x78;

	/* functional unit */
	buffer[0x2f] = 0x00;
	buffer[0x30] = 0x00;

	/* horizontal visible */
	/* b10-3 */
	buffer[0x31] = 0xB2;
	/* b2-0 in b2-0 */
	buffer[0x32] = 0x04;

	/* upper blanking (in field lines) */
	buffer[0x33] = 0x14;

	buffer[0x34] = 0x02;//colorphase or so stuff.
	buffer[0x35] = 0x00;
	buffer[0x36] = 0x00;
	buffer[0x37] = 0xA3;
	buffer[0x38] = 0xC8;
	buffer[0x39] = 0x15;
	buffer[0x3a] = 0x05;
	buffer[0x3b] = 0x3B;

	/* functional unit */
	buffer[0x3c] = 0x3C;
	buffer[0x3d] = 0x00;
}

void gx50_maventv_PAL_timing(gx50_maven_timing *m_timing)
{
	/* values are given in picoseconds */
	m_timing->h_total = 64000000;
	/* the sum of the signal duration below should match h_total! */
	m_timing->h_display = 52148148;
	m_timing->h_sync_length = 4666667;
	m_timing->front_porch = 1407407;
	m_timing->back_porch = 5777778;
	/* colorburst is 'superimposed' on the above timing */
	m_timing->color_burst = 2518518;
	/* number of lines per frame */
	m_timing->v_total = 625;
	/* color carrier frequency in Mhz */
	m_timing->chroma_subcarrier = 4.43361875;
}

void gx50_maventv_NTSC_timing(gx50_maven_timing *m_timing)
{
	/* values are given in picoseconds */
	m_timing->h_total = 63555556;
	/* the sum of the signal duration below should match h_total! */
	m_timing->h_display = 52888889;
	m_timing->h_sync_length = 4666667;
	m_timing->front_porch = 1333333;
	m_timing->back_porch = 4666667;
	/* colorburst is 'superimposed' on the above timing */
	m_timing->color_burst = 2418418;
	/* number of lines per frame */
	m_timing->v_total = 525;
	/* color carrier frequency in Mhz */
	m_timing->chroma_subcarrier = 3.579545454;
}

int maventv_init(display_mode target) 
{
	uint8 val;
	uint8 m_result, n_result, p_result;
	unsigned int ht_new, ht_last_line;
	float calc_pclk = 0;
	/* use a display_mode copy because we might tune it for TVout compatibility */
	display_mode tv_target = target;
	/* used as buffer for TVout signal to generate */
	uint8 maventv_regs[64];
	/* used in G450/G550 to calculate TVout signal timing dependant on pixelclock;
	 * <= G400MAX use fixed settings because base-clock here is the fixed crystal
	 * frequency. */
	//fixme: if <=G400 cards with MAVEN and crystal of 14.31818Mhz exist, modify!?!
	gx50_maven_timing m_timing;

	/* preset new TVout mode */
	if ((tv_target.flags & TV_BITS) == TV_PAL)
	{
		LOG(4, ("MAVENTV: PAL TVout\n"));
		gxx0_maventv_PAL_init(maventv_regs);
		gx50_maventv_PAL_timing(&m_timing);
	}
	else
	{
		LOG(4, ("MAVENTV: NTSC TVout\n"));
		gxx0_maventv_NTSC_init(maventv_regs);
		gx50_maventv_NTSC_timing(&m_timing);
	}

	/* enter mode-program mode */
//	if (si->ps.card_type <= G400MAX) MAVW(PGM, 0x01);
//	else
//	{
//		DXIW(TVO_IDX, ENMAV_PGM);
//		DXIW(TVO_DATA, 0x01);
//	}

	/* tune new TVout mode */
//	if (si->ps.card_type <= G400MAX)
	{
		/* setup TV-mode 'copy' of CRTC2, setup PLL, inputs, outputs and sync-locks */
//		MAVW(MONSET, 0x00);
//		MAVW(MONEN, 0xA2);

		/* xmiscctrl */
		//unknown regs:
//		MAVWW(WREG_0X8E_L, 0x1EFF); 
//		MAVW(BREG_0XC6, 0x01);

//		MAVW(LOCK, 0x01);
//		MAVW(OUTMODE, 0x08);
//		MAVW(LUMA, 0x78);
//		MAVW(STABLE, 0x02);
//		MAVW(MONEN, 0xB3);

		/* setup video PLL */
		g100_g400max_maventv_vid_pll_find(
			tv_target, &ht_new, &ht_last_line, &m_result, &n_result, &p_result);
//		MAVW(PIXPLLM, m_result);
//		MAVW(PIXPLLN, n_result);
//		MAVW(PIXPLLP, (p_result | 0x80));

//		MAVW(MONSET, 0x20);

//		MAVW(TEST, 0x10);

		/* htotal - 2 */
//		MAVWW(HTOTALL, ht_new);

		/* last line in field can have different length */
		/* hlen - 2 */
//		MAVWW(LASTLINEL, ht_last_line);

		/* horizontal vidrst pos: 0 <= vidrst pos <= htotal - 2 */
//		MAVWW(HVIDRSTL, (ht_last_line - si->crtc_delay - 
//						(tv_target.timing.h_sync_end - tv_target.timing.h_sync_start)));
		//ORG (does the same but with limit checks but these limits should never occur!):
//		slen = tv_target.timing.h_sync_end - tv_target.timing.h_sync_start;
//		hcrt = tv_target.timing.h_total - slen - si->crtc_delay;
//		if (ht_last_line < tv_target.timing.h_total) hcrt += ht_last_line;
//		if (hcrt > tv_target.timing.h_total) hcrt -= tv_target.timing.h_total;
//		if (hcrt + 2 > tv_target.timing.h_total) hcrt = 0;	/* or issue warning? */
//		MAVWW(HVIDRSTL, hcrt);

		/* who knows */
//		MAVWW(HSYNCSTRL, 0x0004);//must be 4!!

		/* hblanking end: 100% */
//		MAVWW(HSYNCLENL, (tv_target.timing.h_total - tv_target.timing.h_sync_end));

		/* vertical line count - 1 */
//		MAVWW(VTOTALL, (tv_target.timing.v_total - 1));

		/* vertical vidrst pos */
//		MAVWW(VVIDRSTL, (tv_target.timing.v_total - 2));

		/* something end... [A6]+1..[A8] */
//		MAVWW(VSYNCSTRL, 0x0001);

		/* vblanking end: stop vblanking */
//		MAVWW(VSYNCLENL, (tv_target.timing.v_sync_end - tv_target.timing.v_sync_start - 1));
		//org: no visible diff:
		//MAVWW(VSYNCLENL, (tv_target.timing.v_total - tv_target.timing.v_sync_start - 1));

		/* something start... 0..[A4]-1 */
//		MAVWW(VDISPLAYL, 0x0000);
		//std setmode (no visible difference)
		//MAVWW(VDISPLAYL, (tv_target.timing.v_total - 1));

		/* ... */
//		MAVWW(WREG_0X98_L, 0x0000);

		/* moves picture up/down and so on... */
//		MAVWW(VSOMETHINGL, 0x0001); /* Fix this... 0..VTotal */

		{
			uint32 h_display_tv;
			uint8 h_scale_tv;

			unsigned int ib_min_length;
			unsigned int ib_length;
			int index;

			/* calc hor scale-back factor from input to output picture (in 1.7 format)
			 * the MAVEN has 736 pixels fixed visible? outputline length for TVout */ 
			//fixme: shouldn't this be 768 (= PAL 1:1 output 4:3 ratio format)?!?
			h_scale_tv = (736 << 7) / tv_target.timing.h_total;//should be PLL corrected
			LOG(4,("MAVENTV: horizontal scale-back factor is: %f\n", (h_scale_tv / 128.0)));

			/* limit values to MAVEN capabilities (scale-back factor is 0.5-1.0) */
			//fixme: how about lowres upscaling?
			if (h_scale_tv > 0x80)
			{
				h_scale_tv = 0x80;
				LOG(4,("MAVENTV: limiting horizontal scale-back factor to: %f\n", (h_scale_tv / 128.0)));
			}
			if (h_scale_tv < 0x40)
			{
				h_scale_tv = 0x40;
				LOG(4,("MAVENTV: limiting horizontal scale-back factor to: %f\n", (h_scale_tv / 128.0)));
			}
			/* make sure we get no round-off error artifacts on screen */
			h_scale_tv--;

			/* calc difference in (wanted output picture width (excl. hsync_length)) and
			 * (fixed total output line length (=768)),
			 * based on input picture and scaling factor */
			/* (MAVEN trick (part 1) to get output picture width to fit into just 8 bits) */
			h_display_tv = ((768 - 1) << 7) -
				(((tv_target.timing.h_total - tv_target.timing.h_sync_end)	/* is left margin */
				 + tv_target.timing.h_display - 8)
				 * h_scale_tv);
			/* convert result from 25.7 to 32.0 format */
			h_display_tv = h_display_tv >> 7;
			LOG(4,("MAVENTV: displaying output on %d picture pixels\n",
				((768 - 1) - h_display_tv)));

			/* half result: MAVEN trick (part 2)
			 * (258 - 768 pixels, only even number per line is possible) */
			h_display_tv = h_display_tv >> 1;
			/* limit value to register contraints */
			if (h_display_tv > 0xFF) h_display_tv = 0xFF;
//			MAVW(HSCALETV, h_scale_tv);
//			MAVW(HDISPLAYTV, h_display_tv);


			/* calculate line inputbuffer length */
			/* It must be between (including):
			 * ((input picture left margin) + (input picture hor. resolution) + 4)
			 * AND
			 * (input picture total line length) (PLL corrected) */

			/* calculate minimal line input buffer length */
			ib_min_length = ((tv_target.timing.h_total - tv_target.timing.h_sync_end) +
				 			  tv_target.timing.h_display + 4);

			/* calculate optimal line input buffer length (so top of picture is OK too) */
			/* The following formula applies:
			 * optimal buffer length = ((((0x78 * i) - R) / hor. scaling factor) + Q)
			 *
			 * where (in 4.8 format!)
		     * R      Qmin  Qmax
			 * 0x0E0  0x5AE 0x5BF
			 * 0x100  0x5CF 0x5FF	
			 * 0x180  0x653 0x67F
			 * 0x200  0x6F8 0x6FF
			 */
			index = 1;
			do
			{
				ib_length = ((((((0x7800 << 7) * index) - (0x100 << 7)) / h_scale_tv) + 0x05E7) >> 8);
				index++;
			} while (ib_length < ib_min_length);
			LOG(4,("MAVENTV: optimal line inputbuffer length: %d\n", ib_length));

			if (ib_length >= ht_new + 2)
			{
				ib_length = ib_min_length;
				LOG(4,("MAVENTV: limiting line inputbuffer length, setting minimal usable: %d\n", ib_length));
			}
//			MAVWW(HDISPLAYL, ib_length);
		}

		{
			uint16 t_scale_tv;
			uint32 v_display_tv;

			/* calc total scale-back factor from input to output picture */
			{
				uint32 out_clocks;
				uint32 in_clocks;

				//takes care of green stripes:
				/* calc output clocks per frame */
				out_clocks = m_timing.v_total * (ht_new + 2);

				/* calc input clocks per frame */
				in_clocks = (tv_target.timing.v_total - 1) * (ht_new + 2) +	ht_last_line + 2;

				/* calc total scale-back factor from input to output picture in 1.15 format */
				t_scale_tv = ((((uint64)out_clocks) << 15) / in_clocks); 
				LOG(4,("MAVENTV: total scale-back factor is: %f\n", (t_scale_tv / 32768.0)));

				/* min. scale-back factor is 1.0 for 1:1 output */
				if (t_scale_tv > 0x8000)
				{
					t_scale_tv = 0x8000;
					LOG(4,("MAVENTV: limiting total scale-back factor to: %f\n", (t_scale_tv / 32768.0)));
				}
			}

			/*calc output picture height based on input picture and scaling factor */
			//warning: v_display was 'one' lower originally!
			v_display_tv = 
				((tv_target.timing.v_sync_end - tv_target.timing.v_sync_start) 	/* is sync length */
				 + (tv_target.timing.v_total - tv_target.timing.v_sync_end) 	/* is upper margin */
				 + tv_target.timing.v_display)
				 * t_scale_tv;
			/* convert result from 17.15 to 32.0 format */
			v_display_tv = (v_display_tv >> 15);
			LOG(4,("MAVENTV: displaying output on %d picture frame-lines\n", v_display_tv));

			/* half result, and compensate for internal register offset
			 * (MAVEN trick to get it to fit into just 8 bits).
			 * (allowed output frame height is 292 - 802 lines, only even numbers) */
			v_display_tv = (v_display_tv >> 1) - 146;
			/* limit value to register contraints */
			if (v_display_tv > 0xFF) v_display_tv = 0xFF;
			/* make sure we get no round-off error artifacts on screen */
			t_scale_tv--;

//			MAVWW(TSCALETVL, t_scale_tv);
//			MAVW(VDISPLAYTV, v_display_tv);
		}

//		MAVW(TEST, 0x00);

		/* gamma correction registers */
//		MAVW(GAMMA1, 0x00);
//		MAVW(GAMMA2, 0x00);
//		MAVW(GAMMA3, 0x00);
//		MAVW(GAMMA4, 0x1F);
//		MAVW(GAMMA5, 0x10);
//		MAVW(GAMMA6, 0x10);
//		MAVW(GAMMA7, 0x10);
//		MAVW(GAMMA8, 0x64);	/* 100 */
//		MAVW(GAMMA9, 0xC8);	/* 200 */

		/* set flickerfilter */
		/* OFF: is dependant on MAVEN chip version(?): ENG_TVO_B = $40, else $00.
		 * ON : always set $a2. */
//		MAVW(FFILTER, 0xa2);

		/* 0x10 or anything ored with it */
		//fixme? linux uses 0x14...
//		MAVW(TEST, (MAVR(TEST) & 0x10));

		/* output: SVideo/Composite */
//		MAVW(OUTMODE, 0x08);
	}
//	else /* card_type is >= G450 */
	{
		//fixme: setup an intermediate buffer if vertical res is different than settings below!
		//fixme: setup 2D or 3D engine to do screen_to_screen_scaled_filtered_blit between the buffers
		//       during vertical retrace!
		if ((tv_target.flags & TV_BITS) == TV_PAL)
		{
			int diff;

			/* defined by the PAL standard */
			tv_target.timing.v_total = m_timing.v_total;
			/* we need to center the image on TV vertically.
			 * note that 576 is the maximum supported resolution for the PAL standard,
			 * this is already overscanning by approx 8-10% */
			diff = 576 - tv_target.timing.v_display;
			/* if we cannot display the current vertical resolution fully, clip it */
			if (diff < 0)
			{
				tv_target.timing.v_display = 576;
				diff = 0;
			}
			/* now center the image on TV by centering the vertical sync pulse */
			tv_target.timing.v_sync_start = tv_target.timing.v_display + 1 + (diff / 2);
			tv_target.timing.v_sync_end = tv_target.timing.v_sync_start + 1;
		}
		else
		{
			int diff;

			/* defined by the NTSC standard */
			tv_target.timing.v_total = m_timing.v_total;
			/* we need to center the image on TV vertically.
			 * note that 480 is the maximum supported resolution for the NTSC standard,
			 * this is already overscanning by approx 8-10% */
			diff = 480 - tv_target.timing.v_display;
			/* if we cannot display the current vertical resolution fully, clip it */
			if (diff < 0)
			{
				tv_target.timing.v_display = 480;
				diff = 0;
			}
			/* now center the image on TV by centering the vertical sync pulse */
			tv_target.timing.v_sync_start = tv_target.timing.v_display + 1 + (diff / 2);
			tv_target.timing.v_sync_end = tv_target.timing.v_sync_start + 1;
		}

		/* setup video PLL for G450/G550:
		 * this can be done in the normal way because the MAVEN works in slave mode!
		 * NOTE: must be done before programming CRTC2, or interlaced startup may fail. */

		//fixme: make sure videoPLL is powered up: XPWRCTRL b1=1
		{
			uint16 front_porch, back_porch, h_sync_length, burst_length, h_total, h_display;
			uint32 chromasc;
			uint64 pix_period;
			uint16 h_total_wanted, leftover;

			/* calculate tv_h_display in 'half pixelclocks' and adhere to MAVEN restrictions.
			 * ('half pixelclocks' exist because the MAVEN uses them...) */
			h_display = (((tv_target.timing.h_display << 1) + 3) & ~0x03);
			if (h_display > 2044) h_display = 2044;
			/* copy result to MAVEN TV mode */
			maventv_regs[0x31] = (h_display >> 3);
			maventv_regs[0x32] = (h_display & 0x07);

			/* calculate needed video pixelclock in kHz.
			 * NOTE:
			 * The clock calculated is based on MAVEN output, so each pixelclock period
			 * is in fact a 'half pixelclock' period compared to monitor mode use. */
			tv_target.timing.pixel_clock =
				((((uint64)h_display) * 1000000000) / m_timing.h_display);

			/* tune display_mode adhering to CRTC2 restrictions */
			/* (truncate h_display to 'whole pixelclocks') */
			tv_target.timing.h_display = ((h_display >> 1) & ~0x07);
			tv_target.timing.h_sync_start = tv_target.timing.h_display + 8;

//			g450_g550_maven_vid_pll_find(tv_target, &calc_pclk, &m_result, &n_result, &p_result, 1);
			/* adjust mode to actually used pixelclock */
			tv_target.timing.pixel_clock = (calc_pclk * 1000);

			/* program videoPLL */
//			DXIW(VIDPLLM, m_result);
//			DXIW(VIDPLLN, n_result);
//			DXIW(VIDPLLP, p_result);

			/* calculate videoclock 'half' period duration in picoseconds */
			pix_period = (1000000000 / ((float)tv_target.timing.pixel_clock)) + 0.5;
			LOG(4,("MAVENTV: TV videoclock period is %d picoseconds\n", pix_period));

			/* calculate number of 'half' clocks per line according to pixelclock set */
			/* fixme: try to setup the modes in such a way that
			 * (h_total_clk % 16) == 0 because of the CRTC2 restrictions:
			 * we want to loose the truncating h_total trick below if possible! */
			/* Note:
			 * This is here so we can see the wanted and calc'd timing difference. */
			h_total_wanted = ((m_timing.h_total / ((float)pix_period)) + 0.5);
			LOG(4,("MAVENTV: TV h_total should be %d units\n", h_total_wanted));

			/* calculate chroma subcarrier value to setup:
			 * do this as exact as possible because this signal is very sensitive.. */
			chromasc =
				((((uint64)0x100000000) * (m_timing.chroma_subcarrier / calc_pclk)) + 0.5);
			/* copy result to MAVEN TV mode */
			maventv_regs[0] = ((chromasc >> 24) & 0xff);
			maventv_regs[1] = ((chromasc >> 16) & 0xff);
			maventv_regs[2] = ((chromasc >>  8) & 0xff);
			maventv_regs[3] = ((chromasc >>  0) & 0xff);
			LOG(4,("MAVENTV: TV chroma subcarrier divider set is $%08x\n", chromasc));

			/* calculate front porch in 'half pixelclocks' */
			/* we always round up because of the h_total truncating 'trick' below,
			 * which works in combination with the existing difference between
			 * h_total_clk and h_total */
			//fixme: prevent this if possible!
			front_porch = ((m_timing.front_porch / ((float)pix_period)) + 1);
			/* value must be even */
			front_porch &= ~0x01;

			/* calculate back porch in 'half pixelclocks' */
			/* we always round up because of the h_total truncating 'trick' below,
			 * which works in combination with the existing difference between
			 * h_total_clk and h_total */
			//fixme: prevent this if possible!
			back_porch = ((m_timing.back_porch / ((float)pix_period)) + 1);
			/* value must be even */
			back_porch &= ~0x01;

			/* calculate h_sync length in 'half pixelclocks' */
			/* we always round up because of the h_total truncating 'trick' below,
			 * which works in combination with the existing difference between
			 * h_total_clk and h_total */
			//fixme: prevent this if possible!
			h_sync_length = ((m_timing.h_sync_length / ((float)pix_period)) + 1);
			/* value must be even */
			h_sync_length &= ~0x01;

			/* calculate h_total in 'half pixelclocks' */
			h_total = h_display + front_porch + back_porch + h_sync_length;

			LOG(4,("MAVENTV: TV front_porch is %d clocks\n", front_porch));
			LOG(4,("MAVENTV: TV back_porch is %d clocks\n", back_porch));
			LOG(4,("MAVENTV: TV h_sync_length is %d clocks\n", h_sync_length));
			LOG(4,("MAVENTV: TV h_display is %d clocks \n", h_display));
			LOG(4,("MAVENTV: TV h_total is %d clocks\n", h_total));

			/* calculate color_burst length in 'half pixelclocks' */
			burst_length = (((m_timing.color_burst /*- 1*/) / ((float)pix_period)) + 0.5);
			LOG(4,("MAVENTV: TV color_burst is %d clocks.\n", burst_length));

			/* copy result to MAVEN TV mode */
			maventv_regs[0x09] = burst_length;

			/* Calculate line length 'rest' that remains after truncating
			 * h_total to adhere to the CRTC2 timing restrictions. */
			leftover = h_total & 0x0F;
			/* if some 'rest' exists, we need to compensate for it... */ 
			/* Note:
			 * It's much better to prevent this from happening because this
			 * 'trick' will decay TVout timing! (timing is nolonger official) */
			if (leftover)
			{
				/* truncate line length to adhere to CRTC2 restrictions */
				front_porch -= leftover;
				h_total -= leftover;

				/* now set line length to closest CRTC2 valid match */
				if (leftover < 3)
				{
					/* 1 <= old rest <= 2:
					 * Truncated line length is closest match. */
					LOG(4,("MAVENTV: CRTC2 h_total leftover discarded (< 3)\n"));
				}
				else
				{
					if (leftover < 10)
					{
						/* 3 <= old rest <= 9:
						 * We use the NTSC killer circuitry to get closest match.
						 * (The 'g400_crtc2_set_timing' routine will enable it
						 *  because of the illegal h_total timing we create here.) */
						front_porch += 4;
						h_total += 4;
						LOG(4,("MAVENTV: CRTC2 h_total leftover corrected via killer (> 2, < 10)\n"));
					}
					else
					{
						/* 10 <= old rest <= 15:
						 * Set closest valid CRTC2 match. */
						front_porch += 16;
						h_total += 16;
						LOG(4,("MAVENTV: CRTC2 h_total leftover corrected via increase (> 9, < 16)\n"));
					}
				}
			}

			/* (linux) fixme: maybe MAVEN has requirement 800 < h_total < 1184 */
			maventv_regs[0x2C] = front_porch;
			maventv_regs[0x0A] = back_porch;
			maventv_regs[0x08] = h_sync_length;

			/* change h_total to represent 'whole pixelclocks' */
			h_total = h_total >> 1;

			/* tune display_mode adhering to CRTC2 restrictions */
			tv_target.timing.h_sync_end = (h_total & ~0x07) - 8;
			/* h_total is checked before being programmed! (NTSC killer circuitry) */
			tv_target.timing.h_total = h_total;
		}

		/* output Y/C and CVBS signals (| $40 needed for SCART) */
//		DXIW(TVO_IDX, 0x80);
//		DXIW(TVO_DATA, 0x03);

		/* select input colorspace */
		//fixme?: has no effect on output picture on monitor or TV...
		//DXIW(TVO_IDX, 0x81);
		//DXIW(TVO_DATA, 0x00);

		/* calculate vertical sync point */
		{
			int upper;
		
			/* set 625 lines for PAL or 525 lines for NTSC */
			maventv_regs[0x17] = m_timing.v_total / 4;
			maventv_regs[0x18] = m_timing.v_total & 3;

			/* calculate upper blanking range in field lines */
			upper = (m_timing.v_total - tv_target.timing.v_sync_end) >> 1;

			/* blank TVout above the line number calculated */
			maventv_regs[0x33] = upper - 1;

			/* set calculated vertical sync point */
//			DXIW(TVO_IDX, 0x82);
//			DXIW(TVO_DATA, (upper & 0xff));
//			DXIW(TVO_IDX, 0x83);
//			DXIW(TVO_DATA, ((upper >> 8) & 0xff));
			LOG(4,("MAVENTV: TV upper blanking range set is %d\n", upper));
		}

		/* set fized horizontal sync point */
//		DXIW(TVO_IDX, 0x84);
//		DXIW(TVO_DATA, 0x01);
//		DXIW(TVO_IDX, 0x85);
//		DXIW(TVO_DATA, 0x00);

		/* connect DAC1 to CON1, CRTC2/'DAC2' to CON2 (TVout mode) */
//		DXIW(OUTPUTCONN,0x0d); 
	}

	/* program new TVout mode */
	for (val = 0x00; val <= 0x3D; val++)
	{
/*		if (si->ps.card_type <= G400MAX)
		{
			i2c_maven_write(val, maventv_regs[val]);
		}
		else
*/		{
//			DXIW(TVO_IDX, val);
//			DXIW(TVO_DATA, maventv_regs[val]);
		}
	}

	/* leave mode-program mode */
//	if (si->ps.card_type <= G400MAX) MAVW(PGM, 0x00);
//	else
//	{
//		DXIW(TVO_IDX, ENMAV_PGM);
//		DXIW(TVO_DATA, 0x00);

		/* Select 2.0 Volt MAVEN DAC ref. so we have enough contrast/brightness range */
//		DXIW(GENIOCTRL, DXIR(GENIOCTRL) | 0x40);
//		DXIW(GENIODATA, 0x00);
//	}

	/* setup CRTC2 timing */
	head2_set_timing(tv_target);

	/* start whole thing if needed */
//	if (si->ps.card_type <= G400MAX) MAVW(RESYNC, 0x20);

	return 0;
}
