/*
	Copyright (c) 2002-04, Thomas Kurschel
	

	Part of Radeon accelerant
		
	ImpacTV programming. As this unit is contained in various chips,
	the code to actually access the unit is separated.
*/

#include "radeon_interface.h"
#include "radeon_accelerant.h"

#include "tv_out_regs.h"
#include "utils.h"
#include "set_mode.h"

#include <string.h>


// fixed-point resolution of UV scaler increment
#define TV_UV_INC_FIX_SHIFT 14
#define TV_UV_INC_FIX_SCALE (1 << TV_UV_INC_FIX_SHIFT)

// fixed point resolution of UV scaler initialization (uv_accum_init)
#define TV_UV_INIT_FIX_SHIFT 6


// calculate time when TV timing must be restarted
static void Radeon_CalcImpacTVRestart( 
	impactv_params *params, const display_mode *mode, 
	uint16 h_blank, uint16 f_total )
{
	uint32 h_first, v_first = 0, f_first;
	uint32 tmp_uv_accum_sum;
	uint16 uv_accum_frac, uv_accum_int;
	int line;
	uint32 how_early = 0;
	int32 first_num, restart_to_first_active_pixel_to_FIFO;
	uint32 time_to_active;
	
	// this is all black magic - you are not supposed to understand this	
	h_first = 9;
	f_first = 0;

	tmp_uv_accum_sum = params->uv_accum_init << (TV_UV_INC_FIX_SHIFT - TV_UV_INIT_FIX_SHIFT);
	uv_accum_frac = tmp_uv_accum_sum & (TV_UV_INC_FIX_SCALE - 1);
	uv_accum_int = (tmp_uv_accum_sum >> TV_UV_INC_FIX_SHIFT) & 7;
	
	// at line disp + 18 the accumulator is initialized;
	// simulate timing during vertical blank and find the last CRT line where
	// a new TV line is started
	// (actually, I think this calculation is wrong)
	for( line = mode->timing.v_display - 1 + 18; line < mode->timing.v_total - 2; ++line ) {
		if( uv_accum_int > 0 ) {
			--uv_accum_int;
		} else {
			v_first = line + 1;
			how_early = uv_accum_frac * mode->timing.h_total;
			uv_accum_int = ((uv_accum_frac + params->uv_inc) >> TV_UV_INC_FIX_SHIFT) - 1;
			uv_accum_frac = (uv_accum_frac + params->uv_inc) & (TV_UV_INC_FIX_SCALE - 1);
		}
	}
	
	SHOW_FLOW( 3, "f_first=%d, v_first=%d, h_first=%d", f_first, v_first, h_first );

	// theoretical time when restart should be started
	first_num = 
		f_first * mode->timing.v_total * mode->timing.h_total
		+ v_first * mode->timing.h_total
		+ h_first;
		
	first_num += (how_early + TV_UV_INC_FIX_SCALE / 2) >> TV_UV_INC_FIX_SHIFT;
	
	SHOW_FLOW( 3, "first_num=%d", first_num );
	
	// TV logic needs extra clocks to restart
	//params->tv_clocks_to_active = 0;
	time_to_active = params->tv_clocks_to_active + 3;
    
	SHOW_FLOW( 3, "time_to_active=%d, crt_freq=%d, tv_freq=%d", 
		time_to_active, params->crt_dividers.freq, params->tv_dividers.freq );

	// get delay until first bytes can be read from FIFO
	restart_to_first_active_pixel_to_FIFO =
		(int)( 
			(int64)time_to_active * params->crt_dividers.freq / params->tv_dividers.freq
			- (int64)h_blank * params->crt_dividers.freq / params->tv_dividers.freq / 2)
		- mode->timing.h_display / 2
		+ mode->timing.h_total / 2;

	// do restart a bit early to compensate delays
	first_num -= restart_to_first_active_pixel_to_FIFO;
	
	SHOW_FLOW( 3, "restart_to_first_active_pixel_to_FIFO=%d", restart_to_first_active_pixel_to_FIFO );
	
	SHOW_FLOW( 3, "after delay compensation first_num=%d", first_num );
	
	//first_num = 625592;
	
	// make restart time positive
	// ("%" operator doesn't like negative numbers)
    first_num += f_total * mode->timing.v_total * mode->timing.h_total;

	//SHOW_FLOW( 2, "first_num=%d", first_num );
	
	// convert clocks to screen position
	params->f_restart = (first_num / (mode->timing.v_total * mode->timing.h_total)) % f_total;
	first_num %= mode->timing.v_total * mode->timing.h_total;
	params->v_restart = (first_num / mode->timing.h_total) % mode->timing.v_total;
	first_num %= mode->timing.h_total;
	params->h_restart = first_num;
	
	/*params->v_restart = 623;
	params->h_restart = 580;*/
	
	SHOW_FLOW( 2, "Restart in frame %d, line %d, pixel %d", 
		params->f_restart, params->v_restart, params->h_restart );
}


// thresholds for flicker fixer algorithm
static int8 y_flicker_removal[5] = { 6, 5, 4, 3, 2 };

