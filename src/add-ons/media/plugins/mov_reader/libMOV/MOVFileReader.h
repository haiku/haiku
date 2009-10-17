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
#ifndef _MOV_FILE_READER_H
#define _MOV_FILE_READER_H

#include <File.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <SupportDefs.h>

#include "MOVParser.h"
#include "ChunkSuperIndex.h"

// QT MOV file reader (and maybe MP4 as well)
class MOVFileReader {
private:
	// Atom List
	AtomArray atomChildren;
	uint32	TotalChildren;
	off_t	StreamSize;
	BPositionIO	*theStream;

	AtomBase *GetChildAtom(uint32 patomType, uint32 offset=0);
	uint32	CountChildAtoms(uint32 patomType);
	
	uint32		getMovieTimeScale();

	// Add a atom to the children array (return false on failure)
	bool	AddChild(AtomBase *pChildAtom);

	AudioMetaData	theAudio;
	VideoMetaData	theVideo;
	mov_stream_header theStreamHeader;
	mov_main_header theMainHeader;	
	
	MVHDAtom	*theMVHDAtom;

	ChunkSuperIndex	theChunkSuperIndex;
	
	void	BuildSuperIndex();

public:
			MOVFileReader(BPositionIO *pStream);
	virtual	~MOVFileReader();

	// getter for MVHDAtom
	MVHDAtom	*getMVHDAtom();

	bool	IsEndOfFile(off_t pPosition);
	bool	IsEndOfFile();
	bool	IsEndOfData(off_t pPosition);
	
	// Is this a quicktime file
	static bool	IsSupported(BPositionIO *source);
	
	// How many tracks in file
	uint32		getStreamCount();
	
	// Duration defined by the movie header
	bigtime_t	getMovieDuration();
	
	// The first video track duration indexed by stream_index
	bigtime_t	getVideoDuration(uint32 stream_index);
	// the first audio track duration indexed by stream_index
	bigtime_t	getAudioDuration(uint32 stream_index);
	// the max of all active audio or video durations
	bigtime_t	getMaxDuration();

	// The no of frames in the video track indexed by stream_index
	uint32		getVideoFrameCount(uint32 stream_index);
	// The no of frames in the audio track indexed by stream_index
	uint32		getAudioFrameCount(uint32 stream_index);

	// Is stream (track) a video track
	bool		IsVideo(uint32 stream_index);
	// Is stream (track) a audio track
	bool		IsAudio(uint32 stream_index);

	uint32	getNoFramesInChunk(uint32 stream_index, uint32 pChunkID);
	uint32	getFirstFrameInChunk(uint32 stream_index, uint32 pChunkID);

	uint64	getOffsetForFrame(uint32 stream_index, uint32 pFrameNo, uint32 *chunkFrameCount);
	uint32	getChunkSize(uint32 stream_index, uint32 pFrameNo);
	bool	IsKeyFrame(uint32 stream_index, uint32 pFrameNo);

	status_t	ParseFile();
	BPositionIO *Source() {return theStream;};

	bool	IsActive(uint32 stream_index);

	bool	GetNextChunkInfo(uint32 stream_index, uint32 pFrameNo, off_t *start, uint32 *size, bool *keyframe, uint32 *chunkFrameCount);
	
	// Return all Audio Meta Data
	const 	AudioMetaData 		*AudioFormat(uint32 stream_index);
	// Return all Video Meta Data
	const 	VideoMetaData		*VideoFormat(uint32 stream_index);
	
	// XXX these need work
	const 	mov_main_header		*MovMainHeader();
	const 	mov_stream_header	*StreamFormat(uint32 stream_index);
};

#endif
