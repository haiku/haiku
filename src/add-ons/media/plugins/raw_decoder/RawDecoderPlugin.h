/*
 * Copyright (c) 2003-2004, Marcus Overhagen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _RAW_DECODER_PLUGIN_H
#define _RAW_DECODER_PLUGIN_H

#include <Decoder.h>

using BCodecKit::BMediaPlugin;
using BCodecKit::BDecoder;
using BCodecKit::BDecoderPlugin;


class RawDecoder : public BDecoder
{
public:
	void		GetCodecInfo(media_codec_info *info);

	status_t	Setup(media_format *ioEncodedFormat,
					  const void *infoBuffer, size_t infoSize);
					  
	status_t	NegotiateOutputFormat(media_format *ioDecodedFormat);
	
	status_t	SeekedTo(int64 frame, bigtime_t time);
							 
	status_t	Decode(void *buffer, int64 *frameCount,
					   media_header *mediaHeader, media_decode_info *info);

private:
	status_t	NegotiateVideoOutputFormat(media_format *ioDecodedFormat);
	status_t	NegotiateAudioOutputFormat(media_format *ioDecodedFormat);

private:
	int32			fFrameRate;
	int32			fInputFrameSize;
	int32			fOutputFrameSize;
	int32			fInputSampleSize;
	int32			fOutputSampleSize;
	int32			fOutputBufferFrameCount;
	void 			(*fSwapInput)(void *data, int32 count);
	void 			(*fConvert)(void *dst, const void *src, int32 count);
	void 			(*fSwapOutput)(void *data, int32 count);
	
	const void *	fChunkBuffer;
	size_t			fChunkSize;
	
	bigtime_t		fStartTime;
	
	media_format	fInputFormat;
};


class RawDecoderPlugin : public BDecoderPlugin
{
public:
	BDecoder *	NewDecoder(uint index);
	status_t	GetSupportedFormats(media_format ** formats, size_t * count);
};

#endif
