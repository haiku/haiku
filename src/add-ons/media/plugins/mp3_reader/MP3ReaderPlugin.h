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
#ifndef _MP3_READER_PLUGIN_H
#define _MP3_READER_PLUGIN_H

#include "ReaderPlugin.h"

struct mp3data;

namespace BPrivate { namespace media {

class mp3Reader : public Reader
{
public:
				mp3Reader();
				~mp3Reader();
	
	const char *Copyright();
	
	status_t	Sniff(int32 *streamCount);

	void		GetFileFormatInfo(media_file_format *mff);

	status_t	AllocateCookie(int32 streamNumber, void **cookie);
	status_t	FreeCookie(void *cookie);
	
	status_t	GetStreamInfo(void *cookie, int64 *frameCount, bigtime_t *duration,
							  media_format *format, void **infoBuffer, int32 *infoSize);

	status_t	Seek(void *cookie,
					 uint32 seekTo,
					 int64 *frame, bigtime_t *time);

	status_t	GetNextChunk(void *cookie,
							 void **chunkBuffer, int32 *chunkSize,
							 media_header *mediaHeader);

	BPositionIO *Source() { return fSeekableSource; }

private:
	bool		ParseFile();
	bool 		IsMp3File();
	
	// checks if the buffer contains a valid mp3 stream, length should be
	bool		IsValidStream(uint8 *buffer, int size);
	
	int			GetFrameLength(void *header);
	int			GetXingVbrLength(uint8 *header);
	int			GetFraunhoferVbrLength(uint8 *header);
	int			GetLameVbrLength(uint8 *header);
	int			GetId3v2Length(uint8 *header);
	int			GetInfoCbrLength(uint8 *header);
	
	bool		FindData();

	void		ParseXingVbrHeader(int64 pos);
	void		ParseFraunhoferVbrHeader(int64 pos);
	
	int64		XingSeekPoint(float percent);

	bool		ResynchronizeStream(mp3data *data);
	
private:
	BPositionIO *	fSeekableSource;
	int64			fFileSize;
	
	int64			fDataStart;
	int64			fDataSize;
	
	struct xing_vbr_info;
	struct fhg_vbr_info;
	
	xing_vbr_info *	fXingVbrInfo;
	fhg_vbr_info *	fFhgVbrInfo;
};


class mp3ReaderPlugin : public ReaderPlugin
{
public:
	Reader *NewReader();
};

} } // namespace BPrivate::media

using namespace BPrivate::media;

#endif
