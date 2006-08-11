/*
 * Copyright (c) 2005, David McPaul
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
#ifndef _CHUNK_INDEX_H
#define _CHUNK_INDEX_H


#include <SupportDefs.h>

#include <map>


struct ChunkIndex {
	uint32	stream;
	uint32	chunkid;
	off_t	chunk_start;
};

typedef ChunkIndex* ChunkIndexPtr;
typedef std::map<off_t, ChunkIndexPtr, std::less<off_t> > ChunkIndexArray;

class ChunkSuperIndex {
public:
			ChunkSuperIndex();
	virtual	~ChunkSuperIndex();
	
	void	AddChunkIndex(uint32 stream, uint32 chunkid, off_t chunk_start);
	uint32	getChunkSize(uint32 stream, uint32 chunkid, off_t chunk_start);
	
private:
	ChunkIndexArray	theChunkIndexArray;
};

#endif // _CHUNK_INDEX_H
