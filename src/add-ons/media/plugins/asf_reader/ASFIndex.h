/*
 * Copyright (c) 2009, David McPaul
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
#ifndef _ASF_INDEX_H
#define _ASF_INDEX_H

#include <SupportDefs.h>

#include <vector>

class IndexEntry {
public:
	IndexEntry() {frameNo = 0;noPayloads=0;dataSize=0;pts=0;keyFrame=false;};
	
	uint32		frameNo;		// frame_no or sample_no
	uint8		noPayloads;		// The number of payloads that make up this frame
	uint32		dataSize;		// The size of the data available
	bigtime_t	pts;			// Presentation Time Stamp for this frame
	bool		keyFrame;		// Is this a keyframe.
	uint8		id;				// The id for this Entry
	
	void Clear();
	void AddPayload(uint32 pdataSize);
};

class StreamEntry {
public:
	uint16	streamIndex;

	StreamEntry();
	~StreamEntry();
		
	IndexEntry GetIndex(uint32 frameNo);
	bool HasIndex(uint32 frameNo);
	IndexEntry GetIndex(bigtime_t pts);
	bool HasIndex(bigtime_t pts);

	bigtime_t getMaxPTS() {return maxPTS;};
	uint32	getFrameCount() {return frameCount;};
	bigtime_t getDuration() {return duration;};
	void setDuration(bigtime_t pduration);
	
	void AddPayload(uint32 id, bool keyFrame, bigtime_t pts, uint32 dataSize, bool isLast);
	
private:
	std::vector<IndexEntry> index;
	
	IndexEntry indexEntry;
	uint32	lastID;
	uint32	frameCount;
	bigtime_t maxPTS;
	bigtime_t duration;
};

#endif
