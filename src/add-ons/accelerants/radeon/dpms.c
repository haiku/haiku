/*
	Copyright (c) 2002-2004, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Display Power Management (DPMS) support
*/

#include "radeon_accelerant.h"
#include "mmio.h"
#include "crtc_regs.h"
#include "fp_regs.h"
#include "pll_regs.h"
#include "pll_access.h"
#include "tv_out_regs.h"
#include "theatre_regs.h"
#include "GlobalData.h"
#include "generic.h"


// public function: set DPMS mode
status_t SET_DPMS_MODE(uint32 dpms_flags) 
{
	virtual_card *vc = ai->vc;
	status_t 
		result1 = B_OK, 
		result2 = B_OK;
	
	if( vc->used_crtc[0] )
		result1 = Radeon_SetDPMS( ai, 0, dpms_flags );
	if( vc->used_crtc[0] )
		result1 = Radeon_SetDPMS( ai, 0, dpms_flags );

	if( result1 == B_OK && result2 == B_OK )
		return B_OK;
	else
		return B_ERROR;
}

// public function: report DPMS capabilities
uint32 DPMS_CAPABILITIES(void) 
{
	return 	B_DPMS_ON | B_DPMS_STAND_BY  | B_DPMS_SUSPEND | B_DPMS_OFF;
}


// public function: get current DPMS mode
uint32 DPMS_MODE(void) 
{
	// we just ask the primary virtual head what status it is in
	return Radeon_GetDPMS( ai, ai->vc->used_crtc[0] ? 0 : 1 );
}


// set DPMS state of LVDS port
static void Radeon_SetDPMS_LVDS( accelerator_info *ai, int mode )
{
	vuint8 *regs = ai->regs;

	// for internal flat panel, switch backlight off too
	switch( mode ) {
	case B_DPMS_ON:
		// on my laptop, the display has problems to wake-up, this
		// should hopefully cure that
		// (you get a dark picture first that becomes brighter step by step,
		//  after a couple of seconds you have full brightness again)
		OUTREGP( regs, RADEON_LVDS_GEN_CNTL, RADEON_LVDS_BLON, ~RADEON_LVDS_BLON );
		snooze( ai->si->panel_pwr_delay * 1000 ); 
		OUTREGP( regs, RADEON_LVDS_GEN_CNTL, RADEON_LVDS_ON, ~RADEON_LVDS_ON );
		break;
		
	case B_DPMS_STAND_BY:
	case B_DPMS_SUSPEND:
	case B_DPMS_OFF: {
		uint32 old_pixclks_cntl;
		
		old_pixclks_cntl = Radeon_INPLL( ai->regs, ai->si->asic, RADEON_PIXCLKS_CNTL);
		
		// ASIC bug: when LVDS_ON is reset, LVDS_ALWAYS_ON must be zero
		if( ai->si->is_mobility || ai->si->is_igp ) 
			Radeon_OUTPLLP( ai->regs, ai->si->asic, RADEON_PIXCLKS_CNTL, 0, ~RADEON_PIXCLK_LVDS_ALWAYS_ONb );

		OUTREGP( regs, RADEON_LVDS_GEN_CNTL, 0,	~(RADEON_LVDS_BLON | RADEON_LVDS_ON) );
			
		if( ai->si->is_mobility || ai->si->is_igp ) 
			Radeon_OUTPLL( ai->regs, ai->si->asic, RADEON_PIXCLKS_CNTL, old_pixclks_cntl );

		break; }
	}
}


// set DPMS state of DVI port
static void Radeon_SetDPMS_DVI( accelerator_info *ai, int mode )
{
	vuint8 *regs = ai->regs;

	// it seems that DPMS doesn't work on DVI, so we disable FP completely
	// (according to specs this is the official way to handle DVI though DPMS
	// *should* be supported as well)
	switch( mode ) {
	case B_DPMS_ON:
		OUTREGP( regs, RADEON_FP_GEN_CNTL, RADEON_FP_FPON | RADEON_FP_TMDS_EN, 
			~(RADEON_FP_FPON | RADEON_FP_TMDS_EN));
		break;
	case B_DPMS_STAND_BY:
	case B_DPMS_SUSPEND:
	case B_DPMS_OFF:
		OUTREGP( regs, RADEON_FP_GEN_CNTL, 0, ~RADEON_FP_FPON | RADEON_FP_TMDS_EN );
		break;
	}
}


