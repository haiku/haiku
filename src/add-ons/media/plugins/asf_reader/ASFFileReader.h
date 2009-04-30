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
#ifndef _ASF_FILE_READER_H
#define _ASF_FILE_READER_H

#include <File.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <SupportDefs.h>

#include <vector>

extern "C" {
	#include "libasf/asf.h"
}
#include "ASFIndex.h"

struct ASFAudioFormat {
	uint16 Compression;
	uint16 NoChannels;
	uint32 SamplesPerSec;
	uint32 AvgBytesPerSec;
	uint16 BlockAlign;
	uint16 BitsPerSample;
	uint32 extraDataSize;
	uint8 *extraData;
};

struct ASFVideoFormat {
	uint32 Compression;
	uint32 VideoWidth;
	uint32 VideoHeight;
	uint16 Planes;
	uint16 BitCount;
	uint32 FrameRate;
	uint32 FrameScale;
	uint32 FrameCount;
	uint32 extraDataSize;
	uint8 *extraData;
};

// ASF file reader 
class ASFFileReader {
private:
	off_t	StreamSize;
	BPositionIO	*theStream;
	asf_file_t *asfFile;
	asf_packet_t *packet;
	std::vector<StreamEntry> streams;

	void ParseIndex();

public:
			ASFFileReader(BPositionIO *pStream);
	virtual	~ASFFileReader();

	status_t	ParseFile();

	bool	IsEndOfFile(off_t pPosition);
	bool	IsEndOfFile();
	bool	IsEndOfData(off_t pPosition);
	
	// Is this a asf file
	static bool	IsSupported(BPositionIO *source);
	
	// How many tracks in file
	uint32		getStreamCount();
		
	// the Stream duration indexed by streamIndex
	bigtime_t	getStreamDuration(uint32 streamIndex);

	bool getAudioFormat(uint32 streamIndex, ASFAudioFormat *format);
	bool getVideoFormat(uint32 streamIndex, ASFVideoFormat *format);
	
	// The no of frames in the Stream indexed by streamIndex
	uint32		getFrameCount(uint32 streamIndex);

	// Is stream (track) a video track
	bool		IsVideo(uint32 streamIndex);
	// Is stream (track) a audio track
	bool		IsAudio(uint32 streamIndex);

	BPositionIO *Source() {return theStream;};

	IndexEntry GetIndex(uint32 streamIndex, uint32 frameNo);
	bool HasIndex(uint32 streamIndex, uint32 frameNo);
	
	uint32 GetFrameForTime(uint32 streamIndex, bigtime_t time);

	bool	GetNextChunkInfo(uint32 streamIndex, uint32 pFrameNo, char **buffer, uint32 *size, bool *keyframe, bigtime_t *pts);
	
	static	int32_t read(void *opaque, void *buffer, int32_t size);
	static	int32_t write(void *opaque, void *buffer, int32_t size);
	static	int64_t seek(void *opaque, int64_t offset);
};

#endif
