/*
 * Copyright (c) 2005, David McPaul based on avi_reader copyright (c) 2004 Marcus Overhagen
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
#ifndef _MP4_READER_H
#define _MP4_READER_H

#include "ReaderPlugin.h"
#include "libMP4/MP4FileReader.h"

class mp4Reader : public Reader
{
public:
				mp4Reader();
				~mp4Reader();
	
	virtual	const char*		Copyright();
	
	virtual	status_t		Sniff(int32 *streamCount);

	virtual	void			GetFileFormatInfo(media_file_format *mff);

	virtual	status_t		AllocateCookie(int32 streamNumber, void **cookie);
	virtual	status_t		FreeCookie(void *cookie);
	
	virtual	status_t		GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
							  media_format *format, const void **infoBuffer, size_t *infoSize);

	virtual	status_t		Seek(void *cookie, uint32 flags,
					 			int64 *frame, bigtime_t *time);

	virtual	status_t		FindKeyFrame(void* cookie, uint32 flags,
								int64* frame, bigtime_t* time);

	virtual	status_t		GetNextChunk(void *cookie,
								const void **chunkBuffer, size_t *chunkSize,
								media_header *mediaHeader);
private:
	MP4FileReader *theFileReader;
};


class mp4ReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

MediaPlugin *instantiate_plugin();

#endif