// set DPMS state of external DVI port
static void Radeon_SetDPMS_FP2( accelerator_info *ai, int mode )
{
	vuint8 *regs = ai->regs;

	// it seems that DPMS doesn't work on DVI, so we disable FP completely
	// (according to specs this is the official way to handle DVI though DPMS
	// *should* be supported as well)
	switch( mode ) {
	case B_DPMS_ON:
		OUTREGP( regs, RADEON_FP2_GEN_CNTL, 0, ~RADEON_FP2_BLANK_EN);
		OUTREGP( regs, RADEON_FP2_GEN_CNTL, RADEON_FP2_FPON, ~RADEON_FP2_FPON);
		if (ai->si->asic >= rt_r200) {
			OUTREGP( regs, RADEON_FP2_GEN_CNTL, RADEON_FP2_DV0_EN, ~RADEON_FP2_DV0_EN);
		}
		break;
	case B_DPMS_STAND_BY:
	case B_DPMS_SUSPEND:
	case B_DPMS_OFF:
		OUTREGP( regs, RADEON_FP2_GEN_CNTL, RADEON_FP2_BLANK_EN, ~RADEON_FP2_BLANK_EN );
		OUTREGP( regs, RADEON_FP2_GEN_CNTL, 0, ~RADEON_FP2_FPON);
		if (ai->si->asic >= rt_r200) {
			OUTREGP( regs, RADEON_FP2_GEN_CNTL, 0, ~RADEON_FP2_DV0_EN);
		}
		break;
	}
}


// set DPMS mode for CRT DAC.
// warning: the CRTC-DAC only obbeys this setting if
// connected to CRTC1, else it collides with TV-DAC
static void Radeon_SetDPMS_CRT( accelerator_info *ai, int mode )
{
	vuint8 *regs = ai->regs;
	
	switch( mode ) {
	case B_DPMS_ON:
		OUTREGP( regs, RADEON_CRTC_EXT_CNTL, 0, ~RADEON_CRTC_DISPLAY_DIS );
		break;
		
	case B_DPMS_STAND_BY:
	case B_DPMS_SUSPEND:
	case B_DPMS_OFF:
		OUTREGP( regs, RADEON_CRTC_EXT_CNTL,
			RADEON_CRTC_DISPLAY_DIS, ~RADEON_CRTC_DISPLAY_DIS );
		break;
	}
}


// set DPMS mode for TV-DAC in CRT mode
// warning: if the CRT-DAC is connected to CRTC2, it is
// affected by this setting too
static void Radeon_SetDPMS_TVCRT( accelerator_info *ai, int mode )
{
	vuint8 *regs = ai->regs;
	
	switch( mode ) {
	case B_DPMS_ON:
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, 0, 
			~(RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_VSYNC_DIS | RADEON_CRTC2_HSYNC_DIS) );
		break;
	case B_DPMS_STAND_BY:
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, (RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_HSYNC_DIS), 
			~(RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_VSYNC_DIS | RADEON_CRTC2_HSYNC_DIS) );
		break;
	case B_DPMS_SUSPEND:
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, (RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_VSYNC_DIS), 
			~(RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_VSYNC_DIS | RADEON_CRTC2_HSYNC_DIS) );
		break;
	case B_DPMS_OFF:
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, (RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_VSYNC_DIS | RADEON_CRTC2_HSYNC_DIS), 
			~(RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_VSYNC_DIS | RADEON_CRTC2_HSYNC_DIS) );
		break;
	}
}


