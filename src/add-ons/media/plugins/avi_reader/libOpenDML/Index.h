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
#ifndef _INDEX_H
#define _INDEX_H

#include <SupportDefs.h>
#include <vector>

#include "OpenDMLParser.h"

/*
	This class handles all indexing of an AVI file
	Subclasses should override Init and create index entries based on the
	specialised index.
	
	Seek and GetNextChunk will then work.
	
	Current known subclasses are:
		Standard Index - Original AVI index idx1
		OpenDMLIndex - Open DML Standard Index
		FallBackIndex - Index created from the movi chunk
*/


class BPositionIO;
class OpenDMLParser;

class IndexEntry {
public:
	IndexEntry() {frame_no = 0;position=0;size=0;pts=0;keyframe=false;};
	
	uint64			frame_no;		// frame_no or sample_no
	off_t			position;		// The offset in the stream where the frame is
	uint32			size;			// The size of the data available
	bigtime_t		pts;			// Presentation Time Stamp for this frame
	bool			keyframe;		// Is this a keyframe.
};

class MediaStream {
public:
	MediaStream() {seek_index_next=0;current_chunk=0;} ;
	~MediaStream() {seek_index.clear();};
	std::vector<IndexEntry>	seek_index;
	uint64			seek_index_next;
	uint64			current_chunk;
};

class Index {
public:
						Index(BPositionIO *source, OpenDMLParser *parser);
	virtual				~Index();

	virtual status_t	Init() = 0;

	status_t			GetNextChunkInfo(int stream_index, off_t *start,
							uint32 *size, bool *keyframe);

	status_t			Seek(int stream_index, uint32 seekTo, int64 *frame,
							bigtime_t *time, bool readOnly);

	void				AddIndex(int stream_index, off_t position, uint32 size, uint64 frame, bigtime_t pts, bool keyframe);
	void				DumpIndex(int stream_index);

protected:
	BPositionIO *		fSource;
	OpenDMLParser *		fParser;
	int					fStreamCount;

private:
	std::vector<MediaStream>	fStreamData;
};

#endif	// _INDEX_H
