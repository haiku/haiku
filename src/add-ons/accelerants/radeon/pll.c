/*
	Copyright (c) 2002-2004, Thomas Kurschel


	Part of Radeon accelerant

	Takes care of PLL
*/


#include "radeon_accelerant.h"

#include "pll_regs.h"
#include "pll_access.h"
#include "utils.h"
#include <stdlib.h>
#include "set_mode.h"


static void Radeon_PLLWaitForReadUpdateComplete(
	accelerator_info *ai, int crtc_idx )
{
	int i;

	// we should wait forever, but
	// 1. this is unsafe
	// 2. some r300 loop forever (reported by XFree86)
	for( i = 0; i < 10000; ++i ) {
		if( (Radeon_INPLL( ai->regs, ai->si->asic, crtc_idx == 0 ? RADEON_PPLL_REF_DIV : RADEON_P2PLL_REF_DIV )
			& RADEON_PPLL_ATOMIC_UPDATE_R) == 0 )
			return;
	}
}

static void Radeon_PLLWriteUpdate(
	accelerator_info *ai, int crtc_idx )
{
	Radeon_PLLWaitForReadUpdateComplete( ai, crtc_idx );

    Radeon_OUTPLLP( ai->regs, ai->si->asic,
    	crtc_idx == 0 ? RADEON_PPLL_REF_DIV : RADEON_P2PLL_REF_DIV,
    	RADEON_PPLL_ATOMIC_UPDATE_W,
    	~RADEON_PPLL_ATOMIC_UPDATE_W );
}

