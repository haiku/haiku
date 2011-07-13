/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

/* Generate mode timings using the GTF Timing Standard
 *
 * Copyright (c) 2001, Andy Ritger  aritger@nvidia.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * o Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * o Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer
 *   in the documentation and/or other materials provided with the
 *   distribution.
 * o Neither the name of NVIDIA nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *
 * This program is based on the Generalized Timing Formula(GTF TM)
 * Standard Version: 1.0, Revision: 1.0
 *
 * The GTF Document contains the following Copyright information:
 *
 * Copyright (c) 1994, 1995, 1996 - Video Electronics Standards
 * Association. Duplication of this document within VESA member
 * companies for review purposes is permitted. All other rights
 * reserved.
 *
 * While every precaution has been taken in the preparation
 * of this standard, the Video Electronics Standards Association and
 * its contributors assume no responsibility for errors or omissions,
 * and make no warranties, expressed or implied, of functionality
 * of suitability for any purpose. The sample code contained within
 * this standard may be used without restriction.
 *
 *
 *
 * The GTF EXCEL(TM) SPREADSHEET, a sample (and the definitive)
 * implementation of the GTF Timing Standard, is available at:
 *
 * ftp://ftp.vesa.org/pub/GTF/GTF_V1R1.xls
 *
 *
 *
 * This program takes a desired resolution and vertical refresh rate,
 * and computes mode timings according to the GTF Timing Standard.
 * These mode timings can then be formatted as an XFree86 modeline
 * or a mode description for use by fbset(8).
 *
 *
 *
 * NOTES:
 *
 * The GTF allows for computation of "margins" (the visible border
 * surrounding the addressable video); on most non-overscan type
 * systems, the margin period is zero.  I've implemented the margin
 * computations but not enabled it because 1) I don't really have
 * any experience with this, and 2) neither XFree86 modelines nor
 * fbset fb.modes provide an obvious way for margin timings to be
 * included in their mode descriptions (needs more investigation).
 *
 * The GTF provides for computation of interlaced mode timings;
 * I've implemented the computations but not enabled them, yet.
 * I should probably enable and test this at some point.
 *
 *
 *
 * TODO:
 *
 * o Add support for interlaced modes.
 *
 * o Implement the other portions of the GTF: compute mode timings
 *   given either the desired pixel clock or the desired horizontal
 *   frequency.
 *
 * o It would be nice if this were more general purpose to do things
 *   outside the scope of the GTF: like generate double scan mode
 *   timings, for example.
 *
 * o Error checking.
 *
 */


#include <compute_display_timing.h>

#include <math.h>
#include <stdarg.h>


//#define TRACE_COMPUTE
#ifdef TRACE_COMPUTE
#	define TRACE(x, ...)	debug_printf(x, __VA_ARGS__)
#else
#	define TRACE(x, ...)	;
#endif


#define MARGIN_PERCENT		1.8		// % of active vertical image
#define CELL_GRAN			8.0		// assumed character cell granularity
#define MIN_PORCH			1		// minimum front porch
#define V_SYNC_RQD			3		// width of vsync in lines
#define H_SYNC_PERCENT		8.0		// width of hsync as % of total line
#define MIN_VSYNC_PLUS_BP	550.0	// min time of vsync + back porch (microsec)
#define M					600.0	// blanking formula gradient
#define C					40.0	// blanking formula offset
#define K					128.0	// blanking formula scaling factor
#define J					20.0	// blanking formula scaling factor

// C' and M' are part of the Blanking Duty Cycle computation

#define C_PRIME		   (((C - J) * K/256.0) + J)
#define M_PRIME		   (K/256.0 * M)