// associated filter parameters scaled by 8(!)
static int8 y_saw_tooth_slope[5] = { 1, 2, 2, 4, 8 };
// these values are not scaled
static int8 y_coeff_value[5] = { 2, 2, 0, 4, 0 };
static bool y_coeff_enable[5] = { 1, 1, 0, 1, 0 };

// fixed point resolution of saw filter parameters
#define TV_SAW_FILTER_FIX_SHIFT 13
#define TV_SAW_FILTER_FIX_SCALE (1 << TV_SAW_FILTER_FIX_SHIFT)

// fixed point resolution of flat filter parameter
#define TV_Y_COEFF_FIX_SHIFT 8
#define TV_Y_COEFF_FIX_SCALE (1 << TV_Y_COEFF_FIX_SHIFT)


// calculate flicker fixer parameters
static void Radeon_CalcImpacTVFlickerFixer( 
	impactv_params *params )
{
	int flicker_removal;
	uint i;
	int lower_border, upper_border;
	
	// flicker_removal must be within [uv_inc..uv_inc*2); take care of fraction
	lower_border = ((params->uv_inc + TV_UV_INC_FIX_SCALE - 1) >> TV_UV_INC_FIX_SHIFT);
	upper_border = ((2 * params->uv_inc) >> TV_UV_INC_FIX_SHIFT);
	
	for( i = 0; i < B_COUNT_OF( y_flicker_removal ); ++i ) {
		if( lower_border <= y_flicker_removal[i] &&
			upper_border > y_flicker_removal[i] )
			break;
	}
	
	// use least aggresive filtering if not in list
	if( i >= B_COUNT_OF( y_flicker_removal ))
		i = B_COUNT_OF( y_flicker_removal ) - 1;
		
	flicker_removal = y_flicker_removal[i];
	
	SHOW_FLOW( 3, "flicker removal=%d", flicker_removal );
		
	params->y_saw_tooth_slope = y_saw_tooth_slope[i] * (TV_SAW_FILTER_FIX_SCALE / 8);
	params->y_saw_tooth_amp = ((uint32)params->y_saw_tooth_slope * params->uv_inc) >> TV_UV_INC_FIX_SHIFT;
	params->y_fall_accum_init = ((uint32)params->y_saw_tooth_slope * params->uv_accum_init) >> TV_UV_INIT_FIX_SHIFT;
	
	SHOW_FLOW( 3, "%d < %d ?", 
		(flicker_removal << 16) - ((int32)params->uv_inc << (16 - TV_UV_INC_FIX_SHIFT)),
		((int32)params->uv_accum_init << (16 - TV_UV_INIT_FIX_SHIFT)) );
		
	if( (flicker_removal << 16) - ((int32)params->uv_inc << (16 - TV_UV_INC_FIX_SHIFT)) 
		< ((int32)params->uv_accum_init << (16 - TV_UV_INIT_FIX_SHIFT)))
	{
		params->y_rise_accum_init = 
			(((flicker_removal << TV_UV_INIT_FIX_SHIFT) - params->uv_accum_init) *
			params->y_saw_tooth_slope) >> TV_UV_INIT_FIX_SHIFT;
	} else {
		params->y_rise_accum_init = 
			(((flicker_removal << TV_UV_INIT_FIX_SHIFT) - params->uv_accum_init - params->y_accum_init) *
			params->y_saw_tooth_slope) >> TV_UV_INIT_FIX_SHIFT;
	}
	
	params->y_coeff_enable = y_coeff_enable[i];
	params->y_coeff_value = y_coeff_value[i] * TV_Y_COEFF_FIX_SCALE / 8;
}


// correct sync position after tweaking total size
static void Radeon_AdoptSync( 
	const display_mode *mode, display_mode *tweaked_mode )
{
	uint16 
		h_over_plus, h_sync_width, tweaked_h_over_plus,
		v_over_plus, v_sync_width, tweaked_v_over_plus;
	
	h_over_plus = mode->timing.h_sync_start - mode->timing.h_display;
	h_sync_width = mode->timing.h_sync_end - mode->timing.h_sync_start;

	// we want start of sync at same relative position of blank	
	tweaked_h_over_plus = (uint32)h_over_plus * 
		(tweaked_mode->timing.h_total - mode->timing.h_display - h_sync_width ) /
		(mode->timing.h_total - mode->timing.h_display - h_sync_width);

	tweaked_mode->timing.h_sync_start = mode->timing.h_display + tweaked_h_over_plus;
	tweaked_mode->timing.h_sync_end = tweaked_mode->timing.h_sync_start + h_sync_width;

	v_over_plus = mode->timing.v_sync_start - mode->timing.v_display;
	v_sync_width = mode->timing.v_sync_end - mode->timing.v_sync_start;
	
	tweaked_v_over_plus = (uint32)v_over_plus * 
		(tweaked_mode->timing.v_total - mode->timing.v_display - v_sync_width ) /
		(mode->timing.v_total - mode->timing.v_display - v_sync_width);

	// we really should verify whether the resulting mode is still valid;
	// this is a start
	tweaked_v_over_plus = min( 1, tweaked_v_over_plus );

	tweaked_mode->timing.v_sync_start = mode->timing.v_display + tweaked_v_over_plus;
	tweaked_mode->timing.v_sync_end = tweaked_mode->timing.v_sync_start + v_sync_width;
}


