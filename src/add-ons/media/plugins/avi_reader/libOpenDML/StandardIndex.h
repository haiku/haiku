/*
 * Copyright (c) 2007, Marcus Overhagen
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
#ifndef _STANDARD_INDEX_H
#define _STANDARD_INDEX_H

#include "Index.h"
#include "avi.h"

class StandardIndex : public Index
{
public:
						StandardIndex(BPositionIO *source, OpenDMLParser *parser);
						~StandardIndex();

	status_t			Init();

	status_t			GetNextChunkInfo(int stream_index, int64* start,
							uint32* size, bool* keyframe);
	status_t			Seek(int stream_index, uint32 seekTo, int64* frame,
							bigtime_t* time, bool readOnly);

private:
	void				DumpIndex();

private:
	struct seek_hint
	{
		uint64			stream_pos;
		uint32			index_pos;
	};

	struct stream_data
	{
		uint32			chunk_id;
		uint32			chunk_count;
		uint32			keyframe_count;
		uint32			stream_pos;
		uint64			stream_size;		
		seek_hint *		seek_hints;
		int				seek_hints_count;
		uint32			seek_hints_next;
	};
		
	avi_standard_index_entry *fIndex;
	uint32				fIndexSize;
	stream_data *		fStreamData;
	int					fStreamCount;
	int64				fDataOffset;
};

#endif
