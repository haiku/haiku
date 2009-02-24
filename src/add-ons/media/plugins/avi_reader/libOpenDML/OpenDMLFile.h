/*
 * Copyright (c) 2004-2007, Marcus Overhagen
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
#ifndef _OPEN_DML_FILE_H
#define _OPEN_DML_FILE_H

#include <DataIO.h>
#include "OpenDMLParser.h"
#include "Index.h"

class OpenDMLFile
{
public:
						OpenDMLFile(BPositionIO *source);
						~OpenDMLFile();

	static bool 		IsSupported(BPositionIO *source);

	status_t			Init();
	
	int					StreamCount();

	const OpenDMLStream * StreamInfo(int index);
	
/*
	bigtime_t			Duration();
	uint32				FrameCount();
*/

	bool				IsVideo(int stream_index);
	bool				IsAudio(int stream_index);

	const avi_main_header *AviMainHeader();

	const wave_format_ex *		AudioFormat(int stream_index, size_t *size = 0);
	const bitmap_info_header *	VideoFormat(int stream_index);
	const avi_stream_header *	StreamFormat(int stream_index);
	
	status_t			GetNextChunkInfo(int stream_index, off_t *start,
							uint32 *size, bool *keyframe);
	status_t			Seek(int stream_index, uint32 seekTo, int64 *frame,
							bigtime_t *time, bool readOnly);
	
	BPositionIO *Source() { return fSource; }
		
private:
	BPositionIO *	fSource;
	OpenDMLParser *	fParser;
	Index *			fIndex;
};

#endif
