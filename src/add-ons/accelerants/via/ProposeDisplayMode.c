/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors for NV driver:
	Mark Watson,
	Rudolf Cornelissen 9/2002-11/2005
*/

#define MODULE_BIT 0x00400000

#include "acc_std.h"

#define	T_POSITIVE_SYNC	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)
/* mode flags will be setup as status info by PROPOSEMODE! */
#define MODE_FLAGS 0
#define MODE_COUNT (sizeof (mode_list) / sizeof (display_mode))

/*some monitors only handle a fixed set of modes*/
#include "valid_mode_list"

/* Standard VESA modes,
 * plus panel specific resolution modes which are internally modified during run-time depending on the requirements of the actual
 * panel connected. The modes as listed here, should timing-wise be as compatible with analog (CRT) monitors as can be... */
static const display_mode mode_list[] = {
/* 4:3 modes; 307.2k pixels */
{ { 25175, 640, 656, 752, 800, 480, 490, 492, 525, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
{ { 27500, 640, 672, 768, 864, 480, 488, 494, 530, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* 640X480X60Hz */
{ { 30500, 640, 672, 768, 864, 480, 517, 523, 588, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* SVGA_640X480X60HzNI */
{ { 31500, 640, 664, 704, 832, 480, 489, 492, 520, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
{ { 31500, 640, 656, 720, 840, 480, 481, 484, 500, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
{ { 36000, 640, 696, 752, 832, 480, 481, 484, 509, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(640X480X8.Z1) */
/* 4:3 modes; 480k pixels */
{ { 36000, 800, 824, 896, 1024, 600, 601, 603, 625, 0}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@56Hz_(800X600) from Be, Inc. driver + XFree86 */
{ { 38100, 800, 832, 960, 1088, 600, 602, 606, 620, 0}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* SVGA_800X600X56HzNI */
{ { 40000, 800, 840, 968, 1056, 600, 601, 605, 628, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(800X600X8.Z1) + XFree86 */
{ { 49500, 800, 816, 896, 1056, 600, 601, 604, 625, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(800X600X8.Z1) + XFree86 */
{ { 50000, 800, 856, 976, 1040, 600, 637, 643, 666, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(800X600X8.Z1) + XFree86 */
{ { 56250, 800, 832, 896, 1048, 600, 601, 604, 631, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(800X600X8.Z1) + XFree86 */
/* 4:3 modes; 786.432k pixels */
{ { 65000, 1024, 1048, 1184, 1344, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1024X768X8.Z1) + XFree86 */
{ { 75000, 1024, 1048, 1184, 1328, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(1024X768X8.Z1) + XFree86 */
{ { 78750, 1024, 1040, 1136, 1312, 768, 769, 772, 800, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1024X768X8.Z1) + XFree86 */
{ { 94500, 1024, 1072, 1168, 1376, 768, 769, 772, 808, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1024X768X8.Z1) + XFree86 */
/* 4:3 modes; 995.328k pixels */
{ { 94200, 1152, 1184, 1280, 1472, 864, 865, 868, 914, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
{ { 97800, 1152, 1216, 1344, 1552, 864, 865, 868, 900, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
{ { 108000, 1152, 1216, 1344, 1600, 864, 865, 868, 900, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1152X864X8.Z1) + XFree86 */
{ { 121500, 1152, 1216, 1344, 1568, 864, 865, 868, 911, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1152X864X8.Z1) */
/* 5:4 modes; 1.311M pixels */
{ { 108000, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X1024) from Be, Inc. driver + XFree86 */
{ { 135000, 1280, 1296, 1440, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1280X1024X8.Z1) + XFree86 */
{ { 157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1280X1024X8.Z1) + XFree86 */
/* 4:3 panel mode; 1.47M pixels */
{ { 122600, 1400, 1488, 1640, 1880, 1050, 1051, 1054, 1087, T_POSITIVE_SYNC}, B_CMAP8, 1400, 1050, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1400X1050) */
/* 4:3 modes; 1.92M pixels */
{ { 162000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1600X1200X8.Z1) + XFree86 */
/* identical lines to above one, apart from refreshrate.. */
{ { 175500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@65Hz_(1600X1200X8.Z1) + XFree86 */
{ { 189000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1600X1200X8.Z1) + XFree86 */
{ { 202500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1600X1200X8.Z1) + XFree86 */
{ { 216000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
{ { 229500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS},  /* Vesa_Monitor_@85Hz_(1600X1200X8.Z1) + XFree86 */
/* end identical lines. */
/* 4:3 modes; 2.408M pixels */
{ { 204750, 1792, 1920, 2120, 2448, 1344, 1345, 1348, 1394, B_POSITIVE_VSYNC}, B_CMAP8, 1792, 1344, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1792X1344) from Be, Inc. driver + XFree86 */
{ { 261000, 1792, 1888, 2104, 2456, 1344, 1345, 1348, 1417, B_POSITIVE_VSYNC}, B_CMAP8, 1792, 1344, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1792X1344) from Be, Inc. driver + XFree86 */
/* 4:3 modes; 2.584M pixels */
{ { 218250, 1856, 1952, 2176, 2528, 1392, 1393, 1396, 1439, B_POSITIVE_VSYNC}, B_CMAP8, 1856, 1392, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1856X1392) from Be, Inc. driver + XFree86 */
{ { 288000, 1856, 1984, 2208, 2560, 1392, 1393, 1396, 1500, B_POSITIVE_VSYNC}, B_CMAP8, 1856, 1392, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1856X1392) from Be, Inc. driver + XFree86 */
/* 4:3 modes; 2.765M pixels */
{ { 234000, 1920, 2048, 2256, 2600, 1440, 1441, 1444, 1500, B_POSITIVE_VSYNC}, B_CMAP8, 1920, 1440, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1920X1440) from Be, Inc. driver + XFree86 */
{ { 297000, 1920, 2064, 2288, 2640, 1440, 1441, 1444, 1500, B_POSITIVE_VSYNC}, B_CMAP8, 1920, 1440, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1920X1440) from Be, Inc. driver + XFree86 */
/* 4:3 modes; 3.146M pixels */
{ { 266950, 2048, 2200, 2424, 2800, 1536, 1537, 1540, 1589, B_POSITIVE_VSYNC}, B_CMAP8, 2048, 1536, 0, 0, MODE_FLAGS}, /* From XFree86 posting @60Hz + XFree86 */
/* 16:10 panel mode; 400k pixels */
{ { 31300, 800, 848, 928, 1008, 500, 501, 504, 518, T_POSITIVE_SYNC}, B_CMAP8, 800, 500, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(800X500) */
/* 16:10 panel mode; 655.36k pixels */
{ { 52800, 1024, 1072, 1176, 1328, 640, 641, 644, 663, T_POSITIVE_SYNC}, B_CMAP8, 1024, 640, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1024X640) */
/* 16:10 panel-TV mode; 983.04k pixels */
{ { 80135, 1280, 1344, 1480, 1680, 768, 769, 772, 795, T_POSITIVE_SYNC}, B_CMAP8, 1280, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X768) */
/* 16:10 panel mode; 1.024M pixels */
{ { 83500, 1280, 1344, 1480, 1680, 800, 801, 804, 828, T_POSITIVE_SYNC}, B_CMAP8, 1280, 800, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X800) */
/* 16:10 panel mode; 1.296M pixels */
{ { 106500, 1440, 1520, 1672, 1904, 900, 901, 904, 932, T_POSITIVE_SYNC}, B_CMAP8, 1440, 900, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1440X900) */
/* 16:10 panel mode; 1.764M pixels */
{ { 147100, 1680, 1784, 1968, 2256, 1050, 1051, 1054, 1087, T_POSITIVE_SYNC}, B_CMAP8, 1680, 1050, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1680X1050) */
/* 16:10 panel mode; 2.304M pixels */
{ { 193200, 1920, 2048, 2256, 2592, 1200, 1201, 1204, 1242, T_POSITIVE_SYNC}, B_CMAP8, 1920, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1920X1200) */
};

/*
Check mode is between low and high limits
returns:
	B_OK - found one
	B_BAD_VALUE - mode can be made, but outside limits
	B_ERROR - not possible
*/
/* BOUNDS WARNING:
 * BeOS (tested R5.0.3PE) is failing BWindowScreen.SetFrameBuffer() if PROPOSEMODE
 * returns B_BAD_VALUE. It's called by the OS with target, low and high set to
 * have the same settings for BWindowScreen!
 * Which means we should not return B_BAD_VALUE on anything except for deviations on:
 * display_mode.virtual_width;
 * display_mode.virtual_height;
 * display_mode.timing.h_display;
 * display_mode.timing.v_display;
 */
/* Note:
 * The target mode should be modified to correspond to the mode as it can be made. */
status_t PROPOSE_DISPLAY_MODE(display_mode *target, const display_mode *low, const display_mode *high) 
{
	status_t status = B_OK;
	float pix_clock_found, target_aspect;
	uint8 m,n,p, bpp;
	status_t result;
	uint32 max_vclk, row_bytes, pointer_reservation;
	bool acc_mode;
	double target_refresh = ((double)target->timing.pixel_clock * 1000.0) /
			(
				(double)target->timing.h_total * 
				(double)target->timing.v_total
			);
	bool
		want_same_width = target->timing.h_display == target->virtual_width,
		want_same_height = target->timing.v_display == target->virtual_height;

	LOG(1, ("PROPOSEMODE: (ENTER) requested virtual_width %d, virtual_height %d\n",
									target->virtual_width, target->virtual_height));

	/*check valid list:
		if (VALID_REQUIRED is set)
		{
			if (find modes with same size)
			{
				pick one with nearest pixel clock
			}
			else
			{
				pick next largest with nearest pixel clock and modify visible portion as far as possible
			}
		}
	*/
	#ifdef VALID_MODE_REQUIRED
	{
		int i;
		int closest_mode_ptr;
		uint32 closest_mode_clock;

		LOG(1, ("PROPOSEMODE: valid mode required!\n"));

		closest_mode_ptr = 0xbad;
		closest_mode_clock = 0;
		for (i=0;i<VALID_MODES;i++)
		{
			/*check size is ok and clock is better than any found before*/
			if(
				target->timing.h_display==valid_mode_list[i].h_display &&
				target->timing.v_display==valid_mode_list[i].v_display
			)
			{
				if (
					abs(valid_mode_list[i].pixel_clock-target->timing.pixel_clock)<
					abs(closest_mode_clock-target->timing.pixel_clock)
				)
				{
					closest_mode_clock=valid_mode_list[i].pixel_clock;
					closest_mode_ptr=i;
				}
			}
		}

		if (closest_mode_ptr==0xbad)/*if no modes of correct size*/
		{
			LOG(4, ("PROPOSEMODE: no valid mode found, aborted.\n"));
			return B_ERROR;
		}
		else
		{
			target->timing=valid_mode_list[closest_mode_ptr];
			target_refresh = ((double)target->timing.pixel_clock * 1000.0) /  /*I require this refresh*/
			((double)target->timing.h_total * (double)target->timing.v_total);
		}
	}	
	#endif

	/*find a nearby valid timing from that given*/
	result = head1_validate_timing
	(
		&target->timing.h_display, &target->timing.h_sync_start, &target->timing.h_sync_end, &target->timing.h_total,
		&target->timing.v_display, &target->timing.v_sync_start, &target->timing.v_sync_end, &target->timing.v_total
	);
	if (result == B_ERROR)
	{
		LOG(4, ("PROPOSEMODE: could not validate timing, aborted.\n"));
		return result;
	}

	/* check if all connected output devices can display the requested mode's aspect: */
	/* calculate display mode aspect */
	target_aspect = (target->timing.h_display / ((float)target->timing.v_display));
	/* NOTE:
	 * allow 0.10 difference so 5:4 aspect panels will be able to use 4:3 aspect modes! */
	switch (si->ps.monitors)
	{
	case 0x01: /* digital panel on head 1, nothing on head 2 */
		if (si->ps.panel1_aspect < (target_aspect - 0.10))
		{
			LOG(4, ("PROPOSEMODE: connected panel1 is not widescreen type, aborted.\n"));
			return B_ERROR;
		}
		break;
	case 0x10: /* nothing on head 1, digital panel on head 2 */
		if (si->ps.panel2_aspect < (target_aspect - 0.10))
		{
			LOG(4, ("PROPOSEMODE: connected panel2 is not widescreen type, aborted.\n"));
			return B_ERROR;
		}
		break;
	case 0x11: /* digital panels on both heads */
		if ((si->ps.panel1_aspect < (target_aspect - 0.10)) ||
			(si->ps.panel2_aspect < (target_aspect - 0.10)))
		{
			LOG(4, ("PROPOSEMODE: not all connected panels are widescreen type, aborted.\n"));
			return B_ERROR;
		}
		break;
	default: /* at least one analog monitor is connected, or nothing detected at all */
		if (target_aspect > 1.34)
		{
			LOG(4, ("PROPOSEMODE: not all output devices can display widescreen modes, aborted.\n"));
			return B_ERROR;
		}
		break;
	}

	/* only export widescreen panel-TV modes when an exact resolution match exists,
	 * to prevent the modelist from becoming too crowded */
	if (target_aspect > 1.61)
	{
		status_t panel_TV_stat = B_ERROR;

		if (si->ps.tmds1_active)
		{
			if ((target->timing.h_display == si->ps.p1_timing.h_display) &&
				(target->timing.v_display == si->ps.p1_timing.v_display))
			{
				panel_TV_stat = B_OK;
			}
		}
		if (si->ps.tmds2_active)
		{
			if ((target->timing.h_display == si->ps.p2_timing.h_display) &&
				(target->timing.v_display == si->ps.p2_timing.v_display))
			{
				panel_TV_stat = B_OK;
			}
		}
		if (panel_TV_stat != B_OK)
		{
			LOG(4, ("PROPOSEMODE: WS panel_TV mode requested but no such TV here, aborted.\n"));
			return B_ERROR;
		}
	}

	/* check if panel(s) can display the requested resolution (if connected) */
	if (si->ps.tmds1_active)
	{
		if ((target->timing.h_display > si->ps.p1_timing.h_display) ||
			(target->timing.v_display > si->ps.p1_timing.v_display))
		{
			LOG(4, ("PROPOSEMODE: panel1 can't display requested resolution, aborted.\n"));
			return B_ERROR;
		}
	}
	if (si->ps.tmds2_active)
	{
		if ((target->timing.h_display > si->ps.p2_timing.h_display) ||
			(target->timing.v_display > si->ps.p2_timing.v_display))
		{
			LOG(4, ("PROPOSEMODE: panel2 can't display requested resolution, aborted.\n"));
			return B_ERROR;
		}
	}

	/* validate display vs. virtual */
	if ((target->timing.h_display > target->virtual_width) || want_same_width)
		target->virtual_width = target->timing.h_display;
	if ((target->timing.v_display > target->virtual_height) || want_same_height)
		target->virtual_height = target->timing.v_display;

	/* nail virtual size and 'subsequently' calculate rowbytes */
	result = eng_general_validate_pic_size (target, &row_bytes, &acc_mode);
	if (result == B_ERROR)
	{
		LOG(4, ("PROPOSEMODE: could not validate virtual picture size, aborted.\n"));
		return result;
	}

	/*check if virtual_width is still within the requested limits*/
	if ((target->virtual_width < low->virtual_width) ||
		(target->virtual_width > high->virtual_width))
	{
		status = B_BAD_VALUE;
		LOG(4, ("PROPOSEMODE: WARNING: virtual_width deviates too much\n"));
	}

	/*check if timing found is within the requested horizontal limits*/
	if ((target->timing.h_display < low->timing.h_display) ||
		(target->timing.h_display > high->timing.h_display) ||
		(target->timing.h_sync_start < low->timing.h_sync_start) ||
		(target->timing.h_sync_start > high->timing.h_sync_start) ||
		(target->timing.h_sync_end < low->timing.h_sync_end) ||
		(target->timing.h_sync_end > high->timing.h_sync_end) ||
		(target->timing.h_total < low->timing.h_total) ||
		(target->timing.h_total > high->timing.h_total))
	{
		/* BWindowScreen workaround: we accept everything except h_display deviations */
		if ((target->timing.h_display < low->timing.h_display) ||
			(target->timing.h_display > high->timing.h_display))
		{		
			status = B_BAD_VALUE;
		}
		LOG(4, ("PROPOSEMODE: WARNING: horizontal timing deviates too much\n"));
	}

	/*check if timing found is within the requested vertical limits*/
	if (
		(target->timing.v_display < low->timing.v_display) ||
		(target->timing.v_display > high->timing.v_display) ||
		(target->timing.v_sync_start < low->timing.v_sync_start) ||
		(target->timing.v_sync_start > high->timing.v_sync_start) ||
		(target->timing.v_sync_end < low->timing.v_sync_end) ||
		(target->timing.v_sync_end > high->timing.v_sync_end) ||
		(target->timing.v_total < low->timing.v_total) ||
		(target->timing.v_total > high->timing.v_total)
	)
	{
		/* BWindowScreen workaround: we accept everything except v_display deviations */
		if ((target->timing.v_display < low->timing.v_display) ||
			(target->timing.v_display > high->timing.v_display))
		{		
			status = B_BAD_VALUE;
		}
		LOG(4, ("PROPOSEMODE: WARNING: vertical timing deviates too much\n"));
	}

	/* adjust pixelclock for possible timing modifications done above */
	target->timing.pixel_clock = target_refresh * ((double)target->timing.h_total) * ((double)target->timing.v_total) / 1000.0;

	/* Now find the nearest valid pixelclock we actually can setup for the target mode,
	 * this also makes sure we don't generate more pixel bandwidth than the device can handle */
	/* calculate settings, but do not actually test anything (that costs too much time!) */
	result = head1_pix_pll_find(*target,&pix_clock_found,&m,&n,&p,0);
	/* update the target mode */
	target->timing.pixel_clock = (pix_clock_found * 1000);	

	/* note if we fell outside the limits */
	if ((target->timing.pixel_clock < low->timing.pixel_clock) ||
		(target->timing.pixel_clock > high->timing.pixel_clock)
	)
	{
		/* BWindowScreen workaround: we accept deviations <= 1Mhz */
		if ((target->timing.pixel_clock < (low->timing.pixel_clock - 1000)) ||
			(target->timing.pixel_clock > (high->timing.pixel_clock + 1000)))
		{
			status = B_BAD_VALUE;
		}
		LOG(4, ("PROPOSEMODE: WARNING: pixelclock deviates too much\n"));
	}

	/* checkout space needed for hardcursor (if any) */
	pointer_reservation = 0;
	if (si->settings.hardcursor) pointer_reservation = 2048;
	/* memory requirement for frame buffer */
	if ((row_bytes * target->virtual_height) >
		(si->ps.memory_size - pointer_reservation))
	{
		target->virtual_height = 
			(si->ps.memory_size - pointer_reservation) / row_bytes;
	}
	if (target->virtual_height < target->timing.v_display) 
	{
		LOG(4,("PROPOSEMODE: not enough memory for current mode, aborted.\n"));
		return B_ERROR;
	}
	LOG(4,("PROPOSEMODE: validated virtual_width %d, virtual_height %d pixels\n",
		target->virtual_width, target->virtual_height));

	if ((target->virtual_height < low->virtual_height) ||
		(target->virtual_height > high->virtual_height))
	{
		status = B_BAD_VALUE;
		LOG(4, ("PROPOSEMODE: WARNING: virtual_height deviates too much\n"));
	}

	/* setup status flags */ 
	LOG(1, ("PROPOSEMODE: initial modeflags: $%08x\n", target->flags));
	/* preset to singlehead card without TVout, no overlay support and no hardcursor.
	 * also advice system that app_server and acc engine may touch the framebuffer
	 * simultaneously (fixed). */
	target->flags &=
		~(DUALHEAD_CAPABLE | TV_CAPABLE | B_SUPPORTS_OVERLAYS | B_HARDWARE_CURSOR | B_IO_FB_NA);
	/* we always allow parallel access (fixed), the DAC is always in 'enhanced'
	 * mode (fixed), and all modes support DPMS (fixed);
	 * We support scrolling and panning in every mode, so we 'send a signal' to
	 * BWindowScreen.CanControlFrameBuffer() by setting B_SCROLL.  */
	/* BTW: B_PARALLEL_ACCESS in combination with a hardcursor enables
	 * BDirectWindow windowed modes. */
	target->flags |= (B_PARALLEL_ACCESS | B_8_BIT_DAC | B_DPMS | B_SCROLL);

	/* determine the 'would be' max. pixelclock for the second DAC for the current videomode if dualhead were activated */
	switch (target->space)
	{
		case B_CMAP8:
			max_vclk = si->ps.max_dac2_clock_8;
			bpp = 1;
			break;
		case B_RGB15_LITTLE:
		case B_RGB16_LITTLE:
			max_vclk = si->ps.max_dac2_clock_16;
			bpp = 2;
			break;
		case B_RGB24_LITTLE:
			max_vclk = si->ps.max_dac2_clock_24;
			bpp = 3;
			break;
		case B_RGB32_LITTLE:
			max_vclk = si->ps.max_dac2_clock_32dh;
			bpp = 4;
			break;
		default:
			/* use fail-safe value */
			max_vclk = si->ps.max_dac2_clock_32dh;
			bpp = 4;
			break;
	}

	/* set DUALHEAD_CAPABLE if suitable */
	//fixme: update for independant secondary head use! (reserve fixed memory then)
	if (si->ps.secondary_head && (target->timing.pixel_clock <= (max_vclk * 1000)))
	{
		switch (target->flags & DUALHEAD_BITS)
		{
		case DUALHEAD_ON:
		case DUALHEAD_SWITCH:
			if (((si->ps.memory_size - pointer_reservation) >=
					(row_bytes * target->virtual_height)) &&
			 	((uint16)(row_bytes / bpp) >= (target->timing.h_display * 2)))
			{
				target->flags |= DUALHEAD_CAPABLE;
			}
			break;
		case DUALHEAD_CLONE:
			if ((si->ps.memory_size - pointer_reservation) >=
					(row_bytes * target->virtual_height))
			{
				target->flags |= DUALHEAD_CAPABLE;
			}
			break;
		case DUALHEAD_OFF:
			if ((si->ps.memory_size - pointer_reservation) >=
					(row_bytes * target->virtual_height * 2))
			{
				target->flags |= DUALHEAD_CAPABLE;
			}
			break;
		}
	}

	/* set TV_CAPABLE if suitable: pixelclock is not important (defined by TVstandard) */
	//fixme: modify for G100 and G200 TVout later on...
	if (target->flags & DUALHEAD_CAPABLE)
	{
		if (si->ps.tvout &&
			(target->timing.h_display <= 1024) &&
			(target->timing.v_display <= 768))
		{
			target->flags |= TV_CAPABLE;
		}
	}

	/* set HARDWARE_CURSOR mode if suitable */
	if (si->settings.hardcursor)
		target->flags |= B_HARDWARE_CURSOR;

	/* set SUPPORTS_OVERLAYS */
	target->flags |= B_SUPPORTS_OVERLAYS;

	LOG(1, ("PROPOSEMODE: validated status modeflags: $%08x\n", target->flags));

	/* overrule timing command flags to be (fixed) blank_pedestal = 0.0IRE,
	 * progressive scan (fixed), and sync_on_green not avaible. */
	target->timing.flags &= ~(B_BLANK_PEDESTAL | B_TIMING_INTERLACED | B_SYNC_ON_GREEN);
	/* The HSYNC and VSYNC command flags are actually executed by the driver. */

	if (status == B_OK)	LOG(4, ("PROPOSEMODE: completed successfully.\n"));
	else LOG(4, ("PROPOSEMODE: mode can be made, but outside given limits.\n"));
	return status;
}

/* Return the number of modes this device will return from GET_MODE_LIST().  
	This is precalculated in create_mode_list (called from InitAccelerant stuff)
*/
uint32 ACCELERANT_MODE_COUNT(void)
{
	LOG(1, ("ACCELERANT_MODE_COUNT: the modelist contains %d modes\n",si->mode_count));

	return si->mode_count;
}

/* Copy the list of guaranteed supported video modes to the location provided.*/
status_t GET_MODE_LIST(display_mode *dm)
{
	LOG(1, ("GET_MODE_LIST: exporting the modelist created before.\n"));

	memcpy(dm, my_mode_list, si->mode_count * sizeof(display_mode));
	return B_OK;
}

/* Create a list of display_modes to pass back to the caller.*/
status_t create_mode_list(void)
{
	size_t max_size;
	uint32
		i, j,
		pix_clk_range;
	const display_mode
		*src;
	display_mode
		*dst,
		low,
		high;

	color_space spaces[4] = {B_RGB32_LITTLE,B_RGB16_LITTLE,B_RGB15_LITTLE,B_CMAP8};

	/* figure out how big the list could be, and adjust up to nearest multiple of B_PAGE_SIZE */
	max_size = (((MODE_COUNT * 4) * sizeof(display_mode)) + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);
	/* create an area to hold the info */
	si->mode_area = my_mode_list_area =
		create_area("NV accelerant mode info", (void **)&my_mode_list, B_ANY_ADDRESS, max_size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (my_mode_list_area < B_OK) return my_mode_list_area;

	/* walk through our predefined list and see which modes fit this device */
	src = mode_list;
	dst = my_mode_list;
	si->mode_count = 0;
	for (i = 0; i < MODE_COUNT; i++)
	{
		/* set ranges for acceptable values */
		low = high = *src;
		/* range is 6.25% of default clock: arbitrarily picked */ 
		pix_clk_range = low.timing.pixel_clock >> 5;
		low.timing.pixel_clock -= pix_clk_range;
		high.timing.pixel_clock += pix_clk_range;
		/* 'some cards need wider virtual widths for certain modes':
		 * Not true. They might need a wider pitch, but this is _not_ reflected in
		 * virtual_width, but in fbc.bytes_per_row. */
		//So disable next line: 
		//high.virtual_width = 4096;
		/* do it once for each depth we want to support */
		for (j = 0; j < (sizeof(spaces) / sizeof(color_space)); j++) 
		{
			/* set target values */
			*dst = *src;
			/* poke the specific space */
			dst->space = low.space = high.space = spaces[j];
			/* ask for a compatible mode */
			/* We have to check for B_OK, because otherwise the pix_clk_range
			 * won't be taken into account!! */
			//So don't do this: 
			//if (PROPOSE_DISPLAY_MODE(dst, &low, &high) != B_ERROR) {
			//Instead, do this:
			if (PROPOSE_DISPLAY_MODE(dst, &low, &high) == B_OK) {
				/* count it, and move on to next mode */
				dst++;
				si->mode_count++;
			}
		}
		/* advance to next mode */
		src++;
	}

	return B_OK;
}