static const uint16 hor_timing_NTSC[RADEON_TV_TIMING_SIZE] = {
	// moved to left as much as possible
	0x0007, 0x003f, 0x0263, 0x0a24, 0x2a6b, 0x0a36, 0x126d-100, 0x1bfe, 
	0x1a8f+100, 0x1ec7, 0x3863, 0x1bfe, 0x1bfe, 0x1a2a, 0x1e95, 0x0e31, 
	0x201b, 0 
};

static const uint16 vert_timing_NTSC[RADEON_TV_TIMING_SIZE] = {
	0x2001, 0x200d, 0x1006, 0x0c06, 0x1006, 0x1818, 0x21e3, 0x1006,
	0x0c06, 0x1006, 0x1817, 0x21d4, 0x0002, 0
};

static const uint16 hor_timing_PAL[RADEON_TV_TIMING_SIZE] = {
	0x0007, 0x0058, 0x027c, 0x0a31, 0x2a77, 0x0a95, 0x124f - 60, 0x1bfe, 
	0x1b22 + 60, 0x1ef9, 0x387c, 0x1bfe, 0x1bfe, 0x1b31, 0x1eb5, 0x0e43,
	0x201b, 0
};

static const uint16 vert_timing_PAL[RADEON_TV_TIMING_SIZE] = { 
	0x2001, 0x200c, 0x1005, 0x0c05, 0x1005, 0x1401, 0x1821, 0x2240,
	0x1005, 0x0c05, 0x1005, 0x1401, 0x1822, 0x2230, 0x0002, 0
};

static const uint16 *hor_timings[] = {
	hor_timing_NTSC, 
	hor_timing_PAL, 
	hor_timing_NTSC,		// palm: looks similar to NTSC, but differs slightly
	hor_timing_NTSC,		// palnc: looks a bit like NTSC, probably won't work
	hor_timing_PAL,			// scart pal
	hor_timing_PAL			// pal 60: horizontally, it is PAL
};

static const uint16 *vert_timings[] = {
	vert_timing_NTSC,
	vert_timing_PAL,
	vert_timing_NTSC,		// palm: vertically, this is PAL
	vert_timing_PAL,		// palnc: a bit like PAL, but not really
	vert_timing_PAL,		// scart pal
	vert_timing_NTSC		// pal 60: vertically, it is NTSC
};


// timing of TV standards;
// the index is of type tv_standard
static const tv_timing radeon_std_tv_timing[6] = {
	// TK: hand-tuned v_active_lines and horizontal zoom
	{42954540, 2730, 200, 28, 200, 110, 2170, 525, 466/*440*/, 525, 2, 1, 0, 0.95/*0.88*/ * FIX_SCALE/2},	/* ntsc */
	// TK: frame_size_adjust was -6, but using 12 leads to perfect 25 Hz
	{53203425, 3405, 250, 28, 320, 80, 2627, 625, 555/*498*/, 625, 2, 3, 12, 0.98/*0.91*/ * FIX_SCALE/2},	/* pal */
	{42907338, 2727, 200, 28, 200, 110, 2170, 525, 440, 525, 2, 1, 0, 0.91 * FIX_SCALE/2},	/* palm */
	{42984675, 2751, 202, 28, 202, 110, 2190, 625, 510, 625, 2, 3, 0, 0.91 * FIX_SCALE/2},	/* palnc */
	{53203425, 3405, 250, 28, 320, 80, 2627, 625, 498, 625, 2, 3, 0, 0.91 * FIX_SCALE/2},	/* scart pal ??? */
	{53203425, 3405, 250, 28, 320, 80, 2627, 525, 440, 525, 2, 1, 0, 0.91 * FIX_SCALE/2},	/* pal 60 */
};


// adjust timing so it fills the entire visible area;
// the result may not be CRT compatible!
static void Radeon_MakeOverscanMode( 
	display_timing *timing, tv_standard_e tv_format )
{
	const tv_timing *tv_timing = &radeon_std_tv_timing[tv_format-1];

	// vertical is easy: ratio of displayed lines and blank must be 
	// according to TV standard, having the sync delay of 1 line and
	// sync len of 3 lines is used by most VESA modes
	timing->v_total = timing->v_display * tv_timing->v_total / tv_timing->v_active_lines;
	timing->v_sync_start = timing->v_display + 1;
	timing->v_sync_end = timing->v_sync_start + 3;
	
	// horizontal is tricky: the ratio may not be important (as it's
	// scaled by the TV-out unit anyway), but the sync position and length
	// is pure guessing - VESA modes don't exhibit particular scheme
	timing->h_total = timing->h_display * tv_timing->h_total / tv_timing->h_active_len;
	timing->h_sync_start = min( timing->h_total * 30 / 1000, 2 * 8 ) + timing->h_display;
	timing->h_sync_end = min( timing->h_total * 80 / 1000, 3 * 8 ) + timing->h_sync_start;
	
	// set some pixel clock - it's replaced during fine tuning anyway
	timing->pixel_clock = timing->h_total * timing->v_total * 60;
	// most (but not all) 60 Hz modes have all negative sync, so use that too
	timing->flags = 0;
	
	SHOW_INFO0( 4, "got:" );
	SHOW_INFO( 4, "H: %4d %4d %4d %4d", 
		timing->h_display, timing->h_sync_start, 
		timing->h_sync_end, timing->h_total );
	SHOW_INFO( 4, "V: %4d %4d %4d %4d", 
		timing->v_display, timing->v_sync_start, 
		timing->v_sync_end, timing->v_total );
	SHOW_INFO( 4, "clk: %ld", timing->pixel_clock );
}