// calculate PLL dividers
// pll - info about PLL
// freq - whished frequency in Hz
// fixed_post_div - if != 0, fixed divider to be used
// dividers - filled with proper dividers
void Radeon_CalcPLLDividers(
	const pll_info *pll, uint32 freq, uint fixed_post_div, pll_dividers *dividers )
{
	// the PLL gets the reference
	//		pll_in = ref_freq / ref_div
	// this must be within pll_in_min..pll_in_max
	// the VCO of the PLL has the frequency
	//		vco = pll_in * feedback_div * extra_feedback_div
	//		    = ref_freq / ref_div * feedback_div * extra_feedback_div
	// where pre_feedback_div is hard-wired
	// this must be within vco_min..vco_max
	// the pixel clock is calculated as
	//		pll_out = vco / post_div / extra_post_div
	//		        = ref_freq * feedback_div * extra_feedback_div / (ref_div * post_div * extra_post_div)
	// where extra_post_div _may_ be choosable between 1 and 2

	// synonyms are:
	//		ref_div = M
	//		feedback_div = N
	//		post_div = P

	int
		min_post_div_idx, max_post_div_idx,
		post_div_idx, extra_post_div_idx,
		best_post_div_idx, best_extra_post_div_idx;

	uint32
		best_ref_div, best_feedback_div, best_freq;
	int32
		best_error, best_vco_dev;

	best_error = 999999999;

	// make compiler happy
	best_post_div_idx = 0;
	best_extra_post_div_idx = 0;
	best_ref_div = 1;
	best_feedback_div = 1;
	best_freq = 1;
	best_vco_dev = 1;

	if( fixed_post_div == 0 ) {
		min_post_div_idx = 0;
		for(
			max_post_div_idx = 0;
			pll->post_divs[max_post_div_idx].divider != 0;
			++max_post_div_idx )
			;
		--max_post_div_idx;
	} else {
		for(
			min_post_div_idx = 0;
			pll->post_divs[min_post_div_idx].divider != fixed_post_div;
			++min_post_div_idx )
			;

		max_post_div_idx = min_post_div_idx;

		//SHOW_FLOW( 2, "idx of fixed post divider: %d", min_post_div_idx );
	}

	// post dividers are quite restrictive, so they provide little search space only
	for( extra_post_div_idx = 0; pll->extra_post_divs[extra_post_div_idx].divider != 0; ++extra_post_div_idx ) {
		for( post_div_idx = min_post_div_idx; post_div_idx <= max_post_div_idx; ++post_div_idx ) {
			uint32 ref_div;
			uint32 post_div =
				pll->post_divs[post_div_idx].divider
				* pll->extra_post_divs[extra_post_div_idx].divider;

			// post devider determines VCO frequency, so determine and verify it;
			// freq is in Hz, everything else is in 10 kHz units
			// we use 10 kHz units as long as possible to avoid uint32 overflows
			uint32 vco = (freq / 10000) * post_div;

			//SHOW_FLOW( 2, "post_div=%d, vco=%d", post_div, vco );

			if( vco < pll->vco_min || vco > pll->vco_max )
				continue;

			//SHOW_FLOW0( 2, "jau" );

			// we can either iterate through feedback or reference dividers;
			// usually, there are fewer possible reference dividers, so I picked them
			for( ref_div = pll->min_ref_div; ref_div <= pll->max_ref_div; ++ref_div ) {
				uint32 feedback_div, cur_freq;
				int32 error, vco_dev;

				// this implies the frequency of the lock unit
				uint32 pll_in = pll->ref_freq / ref_div;

				if( pll_in < pll->pll_in_min || pll_in > pll->pll_in_max )
					continue;

				// well, only one variable is left
				// timing is almost certainly valid, time to use Hz units
				feedback_div = RoundDiv64(
					(int64)freq * ref_div * post_div,
					pll->ref_freq * 10000 * pll->extra_feedback_div);

				if( feedback_div < pll->min_feedback_div ||
					feedback_div > pll->max_feedback_div )
					continue;

				// let's see what we've got
				cur_freq = RoundDiv64(
					(int64)pll->ref_freq * 10000 * feedback_div * pll->extra_feedback_div,
					ref_div * post_div );

				// absolute error in terms of output clock
				error = abs( (int32)cur_freq - (int32)freq );
				// deviation from perfect VCO clock
				vco_dev = abs( (int32)vco - (int32)(pll->best_vco) );

				// if there is no optimal VCO frequency, choose setting with less error;
				// if there is an optimal VCO frequency, choose new settings if
				// - error is reduced significantly (100 Hz or more), or
				// - output frequency is almost the same (less then 100 Hz difference) but
				//	 VCO frequency is closer to best frequency
				if( (pll->best_vco == 0 && error < best_error) ||
					(pll->best_vco != 0 &&
					 (error < best_error - 100 ||
					 (abs( error - best_error ) < 100 && vco_dev < best_vco_dev ))))
				{
					//SHOW_FLOW( 2, "got freq=%d, best_freq=%d", freq, cur_freq );
					best_post_div_idx = post_div_idx;
					best_extra_post_div_idx = extra_post_div_idx;
					best_ref_div = ref_div;
					best_feedback_div = feedback_div;
					best_freq = cur_freq;
					best_error = error;
					best_vco_dev = vco_dev;
				}
			}
		}
	}

	dividers->post_code = pll->post_divs[best_post_div_idx].code;
	dividers->post = pll->post_divs[best_post_div_idx].divider;
	dividers->extra_post_code = pll->post_divs[best_extra_post_div_idx].code;
	dividers->extra_post = pll->post_divs[best_extra_post_div_idx].divider;
	dividers->ref = best_ref_div;
	dividers->feedback = best_feedback_div;
	dividers->freq = best_freq;

	/*SHOW_FLOW( 2, "post_code=%d, post=%d, extra_post_code=%d, extra_post=%d, ref=%d, feedback=%d, freq=%d",
		dividers->post_code, dividers->post, dividers->extra_post_code,
		dividers->extra_post, dividers->ref, dividers->feedback, dividers->freq );*/
}


