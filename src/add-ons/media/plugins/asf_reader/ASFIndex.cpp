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

#include "ASFIndex.h"

#include <stdio.h>


void
IndexEntry::Clear() 
{
	frameNo=0;
	noPayloads=0;
	dataSize=0;
	pts=0;
	keyFrame=false;
}

void
IndexEntry::AddPayload(uint32 pdataSize) 
{
	noPayloads++; 
	dataSize += pdataSize;
}

StreamEntry::StreamEntry() 
{
	lastID = 0;
	frameCount = 0;
	maxPTS = 0;
	duration = 0;
}

StreamEntry::~StreamEntry() 
{
	index.clear();
}

void
StreamEntry::setDuration(bigtime_t pduration)
{
	duration = pduration;
}


IndexEntry
StreamEntry::GetIndex(uint32 frameNo)
{
	IndexEntry indexEntry;

	for (std::vector<IndexEntry>::iterator itr = index.begin(); itr != index.end(); ++itr) {
		if (itr->frameNo == frameNo) {
			indexEntry = *itr;
			break;
		}
	}
	return indexEntry;
}
	
bool
StreamEntry::HasIndex(uint32 frameNo)
{
	if (!index.empty()) {
		for (std::vector<IndexEntry>::iterator itr = index.begin();	itr != index.end();	++itr) {
 			if (itr->frameNo == frameNo) {
 				return true;
			}
		}
	}
		
	return false;
}

IndexEntry
StreamEntry::GetIndex(bigtime_t pts)
{
	IndexEntry indexEntry;

	for (std::vector<IndexEntry>::iterator itr = index.begin(); itr != index.end(); ++itr) {
		if (pts <= itr->pts) {
			indexEntry = *itr;
			break;
		}
	}
	return indexEntry;
}
	
bool
StreamEntry::HasIndex(bigtime_t pts)
{
	if (!index.empty()) {
		for (std::vector<IndexEntry>::iterator itr = index.begin();	itr != index.end();	++itr) {
 			if (pts <= itr->pts) {
 				return true;
			}
		}
	}
		
	return false;
}

/*
	Combine payloads with the same id into a single IndexEntry
	When isLast flag is set then no more payloads are available (ie add current entry to index)

*/
void
StreamEntry::AddPayload(uint32 id, bool keyFrame, bigtime_t pts, uint32 dataSize, bool isLast)
{
	if (isLast) {
		maxPTS = indexEntry.pts;
		if (frameCount > 0) {
			index.push_back(indexEntry);
//			printf("Stream %d added Index %ld PTS %Ld payloads %d\n",streamIndex, indexEntry.frameNo, indexEntry.pts, indexEntry.noPayloads);
			printf("Stream Index Loaded for Stream %d Max Frame %ld Max PTS %Ld size %ld\n",streamIndex, frameCount-1, maxPTS, index.size());
		}
	} else {
		if (id != lastID) {
			if (frameCount != 0) {
				// add indexEntry to Index
				index.push_back(indexEntry);
//				printf("Stream %d added Index %ld PTS %Ld payloads %d\n",streamIndex, indexEntry.frameNo, indexEntry.pts, indexEntry.noPayloads);
			}
			lastID = id;
			indexEntry.Clear();
		
			indexEntry.frameNo = frameCount++;
			indexEntry.keyFrame = keyFrame;
			indexEntry.pts = pts;
			indexEntry.dataSize = dataSize;
			indexEntry.noPayloads = 1;
			indexEntry.id = id;
		} else {
			indexEntry.AddPayload(dataSize);
		}	
	}
}