#define TV_VERT_LEAD_IN_LINES 2
#define TV_UV_ADR_INI 0xc8

// calculate TV parameters
void Radeon_CalcImpacTVParams( 
	const general_pll_info *general_pll, impactv_params *params, 
	tv_standard_e tv_format, bool internal_encoder, 
	const display_mode *mode, display_mode *tweaked_mode )
{
	pll_info tv_pll, crt_pll;
	uint16 start_line, lines_before_active;
	const tv_timing *tv_timing = &radeon_std_tv_timing[tv_format-1];
	
	SHOW_FLOW( 2, "internal_encoder=%s, format=%d", 
		internal_encoder ? "yes" : "no", tv_format );
		
	if( tv_format < ts_ntsc || tv_format > ts_max )
		tv_format = ts_ntsc;
	
	params->mode888 = true;
	params->timing = *tv_timing;

	Radeon_GetTVPLLConfiguration( general_pll, &tv_pll, internal_encoder );
	Radeon_CalcPLLDividers( &tv_pll, tv_timing->freq, 0, &params->tv_dividers );
	
	Radeon_GetTVCRTPLLConfiguration( general_pll, &crt_pll, internal_encoder );

	// initially, we try to keep to requested mode
	*tweaked_mode = *mode;
	
	Radeon_MakeOverscanMode( &tweaked_mode->timing, tv_format );

	// tweak CRT mode if necessary to match TV frame timing
	Radeon_MatchCRTPLL( 
		&crt_pll, 
		tv_timing->v_total, tv_timing->h_total, tv_timing->frame_size_adjust,
		tv_timing->freq, 
		tweaked_mode, 2, 40,
		internal_encoder ? 0/*6*/ : 0, 2 + params->mode888,
		&params->crt_dividers, tweaked_mode );

	// adopt synchronization to make tweaked mode look like original mode
	Radeon_AdoptSync( mode, tweaked_mode );

	// timing magic		
	start_line = 
		tv_timing->h_sync_len
		+ tv_timing->h_setup_delay
		+ tv_timing->h_active_delay
		- tv_timing->h_genclk_delay;
		
	lines_before_active =
		(tv_timing->v_field_total - tv_timing->v_active_lines) / 2 - 1 
		- TV_VERT_LEAD_IN_LINES + 1;
		
	SHOW_FLOW( 3, "lines_before_active=%d, start_line=%d", lines_before_active, start_line );

	params->tv_clocks_to_active = (uint32)lines_before_active * tv_timing->h_total + start_line;
	
	// calculate scaling.
	// this must be done before CalcTVRestart() or TVFlickerFixer() is called
	// start accumulator always with 0.25
	params->uv_accum_init = 0x10;
	// this value seems to be fixed (it's not written to any register but used
	// at some calculations)
	params->y_accum_init = 0;
	// for scaling ratio, take care that v_field_total is for full, not for half frames,
	// therefore we devide v_field_total by 2
	params->uv_inc = (tweaked_mode->timing.v_total << TV_UV_INC_FIX_SHIFT) 
		* 2 / tv_timing->v_field_total;
		
	SHOW_FLOW( 3, "uv_inc=%d", params->uv_inc );
		
	params->h_inc = 
		((int64)tweaked_mode->timing.h_display * 4096 /
		(tv_timing->h_active_len + tv_timing->h_active_delay) << (FIX_SHIFT - 1)) / tv_timing->scale;
	
	Radeon_CalcImpacTVRestart( params, tweaked_mode, 
		tv_timing->h_total - tv_timing->h_active_len, tv_timing->f_total );
	Radeon_CalcImpacTVFlickerFixer( params );
}


// standard upsample coefficients (external Theatre only)
static uint32 std_upsample_filter_coeff[RADEON_TV_UPSAMP_COEFF_NUM] = {
	0x3f010000, 0x7b008002, 0x00003f01, 
	0x341b7405, 0x7f3a7617, 0x00003d04, 
	0x2d296c0a, 0x0e316c2c,	0x00003e7d, 
	0x2d1f7503, 0x2927643b, 0x0000056f,
	0x29257205, 0x25295050, 0x00000572
};