// with a TV timing given, find a corresponding CRT timing.
// both timing must meet at the end of a frame, but as the PLL has a
// limited frequency granularity, you don't really get a CRT timing
// with precisely the same frame rate; the solution is to tweak the CRT
// image a bit by making it wider/taller/smaller until the frame rate
// drift is under a given threshold;
// we follow two aims:
// 	- primary, keep frame rate in sync
//  - secondary, only tweak as much as unavoidable
void Radeon_MatchCRTPLL(
	const pll_info *pll,
	uint32 tv_v_total, uint32 tv_h_total, uint32 tv_frame_size_adjust, uint32 freq,
	const display_mode *mode, uint32 max_v_tweak, uint32 max_h_tweak,
	uint32 max_frame_rate_drift, uint32 fixed_post_div,
	pll_dividers *dividers,
	display_mode *tweaked_mode )
{
	uint32 v_tweak;
	int32 v_tweak_dir;
	uint32 pix_per_tv_frame;

	SHOW_FLOW( 2, "fixed post divider: %d", fixed_post_div );

	// number of TV pixels per frame
	pix_per_tv_frame = tv_v_total * tv_h_total + tv_frame_size_adjust;

	// starting with original data we tweak total horizontal and vertical size
	// more and more until we find a proper CRT clock frequency
	for( v_tweak = 0; v_tweak <= max_v_tweak; ++v_tweak ) {
		for( v_tweak_dir = -1; v_tweak_dir <= 1; v_tweak_dir += 2 ) {
			uint32 h_tweak;
			int32 h_tweak_dir;

			uint32 v_total = mode->timing.v_total + v_tweak * v_tweak_dir;

			for( h_tweak = 0; h_tweak <= max_h_tweak; ++h_tweak ) {
				for( h_tweak_dir = -1; h_tweak_dir <= 1; h_tweak_dir += 2 ) {
					uint32 pix_per_crt_frame, frame_rate_drift;
					uint32 crt_freq;
					uint32 abs_crt_error;

					uint32 h_total = mode->timing.h_total + h_tweak * h_tweak_dir;

					// number of CRT pixels per frame
					pix_per_crt_frame = v_total * h_total;

					// frame rate must be:
					//	frame_rate = freq / pix_per_tv_half_frame
					// because of interlace, we must use half frames
					//	pix_per_tv_half_frame = pix_per_tv_frame / 2
					// to get a CRT image with the same frame rate, we get
					//	crt_freq = frame_rate * pix_per_crt_frame
					//	         = freq / (pix_per_tv_frame / 2) * pix_per_crt_frame
					// formula is reordered as usual to improve accuracy
					crt_freq = (uint64)freq * pix_per_crt_frame * 2 / pix_per_tv_frame;

					Radeon_CalcPLLDividers( pll, crt_freq, fixed_post_div, dividers );

					// get absolute CRT clock error per second
					abs_crt_error = abs( (int32)(dividers->freq) - (int32)crt_freq );

					//SHOW_INFO( 2, "whished=%d, is=%d", crt_freq, dividers->freq );

					// convert it to relative CRT clock error:
					//	rel_error = abs_crt_error / crt_freq
					// now to absolute TV clock error per second:
					//	abs_tv_error = rel_error * tv_freq
					// and finally to TV clock error per frame:
					//	frame_rate_drift = abs_tv_error / frame_rate
					//	                 = abs_crt_error / crt_freq * tv_freq / frame_rate
					// this can be simplified by using:
					//	tv_freq = pix_per_tv_frame * frame_rate
					// so we get:
					//	frame_rate_drift = abs_crt_error / crt_freq * pix_per_tv_frame * frame_rate / frame_rate
					//	                 = abs_crt_error / crt_freq * pix_per_tv_frame
					frame_rate_drift = (uint64)abs_crt_error * pix_per_tv_frame / freq;

					// if drift is within threshold, we take this setting and stop
					// searching (later iteration will increasingly tweak screen size,
					// and we don't really want that)
					if( frame_rate_drift <= max_frame_rate_drift ) {
						SHOW_INFO( 2, "frame_rate_drift=%d, crt_freq=%d, v_total=%d, h_total=%d",
							frame_rate_drift, crt_freq, v_total, h_total );

						tweaked_mode->timing.pixel_clock = crt_freq;
						tweaked_mode->timing.v_total = v_total;
						tweaked_mode->timing.h_total = h_total;
						return;
					}
				}
			}
		}
    }
}


// table to map divider to register value
static pll_divider_map post_divs[] = {
	{  1, 0 },
	{  2, 1 },
	{  4, 2 },
	{  8, 3 },
	{  3, 4 },
//	{ 16, 5 },	// at least for pll2 of M6, this value is reserved
	{  6, 6 },
	{ 12, 7 },
	{  0, 0 }
};


// normal PLLs have no extra post divider
static pll_divider_map extra_post_divs[] = {
	{ 1, 1 },
	{ 0, 0 }
};


// extra post-divider provided by Rage Theatre
static pll_divider_map external_extra_post_divs[] = {
	{ 1, 0 },
	{ 2, 1 },
	{ 0, 0 }
};


// post-dividers of Rage Theatre
static pll_divider_map tv_post_divs[] = {
	{  1, 1 },
	{  2, 2 },
	{  3, 3 },
	{  4, 4 },
	{  5, 5 },
	{  6, 6 },
	{  7, 7 },
	{  8, 8 },
	{  9, 9 },
	{ 10, 10 },
	{ 11, 11 },
	{ 12, 12 },
	{ 13, 13 },
	{ 14, 14 },
	{ 15, 15 },
	{  0, 0 }
};