/*!	As defined by the GTF Timing Standard, compute the Stage 1 Parameters
	using the vertical refresh frequency. In other words: input a desired
	resolution and desired refresh rate, and output the GTF mode timings.
*/
status_t
compute_display_timing(uint32 width, uint32 height, float refresh,
	bool interlaced, display_timing* timing)
{
	if (width < 320 || height < 200 || width > 65536 || height > 65536
			|| refresh < 25 || refresh > 1000)
		return B_BAD_VALUE;

	int margins = 0;

	float h_pixels_rnd;
	float v_lines_rnd;
	float v_field_rate_rqd;
	float top_margin;
	float bottom_margin;
	float interlace;
	float h_period_est;
	float vsync_plus_bp;
	float v_back_porch;
	float total_v_lines;
	float v_field_rate_est;
	float h_period;
	float v_field_rate;
	float v_frame_rate;
	float left_margin;
	float right_margin;
	float total_active_pixels;
	float ideal_duty_cycle;
	float h_blank;
	float total_pixels;
	float pixel_freq;
	float h_freq;

	float h_sync;
	float h_front_porch;
	float v_odd_front_porch_lines;

	// 1. In order to give correct results, the number of horizontal
	// pixels requested is first processed to ensure that it is divisible
	// by the character size, by rounding it to the nearest character
	// cell boundary:
	//	[H PIXELS RND] = ((ROUND([H PIXELS]/[CELL GRAN RND],0))*[CELLGRAN RND])
	h_pixels_rnd = rint((float)width / CELL_GRAN) * CELL_GRAN;
	TRACE("[H PIXELS RND] %g\n", h_pixels_rnd);

	// 2. If interlace is requested, the number of vertical lines assumed
	// by the calculation must be halved, as the computation calculates
	// the number of vertical lines per field. In either case, the
	// number of lines is rounded to the nearest integer.
	//	[V LINES RND] = IF([INT RQD?]="y", ROUND([V LINES]/2,0),
	//		ROUND([V LINES],0))
	v_lines_rnd = interlaced
		? (double)height / 2.0 : (double)height;
	TRACE("[V LINES RND] %g\n", v_lines_rnd);

	// 3. Find the frame rate required:
	//	[V FIELD RATE RQD] = IF([INT RQD?]="y", [I/P FREQ RQD]*2,
	//		[I/P FREQ RQD])
	v_field_rate_rqd = interlaced ? refresh * 2.0 : refresh;
	TRACE("[V FIELD RATE RQD] %g\n", v_field_rate_rqd);

	// 4. Find number of lines in Top margin:
	//	[TOP MARGIN (LINES)] = IF([MARGINS RQD?]="Y",
	//		ROUND(([MARGIN%]/100*[V LINES RND]),0), 0)
	top_margin = margins ? rint(MARGIN_PERCENT / 100.0 * v_lines_rnd) : 0.0;
	TRACE("[TOP MARGIN (LINES)] %g\n", top_margin);

	// 5. Find number of lines in Bottom margin:
	//	[BOT MARGIN (LINES)] = IF([MARGINS RQD?]="Y",
	//		ROUND(([MARGIN%]/100*[V LINES RND]),0), 0)
	bottom_margin = margins ? rint(MARGIN_PERCENT/100.0 * v_lines_rnd) : 0.0;
	TRACE("[BOT MARGIN (LINES)] %g\n", bottom_margin);

	// 6. If interlace is required, then set variable [INTERLACE]=0.5:
	//	[INTERLACE]=(IF([INT RQD?]="y",0.5,0))
	interlace = interlaced ? 0.5 : 0.0;
	TRACE("[INTERLACE] %g\n", interlace);

	// 7. Estimate the Horizontal period
	//	[H PERIOD EST] = ((1/[V FIELD RATE RQD]) - [MIN VSYNC+BP]/1000000)
	//			/ ([V LINES RND] + (2*[TOP MARGIN (LINES)])
	//				+ [MIN PORCH RND]+[INTERLACE]) * 1000000
	h_period_est = (((1.0 / v_field_rate_rqd) - (MIN_VSYNC_PLUS_BP / 1000000.0))
		/ (v_lines_rnd + (2 * top_margin) + MIN_PORCH + interlace) * 1000000.0);
	TRACE("[H PERIOD EST] %g\n", h_period_est);

	// 8. Find the number of lines in V sync + back porch:
	//	[V SYNC+BP] = ROUND(([MIN VSYNC+BP]/[H PERIOD EST]),0)
	vsync_plus_bp = rint(MIN_VSYNC_PLUS_BP/h_period_est);
	TRACE("[V SYNC+BP] %g\n", vsync_plus_bp);

	// 9. Find the number of lines in V back porch alone:
	//	[V BACK PORCH] = [V SYNC+BP] - [V SYNC RND]
	//  XXX is "[V SYNC RND]" a typo? should be [V SYNC RQD]?
	v_back_porch = vsync_plus_bp - V_SYNC_RQD;
	TRACE("[V BACK PORCH] %g\n", v_back_porch);

	// 10. Find the total number of lines in Vertical field period:
	//	[TOTAL V LINES] = [V LINES RND] + [TOP MARGIN (LINES)]
	//		+ [BOT MARGIN (LINES)] + [V SYNC+BP] + [INTERLACE] + [MIN PORCH RND]
	total_v_lines = v_lines_rnd + top_margin + bottom_margin + vsync_plus_bp +
		interlace + MIN_PORCH;
	TRACE("[TOTAL V LINES] %g\n", total_v_lines);

	// 11. Estimate the Vertical field frequency:
	//	[V FIELD RATE EST] = 1 / [H PERIOD EST] / [TOTAL V LINES] * 1000000
	v_field_rate_est = 1.0 / h_period_est / total_v_lines * 1000000.0;
	TRACE("[V FIELD RATE EST] %g\n", v_field_rate_est);

	// 12. Find the actual horizontal period:
	//	[H PERIOD] = [H PERIOD EST] / ([V FIELD RATE RQD] / [V FIELD RATE EST])
	h_period = h_period_est / (v_field_rate_rqd / v_field_rate_est);
	TRACE("[H PERIOD] %g\n", h_period);

	// 13. Find the actual Vertical field frequency:
	//	[V FIELD RATE] = 1 / [H PERIOD] / [TOTAL V LINES] * 1000000
	v_field_rate = 1.0 / h_period / total_v_lines * 1000000.0;
	TRACE("[V FIELD RATE] %g\n", v_field_rate);

	// 14. Find the Vertical frame frequency:
	//	[V FRAME RATE] = (IF([INT RQD?]="y", [V FIELD RATE]/2, [V FIELD RATE]))
	v_frame_rate = interlaced ? v_field_rate / 2.0 : v_field_rate;
	TRACE("[V FRAME RATE] %g\n", v_frame_rate);

	// 15. Find number of pixels in left margin:
	//	[LEFT MARGIN (PIXELS)] = (IF( [MARGINS RQD?]="Y",
	//			(ROUND( ([H PIXELS RND] * [MARGIN%] / 100 /
	//				[CELL GRAN RND]),0)) * [CELL GRAN RND], 0))
	left_margin = margins
		? rint(h_pixels_rnd * MARGIN_PERCENT / 100.0 / CELL_GRAN) * CELL_GRAN
		: 0.0;
	TRACE("[LEFT MARGIN (PIXELS)] %g\n", left_margin);

	// 16. Find number of pixels in right margin:
	//	[RIGHT MARGIN (PIXELS)] = (IF( [MARGINS RQD?]="Y",
	//			(ROUND( ([H PIXELS RND] * [MARGIN%] / 100 /
	//				[CELL GRAN RND]),0)) * [CELL GRAN RND], 0))
	right_margin = margins
		? rint(h_pixels_rnd * MARGIN_PERCENT / 100.0 / CELL_GRAN) * CELL_GRAN
		: 0.0;
	TRACE("[RIGHT MARGIN (PIXELS)] %g\n", right_margin);

	// 17. Find total number of active pixels in image and left and right
	// margins:
	//	[TOTAL ACTIVE PIXELS] = [H PIXELS RND] + [LEFT MARGIN (PIXELS)]
	//		+ [RIGHT MARGIN (PIXELS)]
	total_active_pixels = h_pixels_rnd + left_margin + right_margin;
	TRACE("[TOTAL ACTIVE PIXELS] %g\n", total_active_pixels);

	// 18. Find the ideal blanking duty cycle from the blanking duty cycle
	// equation:
	//	[IDEAL DUTY CYCLE] = [C'] - ([M']*[H PERIOD]/1000)
	ideal_duty_cycle = C_PRIME - (M_PRIME * h_period / 1000.0);
	TRACE("[IDEAL DUTY CYCLE] %g\n", ideal_duty_cycle);

	// 19. Find the number of pixels in the blanking time to the nearest
	// double character cell:
	//	[H BLANK (PIXELS)] = (ROUND(([TOTAL ACTIVE PIXELS]
	//			* [IDEAL DUTY CYCLE] / (100-[IDEAL DUTY CYCLE])
	//			/ (2*[CELL GRAN RND])), 0)) * (2*[CELL GRAN RND])
	h_blank = rint(total_active_pixels * ideal_duty_cycle
		/ (100.0 - ideal_duty_cycle) / (2.0 * CELL_GRAN)) * (2.0 * CELL_GRAN);
	TRACE("[H BLANK (PIXELS)] %g\n", h_blank);

	// 20. Find total number of pixels:
	//	[TOTAL PIXELS] = [TOTAL ACTIVE PIXELS] + [H BLANK (PIXELS)]
	total_pixels = total_active_pixels + h_blank;
	TRACE("[TOTAL PIXELS] %g\n", total_pixels);

	// 21. Find pixel clock frequency:
	//	[PIXEL FREQ] = [TOTAL PIXELS] / [H PERIOD]
	pixel_freq = total_pixels / h_period;
	TRACE("[PIXEL FREQ] %g\n", pixel_freq);

	// 22. Find horizontal frequency:
	//	[H FREQ] = 1000 / [H PERIOD]
	h_freq = 1000.0 / h_period;
	TRACE("[H FREQ] %g\n", h_freq);

	// Stage 1 computations are now complete; I should really pass
	// the results to another function and do the Stage 2
	// computations, but I only need a few more values so I'll just
	// append the computations here for now */

	// 17. Find the number of pixels in the horizontal sync period:
	//	[H SYNC (PIXELS)] =(ROUND(([H SYNC%] / 100 * [TOTAL PIXELS]
	//		/ [CELL GRAN RND]),0))*[CELL GRAN RND]
	h_sync = rint(H_SYNC_PERCENT/100.0 * total_pixels / CELL_GRAN) * CELL_GRAN;
	TRACE("[H SYNC (PIXELS)] %g\n", h_sync);

	// 18. Find the number of pixels in the horizontal front porch period:
	//	[H FRONT PORCH (PIXELS)] = ([H BLANK (PIXELS)]/2)-[H SYNC (PIXELS)]
	h_front_porch = (h_blank / 2.0) - h_sync;
	TRACE("[H FRONT PORCH (PIXELS)] %g\n", h_front_porch);

	// 36. Find the number of lines in the odd front porch period:
	//	[V ODD FRONT PORCH(LINES)]=([MIN PORCH RND]+[INTERLACE])
	v_odd_front_porch_lines = MIN_PORCH + interlace;
	TRACE("[V ODD FRONT PORCH(LINES)] %g\n", v_odd_front_porch_lines);

	// finally, pack the results in the mode struct

	timing->pixel_clock = uint32(pixel_freq * 1000);
	timing->h_display = (uint16)h_pixels_rnd;
	timing->h_sync_start = (uint16)(h_pixels_rnd + h_front_porch);
	timing->h_sync_end = (uint16)(h_pixels_rnd + h_front_porch + h_sync);
	timing->h_total = (uint16)total_pixels;
	timing->v_display = (uint16)v_lines_rnd;
	timing->v_sync_start = (uint16)(v_lines_rnd + v_odd_front_porch_lines);
	timing->v_sync_end = (uint16)(v_lines_rnd + v_odd_front_porch_lines
		+ V_SYNC_RQD);
	timing->v_total = (uint16)total_v_lines;
	timing->flags = B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
		| (interlace ? B_TIMING_INTERLACED : 0);

	TRACE("GTF TIMING: %lu kHz, (%u, %u, %u, %u), (%u, %u, %u, %u)\n",
		timing->pixel_clock, timing->h_display, timing->h_sync_start,
		timing->h_sync_end, timing->h_total, timing->v_display,
		timing->v_sync_start, timing->v_sync_end, timing->v_total);

	return B_OK;
}