// compose TV register content
// as TV-Out uses a CRTC, it reprograms a PLL to create an unscaled image;
// as a result, you must not call Radeon_CalcPLLRegisters() afterwards
// TBD: what's special in terms of PLL in TV-Out mode?
void Radeon_CalcImpacTVRegisters( 
	accelerator_info *ai, display_mode *mode, 
	impactv_params *params, impactv_regs *values, int crtc_idx, 
	bool internal_encoder, tv_standard_e tv_format, display_device_e display_device )
{
	const tv_timing *timing = &params->timing;
	
	SHOW_FLOW0( 2, "" );
	
	if( tv_format < ts_ntsc || tv_format > ts_max )
		tv_format = ts_ntsc;
	
	values->tv_ftotal = timing->f_total;
	
	// RE: UV_THINNER should affect sharpness only, but the only effect is that
	// the colour fades out, so I leave it zero
	values->tv_vscaler_cntl1 = RADEON_TV_VSCALER_CNTL1_Y_W_EN;
	
	values->tv_vscaler_cntl1 = 
		(values->tv_vscaler_cntl1 & 0xe3ff0000) |
		params->uv_inc;
		
	if( internal_encoder ) {
		// RE: was on - update: disabling it breaks restart
		values->tv_vscaler_cntl1 |= RADEON_TV_VSCALER_CNTL1_RESTART_FIELD;
		if( mode->timing.h_display == 1024 )
			values->tv_vscaler_cntl1 |= 4 << RADEON_TV_VSCALER_CNTL1_Y_DEL_W_SIG_SHIFT;
		else
			values->tv_vscaler_cntl1 |= 2 << RADEON_TV_VSCALER_CNTL1_Y_DEL_W_SIG_SHIFT;
	} else {
		values->tv_vscaler_cntl1 |= 2 << RADEON_TV_VSCALER_CNTL1_Y_DEL_W_SIG_SHIFT;
	}
	
	values->tv_y_saw_tooth_cntl =
		params->y_saw_tooth_amp |
		(params->y_saw_tooth_slope << RADEON_TV_Y_SAW_TOOTH_CNTL_SLOPE_SHIFT);
	
	values->tv_y_fall_cntl =
		params->y_fall_accum_init |
		RADEON_TV_Y_FALL_CNTL_Y_FALL_PING_PONG |
		(params->y_coeff_enable ? RADEON_TV_Y_FALL_CNTL_Y_COEFF_EN : 0) |
		(params->y_coeff_value << RADEON_TV_Y_FALL_CNTL_Y_COEFF_VALUE_SHIFT);
		
	values->tv_y_rise_cntl = 
		params->y_rise_accum_init |
		RADEON_TV_Y_RISE_CNTL_Y_RISE_PING_PONG;

	// RE: all dither flags/values were zero
	values->tv_vscaler_cntl2 =
		(values->tv_vscaler_cntl2 & 0x00fffff0) | 
		(params->uv_accum_init << RADEON_TV_VSCALER_CNTL2_UV_ACCUM_INIT_SHIFT);
		
	if( internal_encoder ) {
		values->tv_vscaler_cntl2 |= 
			RADEON_TV_VSCALER_CNTL2_DITHER_MODE |
			RADEON_TV_VSCALER_CNTL2_Y_OUTPUT_DITHER_EN |
			RADEON_TV_VSCALER_CNTL2_UV_OUTPUT_DITHER_EN |
			RADEON_TV_VSCALER_CNTL2_UV_TO_BUF_DITHER_EN;
	}

	values->tv_hrestart = params->h_restart;
	values->tv_vrestart = params->v_restart;
	values->tv_frestart = params->f_restart;
	
	values->tv_tv_pll_cntl = 
		(params->tv_dividers.ref & RADEON_TV_PLL_CNTL_TV_M0_LO_MASK) |
		((params->tv_dividers.feedback & RADEON_TV_PLL_CNTL_TV_N0_LO_MASK) 
			<< RADEON_TV_PLL_CNTL_TV_N0_LO_SHIFT) |
		((params->tv_dividers.ref >> RADEON_TV_PLL_CNTL_TV_M0_LO_BITS) 
			<< RADEON_TV_PLL_CNTL_TV_M0_HI_SHIFT) |
		((params->tv_dividers.feedback >> RADEON_TV_PLL_CNTL_TV_N0_LO_BITS) 
			<< RADEON_TV_PLL_CNTL_TV_N0_HI_SHIFT) |
		// RE: was on
		//RADEON_TV_PLL_CNTL_TV_SLIP_EN |
		(params->tv_dividers.post << RADEON_TV_PLL_CNTL_TV_P_SHIFT); //|
		// RE: was on
		//RADEON_TV_PLL_CNTL_TV_DTO_EN;
	values->tv_crt_pll_cntl = 
		(params->crt_dividers.ref & RADEON_TV_CRT_PLL_CNTL_M0_LO_MASK) |
		((params->crt_dividers.feedback & RADEON_TV_CRT_PLL_CNTL_N0_LO_MASK) 
			<< RADEON_TV_CRT_PLL_CNTL_N0_LO_SHIFT) |
		((params->crt_dividers.ref >> RADEON_TV_CRT_PLL_CNTL_M0_LO_BITS) 
			<< RADEON_TV_CRT_PLL_CNTL_M0_HI_SHIFT) |
		((params->crt_dividers.feedback >> RADEON_TV_CRT_PLL_CNTL_N0_LO_BITS) 
			<< RADEON_TV_CRT_PLL_CNTL_N0_HI_SHIFT) |
		(params->crt_dividers.extra_post == 2 ? RADEON_TV_CRT_PLL_CNTL_CLKBY2 : 0);

	// TK: from Gatos
	// in terms of byte clock devider, I have no clue how that works, 
	// but leaving it 1 seems to be save
	values->tv_clock_sel_cntl = 
		0x33 |
		((/*params->crt_dividers.post_code - 1*/0) << RADEON_TV_CLOCK_SEL_CNTL_BYTCLK_SHIFT) |
		(1 << RADEON_TV_CLOCK_SEL_CNTL_BYTCLKD_SHIFT);
    
    values->tv_clkout_cntl = 0x09;
	if( !internal_encoder )
		values->tv_clkout_cntl |= 1 << 5;

	values->tv_htotal = mode->timing.h_total - 1;
	values->tv_hsize = mode->timing.h_display;
	values->tv_hdisp = mode->timing.h_display - 1;
	values->tv_hstart =
		// TK: was -12, but this cuts off the left border of the image
		values->tv_hdisp + 1 - params->mode888 + 12;
		
	values->tv_vtotal = mode->timing.v_total - 1;
	values->tv_vdisp = mode->timing.v_display - 1;
	values->tv_sync_size = mode->timing.h_display + 8;

	values->tv_timing_cntl = 
		(values->tv_timing_cntl & 0xfffff000) |
		params->h_inc;
	
	if( ai->si->asic >= rt_r300 ) {
		// this is a hack to fix improper UV scaling
		// (at least this is what the sample code says)
		values->tv_timing_cntl =
			(values->tv_timing_cntl & 0x00ffffff) |
			((0x72 * 640 / mode->timing.h_display) 
				<< RADEON_TV_TIMING_CNTL_UV_OUTPUT_POST_SCALE_SHIFT);
	}
	
	if( internal_encoder ) {
		// tell TV-DAC to generate proper NTSC/PAL signal
		values->tv_dac_cntl =
			RADEON_TV_DAC_CNTL_NBLANK |
			RADEON_TV_DAC_CNTL_NHOLD |
			(8 << RADEON_TV_DAC_CNTL_BGADJ_SHIFT) |
			(6 << RADEON_TV_DAC_CNTL_DACADJ_SHIFT);
			
		switch( tv_format ) {
		case ts_ntsc:
			values->tv_dac_cntl |= RADEON_TV_DAC_CNTL_STD_NTSC;
			break;
			
		case ts_pal_bdghi:
		case ts_pal_m:
		case ts_pal_nc:
		case ts_scart_pal:
		case ts_pal_60:
			values->tv_dac_cntl |= RADEON_TV_DAC_CNTL_STD_PAL;
			break;
		default:
			;
		}
		
		// enable composite or S-Video DAC
		values->tv_dac_cntl |=
			RADEON_TV_DAC_CNTL_RDACPD |
			RADEON_TV_DAC_CNTL_GDACPD |
			RADEON_TV_DAC_CNTL_BDACPD;
			
		if( (display_device & dd_ctv) != 0 ) {
			values->tv_dac_cntl &=
				~RADEON_TV_DAC_CNTL_BDACPD;
		}
		
		if( (display_device & dd_stv) != 0 ) {
			values->tv_dac_cntl &= 
				~(RADEON_TV_DAC_CNTL_RDACPD |
				  RADEON_TV_DAC_CNTL_GDACPD);
		}
	} else {
		values->tv_dac_cntl =
			(values->tv_dac_cntl & ~(RADEON_TV_DAC_CNTL_STD_NTSC | 0x88 | 
				RADEON_TV_DAC_CNTL_BGSLEEP | RADEON_TV_DAC_CNTL_PEDESTAL)) |
			RADEON_TV_DAC_CNTL_DETECT | 
			RADEON_TV_DAC_CNTL_NBLANK |
			RADEON_TV_DAC_CNTL_NHOLD;
	}

	values->tv_modulator_cntl1 &= ~(
		RADEON_TV_MODULATOR_CNTL1_ALT_PHASE_EN | 
		RADEON_TV_MODULATOR_CNTL1_SYNC_TIP_LEVEL | 
		RADEON_TV_MODULATOR_CNTL1_SET_UP_LEVEL_MASK | 
		RADEON_TV_MODULATOR_CNTL1_BLANK_LEVEL_MASK);
			
	switch( tv_format ) {
	case ts_ntsc:
		// RE: typo?
		//values->tv_dac_cntl |=
		values->tv_modulator_cntl1 |= 
			RADEON_TV_MODULATOR_CNTL1_SYNC_TIP_LEVEL |
			(0x46 << RADEON_TV_MODULATOR_CNTL1_SET_UP_LEVEL_SHIFT) |
			(0x3b << RADEON_TV_MODULATOR_CNTL1_BLANK_LEVEL_SHIFT);
		values->tv_modulator_cntl2 = 
			(-111 & TV_MODULATOR_CNTL2_U_BURST_LEVEL_MASK) |
			((0 & TV_MODULATOR_CNTL2_V_BURST_LEVEL_MASK) << TV_MODULATOR_CNTL2_V_BURST_LEVEL_SHIFT);
		break;
		
	case ts_pal_bdghi:
		values->tv_modulator_cntl1 |= 
			RADEON_TV_MODULATOR_CNTL1_ALT_PHASE_EN |
			RADEON_TV_MODULATOR_CNTL1_SYNC_TIP_LEVEL |
			(0x3b << RADEON_TV_MODULATOR_CNTL1_SET_UP_LEVEL_SHIFT) |
			(0x3b << RADEON_TV_MODULATOR_CNTL1_BLANK_LEVEL_SHIFT);
		values->tv_modulator_cntl2 = 
			(-78 & TV_MODULATOR_CNTL2_U_BURST_LEVEL_MASK) |
			((62 & TV_MODULATOR_CNTL2_V_BURST_LEVEL_MASK) << TV_MODULATOR_CNTL2_V_BURST_LEVEL_SHIFT);
		break;
		
	case ts_scart_pal:
		// from register spec
		values->tv_modulator_cntl1 |= 
			RADEON_TV_MODULATOR_CNTL1_ALT_PHASE_EN |
			RADEON_TV_MODULATOR_CNTL1_SYNC_TIP_LEVEL;
		values->tv_modulator_cntl2 = 
			(0 & TV_MODULATOR_CNTL2_U_BURST_LEVEL_MASK) |
			((0 & TV_MODULATOR_CNTL2_V_BURST_LEVEL_MASK) << TV_MODULATOR_CNTL2_V_BURST_LEVEL_SHIFT);
		break;
		
    default:
		// there are many formats missing, sigh...
		;
    }
    
    // RE:
    values->tv_modulator_cntl1 |=
    	RADEON_TV_MODULATOR_CNTL1_YFLT_EN |
    	RADEON_TV_MODULATOR_CNTL1_UVFLT_EN |
    	RADEON_TV_MODULATOR_CNTL1_SLEW_RATE_LIMIT |
    	(2 << RADEON_TV_MODULATOR_CNTL1_CY_FILT_BLEND_SHIFT);

	if( internal_encoder ) {
		values->tv_data_delay_a = 0x0b0c0a06;
		values->tv_data_delay_b = 0x070a0a0c;
	} else {
		values->tv_data_delay_a = 0x07080604;
		values->tv_data_delay_b = 0x03070607;
	}
	
	values->tv_frame_lock_cntl = internal_encoder ? 0 : 0xf0f;

	if( internal_encoder ) {
		values->tv_pll_cntl1 = 
			(4 << RADEON_TV_PLL_CNTL1_TVPCP_SHIFT) |
			(4 << RADEON_TV_PLL_CNTL1_TVPVG_SHIFT) |
			// RE: was 2
			(1 << RADEON_TV_PLL_CNTL1_TVPDC_SHIFT) |
			RADEON_TV_PLL_CNTL1_TVCLK_SRC_SEL_TVPLLCLK |
			RADEON_TV_PLL_CNTL1_TVPLL_TEST;
			
		values->tv_rgb_cntl = 
			((crtc_idx == 1 ? 2 : 0) << RADEON_TV_RGB_CNTL_RGB_SRC_SEL_SHIFT) |
			RADEON_TV_RGB_CNTL_RGB_DITHER_EN |
			(0xb << RADEON_TV_RGB_CNTL_UVRAM_READ_MARGIN_SHIFT) |
			(7 << RADEON_TV_RGB_CNTL_FIFORAM_FIFOMACRO_READ_MARGIN_SHIFT);
			
		// RE:
		values->tv_rgb_cntl |= 0x4000000;
			
		values->tv_pre_dac_mux_cntl = 
			RADEON_TV_PRE_DAC_MUX_CNTL_Y_RED_EN |
			RADEON_TV_PRE_DAC_MUX_CNTL_C_GRN_EN |
			RADEON_TV_PRE_DAC_MUX_CNTL_CMP_BLU_EN |
			RADEON_TV_PRE_DAC_MUX_CNTL_DAC_DITHER_EN;
		
		// RE:	
		/* |
			(0x2c << RADEON_TV_PRE_DAC_MUX_CNTL_FORCE_DAC_DATA_SHIFT);*/
	} else {
		// this register seems to have completely different meaning on Theatre chip
		values->tv_pll_cntl1 =
			(1 << 3) | (1 << 4) | (4 << 8) | (1 << 11)
		    | (5 << 13) | (4 << 16) | (1 << 19) | (5 << 21);
		    
		// this one too
		values->tv_rgb_cntl = params->mode888;
		
		values->tv_pre_dac_mux_cntl = 
			RADEON_TV_PRE_DAC_MUX_CNTL_Y_RED_EN |
			RADEON_TV_PRE_DAC_MUX_CNTL_C_GRN_EN |
			RADEON_TV_PRE_DAC_MUX_CNTL_CMP_BLU_EN |
			RADEON_TV_PRE_DAC_MUX_CNTL_DAC_DITHER_EN;
			// RE:
			/*(0xaf << RADEON_TV_PRE_DAC_MUX_CNTL_FORCE_DAC_DATA_SHIFT);*/
	}
	
	values->tv_pll_fine_cntl = 0;
	
	// TBD: this is certainly broken 
	// (they do an ((orig & 0xe0) & 0x600) which is constant zero)
	values->tv_master_cntl = 
		RADEON_TV_MASTER_CNTL_CRT_FIFO_CE_EN |
		RADEON_TV_MASTER_CNTL_TV_FIFO_CE_EN;

	if( tv_format == ts_ntsc )
		values->tv_master_cntl |= RADEON_TV_MASTER_CNTL_RESTART_PHASE_FIX;
	else
		values->tv_master_cntl &= ~RADEON_TV_MASTER_CNTL_RESTART_PHASE_FIX;
		
	// this is missing in the sample code
	if( internal_encoder )
		values->tv_master_cntl |= RADEON_TV_MASTER_CNTL_TV_ON;
	else
		values->tv_master_cntl |= 
			RADEON_TV_MASTER_CNTL_RESTART_PHASE_FIX | // RE: guessed
			
			RADEON_TV_MASTER_CNTL_VIN_ASYNC_RST |
			RADEON_TV_MASTER_CNTL_AUD_ASYNC_RST |
			RADEON_TV_MASTER_CNTL_DVS_ASYNC_RST;
	
	values->tv_gain_limit_settings = 0x017f05ff;
	values->tv_linear_gain_settings = 0x01000100;
	values->tv_upsamp_and_gain_cntl = 0x00000005;
	values->tv_crc_cntl = 0;
	
	SHOW_FLOW( 2, "tv_master_cntl=%x", values->tv_master_cntl );

	memcpy( values->tv_upsample_filter_coeff, std_upsample_filter_coeff, 
		RADEON_TV_UPSAMP_COEFF_NUM * sizeof( uint32 ));

	// setup output timing	
	memcpy( values->tv_hor_timing, hor_timings[tv_format-1], 
		RADEON_TV_TIMING_SIZE * sizeof( uint16 ));
	memcpy( values->tv_vert_timing, vert_timings[tv_format-1], 
		RADEON_TV_TIMING_SIZE * sizeof( uint16 ) );
	
	// arbitrary position of vertical timing table in FIFO
	values->tv_uv_adr = TV_UV_ADR_INI;
}