// get PLL parameters of TV PLL
void Radeon_GetTVPLLConfiguration( const general_pll_info *general_pll, pll_info *pll,
	bool internal_encoder )
{
	pll->post_divs = tv_post_divs;
	pll->extra_post_divs = internal_encoder ? extra_post_divs : external_extra_post_divs;
	pll->ref_freq = general_pll->ref_freq;
	pll->vco_min = 10000;
	pll->vco_max = 25000;
	// I'm not sure about the upper limit
	pll->min_ref_div = 4;
	pll->max_ref_div = 0x3ff;
	// in the original code, they set it to 330kHz if PAL is requested and
	// quartz is 27 MHz, but I don't see how these circumstances can effect the
	// mimimal PLL input frequency
	pll->pll_in_min = 20;//40;
	// in the original code, they don't define an upper limit
	pll->pll_in_max = 100;
	pll->extra_feedback_div = 1;
	pll->min_feedback_div = 4;
	pll->max_feedback_div = 0x7ff;
	pll->best_vco = 21000;
}


// get PLL parameters of CRT PLL used in conjunction with TV-out
void Radeon_GetTVCRTPLLConfiguration( const general_pll_info *general_pll, pll_info *pll,
	bool internal_tv_encoder )
{
	pll->post_divs = post_divs;
	pll->extra_post_divs = extra_post_divs;
	pll->ref_freq = general_pll->ref_freq;

	// in sample code, these limits are set in a strange way;
	// as a first shot, I use the BIOS provided limits
	/*pll->vco_min = general_pll->min_pll_freq;
	pll->vco_max = general_pll->max_pll_freq;*/

	// in sample code, they use a variable post divider during calculation, but
	// use a fixed post divider for programming - the variable post divider is
	// multiplied to the feedback divider;
	// because of the fixed post divider (3), the VCO always runs far out of
	// its stable frequency range, so we have hack the limits
	pll->vco_min = 4000;
	pll->vco_max = general_pll->max_pll_freq;

	// in sample code, lower limit is 4, but in register spec they say everything but 0/1
	pll->min_ref_div = 2;
	pll->max_ref_div = 0x3ff;
	pll->pll_in_min = 20;
	pll->pll_in_max = 100;
	pll->extra_feedback_div = 1;
	pll->min_feedback_div = 4;
	pll->max_feedback_div = 0x7ff;
	pll->best_vco = internal_tv_encoder ? 17500 : 21000;
}


// calc PLL dividers for CRT
// mode->timing.pixel_clock must be in Hz because required accuracy in TV-Out mode
void Radeon_CalcCRTPLLDividers(
	const general_pll_info *general_pll, const display_mode *mode, pll_dividers *dividers )
{
	pll_info pll;

	pll.post_divs = post_divs;
	pll.extra_post_divs = extra_post_divs;
	pll.ref_freq = general_pll->ref_freq;
	pll.vco_min = general_pll->min_pll_freq;
	pll.vco_max = general_pll->max_pll_freq;
	pll.min_ref_div = 2;
	pll.max_ref_div = 0x3ff;
	pll.pll_in_min = 40;
	pll.pll_in_max = 100;
	pll.extra_feedback_div = 1;
	pll.min_feedback_div = 4;
	pll.max_feedback_div = 0x7ff;
	pll.best_vco = 0;

	SHOW_FLOW( 2, "freq=%ld", mode->timing.pixel_clock );

	Radeon_CalcPLLDividers( &pll, mode->timing.pixel_clock, 0, dividers );
}


// calculate PLL registers
// mode->timing.pixel_clock must be in Hz because required accuracy in TV-Out mode
// (old: freq is in 10kHz)
void Radeon_CalcPLLRegisters(
	const display_mode *mode, const pll_dividers *dividers, pll_regs *values )
{
	values->dot_clock_freq = dividers->freq;
	values->feedback_div   = dividers->feedback;
	values->post_div       = dividers->post;
	values->pll_output_freq = dividers->freq * dividers->post;

	values->ppll_ref_div   = dividers->ref;
	values->ppll_div_3     = (dividers->feedback | (dividers->post_code << 16));
	// this is mad: the PLL controls the horizontal length in sub-byte precision!
	values->htotal_cntl    = mode->timing.h_total & 7;

	SHOW_FLOW( 2, "dot_clock_freq=%ld, pll_output_freq=%ld, ref_div=%d, feedback_div=%d, post_div=%d",
		values->dot_clock_freq, values->pll_output_freq,
		values->ppll_ref_div, values->feedback_div, values->post_div );
}

