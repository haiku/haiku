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
#ifndef _AIFF_READER_H
#define _AIFF_READER_H
 
#include "ReaderPlugin.h"
#include "aiff.h"

class aiffReader : public Reader
{
public:
				aiffReader();
				~aiffReader();

	const char *Copyright();

	status_t	Sniff(int32 *streamCount);

	void		GetFileFormatInfo(media_file_format *mff);

	status_t	AllocateCookie(int32 streamNumber, void **cookie);
	status_t	FreeCookie(void *cookie);

	status_t	GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
							  media_format *format, const void **infoBuffer, size_t *infoSize);

	status_t	Seek(void *cookie,
					 uint32 seekTo,
					 int64 *frame, bigtime_t *time);

	status_t	GetNextChunk(void *cookie,
							 const void **chunkBuffer, size_t *chunkSize,
							 media_header *mediaHeader);
private:
	uint32		DecodeFrameRate(const void *_80bit_float);

	BPositionIO *Source() { return fSource; }

private:
	BPositionIO *	fSource;

	media_format	fFormat;
	int64			fDataStart;
	int64			fDataSize;

	int64			fFrameCount;
	bigtime_t		fDuration;

	bool			fRaw;

	int64			fPosition;
	
	int				fChannelCount;
	int32			fFrameRate;
	int				fValidBitsPerSample;
	int				fBytesPerSample;
	int				fBytesPerFrame;
	uint32			fFormatCode;
	
	void *			fBuffer;
	int32 			fBufferSize;
};


class aiffReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

MediaPlugin *instantiate_plugin();

#endif