// set DPMS mode for first CRTC
static void Radeon_SetDPMS_CRTC1( accelerator_info *ai, int mode )
{
	vuint8 *regs = ai->regs;
	
	uint32 mask = ~(RADEON_CRTC_DISPLAY_DIS | RADEON_CRTC_VSYNC_DIS | RADEON_CRTC_HSYNC_DIS);
		
	switch( mode ) {
	case B_DPMS_ON:
		OUTREGP( regs, RADEON_CRTC_EXT_CNTL, 0, mask );
		break;
	case B_DPMS_STAND_BY:
		OUTREGP( regs, RADEON_CRTC_EXT_CNTL, 
			(RADEON_CRTC_DISPLAY_DIS | RADEON_CRTC_HSYNC_DIS), mask );
		break;
	case B_DPMS_SUSPEND:
		OUTREGP( regs, RADEON_CRTC_EXT_CNTL, 
			(RADEON_CRTC_DISPLAY_DIS | RADEON_CRTC_VSYNC_DIS), mask );
		break;
	case B_DPMS_OFF:
		OUTREGP( regs, RADEON_CRTC_EXT_CNTL, 
			(RADEON_CRTC_DISPLAY_DIS | RADEON_CRTC_VSYNC_DIS | RADEON_CRTC_HSYNC_DIS), mask );
		break;
	}
	
	// disable/enable memory requests and cursor
	switch( mode ) {
	case B_DPMS_ON:
		/* Screen: On; HSync: On, VSync: On */
		OUTREGP( regs, RADEON_CRTC_GEN_CNTL, 0, ~RADEON_CRTC_DISP_REQ_EN_B );
		Radeon_ShowCursor( ai, 0 );
		break;
	case B_DPMS_STAND_BY:
	case B_DPMS_SUSPEND:
	case B_DPMS_OFF:
		OUTREGP( regs, RADEON_CRTC_GEN_CNTL, RADEON_CRTC_DISP_REQ_EN_B, 
			~(RADEON_CRTC_DISP_REQ_EN_B | RADEON_CRTC_CUR_EN) );
		break;
	}
}


// set DPMS mode of second CRTC
static void Radeon_SetDPMS_CRTC2( accelerator_info *ai, int mode )
{
	vuint8 *regs = ai->regs;
	
	int mask = ~(RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_VSYNC_DIS | RADEON_CRTC2_HSYNC_DIS);
		
	switch( mode ) {
	case B_DPMS_ON:
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, 0, mask );
		break;
	case B_DPMS_STAND_BY:
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, (RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_HSYNC_DIS), mask );
		break;
	case B_DPMS_SUSPEND:
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, (RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_VSYNC_DIS), mask);
		break;
	case B_DPMS_OFF:
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, 
			(RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_VSYNC_DIS | RADEON_CRTC2_HSYNC_DIS), mask);
		break;
	}
	
	switch( mode ) {
	case B_DPMS_ON:
		/* Screen: On; HSync: On, VSync: On */
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, 0, ~RADEON_CRTC2_DISP_REQ_EN_B );
		Radeon_ShowCursor( ai, 1 );
		break;
	case B_DPMS_STAND_BY:
	case B_DPMS_SUSPEND:
	case B_DPMS_OFF:
		OUTREGP( regs, RADEON_CRTC2_GEN_CNTL, RADEON_CRTC2_DISP_REQ_EN_B, 
			~(RADEON_CRTC2_DISP_REQ_EN_B | RADEON_CRTC2_CUR_EN) );
		break;
	}
}


// set DPMS mode of TV-out
static void Radeon_SetDPMS_TVOUT( accelerator_info *ai, int mode )
{
	// we set to gain either to 0 for blank or 1 for normal operation
	if( IS_INTERNAL_TV_OUT( ai->si->tv_chip )) {
		OUTREG( ai->regs, RADEON_TV_LINEAR_GAIN_SETTINGS,
			mode == B_DPMS_ON ? 0x01000100 : 0 );
	} else {
		Radeon_VIPWrite( ai, ai->si->theatre_channel, RADEON_TV_LINEAR_GAIN_SETTINGS, 
			mode == B_DPMS_ON ? 0x01000100 : 0 );
	}
}