// write values into PLL registers
void Radeon_ProgramPLL(
	accelerator_info *ai, int crtc_idx, pll_regs *values )
{
	vuint8 *regs = ai->regs;
	radeon_type asic = ai->si->asic;

	SHOW_FLOW0( 2, "" );

	// use some other PLL for pixel clock source to not fiddling with PLL
	// while somebody is using it
    Radeon_OUTPLLP( regs, asic, crtc_idx == 0 ? RADEON_VCLK_ECP_CNTL : RADEON_PIXCLKS_CNTL,
    	RADEON_VCLK_SRC_CPU_CLK, ~RADEON_VCLK_SRC_SEL_MASK );

    Radeon_OUTPLLP( regs, asic,
		crtc_idx == 0 ? RADEON_PPLL_CNTL : RADEON_P2PLL_CNTL,
	    RADEON_PPLL_RESET
	    | RADEON_PPLL_ATOMIC_UPDATE_EN
	    | RADEON_PPLL_VGA_ATOMIC_UPDATE_EN,
	    ~(RADEON_PPLL_RESET
		| RADEON_PPLL_ATOMIC_UPDATE_EN
		| RADEON_PPLL_VGA_ATOMIC_UPDATE_EN) );

	// select divider 3 (well, only required for first PLL)
    OUTREGP( regs, RADEON_CLOCK_CNTL_INDEX,
	    RADEON_PLL_DIV_SEL_DIV3,
	    ~RADEON_PLL_DIV_SEL_MASK );

	RADEONPllErrataAfterIndex(regs, asic);

	if( ai->si->new_pll && crtc_idx == 0 ) {
		// starting with r300, the reference divider of the first PLL was
		// moved to another bit position; at the old location, you only
		// find the "BIOS suggested divider"; no clue why they did that
		Radeon_OUTPLLP( regs, asic,
    		RADEON_PPLL_REF_DIV,
    		values->ppll_ref_div << RADEON_PPLL_REF_DIV_ACC_SHIFT,
    		~RADEON_PPLL_REF_DIV_ACC_MASK );
	} else {
	    Radeon_OUTPLLP( regs, asic,
    		crtc_idx == 0 ? RADEON_PPLL_REF_DIV : RADEON_P2PLL_REF_DIV,
    		values->ppll_ref_div,
    		~RADEON_PPLL_REF_DIV_MASK );
    }

    Radeon_OUTPLLP( regs, asic,
    	crtc_idx == 0 ? RADEON_PPLL_DIV_3 : RADEON_P2PLL_DIV_0,
    	values->ppll_div_3,
    	~RADEON_PPLL_FB3_DIV_MASK );

    Radeon_OUTPLLP( regs, asic,
    	crtc_idx == 0 ? RADEON_PPLL_DIV_3 : RADEON_P2PLL_DIV_0,
    	values->ppll_div_3,
    	~RADEON_PPLL_POST3_DIV_MASK );

    Radeon_PLLWriteUpdate( ai, crtc_idx );
    Radeon_PLLWaitForReadUpdateComplete( ai, crtc_idx );

    Radeon_OUTPLL( regs, asic,
    	crtc_idx == 0 ? RADEON_HTOTAL_CNTL : RADEON_HTOTAL2_CNTL,
    	values->htotal_cntl );

	Radeon_OUTPLLP( regs, asic,
		crtc_idx == 0 ? RADEON_PPLL_CNTL : RADEON_P2PLL_CNTL, 0,
		~(RADEON_PPLL_RESET
		| RADEON_PPLL_SLEEP
		| RADEON_PPLL_ATOMIC_UPDATE_EN
		| RADEON_PPLL_VGA_ATOMIC_UPDATE_EN) );

	// there is no way to check whether PLL has settled, so wait a bit
	snooze( 5000 );

	// use PLL for pixel clock again
    Radeon_OUTPLLP( regs, asic,
    	crtc_idx == 0 ? RADEON_VCLK_ECP_CNTL : RADEON_PIXCLKS_CNTL,
    	RADEON_VCLK_SRC_PPLL_CLK, ~RADEON_VCLK_SRC_SEL_MASK );
}
