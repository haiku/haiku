/*
	Copyright (c) 2002/03, Thomas Kurschel
	

	Part of Radeon accelerant
		
	Programming of TV-out unit, both internal and Rage Theatre
*/

#include "radeon_interface.h"
#include "radeon_accelerant.h"

#include "pll_regs.h"
#include "theatre_regs.h"
#include "tv_out_regs.h"
#include "pll_access.h"
#include "utils.h"
#include "stddef.h"

// fixed-point resolution of UV scaler increment
#define TV_UV_INC_FIX_SHIFT 14
#define TV_UV_INC_FIX_SCALE (1 << TV_UV_INC_FIX_SHIFT)

// fixed point resolution of UV scaler initialization
#define TV_UV_INIT_FIX_SHIFT 6


// calculate time when TV timing must be restarted
static void Radeon_CalcTVRestart( tv_params *params, const display_mode *mode, 
	uint16 h_blank, uint16 f_total )
{
	uint32 h_first, v_first = 0, f_first;
	uint32 tmp_uv_accum_sum;
	uint16 uv_accum_frac, uv_accum_int;
	uint line;
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
	for( line = mode->timing.v_display - 1 + 18; line < mode->timing.v_total; ++line ) {
		if( uv_accum_int > 0 ) {
			--uv_accum_int;
		} else {
			v_first = line + 1;
			how_early = uv_accum_frac * mode->timing.h_total;
			uv_accum_int = ((uv_accum_frac + params->uv_inc) >> TV_UV_INC_FIX_SHIFT) - 1;
			uv_accum_frac = (uv_accum_frac + params->uv_inc) & (TV_UV_INC_FIX_SCALE - 1);
		}
	}
	
	//SHOW_FLOW( 2, "f_first=%d, v_first=%d, h_first=%d", f_first, v_first, h_first );

	// theoretical time when restart should be started
	first_num = 
		f_first * mode->timing.v_total * mode->timing.h_total
		+ v_first * mode->timing.h_total
		+ h_first;
		
	first_num += (how_early + TV_UV_INC_FIX_SCALE / 2) >> TV_UV_INC_FIX_SHIFT;
	
	// TV logic needs extra clocks to restart
    time_to_active = params->tv_clocks_to_active + 3;

	// get delay until first bytes can be read from FIFO
	restart_to_first_active_pixel_to_FIFO =
		(int)( 
			(int64)time_to_active * params->crt_dividers.freq / params->tv_dividers.freq
			- (int64)(h_blank * params->crt_dividers.freq / params->tv_dividers.freq) / 2)
		- mode->timing.h_display / 2
		+ mode->timing.h_total / 2;

	// do restart a bit early to compensate delays
	first_num -= restart_to_first_active_pixel_to_FIFO;
	
	SHOW_FLOW( 2, "restart_to_first_active_pixel_to_FIFO=%d", restart_to_first_active_pixel_to_FIFO );
	
	// make restart time positive
	// ("%" operator doesn't like negative numbers)
    first_num += f_total * mode->timing.v_total * mode->timing.h_total;

	//SHOW_FLOW( 2, "first_num=%d", first_num );
	
	// convert clocks to screen position
	params->f_restart = (first_num / (mode->timing.v_total * mode->timing.h_total)) % f_total;
	first_num %= mode->timing.v_total * mode->timing.h_total;
	params->v_restart = (first_num / mode->timing.h_total) % mode->timing.v_total;
	first_num %= mode->timing.v_total;
	params->h_restart = first_num;
	
	SHOW_FLOW( 2, "Restart in frame %d, line %d, pixel %d", 
		params->f_restart, params->v_restart, params->h_restart );
}


// thresholds for flicker fixer algorithm
static int8 y_flicker_removal[5] = { 6, 5, 4, 3, 2 };

// associated filter parameters scaled by 8(!)
static int8 y_saw_tooth_slope[5] = { 1, 2, 2, 4, 8 };
static int8 y_coeff_value[5] = { 2, 2, 0, 4, 0 };
// these values are not scaled
static bool y_coeff_enable[5] = { 1, 1, 0, 1, 0 };