// set DPMS mode of one port
// engine lock is assumed to be hold
status_t Radeon_SetDPMS( accelerator_info *ai, int crtc_idx, int mode )
{
	crtc_info *crtc = &ai->si->crtc[crtc_idx];
	
	// test validity of mode once and for all	
	switch( mode ) {
	case B_DPMS_ON:
	case B_DPMS_STAND_BY:
	case B_DPMS_SUSPEND:
	case B_DPMS_OFF:
		break;
	default:
		return B_BAD_VALUE;
	}

	if( crtc_idx == 0 )
		Radeon_SetDPMS_CRTC1( ai, mode );
	else
		Radeon_SetDPMS_CRTC2( ai, mode );

	// possible ASIC bug: if CRT-DAC is connected to CRTC1, it obbeys 
	// RADEON_CRTC_DISPLAY_DIS; if it is connected to CRTC2, to
	// RADEON_CRTC2_DISP_DIS - i.e. it follows the CRTC;
	// but the TV-DAC always listens to RADEON_CRTC2_DISP_DIS, independant
	// of the CRTC it gets its signal from;
	// this is a guarantee that two virtual cards will collide!
	if( crtc_idx == 0 || 1/* && (crtc->active_displays & dd_crt) != 0 */)
		Radeon_SetDPMS_CRT( ai, mode );

	if( crtc_idx == 1 || (crtc->active_displays & (dd_tv_crt | dd_ctv | dd_stv)) != 0 )
		Radeon_SetDPMS_TVCRT( ai, mode );

	// TV-Out ignores DPMS completely, including the blank-screen trick		
	if( (crtc->active_displays & (dd_ctv | dd_stv)) != 0 )
		Radeon_SetDPMS_TVOUT( ai, mode );

	if( (crtc->active_displays & dd_lvds) != 0 )
		Radeon_SetDPMS_LVDS( ai, mode );

	if( (crtc->active_displays & dd_dvi) != 0 )
		Radeon_SetDPMS_DVI( ai, mode );
		
	if( (crtc->active_displays & dd_dvi_ext) != 0 )
		Radeon_SetDPMS_FP2( ai, mode );
		
	return B_OK;
}


// get DPMS mode of first port
static uint32 Radeon_GetDPMS_CRTC1( accelerator_info *di )
{
	uint32 tmp;
	
	tmp = INREG( di->regs, RADEON_CRTC_EXT_CNTL );
	
	if( (tmp & RADEON_CRTC_DISPLAY_DIS) == 0 )
		return B_DPMS_ON;
		
	if( (tmp & RADEON_CRTC_VSYNC_DIS) == 0 )
		return B_DPMS_STAND_BY;
		
	if( (tmp & RADEON_CRTC_HSYNC_DIS) == 0 )
		return B_DPMS_SUSPEND;
		
	return B_DPMS_OFF;
}


// get DPMS mode of second port
static uint32 Radeon_GetDPMS_CRTC2( accelerator_info *di )
{
	uint32 tmp;
	
	tmp = INREG( di->regs, RADEON_CRTC2_GEN_CNTL );
	
	if( (tmp & RADEON_CRTC2_DISP_DIS) == 0 )
		return B_DPMS_ON;
		
	if( (tmp & RADEON_CRTC2_VSYNC_DIS) == 0 )
		return B_DPMS_STAND_BY;
		
	if( (tmp & RADEON_CRTC2_HSYNC_DIS) == 0 )
		return B_DPMS_SUSPEND;
		
	return B_DPMS_OFF;
}


// get DPMS mode of one port
uint32 Radeon_GetDPMS( accelerator_info *ai, int crtc_idx )
{
	if( crtc_idx == 0 )
		return Radeon_GetDPMS_CRTC1( ai );
	else
		return Radeon_GetDPMS_CRTC2( ai );
}
