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
#ifndef _MP4_FILE_READER_H
#define _MP4_FILE_READER_H

#include <File.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <SupportDefs.h>

#include "MP4Parser.h"

// MP4 file reader 
class MP4FileReader {
private:
	// Atom List
	AtomArray atomChildren;
	uint32	TotalChildren;
	off_t	StreamSize;
	BPositionIO	*theStream;

	AtomBase *GetChildAtom(uint32 patomType, uint32 offset=0);
	uint32	CountChildAtoms(uint32 patomType);
	
	TRAKAtom *GetTrack(uint32 streamIndex);
	
	uint32		GetMovieTimeScale();

	// Add a atom to the children array (return false on failure)
	bool	AddChild(AtomBase *pChildAtom);

	AudioMetaData	theAudio;
	VideoMetaData	theVideo;
	mp4_stream_header theStreamHeader;
	mp4_main_header theMainHeader;	
	
	MVHDAtom	*theMVHDAtom;
	std::map<uint32, TRAKAtom*, std::less<uint32> > tracks;
	
public:
			MP4FileReader(BPositionIO *pStream);
	virtual	~MP4FileReader();

	// getter for MVHDAtom
	MVHDAtom	*GetMVHDAtom();

	bool	IsEndOfFile(off_t pPosition);
	bool	IsEndOfFile();
	bool	IsEndOfData(off_t pPosition);
	
	// Is this a quicktime file
	static bool	IsSupported(BPositionIO *source);
	
	// How many tracks in file
	uint32		GetStreamCount();
	
	// Duration defined by the movie header
	bigtime_t	GetMovieDuration();
	
	// The first video track duration indexed by streamIndex
	bigtime_t	GetVideoDuration(uint32 streamIndex);
	// the first audio track duration indexed by streamIndex
	bigtime_t	GetAudioDuration(uint32 streamIndex);
	// the max of all active audio or video durations
	bigtime_t	GetMaxDuration();

	// The no of frames in the video track indexed by streamIndex
	uint32		GetFrameCount(uint32 streamIndex);
	// The no of chunks in the audio track indexed by streamIndex
	uint32		GetAudioChunkCount(uint32 streamIndex);
	// Is stream (track) a video track
	bool		IsVideo(uint32 streamIndex);
	// Is stream (track) a audio track
	bool		IsAudio(uint32 streamIndex);

	// Frame functions
	uint64	GetOffsetForFrame(uint32 streamIndex, uint32 pFrameNo);
	uint32	GetChunkForFrame(uint32 streamIndex, uint32 pFrameNo);
	uint32	GetFrameSize(uint32 streamIndex, uint32 pFrameNo);

	// Chunk functions
	uint64	GetOffsetForChunk(uint32 streamIndex, uint32 pChunkIndex);
	uint32	GetFirstFrameInChunk(uint32 streamIndex, uint32 pChunkIndex);
	uint32	GetChunkSize(uint32 streamIndex, uint32 pChunkIndex);
	uint32	GetNoFramesInChunk(uint32 streamIndex, uint32 pChunkIndex);
	
	bool	isValidChunkIndex(uint32 streamIndex, uint32 pChunkIndex);
	bool	isValidFrame(uint32 streamIndex, uint32 pFrameNo);
	bool	IsKeyFrame(uint32 streamIndex, uint32 pFrameNo);

	bigtime_t GetTimeForFrame(uint32 streamIndex, uint32 pFrameNo);
	uint32	GetFrameForTime(uint32 streamIndex, bigtime_t time);
	uint32	GetFrameForSample(uint32 streamIndex, uint32 sample);

	uint32	GetSampleForTime(uint32 streamIndex, bigtime_t time);
	uint32	GetTimeForSample(uint32 streamIndex, uint32 sample);
	
	status_t	ParseFile();
	BPositionIO *Source() {return theStream;};

	bool	IsActive(uint32 streamIndex);

	bool	GetBufferForChunk(uint32 streamIndex, uint32 pChunkIndex, off_t *start, uint32 *size, bool *keyframe, bigtime_t *time, uint32 *framesInBuffer);
	bool	GetBufferForFrame(uint32 streamIndex, uint32 pFrameNo, off_t *start, uint32 *size, bool *keyframe, bigtime_t *time, uint32 *framesInBuffer);
	
	// Return all Audio Meta Data
	const 	AudioMetaData 		*AudioFormat(uint32 streamIndex, size_t *size = 0);
	// Return all Video Meta Data
	const 	VideoMetaData		*VideoFormat(uint32 streamIndex);
	
	// XXX these need work
	const 	mp4_main_header		*MovMainHeader();
	const 	mp4_stream_header	*StreamFormat(uint32 streamIndex);
};

#endif
