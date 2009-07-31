/*
 * Copyright 2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

/* Copyright (c) 2001, Andy Ritger  aritger@nvidia.com
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
 */


/*!	This file contains function(s) to generate video mode timings using the
	GTF Timing Standard, based on VESA.org's GTF_V1R1.xls.
*/


#include "gtf.h"

#include <math.h>


#define CELL_GRAN         8.0   // assumed character cell granularity
#define MIN_PORCH         1     // minimum front porch
#define V_SYNC_RQD        3     // width of vsync in lines
#define H_SYNC_PERCENT    8.0   // width of hsync as % of total line
#define MIN_VSYNC_PLUS_BP 550.0 // min time of vsync + back porch (microsec)

// C' and M' are part of the Blanking Duty Cycle computation.

#define C_PRIME           30.0
#define M_PRIME           300.0


/*!	Computes the timing values for the specified video mode using the
	VESA Generalized Timing Formula (GTF).
	\a width is the display width in pixels, \a lines is the display height
	in lines, and \a refreshRate is the refresh rate in Hz.  The computed
	timing values are returned in \a modeTiming.
*/
void
ComputeGTFVideoTiming(int width, int lines, double refreshRate,
	display_timing& modeTiming)
{
	// In order to give correct results, the number of horizontal pixels
	// requested is first processed to ensure that it is divisible by the
	// character size, by rounding it to the nearest character cell boundary.

	width = int((width / CELL_GRAN) * CELL_GRAN);

	// Estimate the Horizontal period.

	double horizontalPeriodEstimate = (((1.0 / refreshRate)
		- (MIN_VSYNC_PLUS_BP / 1000000.0)) / (lines + MIN_PORCH) * 1000000.0);

	// Compute the number of lines in V sync + back porch.

	double verticalSyncAndBackPorch
		= rint(MIN_VSYNC_PLUS_BP / horizontalPeriodEstimate);

	// Compute the total number of lines in Vertical field period.

	double totalLines = lines + verticalSyncAndBackPorch + MIN_PORCH;

	// Estimate the Vertical field frequency.

	double verticalFieldRateEstimate = 1.0 / horizontalPeriodEstimate
		/ totalLines * 1000000.0;

	// Compute the actual horizontal period.

	double horizontalPeriod = horizontalPeriodEstimate
		/ (refreshRate / verticalFieldRateEstimate);

	// Compute the ideal blanking duty cycle from the blanking duty cycle
	// equation.

	double idealDutyCycle = C_PRIME - (M_PRIME * horizontalPeriod / 1000.0);

	// Compute the number of pixels in the horizontal blanking time to the
	// nearest double character cell.

	double horizontalBlank = rint(width * idealDutyCycle
		/ (100.0 - idealDutyCycle)
		/ (2.0 * CELL_GRAN)) * (2.0 * CELL_GRAN);

	// Compute the total number of pixels in a horizontal line.

	double totalWidth = width + horizontalBlank;

	// Compute the number of pixels in the horizontal sync period.

	double horizontalSync
		= rint(H_SYNC_PERCENT / 100.0 * totalWidth / CELL_GRAN) * CELL_GRAN;

	// Compute the number of pixels in the horizontal front porch period.

	double horizontalFrontPorch = (horizontalBlank / 2.0) - horizontalSync;

	// Finally, return the results in a display_timing struct.

	modeTiming.pixel_clock	= uint32(totalWidth * 1000.0 / horizontalPeriod);

	modeTiming.h_display = uint16(width);
	modeTiming.h_sync_start = uint16(width + horizontalFrontPorch);
	modeTiming.h_sync_end
		= uint16(width + horizontalFrontPorch + horizontalSync);
	modeTiming.h_total = uint16(totalWidth);

	modeTiming.v_display = uint16(lines);
	modeTiming.v_sync_start = uint16(lines + MIN_PORCH);
	modeTiming.v_sync_end = uint16(lines + MIN_PORCH + V_SYNC_RQD);
	modeTiming.v_total = uint16(totalLines);

	modeTiming.flags = B_POSITIVE_VSYNC;
		// GTF timings use -hSync and +vSync
}
