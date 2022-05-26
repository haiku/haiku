/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2014, Colin Günther <coling@gmx.de>
 * Copyright 2018, Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the GNU L-GPL license.
 */
#ifndef UTILITIES_H
#define UTILITIES_H


/*! \brief This file contains functions to convert and calculate values from
		FFmpeg to Media Kit and vice versa.
*/


#include <assert.h>
#include <stdio.h>

#include <GraphicsDefs.h>

extern "C" {
	#include "avcodec.h"
}


/*! \brief Converts FFmpeg notation of video aspect ratio into the Media Kits
		notation.

	\see ConvertVideoAspectWidthAndHeightToAVCodecContext() for converting in
		the other direction.

	\param contextIn An AVCodeContext structure of FFmpeg containing the values
		needed to calculate the Media Kit video aspect ratio.
		The following fields are used for the calculation:
			- AVCodecContext.sample_aspect_ratio.num (optional)
			- AVCodecContext.sample_aspect_ratio.den (optional)
			- AVCodecContext.width (must)
			- AVCodecContext.height (must)
	\param pixelWidthAspectOut On return contains Media Kits notation of the
		video aspect ratio width. E.g. 16:9 -> 16 is returned here
	\param pixelHeightAspectOut On return contains Media Kits notation of the
		video aspect ratio height. E.g. 16:9 -> 9 is returned here
*/
inline void
ConvertAVCodecContextToVideoAspectWidthAndHeight(AVCodecContext& contextIn,
	uint16& pixelWidthAspectOut, uint16& pixelHeightAspectOut)
{
	if (contextIn.width <= 0 || contextIn.height <= 0) {
		fprintf(stderr, "Cannot compute video aspect ratio correctly\n");
		pixelWidthAspectOut = 1;
		pixelHeightAspectOut = 1;
		return;
	}

	assert(contextIn.sample_aspect_ratio.num >= 0);

	AVRational pixelAspectRatio;

	if (contextIn.sample_aspect_ratio.num == 0
		|| contextIn.sample_aspect_ratio.den == 0) {
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


inline void
ConvertAVCodecParametersToVideoAspectWidthAndHeight(AVCodecParameters& parametersIn,
	uint16& pixelWidthAspectOut, uint16& pixelHeightAspectOut)
{
	if (parametersIn.width <= 0 || parametersIn.height <= 0) {
		fprintf(stderr, "Cannot compute video aspect ratio correctly\n");
		pixelWidthAspectOut = 1;
		pixelHeightAspectOut = 1;
		return;
	}

	assert(parametersIn.sample_aspect_ratio.num >= 0);

	AVRational pixelAspectRatio;

	if (parametersIn.sample_aspect_ratio.num == 0
		|| parametersIn.sample_aspect_ratio.den == 0) {
		// AVCodecContext doesn't contain a video aspect ratio, so calculate it
		// ourselve based solely on the video dimensions
		av_reduce(&pixelAspectRatio.num, &pixelAspectRatio.den, parametersIn.width,
			parametersIn.height, 1024 * 1024);

		pixelWidthAspectOut = static_cast<int16>(pixelAspectRatio.num);
		pixelHeightAspectOut = static_cast<int16>(pixelAspectRatio.den);
		return;
	}

	// AVCodecContext contains a video aspect ratio, so use it
	av_reduce(&pixelAspectRatio.num, &pixelAspectRatio.den,
		parametersIn.width * parametersIn.sample_aspect_ratio.num,
		parametersIn.height * parametersIn.sample_aspect_ratio.den,
		1024 * 1024);

	pixelWidthAspectOut = static_cast<int16>(pixelAspectRatio.num);
	pixelHeightAspectOut = static_cast<int16>(pixelAspectRatio.den);
}


/*!	\brief Converts the Media Kits notation of video aspect ratio into FFmpegs
		notation.

	\see ConvertAVCodecContextToVideoAspectWidthAndHeight() for converting in
		the other direction.

	\param pixelWidthAspectIn Contains Media Kits notation of the video aspect
		ratio width. E.g. 16:9 -> 16 is passed here.
	\param pixelHeightAspectIn Contains Media Kits notation of the video aspect
		ratio height. E.g. 16:9 -> 9 is passed here.
	\param contextInOut	An AVCodecContext structure of FFmpeg.
		On input must contain the following fields already initialized
		otherwise the behaviour is undefined:
			- AVCodecContext.width (must)
			- AVCodecContext.height (must)
		On output contains converted values in the following fields (other
		fields stay as they were on input):
			- AVCodecContext.sample_aspect_ratio.num
			- AVCodecContext.sample_aspect_ratio.den
*/
inline void
ConvertVideoAspectWidthAndHeightToAVCodecContext(uint16 pixelWidthAspectIn,
	uint16 pixelHeightAspectIn, AVCodecContext& contextInOut)
{
	if (contextInOut.width <= 0 || contextInOut.height <= 0) {
		fprintf(stderr, "Cannot compute video aspect ratio correctly\n");
		// We can't do anything, set the aspect ratio to 'ignore'.
		contextInOut.sample_aspect_ratio.num = 0;
		contextInOut.sample_aspect_ratio.den = 1;
		return;
	}

	assert(pixelWidthAspectIn > 0);
	assert(pixelHeightAspectIn > 0);

	AVRational pureVideoDimensionAspectRatio;
	av_reduce(&pureVideoDimensionAspectRatio.num,
		&pureVideoDimensionAspectRatio.den, contextInOut.width,
		contextInOut.height, 1024 * 1024);

	if (pureVideoDimensionAspectRatio.num == pixelWidthAspectIn
		&& pureVideoDimensionAspectRatio.den == pixelHeightAspectIn) {
		// The passed Media Kit pixel aspect ratio equals the video dimension
		// aspect ratio. Set sample_aspect_ratio to "ignore".
		contextInOut.sample_aspect_ratio.num = 0;
		contextInOut.sample_aspect_ratio.den = 1;
		return;
	}

	av_reduce(&contextInOut.sample_aspect_ratio.num,
		&contextInOut.sample_aspect_ratio.den,
		contextInOut.height * pixelWidthAspectIn,
		contextInOut.width * pixelHeightAspectIn,
		1024 * 1024);
}


/*! \brief Calculates bytes per row for a video frame.

	\param colorSpace The Media Kit color space the video frame uses.
	\param videoWidth The width of the video frame.

	\returns bytes per video frame row
	\returns Zero, when bytes per video frame cannot be calculated.
*/
inline uint32
CalculateBytesPerRowWithColorSpaceAndVideoWidth(color_space colorSpace, int videoWidth)
{
	assert(videoWidth >= 0);

	const uint32 kBytesPerRowUnknown = 0;
	size_t pixelChunk;
	size_t rowAlignment;
	size_t pixelsPerChunk;

	if (get_pixel_size_for(colorSpace, &pixelChunk, &rowAlignment, &pixelsPerChunk) != B_OK)
		return kBytesPerRowUnknown;

	uint32 bytesPerRow = pixelChunk * videoWidth / pixelsPerChunk;
	uint32 numberOfUnalignedBytes = bytesPerRow % rowAlignment;

	if (numberOfUnalignedBytes == 0)
		return bytesPerRow;

	uint32 numberOfBytesNeededForAlignment = rowAlignment - numberOfUnalignedBytes;
	bytesPerRow += numberOfBytesNeededForAlignment;

	return bytesPerRow;
}


/*! \brief Converts FFmpeg notation of video frame rate into the Media Kits
		notation.

	\see ConvertAVCodecContextToVideoFrameRate() for converting in the other
		direction.

	\param contextIn An AVCodeContext structure of FFmpeg containing the values
		needed to calculate the Media Kit video frame rate.
		The following fields are used for the calculation:
			- AVCodecContext.time_base.num (must)
			- AVCodecContext.time_base.den (must)
			- AVCodecContext.ticks_per_frame (must)
	\param frameRateOut On return contains Media Kits notation of the video
		frame rate.
*/
inline void
ConvertAVCodecContextToVideoFrameRate(AVCodecContext& contextIn, float& frameRateOut)
{
	// A framerate of 0 is allowed for single-frame "video" (cover art, for
	// example)
	if (contextIn.time_base.num == 0)
		frameRateOut = 0.0f;

	// assert that we can compute something
	assert(contextIn.time_base.den > 0);

	// The following code is based on private get_fps() function of FFmpeg's
	// ratecontrol.c:
	// https://lists.ffmpeg.org/pipermail/ffmpeg-cvslog/2012-April/049280.html
	double possiblyInterlacedFrameRate = 1.0 / av_q2d(contextIn.time_base);
	double numberOfInterlacedFramesPerFullFrame = FFMAX(contextIn.ticks_per_frame, 1);

	frameRateOut
		= possiblyInterlacedFrameRate / numberOfInterlacedFramesPerFullFrame;
}


/*!	\brief Converts the Media Kits notation of video frame rate to FFmpegs
	notation.
	
	\see ConvertAVCodecContextToVideoFrameRate() for converting in the other
		direction.

	\param frameRateIn Contains Media Kits notation of the video frame rate
		that will be converted into FFmpegs notation. Must be greater than
		zero.
	\param contextOut An AVCodecContext structure of FFmpeg.
		On output contains converted values in the following fields (other
		fields stay as they were on input):
			- AVCodecContext.time_base.num
			- AVCodecContext.time_base.den
			- AVCodecContext.ticks_per_frame is set to 1
*/
inline void
ConvertVideoFrameRateToAVCodecContext(float frameRateIn,
	AVCodecContext& contextOut)
{
	assert(frameRateIn > 0);

	contextOut.ticks_per_frame = 1;
	contextOut.time_base = av_d2q(1.0 / frameRateIn, 1024);
}


/*!	\brief Converts the Media Kits notation of an audio sample format to
		FFmpegs notation.

	\see ConvertAVSampleFormatToRawAudioFormat() for converting in the other
		direction.

	\param rawAudioFormatIn Contains Media Kits notation of an audio sample
		format that will be converted into FFmpegs notation.
	\param sampleFormatOut On output contains FFmpegs notation of the passed
		audio sample format. Might return AV_SAMPLE_FMT_NONE if there is no
		conversion path.
*/
inline void
ConvertRawAudioFormatToAVSampleFormat(uint32 rawAudioFormatIn,
	AVSampleFormat& sampleFormatOut)
{
	switch (rawAudioFormatIn) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			sampleFormatOut = AV_SAMPLE_FMT_FLT;
			return;

		case media_raw_audio_format::B_AUDIO_DOUBLE:
			sampleFormatOut = AV_SAMPLE_FMT_DBL;
			return;

		case media_raw_audio_format::B_AUDIO_INT:
			sampleFormatOut = AV_SAMPLE_FMT_S32;
			return;

		case media_raw_audio_format::B_AUDIO_SHORT:
			sampleFormatOut = AV_SAMPLE_FMT_S16;
			return;

		case media_raw_audio_format::B_AUDIO_UCHAR:
			sampleFormatOut = AV_SAMPLE_FMT_U8;
			return;

		default:
			// Silence compiler warnings about unhandled enumeration values.
			break;
	}

	sampleFormatOut = AV_SAMPLE_FMT_NONE;
}


/*!	\brief Converts FFmpegs notation of an audio sample format to the Media
		Kits notation.

	\see ConvertAVSampleFormatToRawAudioFormat() for converting in the other
		direction.

	\param sampleFormatIn Contains FFmpegs notation of an audio sample format
		that will be converted into the Media Kits notation.
	\param rawAudioFormatOut On output contains Media Kits notation of the
		passed audio sample format. Might return 0 if there is no conversion
		path.
*/
inline void
ConvertAVSampleFormatToRawAudioFormat(AVSampleFormat sampleFormatIn,
	uint32& rawAudioFormatOut)
{
	switch (sampleFormatIn) {
		case AV_SAMPLE_FMT_FLT:
		case AV_SAMPLE_FMT_FLTP:
			rawAudioFormatOut = media_raw_audio_format::B_AUDIO_FLOAT;
			return;

		case AV_SAMPLE_FMT_DBL:
		case AV_SAMPLE_FMT_DBLP:
			rawAudioFormatOut = media_raw_audio_format::B_AUDIO_DOUBLE;
			return;

		case AV_SAMPLE_FMT_S32:
		case AV_SAMPLE_FMT_S32P:
			rawAudioFormatOut = media_raw_audio_format::B_AUDIO_INT;
			return;

		case AV_SAMPLE_FMT_S16:
		case AV_SAMPLE_FMT_S16P:
			rawAudioFormatOut = media_raw_audio_format::B_AUDIO_SHORT;
			return;

		case AV_SAMPLE_FMT_U8:
		case AV_SAMPLE_FMT_U8P:
			rawAudioFormatOut = media_raw_audio_format::B_AUDIO_UCHAR;
			return;

		default:
			// Silence compiler warnings about unhandled enumeration values.
			break;
	}

	const uint32 kBAudioNone = 0;
	rawAudioFormatOut = kBAudioNone;
}


#endif // UTILITIES_H
