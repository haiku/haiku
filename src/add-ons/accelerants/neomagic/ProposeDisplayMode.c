/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors for nm driver:
	Rudolf Cornelissen 4/2003-1/2006
*/

#define MODULE_BIT 0x00400000

#include "acc_std.h"

#define	T_POSITIVE_SYNC	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)
/* mode flags will be setup as status info by PROPOSEMODE! */
#define MODE_FLAGS 0
#define MODE_COUNT (sizeof (mode_list) / sizeof (display_mode))

/*some monitors only handle a fixed set of modes*/
#include "valid_mode_list"

/* Standard VESA modes within NM2380 range, which is the high-end supported NeoMagic card */
static const display_mode mode_list[] = {
{ { 25175, 640, 656, 752, 800, 480, 490, 492, 525, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
{ { 27500, 640, 672, 768, 864, 480, 488, 494, 530, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* 640X480X60Hz */
{ { 30500, 640, 672, 768, 864, 480, 517, 523, 588, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* SVGA_640X480X60HzNI */
{ { 31500, 640, 664, 704, 832, 480, 489, 492, 520, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
{ { 31500, 640, 656, 720, 840, 480, 481, 484, 500, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
{ { 36000, 640, 696, 752, 832, 480, 481, 484, 509, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(640X480X8.Z1) */
{ { 36000, 800, 824, 896, 1024, 600, 601, 603, 625, 0}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@56Hz_(800X600) from Be, Inc. driver + XFree86 */
{ { 38100, 800, 832, 960, 1088, 600, 602, 606, 620, 0}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* SVGA_800X600X56HzNI */
{ { 40000, 800, 840, 968, 1056, 600, 601, 605, 628, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(800X600X8.Z1) + XFree86 */
{ { 49500, 800, 816, 896, 1056, 600, 601, 604, 625, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(800X600X8.Z1) + XFree86 */
{ { 50000, 800, 856, 976, 1040, 600, 637, 643, 666, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(800X600X8.Z1) + XFree86 */
{ { 56250, 800, 832, 896, 1048, 600, 601, 604, 631, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(800X600X8.Z1) + XFree86 */
{ { 65000, 1024, 1048, 1184, 1344, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1024X768X8.Z1) + XFree86 */
{ { 75000, 1024, 1048, 1184, 1328, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(1024X768X8.Z1) + XFree86 */
{ { 78750, 1024, 1040, 1136, 1312, 768, 769, 772, 800, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1024X768X8.Z1) + XFree86 */
{ { 94500, 1024, 1072, 1168, 1376, 768, 769, 772, 808, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1024X768X8.Z1) + XFree86 */
{ { 94200, 1152, 1184, 1280, 1472, 864, 865, 868, 914, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
{ { 97800, 1152, 1216, 1344, 1552, 864, 865, 868, 900, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
{ { 108000, 1152, 1216, 1344, 1600, 864, 865, 868, 900, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1152X864X8.Z1) + XFree86 */
{ { 108000, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X1024) from Be, Inc. driver + XFree86 */
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
	float pix_clock_found;
	uint8 m,n,p;
	status_t result;
	uint32 row_bytes, pointer_reservation;
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

	/* NM2070 cannot do 24 bit color */
	if ((si->ps.card_type == NM2070) && (target->space == B_RGB24_LITTLE))
	{
		LOG(4, ("PROPOSEMODE: 24bit color not supported on NM2070, aborted.\n"));
		return B_ERROR;
	}

	/*find a nearby valid timing from that given*/
	result = nm_crtc_validate_timing
	(
		&target->timing.h_display, &target->timing.h_sync_start, &target->timing.h_sync_end, &target->timing.h_total,
		&target->timing.v_display, &target->timing.v_sync_start, &target->timing.v_sync_end, &target->timing.v_total
	);
	if (result == B_ERROR)
	{
		LOG(4, ("PROPOSEMODE: could not validate timing, aborted.\n"));
		return result;
	}

	/* validate display vs. virtual */
	if ((target->timing.h_display > target->virtual_width) || want_same_width)
		target->virtual_width = target->timing.h_display;
	if ((target->timing.v_display > target->virtual_height) || want_same_height)
		target->virtual_height = target->timing.v_display;

	/* nail virtual size and 'subsequently' calculate rowbytes */
	result = nm_general_validate_pic_size (target, &row_bytes, &acc_mode);
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
	result = nm_dac_pix_pll_find(*target,&pix_clock_found,&m,&n,&p);
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
	if (si->settings.hardcursor) pointer_reservation = si->ps.curmem_size;
	/* memory requirement for frame buffer */
	if ((row_bytes * target->virtual_height) >
		((si->ps.memory_size * 1024) - pointer_reservation))
	{
		target->virtual_height = 
			((si->ps.memory_size * 1024) - pointer_reservation) / row_bytes;
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
//fixme: introduce dualhead_clone_only flag compatible with matrox so the same prefs
//util can be used
	target->flags &=
		~(DUALHEAD_CAPABLE | TV_CAPABLE | B_SUPPORTS_OVERLAYS | B_HARDWARE_CURSOR | B_IO_FB_NA);
	/* we always allow parallel access (fixed), the DAC is always in 'enhanced'
	 * mode (fixed), and all modes support DPMS (fixed);
	 * We support scrolling and panning in every mode, so we 'send a signal' to
	 * BWindowScreen.CanControlFrameBuffer() by setting B_SCROLL.  */
	/* BTW: B_PARALLEL_ACCESS in combination with a hardcursor enables
	 * BDirectWindow windowed modes. */
	target->flags |= (B_PARALLEL_ACCESS | B_8_BIT_DAC | B_DPMS | B_SCROLL);

	/* if not dualhead capable card clear dualhead flags */
	if (!(target->flags & DUALHEAD_CAPABLE))
	{
		target->flags &= ~DUALHEAD_BITS;
	}

	/* if not TVout capable card clear TVout flags */
	if (!(target->flags & TV_CAPABLE))
	{
		target->flags &= ~TV_BITS;
	}

	/* TVout is on primary head (presumably, if we'd have that) */
	target->flags |= TV_PRIMARY;

	/* set HARDWARE_CURSOR mode if suitable */
	if (si->settings.hardcursor)
		target->flags |= B_HARDWARE_CURSOR;

	/* set SUPPORTS_OVERLAYS if suitable */
	if (si->ps.card_type > NM2070)
		target->flags |= B_SUPPORTS_OVERLAYS;

	LOG(1, ("PROPOSEMODE: validated modeflags: $%08x\n", target->flags));

	/* overrule timing command flags to be (fixed) blank_pedestal = 0.0IRE,
	 * progressive scan (fixed), and sync_on_green not used */
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
status_t create_mode_list(void) {
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

	color_space spaces[4] = {B_RGB24_LITTLE,B_RGB16_LITTLE,B_RGB15_LITTLE,B_CMAP8};

	/* figure out how big the list could be, and adjust up to nearest multiple of B_PAGE_SIZE*/
	max_size = (((MODE_COUNT * 4) * sizeof(display_mode)) + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);
	/* create an area to hold the info */
	si->mode_area = my_mode_list_area =
		create_area("nm accelerant mode info", (void **)&my_mode_list, B_ANY_ADDRESS, max_size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (my_mode_list_area < B_OK) return my_mode_list_area;

	/* walk through our predefined list and see which modes fit this device */
	src = mode_list;
	dst = my_mode_list;
	si->mode_count = 0;
	for (i = 0; i < MODE_COUNT; i++) {
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
