/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors for MGA driver:
	Mark Watson,
	Rudolf Cornelissen 9/2002
*/

#define MODULE_BIT 0x00400000

#include "acc_std.h"

#define	T_POSITIVE_SYNC	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)
#define MODE_FLAGS	(B_SCROLL | B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS)
#define MODE_COUNT (sizeof (mode_list) / sizeof (display_mode))

/*some monitors only handle a fixed set of modes*/
#include "valid_mode_list"

/*Standard VESA modes*/
static const display_mode mode_list[] = {
{ { 25175, 640, 656, 752, 800, 480, 490, 492, 525, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
{ { 27500, 640, 672, 768, 864, 480, 488, 494, 530, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* 640X480X60Hz */
{ { 30500, 640, 672, 768, 864, 480, 517, 523, 588, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* SVGA_640X480X60HzNI */
{ { 31500, 640, 664, 704, 832, 480, 489, 492, 520, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
{ { 31500, 640, 656, 720, 840, 480, 481, 484, 500, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
{ { 36000, 640, 696, 752, 832, 480, 481, 484, 509, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(640X480X8.Z1) */
{ { 38100, 800, 832, 960, 1088, 600, 602, 606, 620, 0}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* SVGA_800X600X56HzNI */
{ { 40000, 800, 840, 968, 1056, 600, 601, 605, 628, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(800X600X8.Z1) */
{ { 49500, 800, 816, 896, 1056, 600, 601, 604, 625, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(800X600X8.Z1) */
{ { 50000, 800, 856, 976, 1040, 600, 637, 643, 666, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(800X600X8.Z1) */
{ { 56250, 800, 832, 896, 1048, 600, 601, 604, 631, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(800X600X8.Z1) */
{ { 65000, 1024, 1048, 1184, 1344, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1024X768X8.Z1) */
{ { 75000, 1024, 1048, 1184, 1328, 768, 771, 777, 806, 0}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(1024X768X8.Z1) */
{ { 78750, 1024, 1040, 1136, 1312, 768, 769, 772, 800, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1024X768X8.Z1) */
{ { 94500, 1024, 1072, 1168, 1376, 768, 769, 772, 808, T_POSITIVE_SYNC}, B_CMAP8, 1024, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1024X768X8.Z1) */
{ { 94200, 1152, 1184, 1280, 1472, 864, 865, 868, 914, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
{ { 108000, 1152, 1216, 1344, 1550, 864, 865, 868, 900, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1152X864X8.Z1) */
{ { 120000, 1152, 1216, 1344, 1600, 864, 865, 868, 900, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1152X864X8.Z1) */
{ { 120000, 1152, 1216, 1344, 1568, 864, 865, 868, 911, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1152X864X8.Z1) */
{ { 108000, 1280, 1328, 1440, 1680, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X1024X8.Z1) */
{ { 135000, 1280, 1296, 1440, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1280X1024X8.Z1) */
{ { 157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1280X1024X8.Z1) */
{ { 162000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1600X1200X8.Z1) */
{ { 175500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@65Hz_(1600X1200X8.Z1) */
{ { 189000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1600X1200X8.Z1) */
{ { 202500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1600X1200X8.Z1) */
{ { 216000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
{ { 229500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}  /* Vesa_Monitor_@85Hz_(1600X1200X8.Z1) */
};

// apsed, adjust virtual width for CRTC offset constraints
// PB with MIl2 800*600 with bpp < 32
static status_t adjust_width (display_mode *target, bool want_same_width)
{
	uint32 video_pitch = target->virtual_width;
	uint32 multiple;
	
	if (si->ps.card_type < G100) switch (target->space & 0x0fff) { // MIL2
		case B_CMAP8: multiple = 128; break;
		case B_RGB15: multiple =  64; break;
		case B_RGB16: multiple =  64; break;
		case B_RGB24: multiple = 128; break;
		case B_RGB32: multiple =  32; break;
		default:
			LOG(8,("PROPOSEMODE: unknown color space: 0x%08x\n", target->space));
			return B_ERROR;
	} else switch (target->space & 0x0fff) { // G100, G200, G400 
		case B_CMAP8: multiple = 16; break;
		case B_RGB15: multiple =  8; break;
		case B_RGB16: multiple =  8; break;
		case B_RGB24: multiple = 16; break;
		case B_RGB32: multiple =  4; break;
		default:
			LOG(8,("PROPOSEMODE: unknown color space: 0x%08x\n", target->space));
			return B_ERROR;
	}
	video_pitch  = (video_pitch+multiple-1)/multiple;
	video_pitch *= multiple;
	if (target->virtual_width != video_pitch) {
		LOG(2,("PROPOSEMODE: color space 0x%08x, virtual_width %d adjusted to %d \n", 
			target->space, target->virtual_width, video_pitch));
		target->virtual_width = video_pitch;
		if (want_same_width) target->timing.h_display = video_pitch;
	}
	return B_OK;
}

/*Check mode is between low and high limits
	returns:
		B_OK - found one
		B_BAD_VALUE - mode can be made, but outside limits
		B_ERROR - not possible
*/
status_t PROPOSE_DISPLAY_MODE(display_mode *target, const display_mode *low, const display_mode *high) 
{
	status_t status;
	float pix_clock_found;
	uint8 m,n,p;
	status_t result = B_OK;
	uint32 row_bytes, limit_clock, max_vclk;
	double target_refresh = ((double)target->timing.pixel_clock * 1000.0) /
			(
				(double)target->timing.h_total * 
				(double)target->timing.v_total
			);
	bool
		want_same_width = target->timing.h_display == target->virtual_width,
		want_same_height = target->timing.v_display == target->virtual_height;

//	// apsed, adjust virtual width for CRTC offset constraints
//	status = adjust_width (target, want_same_width);
//	if (status != B_OK) return status;

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
			return B_ERROR;
		}
		else
		{
			target->timing=valid_mode_list[closest_mode_ptr];
			target_refresh = ((double)target->timing.pixel_clock * 1000.0) /  /*I require this refresh*/
			(
				(double)target->timing.h_total * 
				(double)target->timing.v_total
			);
		}
	}	
	#endif

	/*find a nearby valid timing from that given*/
	result = gx00_crtc_validate_timing
	(
		&target->timing.h_display, &target->timing.h_sync_start, &target->timing.h_sync_end, &target->timing.h_total,
		&target->timing.v_display, &target->timing.v_sync_start, &target->timing.v_sync_end, &target->timing.v_total
	);
	if (result == B_ERROR) return result;

	/*check if timing found is within the requested horizontal limits*/
	if (
		(target->timing.h_display < low->timing.h_display) ||
		(target->timing.h_display > high->timing.h_display) ||
		(target->timing.h_sync_start < low->timing.h_sync_start) ||
		(target->timing.h_sync_start > high->timing.h_sync_start) ||
		(target->timing.h_sync_end < low->timing.h_sync_end) ||
		(target->timing.h_sync_end > high->timing.h_sync_end) ||
		(target->timing.h_total < low->timing.h_total) ||
		(target->timing.h_total > high->timing.h_total)
	) result = B_BAD_VALUE;

	/*check if timing found is within the requested vertical limits*/
	if (
		(target->timing.v_display < low->timing.v_display) ||
		(target->timing.v_display > high->timing.v_display) ||
		(target->timing.v_sync_start < low->timing.v_sync_start) ||
		(target->timing.v_sync_start > high->timing.v_sync_start) || // apsed
		(target->timing.v_sync_end < low->timing.v_sync_end) ||
		(target->timing.v_sync_end > high->timing.v_sync_end) ||
		(target->timing.v_total < low->timing.v_total) ||
		(target->timing.v_total > high->timing.v_total)
	) result = B_BAD_VALUE;

	//rudolf
	/* adjust pixelclock for possible timing modifications done above */
	target->timing.pixel_clock = target_refresh * ((double)target->timing.h_total) * ((double)target->timing.v_total) / 1000.0;

	/* Now find the nearest valid pixelclock we actually can setup for the target mode,
	 * this also makes sure we don't generate more pixel bandwidth than the device can handle */
	if (si->ps.card_type >= G100)
	{
		/* calculate settings, but do not actually test anything (that costs too much time!) */
		status = gx00_dac_pix_pll_find(*target,&pix_clock_found,&m,&n,&p,0);
	}
	else
	{
		//fixme: implement pixelclock limits check (and modify if needed) for target mode inside mil2_find routine!
		//temp until then:
		limit_clock = si->ps.max_dac1_clock * 1000;
		if (target->timing.pixel_clock > limit_clock) target->timing.pixel_clock = limit_clock;

		status = mil2_dac_pix_pll_find((float)target->timing.pixel_clock/1000.0,&pix_clock_found,&m,&n,&p);
	}
	target->timing.pixel_clock = pix_clock_found*1000;
	//end rudolf
	
	/* note if we fell outside the limits */
	if (
		(target->timing.pixel_clock < low->timing.pixel_clock) ||
		(target->timing.pixel_clock > high->timing.pixel_clock)
	) result = B_BAD_VALUE;

	/* validate display vs. virtual */
	if ((target->timing.h_display > target->virtual_width) || want_same_width)
		target->virtual_width = target->timing.h_display;
	if ((target->timing.v_display > target->virtual_height) || want_same_height)
		target->virtual_height = target->timing.v_display;
	if (target->virtual_width > 4096)
		target->virtual_width = 4096;
	if (target->virtual_height > 2048)
		target->virtual_height = 2048;

	/* adjust virtual width for engine limitations - must be multiple of 8 */
	target->virtual_width = (target->virtual_width + 7) & ~7;
	if (
		(target->virtual_width < low->virtual_width) ||
		(target->virtual_width > high->virtual_width)
	) result = B_BAD_VALUE;

	// apsed, adjust virtual width for CRTC offset constraints
	status = adjust_width (target, want_same_width);
	if (status != B_OK) return status;

	/* calculate rowbytes after we've nailed the virtual width */
	switch (target->space & 0x0fff) {
		case B_CMAP8:
			row_bytes = 1;
			break;
		case B_RGB15:
		case B_RGB16:
			row_bytes = 2;
			break;
		case B_RGB32:
			row_bytes = 4;
			break;
		default:
			/* no amount of adjusting will fix not being able to support the pixel format */
			LOG(8,("PROPOSEMODE: unknown target space: 0x%08x\n", target->space));
			return B_ERROR;
	}
	row_bytes *= target->virtual_width;

	/* memory requirement for frame buffer */
	if ((row_bytes * target->virtual_height) > (si->ps.memory_size * 1024 * 1024))
		target->virtual_height = (si->ps.memory_size * 1024 * 1024) / row_bytes;
	if (target->virtual_height < target->timing.v_display) 
	{
		LOG(8,("PROPOSEMODE: virtual_height required %d < %d\n", target->virtual_height, target->timing.v_display));
		return B_ERROR;
	}
	else if (
		(target->virtual_height < low->virtual_height) ||
		(target->virtual_height > high->virtual_height)
	) result = B_BAD_VALUE;

	/* determine the 'would be' max. pixelclock for the second DAC for the current videomode if dualhead were activated */
	switch (target->space)
	{
		case B_CMAP8:
			max_vclk = si->ps.max_dac2_clock_8;
			break;
		case B_RGB15_LITTLE:
		case B_RGB16_LITTLE:
			max_vclk = si->ps.max_dac2_clock_16;
			break;
		case B_RGB24_LITTLE:
			max_vclk = si->ps.max_dac2_clock_24;
			break;
		case B_RGB32_LITTLE:
			max_vclk = si->ps.max_dac2_clock_32dh;
			break;
		default:
			/* use fail-safe value */
			max_vclk = si->ps.max_dac2_clock_32dh;
			break;
	}

	/*clear DUALHEAD_CAPABLE if any problems*/
	if 
	(
		((target->flags)& DUALHEAD_CAPABLE ) &&
		(
			(!si->ps.secondary_head) ||
			((1024 + (row_bytes * (target->virtual_height+1) * 2)) > (si->ps.memory_size * 1024 * 1024)) || /*note: extra line for maven vblank!*/
			(target->space == B_CMAP8) ||
			(target->space == B_RGB15_LITTLE) ||
			(target->timing.pixel_clock > (max_vclk * 1000))
		)
	)
	{
		target->flags&=~DUALHEAD_CAPABLE;
	}

	/*set tv capable suitable*/
	if (target->flags&DUALHEAD_CAPABLE)
	{
		if ((target->timing.pixel_clock <= 120000 ) && (target->timing.pixel_clock >= 40000))
		{
			target->flags|=TV_CAPABLE;
		}
	}

	return result;
}

/* Return the number of modes this device will return from GET_MODE_LIST().  
	This is precalculated in create_mode_list (called from InitAccelerant stuff)
*/
uint32 ACCELERANT_MODE_COUNT(void) {
	return si->mode_count;
}

/* Copy the list of guaranteed supported video modes to the location provided.*/
status_t GET_MODE_LIST(display_mode *dm) {
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

	color_space spaces[4] = {B_RGB32_LITTLE,B_RGB16_LITTLE,B_RGB15_LITTLE,B_CMAP8};

	/* figure out how big the list could be, and adjust up to nearest multiple of B_PAGE_SIZE*/
	max_size = (((MODE_COUNT * 4) * sizeof(display_mode)) + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);
	/* create an area to hold the info */
	si->mode_area = my_mode_list_area =
		create_area("G400 accelerant mode info", (void **)&my_mode_list, B_ANY_ADDRESS, max_size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
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
		/* some cards need wider virtual widths for certain modes */
		high.virtual_width = 4096;
		/* do it once for each depth we want to support */
		for (j = 0; j < (sizeof(spaces) / sizeof(color_space)); j++) 
		{
			/* set target values for single head (propose will return if capable)*/
			*dst = *src;
			/* poke the specific space*/
			dst->space = low.space = high.space = spaces[j];
			dst->flags |= DUALHEAD_CAPABLE;
			
			dst->flags |= B_SUPPORTS_OVERLAYS;
			//fixme: we need to distinquish somehow between head 1 and head 2, as overlay only works on head 1... 
			
			/* ask for a compatible mode */
			if (PROPOSE_DISPLAY_MODE(dst, &low, &high) != B_ERROR) {
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