// get address of horizontal timing table in FIFO 
static uint16 getHorTimingTableAddr( 
	impactv_regs *values, bool internal_encoder )
{
	switch( (values->tv_uv_adr & RADEON_TV_UV_ADR_HCODE_TABLE_SEL_MASK) 
		>> RADEON_TV_UV_ADR_HCODE_TABLE_SEL_SHIFT ) 
	{
	case 0:
		return internal_encoder ? RADEON_TV_MAX_FIFO_ADDR_INTERN : RADEON_TV_MAX_FIFO_ADDR;
		
	case 1:
		return ((values->tv_uv_adr & RADEON_TV_UV_ADR_TABLE1_BOT_ADR_MASK) 
			>> RADEON_TV_UV_ADR_TABLE1_BOT_ADR_SHIFT) * 2;
	case 2:
		return ((values->tv_uv_adr & RADEON_TV_UV_ADR_TABLE3_TOP_ADR_MASK) 
			>> RADEON_TV_UV_ADR_TABLE3_TOP_ADR_SHIFT) * 2;
			
	default:
		return 0;
	}
}

// get address of vertical timing table in FIFO 
static uint16 getVertTimingTableAddr( 
	impactv_regs *values )
{
	switch( (values->tv_uv_adr & RADEON_TV_UV_ADR_VCODE_TABLE_SEL_MASK) 
		>> RADEON_TV_UV_ADR_VCODE_TABLE_SEL_SHIFT )
	{
	case 0:
		return ((values->tv_uv_adr & RADEON_TV_UV_ADR_MAX_UV_ADR_MASK) 
			>> RADEON_TV_UV_ADR_MAX_UV_ADR_SHIFT) * 2 + 1;
	
	case 1:
		return ((values->tv_uv_adr & RADEON_TV_UV_ADR_TABLE1_BOT_ADR_MASK) 
			>> RADEON_TV_UV_ADR_TABLE1_BOT_ADR_SHIFT) * 2 + 1;
	
	case 2:
		return ((values->tv_uv_adr & RADEON_TV_UV_ADR_TABLE3_TOP_ADR_MASK) 
			>> RADEON_TV_UV_ADR_TABLE3_TOP_ADR_SHIFT) * 2 + 1;
	
	default:
		return 0;
	}
}