#define countof( a ) (sizeof( (a) ) / sizeof( (a)[0] ))

// fixed point resolution of saw filter parameters
#define TV_SAW_FILTER_FIX_SHIFT 13
#define TV_SAW_FILTER_FIX_SCALE (1 << TV_SAW_FILTER_FIX_SHIFT)

// fixed point resolution of flat filter parameter
#define TV_Y_COEFF_FIX_SHIFT 8
#define TV_Y_COEFF_FIX_SCALE (1 << TV_Y_COEFF_FIX_SHIFT)


// calculate flicker fixer parameters
static void Radeon_CalcTVFlickerFixer( tv_params *params )
{
	int8 flicker_removal;
	uint i;
	
	// first, we determine how much flickering should be removed
	// I reckon that we could tweak it a bit hear as only
	// 	uv_inc <= flicker_removal < uv_inc * 2
	// must be assured
	flicker_removal = (params->uv_inc + (TV_UV_INC_FIX_SCALE / 2)) >> TV_UV_INC_FIX_SHIFT;
	
	for( i = 0; i < countof( y_flicker_removal ); ++i ) {
		if( flicker_removal == y_flicker_removal[i] )
			break;
	}
	
	// use most aggresive filtering if not in list
	if( i > countof( y_flicker_removal ))
		i = countof( y_flicker_removal ) - 1;
		
	params->y_saw_tooth_slope = y_saw_tooth_slope[i] * (TV_SAW_FILTER_FIX_SCALE / 8);
	params->y_saw_tooth_amp = ((uint32)params->y_saw_tooth_slope * params->uv_inc) >> TV_UV_INC_FIX_SHIFT;
	params->y_fall_accum_init = ((uint32)params->y_saw_tooth_slope * params->uv_accum_init) >> TV_UV_INC_FIX_SHIFT;
	if( flicker_removal - (params->uv_inc >> TV_UV_INC_FIX_SHIFT) 
		< (params->uv_accum_init >> TV_UV_INIT_FIX_SHIFT))
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
static void Radeon_AdoptSync( const display_mode *mode, display_mode *tweaked_mode )
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

#define TV_VERT_LEAD_IN_LINES 2


// calculate TV parameters
void Radeon_CalcTVParams( const general_pll_info *general_pll, tv_params *params, 
	const tv_timing *tv_timing, bool internal_encoder, 
	const display_mode *mode, display_mode *tweaked_mode )
{
	pll_info tv_pll, crt_pll;
	uint16 start_line, lines_before_active;
	
	SHOW_FLOW( 2, "internal_encoder=%s", internal_encoder ? "yes" : "no" );
	
	params->mode888 = true;

	Radeon_GetTVPLLConfiguration( general_pll, &tv_pll, internal_encoder );
	Radeon_CalcPLLDividers( &tv_pll, tv_timing->freq, 0, &params->tv_dividers );
	
	Radeon_GetTVCRTPLLConfiguration( general_pll, &crt_pll, internal_encoder );

	// initially, we try to keep to requested mode
	*tweaked_mode = *mode;

	// tweak CRT mode if necessary to match TV frame timing
	Radeon_MatchCRTPLL( 
		&crt_pll, 
		tv_timing->v_total, tv_timing->h_total, tv_timing->frame_size_adjust,
		tv_timing->freq, 
		mode, 2, 40,
		internal_encoder ? 6 : 0, 2 + params->mode888,
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

	params->tv_clocks_to_active = (uint32)lines_before_active * tv_timing->h_total + start_line;
	
	// calculate scaling.
	// this must be done CalcTVRestart() or TVFlickerFixer() is called
	// start accumulator always with 0.25
	params->uv_accum_init = 0x10;
	// this value seems to be fixed (it's not written to any register but used
	// at some calculations)
	params->y_accum_init = 0;
	// for scaling ratio, take care that v_field_total is for full, not for half frames,
	// therefore we devide it v_field_total by 2
	params->uv_inc = (tweaked_mode->timing.v_total << TV_UV_INC_FIX_SHIFT) 
		* 2 / tv_timing->v_field_total;
	params->h_inc = 
		((int64)tweaked_mode->timing.h_display * 4096 /
		(tv_timing->h_active_len + tv_timing->h_active_delay) << FIX_SHIFT) / tv_timing->scale;
	
	Radeon_CalcTVRestart( params, tweaked_mode, 
		tv_timing->h_total - tv_timing->h_active_len, tv_timing->f_total );
	Radeon_CalcTVFlickerFixer( params );
}


// timing of TV standards;
// the index is of type tv_standard
tv_timing Radeon_std_tv_timing[6] = {
	{42954540, 2730, 200, 28, 200, 110, 2170, 525, 440, 525, 2, 1, 0, 0.88 * FIX_SCALE},	/* ntsc */
	{53203425, 3405, 250, 28, 320, 80, 2627, 625, 498, 625, 2, 3, -6, 0.91 * FIX_SCALE},	/* pal */
	{42907338, 2727, 200, 28, 200, 110, 2170, 525, 440, 525, 2, 1, 0, 0.91 * FIX_SCALE},	/* palm */
	{42984675, 2751, 202, 28, 202, 110, 2190, 625, 510, 625, 2, 3, 0, 0.91 * FIX_SCALE},	/* palnc */
	{53203425, 3405, 250, 28, 320, 80, 2627, 625, 498, 625, 2, 3, 0, 0.91 * FIX_SCALE},	/* scart pal ??? */
	{53203425, 3405, 250, 28, 320, 80, 2627, 525, 440, 525, 2, 1, 0, 0.91 * FIX_SCALE},	/* pal 60 */
};


// compose TV register content
// as TV-Out uses a CRTC, it reprograms a PLL to create an unscaled image;
// as a result, you must not call Radeon_CalcPLLRegisters() afterwards
// TBD: what's special in terms of PLL in TV-Out mode?
void Radeon_CalcTVRegisters( accelerator_info *ai, display_mode *mode, tv_timing *timing, 
	tv_params *params, port_regs *values, physical_head *head, 
	bool internal_encoder, tv_standard tv_format )
{
	// some register's content isn't created from scratch but
	// only modified, so we need the original content first
	Radeon_ReadTVRegisters( ai, values, internal_encoder );
	
	values->tv_ftotal = timing->f_total;
	
	values->tv_vscaler_cntl1 = 
		(values->tv_vscaler_cntl1 & 0xe3ff0000) |
		params->uv_inc;
		
	if( internal_encoder ) {
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

	values->tv_vscaler_cntl2 =
		(values->tv_vscaler_cntl2 & 0x00ffffff) | 
		RADEON_TV_VSCALER_CNTL2_DITHER_MODE |
		RADEON_TV_VSCALER_CNTL2_Y_OUTPUT_DITHER_EN |
		RADEON_TV_VSCALER_CNTL2_UV_OUTPUT_DITHER_EN |
		RADEON_TV_VSCALER_CNTL2_UV_TO_BUF_DITHER_EN |
		(params->uv_accum_init << RADEON_TV_VSCALER_CNTL2_UV_ACCUM_INIT_SHIFT);

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
		RADEON_TV_PLL_CNTL_TV_SLIP_EN |
		(params->tv_dividers.post << RADEON_TV_PLL_CNTL_TV_P_SHIFT) |
		RADEON_TV_PLL_CNTL_TV_DTO_EN;
	values->tv_crt_pll_cntl = 
		(params->crt_dividers.ref & RADEON_TV_CRT_PLL_CNTL_M0_LO_MASK) |
		((params->crt_dividers.feedback & RADEON_TV_CRT_PLL_CNTL_N0_LO_MASK) 
			<< RADEON_TV_CRT_PLL_CNTL_N0_LO_SHIFT) |
		((params->crt_dividers.ref >> RADEON_TV_CRT_PLL_CNTL_M0_LO_BITS) 
			<< RADEON_TV_CRT_PLL_CNTL_M0_HI_SHIFT) |
		((params->crt_dividers.feedback >> RADEON_TV_CRT_PLL_CNTL_N0_LO_BITS) 
			<< RADEON_TV_CRT_PLL_CNTL_N0_HI_SHIFT) |
		(params->crt_dividers.extra_post == 2 ? RADEON_TV_CRT_PLL_CNTL_CLKBY2 : 0);

	values->tv_clock_sel_cntl = 
		(values->tv_clock_sel_cntl & ~0x3d) |
		0x33 |
		((params->crt_dividers.post_code - 1) << 2);
    
    values->tv_clkout_cntl = 0x09;
    if( !internal_encoder )
    	values->tv_clkout_cntl |= 1 << 5;

	values->tv_htotal = mode->timing.h_total - 1;
	values->tv_hsize = mode->timing.h_display;
	values->tv_hdisp = mode->timing.h_display - 1;
	values->tv_hstart =
		internal_encoder ? 
		mode->timing.h_display - params->mode888 - 12 :
		mode->timing.h_display - params->mode888 + 12;
		
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
	
	values->feedback_div = params->crt_dividers.feedback;
	values->post_div = params->crt_dividers.post;
	values->ppll_ref_div = params->crt_dividers.ref;
	values->ppll_div_3 = 
		params->crt_dividers.feedback |
		(params->crt_dividers.post_code << 16);
	values->htotal_cntl = mode->timing.h_total & 7;

	if( internal_encoder ) {
		values->tv_dac_cntl = 
		// TBD: DAC is always set to NTSC mode, though there is a PAL mode!
		values->tv_dac_cntl =
			RADEON_TV_DAC_CNTL_NBLANK |
			RADEON_TV_DAC_CNTL_NHOLD |
			RADEON_TV_DAC_CNTL_STD_NTSC/*RADEON_TV_DAC_CNTL_STD_PAL*/ |
			(8 << RADEON_TV_DAC_CNTL_BGADJ_SHIFT) |
			(6 << RADEON_TV_DAC_CNTL_DACADJ_SHIFT);
	} else {
		values->tv_dac_cntl =
			(values->tv_dac_cntl & ~(RADEON_TV_DAC_CNTL_STD_NTSC | 0x88 | 
				RADEON_TV_DAC_CNTL_BGSLEEP | RADEON_TV_DAC_CNTL_PEDESTAL)) |
			RADEON_TV_DAC_CNTL_DETECT | 
			RADEON_TV_DAC_CNTL_NBLANK |
			RADEON_TV_DAC_CNTL_NHOLD;
	}

	values->tv_modulator_cntl1 = 
		values->tv_modulator_cntl1 & ~(
			RADEON_TV_MODULATOR_CNTL1_ALT_PHASE_EN | 
			RADEON_TV_MODULATOR_CNTL1_SYNC_TIP_LEVEL | 
			RADEON_TV_MODULATOR_CNTL1_SET_UP_LEVEL_MASK | 
			RADEON_TV_MODULATOR_CNTL1_BLANK_LEVEL_MASK);
			
	switch( tv_format ) {
	case ts_ntsc:
		values->tv_dac_cntl |=
		values->tv_modulator_cntl1 |= 
			RADEON_TV_MODULATOR_CNTL1_SYNC_TIP_LEVEL |
			(0x46 << RADEON_TV_MODULATOR_CNTL1_SET_UP_LEVEL_SHIFT) |
			(0x3b << RADEON_TV_MODULATOR_CNTL1_BLANK_LEVEL_SHIFT);
		values->tv_modulator_cntl2 = 
			(-111 & TV_MODULATOR_CNTL2_U_BURST_LEVEL_MASK) |
			((0 & TV_MODULATOR_CNTL2_V_BURST_LEVEL_MASK) << TV_MODULATOR_CNTL2_V_BURST_LEVEL_SHIFT);
		break;
		
	case ts_pal:
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
    }

	values->tv_data_delay_a = 0x0b0c0a06;
	values->tv_data_delay_b = 0x070a0a0c;
	
	values->tv_frame_lock_cntl = internal_encoder ? 0 : 0xf;

	if( internal_encoder ) {
		values->tv_pll_cntl1 = 
			(4 << RADEON_TV_PLL_CNTL1_TVPCP_SHIFT) |
			(4 << RADEON_TV_PLL_CNTL1_TVPVG_SHIFT) |
			(2 << RADEON_TV_PLL_CNTL1_TVPDC_SHIFT) |
			RADEON_TV_PLL_CNTL1_TVCLK_SRC_SEL_TVPLLCLK |
			RADEON_TV_PLL_CNTL1_TVPLL_TEST;
			
		values->tv_rgb_cntl = 
			((head->is_crtc2 ? 2 : 0) << RADEON_TV_RGB_CNTL_RGB_SRC_SEL_SHIFT) |
			RADEON_TV_RGB_CNTL_RGB_DITHER_EN |
			(0xb << RADEON_TV_RGB_CNTL_UVRAM_READ_MARGIN_SHIFT) |
			(7 << RADEON_TV_RGB_CNTL_FIFORAM_FIFOMACRO_READ_MARGIN_SHIFT);
			
		values->tv_pre_dac_mux_cntl = 
			RADEON_TV_PRE_DAC_MUX_CNTL_Y_RED_EN |
			RADEON_TV_PRE_DAC_MUX_CNTL_C_GRN_EN |
			RADEON_TV_PRE_DAC_MUX_CNTL_CMP_BLU_EN |
			RADEON_TV_PRE_DAC_MUX_CNTL_DAC_DITHER_EN |
			(0x2c << RADEON_TV_PRE_DAC_MUX_CNTL_FORCE_DAC_DATA_SHIFT);
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
			RADEON_TV_PRE_DAC_MUX_CNTL_DAC_DITHER_EN |
			(0xaf << RADEON_TV_PRE_DAC_MUX_CNTL_FORCE_DAC_DATA_SHIFT);
	}
	
	values->tv_pll_fine_cntl = 0;
	
	// TBD: this is certainly broken 
	// (they do an ((orig & 0xe0) & 0x600) which is constant zero)
	values->tv_master_cntl = 0;

	if( tv_format == ts_ntsc )
		values->tv_master_cntl |= RADEON_TV_MASTER_CNTL_RESTART_PHASE_FIX;
	else
		values->tv_master_cntl &= ~RADEON_TV_MASTER_CNTL_RESTART_PHASE_FIX;
		
	// this is missing in the sample code
	values->tv_master_cntl |= RADEON_TV_MASTER_CNTL_TV_ON;
	
	SHOW_FLOW( 2, "tv_master_cntl=%x", values->tv_master_cntl );

	values->tv_uv_adr = 0xc8;
}


// mapping of offset in port_regs to register address 
typedef struct register_mapping {
	uint16		address;			// register address
	uint16		offset;				// offset in port_regs
} register_mapping;


// internal TV-encoder:

// registers to write before programming PLL
static const register_mapping intern_reg_mapping_before_pll[] = {
	{ RADEON_TV_MASTER_CNTL,		offsetof( port_regs, tv_master_cntl ) },
	{ RADEON_TV_HRESTART,			offsetof( port_regs, tv_hrestart ) },
	{ RADEON_TV_VRESTART,			offsetof( port_regs, tv_vrestart ) },
	{ RADEON_TV_FRESTART,			offsetof( port_regs, tv_frestart ) },
	{ RADEON_TV_FTOTAL,				offsetof( port_regs, tv_ftotal ) },
	{ 0, 0 }
};

// PLL registers to program
static const register_mapping intern_reg_mapping_pll[] = {
	{ RADEON_TV_PLL_CNTL,			offsetof( port_regs, tv_tv_pll_cntl ) },
	{ RADEON_TV_PLL_CNTL1,			offsetof( port_regs, tv_pll_cntl1 ) },
	{ RADEON_TV_PLL_FINE_CNTL,		offsetof( port_regs, tv_pll_fine_cntl ) },
	{ 0, 0 }
};

// registers to write after programming of PLL
static const register_mapping intern_reg_mapping_after_pll[] = {
	{ RADEON_TV_HTOTAL,				offsetof( port_regs, tv_htotal ) },
	{ RADEON_TV_HDISP,				offsetof( port_regs, tv_hdisp ) },
	{ RADEON_TV_HSTART,				offsetof( port_regs, tv_hstart ) },
	{ RADEON_TV_VTOTAL,				offsetof( port_regs, tv_vtotal ) },
	{ RADEON_TV_VDISP,				offsetof( port_regs, tv_vdisp ) },
	{ RADEON_TV_TIMING_CNTL,		offsetof( port_regs, tv_timing_cntl ) },
	{ RADEON_TV_VSCALER_CNTL1,		offsetof( port_regs, tv_vscaler_cntl1 ) },
	{ RADEON_TV_VSCALER_CNTL2,		offsetof( port_regs, tv_vscaler_cntl2 ) },
	{ RADEON_TV_Y_SAW_TOOTH_CNTL,	offsetof( port_regs, tv_y_saw_tooth_cntl ) },
	{ RADEON_TV_Y_RISE_CNTL,		offsetof( port_regs, tv_y_rise_cntl ) },
	{ RADEON_TV_Y_FALL_CNTL,		offsetof( port_regs, tv_y_fall_cntl ) },
	{ RADEON_TV_MODULATOR_CNTL1,	offsetof( port_regs, tv_modulator_cntl1 ) },
	{ RADEON_TV_MODULATOR_CNTL2,	offsetof( port_regs, tv_modulator_cntl2 ) },
	{ RADEON_TV_RGB_CNTL,			offsetof( port_regs, tv_rgb_cntl ) },
	{ RADEON_TV_UV_ADR,				offsetof( port_regs, tv_uv_adr ) },
	{ RADEON_TV_PRE_DAC_MUX_CNTL, 	offsetof( port_regs, tv_pre_dac_mux_cntl ) },
	{ 0, 0 }
};

// registers to write when things settled down
static const register_mapping intern_reg_mapping_finish[] = {
	{ RADEON_TV_DAC_CNTL,			offsetof( port_regs, tv_dac_cntl ) },
	{ RADEON_TV_MASTER_CNTL,		offsetof( port_regs, tv_master_cntl ) },
	{ 0, 0 }
};


// Rage Theatre TV-Out:

// registers to write at first
static const register_mapping theatre_reg_mapping_start[] = {
	{ THEATRE_VIP_MASTER_CNTL,		offsetof( port_regs, tv_master_cntl ) },
	{ THEATRE_VIP_TVO_DATA_DELAY_A,	offsetof( port_regs, tv_data_delay_a ) },
	{ THEATRE_VIP_TVO_DATA_DELAY_B,	offsetof( port_regs, tv_data_delay_b ) },
	
	{ THEATRE_VIP_CLKOUT_CNTL,		offsetof( port_regs, tv_clkout_cntl ) },
	{ THEATRE_VIP_PLL_CNTL0,		offsetof( port_regs, tv_pll_cntl1 ) },
	
	{ THEATRE_VIP_HRESTART,			offsetof( port_regs, tv_hrestart ) },
	{ THEATRE_VIP_VRESTART,			offsetof( port_regs, tv_vrestart ) },
	{ THEATRE_VIP_FRESTART,			offsetof( port_regs, tv_frestart ) },
	{ THEATRE_VIP_FTOTAL,			offsetof( port_regs, tv_ftotal ) },
	
	{ THEATRE_VIP_CLOCK_SEL_CNTL, 	offsetof( port_regs, tv_clock_sel_cntl ) },
	{ THEATRE_VIP_TV_PLL_CNTL,		offsetof( port_regs, tv_tv_pll_cntl ) },
	{ THEATRE_VIP_CRT_PLL_CNTL,		offsetof( port_regs, tv_crt_pll_cntl ) },

	{ THEATRE_VIP_HTOTAL,			offsetof( port_regs, tv_htotal ) },
	{ THEATRE_VIP_HSIZE,			offsetof( port_regs, tv_hsize ) },
	{ THEATRE_VIP_HDISP,			offsetof( port_regs, tv_hdisp ) },
	{ THEATRE_VIP_HSTART,			offsetof( port_regs, tv_hstart ) },
	{ THEATRE_VIP_VTOTAL,			offsetof( port_regs, tv_vtotal ) },
	{ THEATRE_VIP_VDISP,			offsetof( port_regs, tv_vdisp ) },

	{ THEATRE_VIP_TIMING_CNTL,		offsetof( port_regs, tv_timing_cntl ) },

	{ THEATRE_VIP_VSCALER_CNTL,		offsetof( port_regs, tv_vscaler_cntl1 ) },
	{ THEATRE_VIP_VSCALER_CNTL2,	offsetof( port_regs, tv_vscaler_cntl2 ) },
	{ THEATRE_VIP_SYNC_SIZE,		offsetof( port_regs, tv_sync_size ) },
	{ THEATRE_VIP_Y_SAW_TOOTH_CNTL, offsetof( port_regs, tv_y_saw_tooth_cntl ) },
	{ THEATRE_VIP_Y_RISE_CNTL,		offsetof( port_regs, tv_y_rise_cntl ) },
	{ THEATRE_VIP_Y_FALL_CNTL,		offsetof( port_regs, tv_y_fall_cntl ) },
	
	{ THEATRE_VIP_MODULATOR_CNTL1,	offsetof( port_regs, tv_modulator_cntl1 ) },
	{ THEATRE_VIP_MODULATOR_CNTL2,	offsetof( port_regs, tv_modulator_cntl2 ) },
	
	{ THEATRE_VIP_RGB_CNTL,			offsetof( port_regs, tv_rgb_cntl ) },
	
	{ THEATRE_VIP_UV_ADR,			offsetof( port_regs, tv_uv_adr ) },
	
	{ THEATRE_VIP_PRE_DAC_MUX_CNTL, offsetof( port_regs, tv_pre_dac_mux_cntl ) },
	{ THEATRE_VIP_FRAME_LOCK_CNTL,	offsetof( port_regs, tv_frame_lock_cntl ) },
	{ 0, 0 }
};

// registers to write when things settled down
static const register_mapping theatre_reg_mapping_finish[] = {
	{ THEATRE_VIP_TV_DAC_CNTL,		offsetof( port_regs, tv_dac_cntl ) },
	{ THEATRE_VIP_MASTER_CNTL,		offsetof( port_regs, tv_master_cntl ) },
	{ 0, 0 }
};


// write list of MM I/O registers
static void writeMMIORegList( accelerator_info *ai, port_regs *values, const register_mapping *mapping )
{	
	vuint8 *regs = ai->regs;
	
	for( ; mapping->address != 0 && mapping->offset != 0; ++mapping ) {
		SHOW_FLOW( 2, "%x=%x", mapping->address, 
			*(uint32 *)((char *)(values) + mapping->offset) );

		OUTREG( regs, mapping->address, *(uint32 *)((char *)(values) + mapping->offset) );
	}
}


// write list of PLL registers
static void writePLLRegList( accelerator_info *ai, port_regs *values, const register_mapping *mapping )
{
	for( ; mapping->address != 0 && mapping->offset != 0; ++mapping ) {
		SHOW_FLOW( 2, "%x=%x", mapping->address, 
			*(uint32 *)((char *)(values) + mapping->offset) );

		Radeon_OUTPLL( ai->regs, ai->si->asic,
			mapping->address, *(uint32 *)((char *)(values) + mapping->offset) );
	}
}


// write list of Rage Theatre registers
static void writeTheatreRegList( accelerator_info *ai, port_regs *values, const register_mapping *mapping )
{
	for( ; mapping->address != 0 && mapping->offset != 0; ++mapping ) {
		Radeon_VIPWrite( ai, ai->si->theatre_channel, mapping->address,
			*(uint32 *)((char *)(values) + mapping->offset) );
	}
}


// program TV-Out registers
void Radeon_ProgramTVRegisters( accelerator_info *ai, port_regs *values, bool internal_encoder )
{	
	uint32 orig_tv_master_cntl = values->tv_master_cntl;
	
	// disable TV-out when registers are setup
	// it gets enabled again when things have settled down		
	values->tv_master_cntl |=
		RADEON_TV_MASTER_CNTL_TV_ASYNC_RST |
		RADEON_TV_MASTER_CNTL_CRT_ASYNC_RST |
		(uint32)0xf0;
		
	if( internal_encoder ) {
		writeMMIORegList( ai, values, intern_reg_mapping_before_pll );
		writePLLRegList( ai, values, intern_reg_mapping_pll );
		writeMMIORegList( ai, values, intern_reg_mapping_after_pll );
	
		snooze( 50000 );
	
		values->tv_master_cntl = orig_tv_master_cntl;
		writeMMIORegList( ai, values, intern_reg_mapping_finish );
	} else {	
		writeTheatreRegList( ai, values, theatre_reg_mapping_start );
		
		snooze( 50000 );
	
		values->tv_master_cntl = orig_tv_master_cntl;
		writeTheatreRegList( ai, values, intern_reg_mapping_finish );
	}
}


// read list of MM I/O registers
static void readMMIORegList( accelerator_info *ai, port_regs *values, const register_mapping *mapping )
{
	vuint8 *regs = ai->regs;
	
	for( ; mapping->address != 0 && mapping->offset != 0; ++mapping ) {
		*(uint32 *)((char *)(values) + mapping->offset) = 
			INREG( regs, mapping->address );
			
/*		SHOW_FLOW( 2, "%x=%x", mapping->address, 
			*(uint32 *)((char *)(values) + mapping->offset) );*/
	}
}


// read list of PLL registers
static void readPLLRegList( accelerator_info *ai, port_regs *values, const register_mapping *mapping )
{
	for( ; mapping->address != 0 && mapping->offset != 0; ++mapping ) {
		*(uint32 *)((char *)(values) + mapping->offset) = 
			Radeon_INPLL( ai->regs, ai->si->asic, mapping->address );
			
/*		SHOW_FLOW( 2, "%x=%x", mapping->address, 
			*(uint32 *)((char *)(values) + mapping->offset) );*/
	}
}


// read list of Rage Theatre registers
static void readTheatreRegList( accelerator_info *ai, port_regs *values, const register_mapping *mapping )
{
	for( ; mapping->address != 0 && mapping->offset != 0; ++mapping ) {
		Radeon_VIPRead( ai, ai->si->theatre_channel, mapping->address,
			(uint32 *)((char *)(values) + mapping->offset) );
	}
}


// read TV-Out registers
void Radeon_ReadTVRegisters( accelerator_info *ai, port_regs *values, bool internal_encoder )
{
	if( internal_encoder ) {
		readMMIORegList( ai, values, intern_reg_mapping_before_pll );
		readPLLRegList( ai, values, intern_reg_mapping_pll );
		readMMIORegList( ai, values, intern_reg_mapping_after_pll );
		readMMIORegList( ai, values, intern_reg_mapping_finish );
	} else {	
		readTheatreRegList( ai, values, theatre_reg_mapping_start );
		readTheatreRegList( ai, values, intern_reg_mapping_finish );
	}
}


// detect TV-Out encoder
void Radeon_DetectTVOut( accelerator_info *ai )
{
	shared_info *si = ai->si;
	
	SHOW_FLOW0( 0, "" );
	
	switch( si->tv_chip ) {
	case tc_external_rt1: {
		// for external encoder, we need the VIP channel
		int channel = Radeon_FindVIPDevice( ai, THEATRE_ID );
		
		if( channel < 0 ) {
			SHOW_ERROR0( 2, "This card needs a Rage Theatre for TV-Out, but there is none." );
			si->tv_chip = tc_none;
		} else {
			SHOW_INFO( 2, "Rage Theatre found on VIP channel %d", channel );
			si->theatre_channel = channel;
		}
		break; }
	default:
		// for internal encoder, we don't have to look farther - it must be there
	}
}
