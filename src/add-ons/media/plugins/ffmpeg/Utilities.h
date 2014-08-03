/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2014, Colin Günther <coling@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU L-GPL license.
 */
#ifndef UTILITIES_H
#define UTILITIES_H


/*! \brief This file contains functions to convert values from FFmpeg to Media
		Kit and vice versa.
*/


#include <assert.h>

extern "C" {
	#include "avcodec.h"
}


/*! \brief Converts FFmpeg notation of video aspect ratio into the Media Kits
		notation.

	\param pixelWidthAspectOut On return contains the Media Kits notation of
		the video aspect ratio width. E.g. 16:9 -> 16 is returned here
	\param pixelHeightAspectOut On return contains the Media Kits notation of
		the video aspect ratio height. E.g. 16:9 -> 9 is returned here
	\param contextIn An AVCodeContext structure of FFmpeg containing the values
		needed to calculate the Media Kit video aspect ratio.
		The following fields are used for the calculation:
			- AVCodecContext.sample_aspect_ratio.num (optional)
			- AVCodecContext.sample_aspect_ratio.den (optional)
			- AVCodecContext.width (must)
			- AVCodecContext.height (must)
*/
inline void
ConvertAVCodecContextToVideoAspectWidthAndHeight(AVCodecContext& contextIn,
	uint16& pixelWidthAspectOut, uint16& pixelHeightAspectOut)
{
	assert(contextIn.sample_aspect_ratio.num >= 0);
	assert(contextIn.sample_aspect_ratio.den > 0);
	assert(contextIn.width > 0);
	assert(contextIn.height > 0);

	// The following code is based on code originally located in
	// AVFormatReader::Stream::Init() and thus should be copyrighted to Stephan
	// Aßmus
	AVRational pixelAspectRatio;

	if (contextIn.sample_aspect_ratio.num == 0) {
		// AVCodecContext doesn't contain a video aspect ratio, so calculate it
		// ourselve based solely on the video dimensions
		av_reduce(&pixelAspectRatio.num, &pixelAspectRatio.den, contextIn.width,
			contextIn.height, 1024 * 1024);
	
		pixelWidthAspectOut = static_cast<int16>(pixelAspectRatio.num);
		pixelHeightAspectOut = static_cast<int16>(pixelAspectRatio.den);
		return;
	}

	// AVCodecContext contains a video aspect ratio, so use it
	av_reduce(&pixelAspectRatio.num, &pixelAspectRatio.den,
		contextIn.width * contextIn.sample_aspect_ratio.num,
		contextIn.height * contextIn.sample_aspect_ratio.den,
		1024 * 1024);

	pixelWidthAspectOut = static_cast<int16>(pixelAspectRatio.num);
	pixelHeightAspectOut = static_cast<int16>(pixelAspectRatio.den);
}



#endif // UTILITIES_H
