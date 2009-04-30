/*
 * Copyright (c) 2003, Marcus Overhagen
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
#ifndef _WAV_READER_PLUGIN_H
#define _WAV_READER_PLUGIN_H

#include "ReaderPlugin.h"
#include "wav.h"

class WavReader : public Reader
{
public:
				WavReader();
				~WavReader();
	
	const char *Copyright();
	
	status_t	Sniff(int32 *streamCount);

	void		GetFileFormatInfo(media_file_format *mff);

	status_t	AllocateCookie(int32 streamNumber, void **cookie);
	status_t	FreeCookie(void *cookie);
	
	status_t	GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
					media_format *format, const void **infoBuffer, size_t *infoSize);

	status_t	Seek(void *cookie, uint32 flags,
					int64 *frame, bigtime_t *time);

	status_t	FindKeyFrame(void* cookie, uint32 flags,
					int64* frame, bigtime_t* time);

	status_t	GetNextChunk(void *cookie,
					const void **chunkBuffer, size_t *chunkSize,
					media_header *mediaHeader);
					
	status_t	CalculateNewPosition(void* cookie, uint32 flags,
					int64 *frame, bigtime_t *time, int64 *position);

	BPositionIO *Source() { return fSource; }
	
private:
	BPositionIO *	fSource;
	int64			fFileSize;
	int64			fDataStart;
	int64			fDataSize;
	
	wave_format_ex	fMetaData;

	int64			fFrameCount;
	int32			fFrameRate;
	uint16			fBufferSize;
};


class WavReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

MediaPlugin *instantiate_plugin();

#endif
