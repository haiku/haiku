/*
 * Copyright 2012 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

// The code in this file was adapted from the X.org intel driver which had
// the following copyright and license.

/**************************************************************************
Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************/

#include "accelerant.h"


struct WatermarkInfo {
	double freq;
	uint32 watermark;
};

static WatermarkInfo watermarks_8[] = {
	{	0, 0x22003000},
	{25.2, 0x22003000},
	{28.0, 0x22003000},
	{31.5, 0x22003000},
	{36.0, 0x22007000},
	{40.0, 0x22007000},
	{45.0, 0x22007000},
	{49.5, 0x22008000},
	{50.0, 0x22008000},
	{56.3, 0x22008000},
	{65.0, 0x22008000},
	{75.0, 0x22008000},
	{78.8, 0x22008000},
	{80.0, 0x22008000},
	{94.0, 0x22008000},
	{96.0, 0x22107000},
	{99.0, 0x22107000},
	{108.0, 0x22107000},
	{121.0, 0x22107000},
	{128.9, 0x22107000},
	{132.0, 0x22109000},
	{135.0, 0x22109000},
	{157.5, 0x2210b000},
	{162.0, 0x2210b000},
	{175.5, 0x2210b000},
	{189.0, 0x2220e000},
	{202.5, 0x2220e000}
};

static WatermarkInfo watermarks_16[] = {
	{	0, 0x22004000},
	{25.2, 0x22006000},
	{28.0, 0x22006000},
	{31.5, 0x22007000},
	{36.0, 0x22007000},
	{40.0, 0x22007000},
	{45.0, 0x22007000},
	{49.5, 0x22009000},
	{50.0, 0x22009000},
	{56.3, 0x22108000},
	{65.0, 0x2210e000},
	{75.0, 0x2210e000},
	{78.8, 0x2210e000},
	{80.0, 0x22210000},
	{94.5, 0x22210000},
	{96.0, 0x22210000},
	{99.0, 0x22210000},
	{108.0, 0x22210000},
	{121.0, 0x22210000},
	{128.9, 0x22210000},
	{132.0, 0x22314000},
	{135.0, 0x22314000},
	{157.5, 0x22415000},
	{162.0, 0x22416000},
	{175.5, 0x22416000},
	{189.0, 0x22416000},
	{195.0, 0x22416000},
	{202.5, 0x22416000}
};



uint32
I810_GetWatermark(const DisplayModeEx& mode)
{
	WatermarkInfo *table;
	uint32 tableLen;

	// Get burst length and FIFO watermark based upon the bus frequency and
	// pixel clock.

	switch (mode.bitsPerPixel) {
	case 8:
		table = watermarks_8;
		tableLen = ARRAY_SIZE(watermarks_8);
		break;
	case 16:
		table = watermarks_16;
		tableLen = ARRAY_SIZE(watermarks_16);
		break;
	default:
		return 0;
	}

	uint32 i;
	double clockFreq = mode.timing.pixel_clock / 1000.0;

	for (i = 0; i < tableLen && table[i].freq < clockFreq; i++)
		;

	if (i == tableLen)
		i--;

	TRACE("chosen watermark 0x%lx  (freq %f)\n", table[i].watermark,
		table[i].freq);

	return table[i].watermark;
}