// write horizontal timing table
void Radeon_ImpacTVwriteHorTimingTable( 
	accelerator_info *ai, impactv_write_FIFO write, impactv_regs *values, bool internal_encoder )
{
	uint16 addr = getHorTimingTableAddr( values, internal_encoder );
	int i;
	
	for( i = 0; i < RADEON_TV_TIMING_SIZE; i += 2, --addr ) {
		uint32 value = 
			((uint32)values->tv_hor_timing[i] << 14) | 
			values->tv_hor_timing[i + 1];
			
		write( ai, addr, value );

		if( values->tv_hor_timing[i] == 0 ||
			values->tv_hor_timing[i + 1] == 0 )
			break;
	}
}


// write vertical timing table
void Radeon_ImpacTVwriteVertTimingTable( 
	accelerator_info *ai, impactv_write_FIFO write, impactv_regs *values )
{
	uint16 addr = getVertTimingTableAddr( values );
	int i;

	for( i = 0; i < RADEON_TV_TIMING_SIZE; i += 2 , ++addr ) {
		uint32 value =
			((uint32)values->tv_vert_timing[i + 1] << 14) | 
			values->tv_vert_timing[i];

		write( ai, addr, value );

		if( values->tv_vert_timing[i + 1] == 0 ||
			values->tv_vert_timing[i] == 0 )
			break;
	}
}
