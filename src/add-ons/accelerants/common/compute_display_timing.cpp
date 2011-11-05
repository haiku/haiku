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


#define MARGIN_PERCENT				1.8		// % of active vertical image
#define CELL_GRANULARITY			8.0
	// assumed character cell granularity
#define MIN_PORCH					1		// minimum front porch
#define V_SYNC_WIDTH				3		// width of vsync in lines
#define H_SYNC_PERCENT				8.0		// width of hsync as % of total line
#define MIN_VSYNC_PLUS_BACK_PORCH	550.0	// time in microsec

// C' and M' are part of the Blanking Duty Cycle computation

#define M					600.0	// blanking formula gradient
#define C					40.0	// blanking formula offset
#define K					128.0	// blanking formula scaling factor
#define J					20.0	// blanking formula scaling factor
#define C_PRIME				(((C - J) * K / 256.0) + J)
#define M_PRIME				(K / 256.0 * M)


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

	bool margins = false;

	// 1. In order to give correct results, the number of horizontal
	// pixels requested is first processed to ensure that it is divisible
	// by the character size, by rounding it to the nearest character
	// cell boundary:
	//	[H PIXELS RND] = ((ROUND([H PIXELS]/[CELL GRAN RND],0))*[CELLGRAN RND])
	width = (uint32)(rint(width / CELL_GRANULARITY) * CELL_GRANULARITY);

	// 2. If interlace is requested, the number of vertical lines assumed
	// by the calculation must be halved, as the computation calculates
	// the number of vertical lines per field. In either case, the
	// number of lines is rounded to the nearest integer.
	//	[V LINES RND] = IF([INT RQD?]="y", ROUND([V LINES]/2,0),
	//		ROUND([V LINES],0))
	float verticalLines = interlaced ? (double)height / 2.0 : (double)height;

	// 3. Find the frame rate required:
	//	[V FIELD RATE RQD] = IF([INT RQD?]="y", [I/P FREQ RQD]*2,
	//		[I/P FREQ RQD])
	float verticalFieldRate = interlaced ? refresh * 2.0 : refresh;

	// 4. Find number of lines in Top margin:
	//	[TOP MARGIN (LINES)] = IF([MARGINS RQD?]="Y",
	//		ROUND(([MARGIN%]/100*[V LINES RND]),0), 0)
	float topMargin = margins ? rint(MARGIN_PERCENT / 100.0 * verticalLines)
		: 0.0;

	// 5. Find number of lines in Bottom margin:
	//	[BOT MARGIN (LINES)] = IF([MARGINS RQD?]="Y",
	//		ROUND(([MARGIN%]/100*[V LINES RND]),0), 0)
	float bottomMargin = margins ? rint(MARGIN_PERCENT / 100.0 * verticalLines)
		: 0.0;

	// 6. If interlace is required, then set variable [INTERLACE]=0.5:
	//	[INTERLACE]=(IF([INT RQD?]="y",0.5,0))
	float interlace = interlaced ? 0.5 : 0.0;

	// 7. Estimate the Horizontal period
	//	[H PERIOD EST] = ((1/[V FIELD RATE RQD]) - [MIN VSYNC+BP]/1000000)
	//			/ ([V LINES RND] + (2*[TOP MARGIN (LINES)])
	//				+ [MIN PORCH RND]+[INTERLACE]) * 1000000
	float horizontalPeriodEstimate = (1.0 / verticalFieldRate
			- MIN_VSYNC_PLUS_BACK_PORCH / 1000000.0)
		/ (verticalLines + (2 * topMargin) + MIN_PORCH + interlace) * 1000000.0;

	// 8. Find the number of lines in V sync + back porch:
	//	[V SYNC+BP] = ROUND(([MIN VSYNC+BP]/[H PERIOD EST]),0)
	float verticalSyncPlusBackPorch = rint(MIN_VSYNC_PLUS_BACK_PORCH
		/ horizontalPeriodEstimate);

	// 10. Find the total number of lines in Vertical field period:
	//	[TOTAL V LINES] = [V LINES RND] + [TOP MARGIN (LINES)]
	//		+ [BOT MARGIN (LINES)] + [V SYNC+BP] + [INTERLACE] + [MIN PORCH RND]
	float totalVerticalLines = verticalLines + topMargin + bottomMargin
		+ verticalSyncPlusBackPorch + interlace + MIN_PORCH;

	// 11. Estimate the Vertical field frequency:
	//	[V FIELD RATE EST] = 1 / [H PERIOD EST] / [TOTAL V LINES] * 1000000
	float verticalFieldRateEstimate = 1.0 / horizontalPeriodEstimate
		/ totalVerticalLines * 1000000.0;

	// 12. Find the actual horizontal period:
	//	[H PERIOD] = [H PERIOD EST] / ([V FIELD RATE RQD] / [V FIELD RATE EST])
	float horizontalPeriod = horizontalPeriodEstimate
		/ (verticalFieldRate / verticalFieldRateEstimate);

	// 15. Find number of pixels in left margin:
	//	[LEFT MARGIN (PIXELS)] = (IF( [MARGINS RQD?]="Y",
	//			(ROUND( ([H PIXELS RND] * [MARGIN%] / 100 /
	//				[CELL GRAN RND]),0)) * [CELL GRAN RND], 0))
	float leftMargin = margins ? rint(width * MARGIN_PERCENT / 100.0
			/ CELL_GRANULARITY) * CELL_GRANULARITY : 0.0;

	// 16. Find number of pixels in right margin:
	//	[RIGHT MARGIN (PIXELS)] = (IF( [MARGINS RQD?]="Y",
	//			(ROUND( ([H PIXELS RND] * [MARGIN%] / 100 /
	//				[CELL GRAN RND]),0)) * [CELL GRAN RND], 0))
	float rightMargin = margins ? rint(width * MARGIN_PERCENT / 100.0
			/ CELL_GRANULARITY) * CELL_GRANULARITY : 0.0;

	// 17. Find total number of active pixels in image and left and right
	// margins:
	//	[TOTAL ACTIVE PIXELS] = [H PIXELS RND] + [LEFT MARGIN (PIXELS)]
	//		+ [RIGHT MARGIN (PIXELS)]
	float totalActivePixels = width + leftMargin + rightMargin;

	// 18. Find the ideal blanking duty cycle from the blanking duty cycle
	// equation:
	//	[IDEAL DUTY CYCLE] = [C'] - ([M']*[H PERIOD]/1000)
	float idealDutyCycle = C_PRIME - (M_PRIME * horizontalPeriod / 1000.0);

	// 19. Find the number of pixels in the blanking time to the nearest
	// double character cell:
	//	[H BLANK (PIXELS)] = (ROUND(([TOTAL ACTIVE PIXELS]
	//			* [IDEAL DUTY CYCLE] / (100-[IDEAL DUTY CYCLE])
	//			/ (2*[CELL GRAN RND])), 0)) * (2*[CELL GRAN RND])
	float horizontalBlank = rint(totalActivePixels * idealDutyCycle
			/ (100.0 - idealDutyCycle) / (2.0 * CELL_GRANULARITY))
		* (2.0 * CELL_GRANULARITY);

	// 20. Find total number of pixels:
	//	[TOTAL PIXELS] = [TOTAL ACTIVE PIXELS] + [H BLANK (PIXELS)]
	float totalPixels = totalActivePixels + horizontalBlank;

	// 21. Find pixel clock frequency:
	//	[PIXEL FREQ] = [TOTAL PIXELS] / [H PERIOD]
	float pixelFrequency = totalPixels / horizontalPeriod;

	// Stage 1 computations are now complete; I should really pass
	// the results to another function and do the Stage 2
	// computations, but I only need a few more values so I'll just
	// append the computations here for now */

	// 17. Find the number of pixels in the horizontal sync period:
	//	[H SYNC (PIXELS)] =(ROUND(([H SYNC%] / 100 * [TOTAL PIXELS]
	//		/ [CELL GRAN RND]),0))*[CELL GRAN RND]
	float horizontalSync = rint(H_SYNC_PERCENT / 100.0 * totalPixels
			/ CELL_GRANULARITY) * CELL_GRANULARITY;

	// 18. Find the number of pixels in the horizontal front porch period:
	//	[H FRONT PORCH (PIXELS)] = ([H BLANK (PIXELS)]/2)-[H SYNC (PIXELS)]
	float horizontalFrontPorch = (horizontalBlank / 2.0) - horizontalSync;

	// 36. Find the number of lines in the odd front porch period:
	//	[V ODD FRONT PORCH(LINES)]=([MIN PORCH RND]+[INTERLACE])
	float verticalOddFrontPorchLines = MIN_PORCH + interlace;

	// finally, pack the results in the mode struct

	timing->pixel_clock = uint32(pixelFrequency * 1000);
	timing->h_display = (uint16)width;
	timing->h_sync_start = (uint16)(width + horizontalFrontPorch);
	timing->h_sync_end
		= (uint16)(width + horizontalFrontPorch + horizontalSync);
	timing->h_total = (uint16)totalPixels;
	timing->v_display = (uint16)verticalLines;
	timing->v_sync_start = (uint16)(verticalLines + verticalOddFrontPorchLines);
	timing->v_sync_end
		= (uint16)(verticalLines + verticalOddFrontPorchLines + V_SYNC_WIDTH);
	timing->v_total = (uint16)totalVerticalLines;
	timing->flags = B_POSITIVE_HSYNC | B_POSITIVE_VSYNC
		| (interlace ? B_TIMING_INTERLACED : 0);

	TRACE("GTF TIMING: %lu kHz, (%u, %u, %u, %u), (%u, %u, %u, %u)\n",
		timing->pixel_clock, timing->h_display, timing->h_sync_start,
		timing->h_sync_end, timing->h_total, timing->v_display,
		timing->v_sync_start, timing->v_sync_end, timing->v_total);

	return B_OK;
}
