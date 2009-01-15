/*
	Copyright (c) 2002-2004, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Everything concerning getting/testing display modes
*/

#include "radeon_accelerant.h"
#include "generic.h"
#include <string.h>
#include "GlobalData.h"

#include "crtc_regs.h"
#include "utils.h"
#include "set_mode.h"

// standard mode list
// all drivers contain this list - this should really be moved to
// something like the screen preferences panel

#define	T_POSITIVE_SYNC	(B_POSITIVE_HSYNC | B_POSITIVE_VSYNC)
#define MODE_FLAGS	(B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS | B_DPMS | B_SUPPORTS_OVERLAYS)
//#define MODE_COUNT (sizeof (mode_list) / sizeof (display_mode))

static const display_mode base_mode_list[] = {
// test for PAL
//{ { 25175, 640, 656, 752, 816, 480, 490, 492, 625, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
// test for NTSC
//{ { 43956, 800, 824, 952, 992, 600, 632, 635, 740, T_POSITIVE_SYNC}, B_CMAP8, 800, 600, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(800X600X8.Z1) */

{ { 25175, 640, 656, 752, 800, 480, 490, 492, 525, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(640X480X8.Z1) */
{ { 27500, 640, 672, 768, 864, 480, 488, 494, 530, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* 640X480X60Hz */
{ { 30500, 640, 672, 768, 864, 480, 517, 523, 588, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* SVGA_640X480X60HzNI */
{ { 31500, 640, 664, 704, 832, 480, 489, 492, 520, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70-72Hz_(640X480X8.Z1) */
{ { 31500, 640, 656, 720, 840, 480, 481, 484, 500, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(640X480X8.Z1) */
{ { 36000, 640, 696, 752, 832, 480, 481, 484, 509, 0}, B_CMAP8, 640, 480, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(640X480X8.Z1) */
{ { 25175, 640, 656, 752, 800, 400, 412, 414, 449, B_POSITIVE_VSYNC}, B_CMAP8, 640, 400, 0, 0, MODE_FLAGS}, /* 640x400 - www.epanorama.net/documents/pc/vga_timing.html) */
{ { 25175, 640, 656, 752, 800, 350, 387, 389, 449, B_POSITIVE_HSYNC}, B_CMAP8, 640, 350, 0, 0, MODE_FLAGS}, /* 640x350 - www.epanorama.net/documents/pc/vga_timing.html) */

// NTSC non-isometric resolution (isometric resolution is 640x480)
{ { 26720, 720, 736, 808, 896, 480, 481, 484, 497, B_POSITIVE_VSYNC}, B_CMAP8, 720, 480, 0, 0, MODE_FLAGS},	/* 720x480@60Hz according to GMTF */

// PAL resolutions
{ { 26570, 720, 736, 808, 896, 576, 577, 580, 593, B_POSITIVE_VSYNC}, B_CMAP8, 720, 576, 0, 0, MODE_FLAGS},	/* 720x576@50Hz according to GMTF */
{ { 28460, 768, 784, 864, 960, 576, 577, 580, 593, B_POSITIVE_VSYNC}, B_CMAP8, 768, 576, 0, 0, MODE_FLAGS},	/* 768x576@50Hz according to GMTF */

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
{ { 108000, 1152, 1216, 1344, 1600, 864, 865, 868, 900, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1152X864X8.Z1) */
{ { 121500, 1152, 1216, 1344, 1568, 864, 865, 868, 911, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1152X864X8.Z1) */

{ { 108000, 1280, 1376, 1488, 1800, 960, 961, 964, 1000, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X960X8.Z1) - not in Be's list */
{ { 148500, 1280, 1344, 1504, 1728, 960, 961, 964, 1011, T_POSITIVE_SYNC}, B_CMAP8, 1152, 864, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1280X960X8.Z1) - not in Be's list */

{ { 147100, 1680, 1784, 1968, 2256, 1050, 1051, 1054, 1087, T_POSITIVE_SYNC}, B_CMAP8, 1680, 1050, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1680X1050) */

{ { 108000, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X1024X8.Z1) */
{ { 135000, 1280, 1296, 1440, 1688, 1024, 1025, 1028, 1066, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1280X1024X8.Z1) */
{ { 157500, 1280, 1344, 1504, 1728, 1024, 1025, 1028, 1072, T_POSITIVE_SYNC}, B_CMAP8, 1280, 1024, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1280X1024X8.Z1) */
{ { 122600, 1400, 1488, 1640, 1880, 1050, 1051, 1054, 1087, T_POSITIVE_SYNC}, B_CMAP8, 1400, 1050, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1400X1050) */
{ { 162000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1600X1200X8.Z1) */
{ { 175500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@65Hz_(1600X1200X8.Z1) */
{ { 189000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@70Hz_(1600X1200X8.Z1) */
{ { 202500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@75Hz_(1600X1200X8.Z1) */
{ { 216000, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@80Hz_(1600X1200X8.Z1) */
{ { 229500, 1600, 1664, 1856, 2160, 1200, 1201, 1204, 1250, T_POSITIVE_SYNC}, B_CMAP8, 1600, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@85Hz_(1600X1200X8.Z1) */

// widescreen TV
{ { 84490, 1360, 1392, 1712, 1744, 768, 783, 791, 807, T_POSITIVE_SYNC}, B_CMAP8, 1360, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1360X768) */
{ { 84970, 1368, 1400, 1720, 1752, 768, 783, 791, 807, T_POSITIVE_SYNC}, B_CMAP8, 1368, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1368X768) */

// widescreen resolutions, 16:10
{ { 31300, 800, 848, 928, 1008, 500, 501, 504, 518, T_POSITIVE_SYNC}, B_CMAP8, 800, 500, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(800X500) */
{ { 52800, 1024, 1072, 1176, 1328, 640, 641, 644, 663, T_POSITIVE_SYNC}, B_CMAP8, 1024, 640, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1024X640) */
{ { 80135, 1280, 1344, 1480, 1680, 768, 769, 772, 795, T_POSITIVE_SYNC}, B_CMAP8, 1280, 768, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X768) */
{ { 83500, 1280, 1344, 1480, 1680, 800, 801, 804, 828, T_POSITIVE_SYNC}, B_CMAP8, 1280, 800, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X800) */
{ { 106500, 1440, 1520, 1672, 1904, 900, 901, 904, 932, T_POSITIVE_SYNC}, B_CMAP8, 1440, 900, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1440X900) */
{ { 147100, 1680, 1784, 1968, 2256, 1050, 1051, 1054, 1087, T_POSITIVE_SYNC}, B_CMAP8, 1680, 1050, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1680X1050) */
/* 16:10 panel mode; 2.304M pixels */
{ { 193160, 1920, 2048, 2256, 2592, 1200, 1201, 1204, 1242, T_POSITIVE_SYNC}, B_CMAP8, 1920, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1920X1200) */
//{ { 160000, 1920, 2010, 2060, 2110, 1200, 1202, 1208, 1235, T_POSITIVE_SYNC}, B_CMAP8, 1920, 1200, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1920X1200) */
// widescreen resolutions, 16:9
{ { 74520, 1280, 1368, 1424, 1656, 720, 724, 730, 750, T_POSITIVE_SYNC}, B_CMAP8, 1280, 720, 0, 0, MODE_FLAGS}, /* Vesa_Monitor_@60Hz_(1280X720) */
};


// convert Be colour space to Radeon data type
// returns true, if supported colour space
//	space - Be colour space
//	format - (out) Radeon data type
//	bpp - (out) bytes per pixel
bool Radeon_GetFormat( int space, int *format, int *bpp )
{
	switch( space ) {
    /*case 4:  format = 1; bytpp = 0; break;*/
    case B_CMAP8:  *format = 2; *bpp = 1; break;
    case B_RGB15_LITTLE: *format = 3; *bpp = 2; break;      /*  555 */
    case B_RGB16_LITTLE: *format = 4; *bpp = 2; break;      /*  565 */
    case B_RGB24_LITTLE: *format = 5; *bpp = 3; break;      /*  RGB */
    case B_RGB32_LITTLE: *format = 6; *bpp = 4; break;      /* xRGB */
    default:
		SHOW_ERROR( 1, "Unsupported color space (%d)", space );
		return false;
    }
    
    return true;
}


// macros to convert between register values and pixels
#define H_DISPLAY_2REG( a ) ((a) / 8 - 1)
#define H_DISPLAY_2PIX( a ) (((a) + 1) * 8)
#define H_TOTAL_2REG( a ) ((a) / 8 - 1)
#define H_TOTAL_2PIX( a ) (((a) + 1) * 8)
#define H_SSTART_2REG( a ) ((a) - 8 + h_sync_fudge)
#define H_SSTART_2PIX( a ) ((a) + 8 - h_sync_fudge)
#define H_SWID_2REG( a ) ((a) / 8)
#define H_SWID_2PIX( a ) ((a) * 8)

#define V_2REG( a ) ((a) - 1)
#define V_2PIX( a ) ((a) + 1)

/*
	Validate a target display mode is both
		a) a valid display mode for this device and
		b) falls between the contraints imposed by "low" and "high"
	
	If the mode is not (or cannot) be made valid for this device, return B_ERROR.
	If a valid mode can be constructed, but it does not fall within the limits,
	 return B_BAD_VALUE.
	If the mode is both valid AND falls within the limits, return B_OK.
*/
static status_t
Radeon_ProposeDisplayMode(shared_info *si, crtc_info *crtc,
	general_pll_info *pll, display_mode *target,
	const display_mode *low, const display_mode *high)
{
	status_t result = B_OK;
	
	uint64 target_refresh;
	bool want_same_width, want_same_height;
	int format, bpp;
	uint32 row_bytes;
	int eff_virtual_width;
	fp_info *flatpanel = &si->flatpanels[crtc->flatpanel_port];

	SHOW_FLOW( 4, "CRTC %d, DVI %d", (crtc == &si->crtc[0]) ? 0 : 1, crtc->flatpanel_port );
	SHOW_FLOW( 4, "X %d, virtX %d", target->timing.h_display,  target->virtual_width);
	SHOW_FLOW( 4, "fpRes %dx%d", flatpanel->panel_xres,  flatpanel->panel_yres);

	if (target->timing.h_total * target->timing.v_total == 0)
		return B_BAD_VALUE;

	// save refresh rate - we want to leave this (artifical) value untouched
	// don't use floating point, we are in kernel mode
	target_refresh = 
		(((uint64)target->timing.pixel_clock * 1000) << FIX_SHIFT) / 
		((uint64)target->timing.h_total * target->timing.v_total);
		
	want_same_width = target->timing.h_display == target->virtual_width;
	want_same_height = target->timing.v_display == target->virtual_height;
	
	if( !Radeon_GetFormat( target->space, &format, &bpp ))
		return B_ERROR;
		
	// for flat panels, check maximum resolution;
	// all the other tricks (like fixed resolution and resulting scaling)
	// are done automagically by set_display_mode
    if( (crtc->chosen_displays & (dd_lvds | dd_dvi)) != 0 ) {
		if( target->timing.h_display > flatpanel->panel_xres )
			target->timing.h_display = flatpanel->panel_xres;
		
		if(	target->timing.v_display > flatpanel->panel_yres )
			target->timing.v_display = flatpanel->panel_yres;
	}

	// for secondary flat panels there is no RMX unit for
	// scaling up lower resolutions.  Until we can do centered timings
	// we need to disable the screen unless it is the native resolution.
	// if the DVI input has a scaler we would need to know about it somehow...
    if( (crtc->chosen_displays & dd_dvi_ext) != 0 ) {
		SHOW_FLOW0( 4, "external (secondary) DVI cannot support non-native resolutions" );
		if( ( target->timing.h_display != flatpanel->panel_xres ) ||
			( target->timing.v_display != flatpanel->panel_yres ) )
			return B_ERROR;
	}

/*
	// the TV-Out encoder can "only" handle up to 1024x768
	if( (head->chosen_displays & (dd_ctv | dd_stv)) != 0 ) {
		if( target->timing.h_display > 1024 )
			target->timing.h_display = 1024;
		
		if(	target->timing.v_display > 768 )
			target->timing.v_display = 768;
	}
*/
				
	// validate horizontal timings
	{
		int h_sync_fudge, h_display, h_sync_start, h_sync_wid, h_total;
			
		h_display = target->timing.h_display;
		h_sync_fudge = Radeon_GetHSyncFudge( crtc, format );
		h_sync_start = target->timing.h_sync_start;
		h_sync_wid = target->timing.h_sync_end - target->timing.h_sync_start;
		h_total = target->timing.h_total;

		// make sure, display is not too small 
		// (I reckon Radeon doesn't care, but your monitor probably does)
		if( h_display < 320 ) 
			h_display = 320;
		// apply hardware restrictions
		// as h_display is the smallest register, it's always possible 
		// to adjust other values to keep them in supported range
		if( h_display > H_DISPLAY_2PIX( RADEON_CRTC_H_DISP >> RADEON_CRTC_H_DISP_SHIFT ) )
			h_display = H_DISPLAY_2PIX( RADEON_CRTC_H_DISP >> RADEON_CRTC_H_DISP_SHIFT );
		// round properly
		h_display = H_DISPLAY_2PIX( H_DISPLAY_2REG( h_display ));
			
		// ensure minimum time before sync
		if( h_sync_start < h_display + 2*8 )
			h_sync_start = h_display + 2*8;
		// sync has wider range than display are, so we won't collide there,
		// but total width has same range as sync start, so leave some space
		if( h_sync_start > H_SSTART_2PIX( RADEON_CRTC_H_SYNC_STRT_CHAR | RADEON_CRTC_H_SYNC_STRT_PIX ) - 4*8 )
			h_sync_start = H_SSTART_2PIX( RADEON_CRTC_H_SYNC_STRT_CHAR | RADEON_CRTC_H_SYNC_STRT_PIX ) - 4*8;

		// ensure minimum sync length
		if( h_sync_wid < H_SWID_2PIX( 3 ))
			h_sync_wid = H_SWID_2PIX( 3 );
		// allowed range is quite small, so make sure sync isn't too long
		if( h_sync_wid > H_SWID_2PIX( RADEON_CRTC_H_SYNC_WID >> RADEON_CRTC_H_SYNC_WID_SHIFT ) )
			h_sync_wid = H_SWID_2PIX( RADEON_CRTC_H_SYNC_WID >> RADEON_CRTC_H_SYNC_WID_SHIFT );
		// round properly
		h_sync_wid = H_SWID_2PIX( H_SWID_2REG( h_sync_wid ));

		// last but not least adapt total width
		// "+7" is needed for rounding up: sync_start isn't rounded, but h_total is
		if( h_total < h_sync_start + h_sync_wid + 1*8 + 7 )
			h_total = h_sync_start + h_sync_wid + 1*8 + 7;
		// we may get a too long total width; this can only happen 
		// because sync is too long, so truncate sync accordingly
		if( h_total > H_TOTAL_2PIX( RADEON_CRTC_H_TOTAL ) ) {
			h_total = H_TOTAL_2PIX( RADEON_CRTC_H_TOTAL );
			h_sync_wid = min( h_sync_wid, h_total - h_sync_start );
			h_sync_wid = H_SWID_2PIX( H_SWID_2REG( h_sync_wid ));
		}
		// round properly
		h_total = H_TOTAL_2PIX( H_TOTAL_2REG( h_total ));

		target->timing.h_display = h_display;
		target->timing.h_sync_start = h_sync_start;
		target->timing.h_sync_end = h_sync_start + h_sync_wid;
		target->timing.h_total = h_total;
	}

	// did we fall out of one of the limits?
	if( target->timing.h_display < low->timing.h_display ||
		target->timing.h_display > high->timing.h_display ||
		target->timing.h_sync_start < low->timing.h_sync_start ||
		target->timing.h_sync_start > high->timing.h_sync_start ||
		target->timing.h_sync_end < low->timing.h_sync_end ||
		target->timing.h_sync_end > high->timing.h_sync_end ||
		target->timing.h_total < low->timing.h_total ||
		target->timing.h_total > high->timing.h_total) 
	{
		SHOW_FLOW0( 4, "out of horizontal limits" );
		result = B_BAD_VALUE;
	}

	// validate vertical timings
	{
		int v_display, v_sync_start, v_sync_wid, v_total;
		
		v_display = target->timing.v_display;
		v_sync_start = target->timing.v_sync_start;
		v_sync_wid = target->timing.v_sync_end - target->timing.v_sync_start;
		v_total = target->timing.v_total;

		// apply a reasonable minimal height to make monitor happy
		if( v_display < 200 )
			v_display = 200;
		// apply limits but make sure we have enough lines left for blank and sync
		if( v_display > V_2PIX(RADEON_CRTC_V_DISP >> RADEON_CRTC_V_DISP_SHIFT) - 5)
			v_display = V_2PIX(RADEON_CRTC_V_DISP >> RADEON_CRTC_V_DISP_SHIFT) - 5;

		// leave at least one line before sync
		// (some flat panel have zero gap here; probably, this leads to
		// the infamous bright line at top of screen)
		if( v_sync_start < v_display + 1 )
			v_sync_start = v_display + 1;
		// apply hardware limit and leave some lines for sync
		if( v_sync_start > V_2PIX(RADEON_CRTC_V_SYNC_STRT) - 4)
			v_sync_start = V_2PIX(RADEON_CRTC_V_SYNC_STRT) - 4;

		// don't make sync too short
		if( v_sync_wid < 2 )
			v_sync_wid = 2;
		// sync width is quite restricted
		if( v_sync_wid > (RADEON_CRTC_V_SYNC_WID >> RADEON_CRTC_V_SYNC_WID_SHIFT))
			v_sync_wid = (RADEON_CRTC_V_SYNC_WID >> RADEON_CRTC_V_SYNC_WID_SHIFT);

		// leave a gap of at least 1 line
		if( v_total < v_sync_start + v_sync_wid + 1 )
			v_total = v_sync_start + v_sync_wid + 1;
		// if too long, truncate it and adapt sync len
		if( v_total > V_2PIX( RADEON_CRTC_V_TOTAL ) ) {
			v_total = V_2PIX( RADEON_CRTC_V_TOTAL );
			v_sync_wid = min( v_sync_wid, v_total - v_sync_start - 4 );
		}

		target->timing.v_display = v_display;
		target->timing.v_sync_start = v_sync_start;
		target->timing.v_sync_end = v_sync_start + v_sync_wid;
		target->timing.v_total = v_total;
	}

	// did we fall out of one of the limits?
	if(	target->timing.v_display < low->timing.v_display ||
		target->timing.v_display > high->timing.v_display ||
		target->timing.v_sync_start < low->timing.v_sync_start ||
		target->timing.v_sync_start > high->timing.h_sync_start ||
		target->timing.v_sync_end < low->timing.v_sync_end ||
		target->timing.v_sync_end > high->timing.v_sync_end ||
		target->timing.v_total < low->timing.v_total ||
		target->timing.v_total > high->timing.v_total ) 
	{
		SHOW_FLOW0( 4, "out of vertical limits" );
		result = B_BAD_VALUE;
	}

	// restore whished refresh rate
	target->timing.pixel_clock = 
		((uint64)target_refresh / 1000 * target->timing.h_total * target->timing.v_total + FIX_SCALE / 2) 
		>> FIX_SHIFT;

	// apply PLL restrictions
	if( target->timing.pixel_clock / 10 > pll->max_pll_freq || 
		target->timing.pixel_clock / 10 * 12 < pll->min_pll_freq ) 
	{
		SHOW_ERROR( 4, "pixel_clock (%ld) out of range (%d, %d)", target->timing.pixel_clock, 
			pll->max_pll_freq * 10, pll->min_pll_freq / 12 );
		return B_ERROR;
	}

	// make sure virtual_size > visible_size
	// additionally, restore virtual_size == visible_size if it was so on entry
	if ((target->timing.h_display > target->virtual_width) || want_same_width)
		target->virtual_width = target->timing.h_display;
	if ((target->timing.v_display > target->virtual_height) || want_same_height)
		target->virtual_height = target->timing.v_display;

	// TBD: limit is taken from XFree86
	// this is probably a CRTC limit; don't know about the accelerator limit (if any)
	// h_display can be at most 512*8, so we don't risk h_virtual < h_display 
	// after applying this restriction
	if (target->virtual_width > 1024*8)
		target->virtual_width = 1024*8;

	if (target->virtual_width < low->virtual_width ||
		target->virtual_width > high->virtual_width )
	{
		SHOW_FLOW0( 4, "out of virtual horizontal limits" );
		result = B_BAD_VALUE;
	}

	// we may have to use a larger virtual width -
	// take care of that when calculating memory consumption	
	eff_virtual_width = Radeon_RoundVWidth( target->virtual_height, bpp );

	// calculate rowbytes after we've nailed the virtual width
	row_bytes = eff_virtual_width * bpp;

	// if we haven't enough memory, reduce virtual height
	// (some programs create back buffers by asking for a huge
	// virtual screen; they actually want to know what is possible
	// to adjust the number of back buffers according to amount
	// of graphics memory)
	
	// careful about additionally required memory: 
	// 1024 bytes are needed for hardware cursor
	if ((row_bytes * target->virtual_height) > si->memory[mt_local].size - 1024 )
		target->virtual_height = (si->memory[mt_local].size - 1024) / row_bytes;

	// make sure we haven't shrunk virtual height too much
	if (target->virtual_height < target->timing.v_display) {
		SHOW_ERROR( 4, "not enough memory for this mode (could show only %d of %d lines)", 
			target->virtual_height, target->timing.v_display );
		return B_ERROR;
	}
		
	if (target->virtual_height < low->virtual_height ||
		target->virtual_height > high->virtual_height )
	{
		SHOW_FLOW0( 4, "out of virtual vertical limits" );
		result = B_BAD_VALUE;
	}

	// we ignore flags - in the sample driver, they did the same,
	// so why bother?
	return result;
}

// public function: return number of display modes returned by get_mode_list
uint32 ACCELERANT_MODE_COUNT( void )
{
	return ai->si->mode_count;
}

// public function: get list of standard display modes
//	dm - modes are copied to here (to be allocated by caller)
status_t GET_MODE_LIST( display_mode *dm ) 
{
	memcpy( dm, ai->mode_list, ai->si->mode_count * sizeof(display_mode) );
	
	return B_OK;
}


static const color_space spaces[4] = {
	B_CMAP8, B_RGB15_LITTLE, B_RGB16_LITTLE, B_RGB32_LITTLE
};

// if given mode is possible on this card, add it to standard mode list
//	mode - mode to add (colourspace is ignored but replaced 
//	       by each officially supported colour space in turn)
//	ignore_timing - don't care if timing has to be modified to make mode valid 
//	                (used for fp modes - we just want their resolution)
static void checkAndAddMode( accelerator_info *ai, const display_mode *mode, bool ignore_timing )
{
	shared_info *si = ai->si;
	uint i;
	display_mode low, high;
	uint32 pix_clk_range;
	display_mode *dst;

	if( ignore_timing ) {
		// for fp modes: don't add mode if its resolution is already in official mode list
		for( i = 0; i < si->mode_count; ++i ) {
			if( ai->mode_list[i].timing.h_display == mode->timing.h_display &&
				ai->mode_list[i].timing.v_display == mode->timing.v_display &&
				ai->mode_list[i].virtual_width == mode->virtual_width &&
				ai->mode_list[i].virtual_height == mode->virtual_height )
				return;
		}
	}
	
	// set ranges for acceptable values
	low = high = *mode;
	
	// range is 6.25% of default clock: arbitrarily picked
	pix_clk_range = low.timing.pixel_clock >> 5;
	low.timing.pixel_clock -= pix_clk_range;
	high.timing.pixel_clock += pix_clk_range;
	
	if( ignore_timing ) {
		low.timing.h_total = 0;
		low.timing.h_sync_start = 0;
		low.timing.h_sync_end = 0;
		low.timing.v_total = 0;
		low.timing.v_sync_start = 0;
		low.timing.v_sync_end = 0;
		high.timing.h_total = 0xffff;
		high.timing.h_sync_start = 0xffff;
		high.timing.h_sync_end = 0xffff;
		high.timing.v_total = 0xffff;
		high.timing.v_sync_start = 0xffff;
		high.timing.v_sync_end = 0xffff;
	}
	
	dst = &ai->mode_list[si->mode_count];
	
	// iterator through all colour spaces
	for( i = 0; i < (sizeof(spaces) / sizeof(color_space)); i++ ) {
		// check whether first port can handle it
		*dst = *mode;
		dst->space = low.space = high.space = spaces[i];

		if( Radeon_ProposeDisplayMode( si, &si->crtc[0], 
			&si->pll, dst, &low, &high ) == B_OK ) 
		{
			si->mode_count++;
			++dst;
			
		} else {
			// it can't, so try second port
			*dst = *mode;
			dst->space = spaces[i];
			
			if( Radeon_ProposeDisplayMode( si, &si->crtc[1],
				&si->pll, dst, &low, &high ) == B_OK )
			{
				si->mode_count++;
				++dst;
				
			} else
				SHOW_FLOW( 4, "%ld, %ld not supported", dst->virtual_width, dst->virtual_height );
		} 
	}
}


// add display mode including span mode variations to offical list
static void checkAndAddMultiMode( accelerator_info *ai, const display_mode *mode, 
	bool ignore_timing )
{
	display_mode wide_mode;
	
	SHOW_FLOW( 4, "%ld, %ld", mode->virtual_width, mode->virtual_height );
	
	// plain mode 
	checkAndAddMode( ai, mode, ignore_timing );

	// double width mode
	wide_mode = *mode;
	wide_mode.virtual_width *= 2;
	wide_mode.flags |= B_SCROLL;
	checkAndAddMode( ai, &wide_mode, ignore_timing );
	
	// double height mode
	wide_mode = *mode;
	wide_mode.virtual_height *= 2;
	wide_mode.flags |= B_SCROLL;
	checkAndAddMode( ai, &wide_mode, ignore_timing );
}

// add display mode of flat panel to official list
static void addFPMode( accelerator_info *ai )
{
	shared_info *si = ai->si;
	
	fp_info *fp_info = &si->flatpanels[0];	//todo fix the hardcoding what about ext dvi?
	
    if( (ai->vc->connected_displays & (dd_dvi | dd_dvi_ext | dd_lvds)) != 0 ) {
    	display_mode mode;
    	SHOW_FLOW0( 2, "" );
		mode.virtual_width = mode.timing.h_display = fp_info->panel_xres;
		mode.virtual_height = mode.timing.v_display = fp_info->panel_yres;
		
		mode.timing.h_total = mode.timing.h_display + fp_info->h_blank;
		mode.timing.h_sync_start = mode.timing.h_display + fp_info->h_over_plus;
		mode.timing.h_sync_end = mode.timing.h_sync_start + fp_info->h_sync_width;
		mode.timing.v_total = mode.timing.v_display + fp_info->v_blank;
		mode.timing.v_sync_start = mode.timing.v_display + fp_info->v_over_plus;
		mode.timing.v_sync_end = mode.timing.v_sync_start + fp_info->v_sync_width;
		
		mode.timing.pixel_clock = fp_info->dot_clock;
		
		// if we have no pixel clock, assume 60 Hz 
		// (as we don't program PLL in this case, it doesn't matter
		// if it's wrong, we just want this resolution in the mode list)
		if( mode.timing.pixel_clock == 0 ) {
			// devide by 1000 as clock is in kHz
			mode.timing.pixel_clock = 
				((uint32)mode.timing.h_total * mode.timing.v_total * 60) / 1000;
		}
		
		mode.flags = MODE_FLAGS;
		mode.h_display_start = 0;
		mode.v_display_start = 0;
		
		SHOW_FLOW( 2, "H: %4d %4d %4d %4d (v=%4d)", 
			mode.timing.h_display, mode.timing.h_sync_start, 
			mode.timing.h_sync_end, mode.timing.h_total, mode.virtual_width );
		SHOW_FLOW( 2, "V: %4d %4d %4d %4d (h=%4d)", 
			mode.timing.v_display, mode.timing.v_sync_start, 
			mode.timing.v_sync_end, mode.timing.v_total, mode.virtual_height );
		SHOW_FLOW( 2, "clk: %ld", mode.timing.pixel_clock );
		
		// flat panels seem to have strange timings;
		// as we ignore user-supplied timing for FPs anyway,
		// the mode can (and usually has to) be modified to be 
		// used for normal CRTs
		checkAndAddMultiMode( ai, &mode, true );
	}
}

// create list of officially supported modes
status_t Radeon_CreateModeList( shared_info *si )
{
	size_t max_size;
	uint i;
	uint max_num_modes;

	// maximum number of official modes:	
	// (predefined-modes + fp-modes) * number-of-colour-spaces * number-of-(non)-span-modes
	max_num_modes = ((sizeof( base_mode_list ) / sizeof( base_mode_list[0] ) + 1) * 4 * 3);

	max_size = (max_num_modes * sizeof(display_mode) + (B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);
		
	si->mode_list_area = create_area("Radeon accelerant mode info", 
		(void **)&ai->mode_list, B_ANY_ADDRESS, 
		max_size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		
	if( si->mode_list_area < B_OK ) 
		return si->mode_list_area;

	si->mode_count = 0;
	
	// check standard modes
	for( i = 0; i < sizeof( base_mode_list ) / sizeof( base_mode_list[0] ); i++ )
		checkAndAddMultiMode( ai, &base_mode_list[i], false );

	// plus fp mode
	addFPMode( ai );

	// as we've created the list ourself, we don't clone it
	ai->mode_list_area = si->mode_list_area;
	
	return B_OK;
}


//! public function: wraps for internal propose_display_mode
status_t
PROPOSE_DISPLAY_MODE(display_mode *target, const display_mode *low,
	const display_mode *high)
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
	status_t result1, result2;
	bool isTunneled;
	status_t result;
	display_mode tmp_target;
	
	// check whether we got a tunneled settings command
	result = Radeon_CheckMultiMonTunnel( vc, target, low, high, &isTunneled );
	if( isTunneled )
		return result;

	// check how many heads are needed by target mode
	tmp_target = *target;
	Radeon_DetectMultiMode( vc, &tmp_target );
		
	// before checking multi-monitor mode, we must define a monitor signal routing
	// TBD: this may be called a bit too frequently if someone scans available modes
	// via successive Propose_Display_Mode; though this doesn't do any _real_ harm
	// it leads to annoying distortions on screen!!
	Radeon_DetectDisplays( ai);
	Radeon_SetupDefaultMonitorRouting( 
		ai, Radeon_DifferentPorts( &tmp_target ), vc->use_laptop_panel );

	// transform to multi-screen mode first
	Radeon_DetectMultiMode( vc, target );
	Radeon_VerifyMultiMode( vc, si, target );
	
	SHOW_FLOW0( 2, "wished:" );
	SHOW_FLOW( 2, "H: %4d %4d %4d %4d (v=%4d)", 
		target->timing.h_display, target->timing.h_sync_start, 
		target->timing.h_sync_end, target->timing.h_total, target->virtual_width );
	SHOW_FLOW( 2, "V: %4d %4d %4d %4d (h=%4d)", 
		target->timing.v_display, target->timing.v_sync_start, 
		target->timing.v_sync_end, target->timing.v_total, target->virtual_height );
	SHOW_FLOW( 2, "clk: %ld", target->timing.pixel_clock );
	
	// we must assure that each ProposeMode call doesn't tweak the mode in
	// a way that it cannot be handled by the other port anymore
	result1 = Radeon_ProposeDisplayMode( si, &si->crtc[0], 
		&si->pll, target, low, high );
		
	if( result1 == B_ERROR )
		return B_ERROR;
		
	if( Radeon_NeedsSecondPort( target )) {
		// if both ports are used, make sure both can handle mode
		result2 = Radeon_ProposeDisplayMode( si, &si->crtc[1],
			&si->pll, target, low, high );
			
		if( result2 == B_ERROR )
			return B_ERROR;
	} else {
		result2 = B_OK;
	}
	
	SHOW_INFO0( 2, "got:" );
	SHOW_INFO( 2, "H: %4d %4d %4d %4d (v=%4d)", 
		target->timing.h_display, target->timing.h_sync_start, 
		target->timing.h_sync_end, target->timing.h_total, target->virtual_width );
	SHOW_INFO( 2, "V: %4d %4d %4d %4d (h=%4d)", 
		target->timing.v_display, target->timing.v_sync_start, 
		target->timing.v_sync_end, target->timing.v_total, target->virtual_height );
	SHOW_INFO( 2, "clk: %ld", target->timing.pixel_clock );

	Radeon_HideMultiMode( vc, target );
	
	if( result1 == B_OK && result2 == B_OK )
		return B_OK;
	else
		return B_BAD_VALUE;
}
