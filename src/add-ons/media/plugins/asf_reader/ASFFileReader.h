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

class IndexEntry {
public:
	IndexEntry() {frameNo = 0;data=NULL;size=0;pts=0;keyFrame=false;};
	
	uint32			frameNo;		// frame_no or sample_no
	uint8 *			data;			// The data for this frame
	uint32			size;			// The size of the data available
	bigtime_t		pts;			// Presentation Time Stamp for this frame
	bool			keyFrame;		// Is this a keyframe.
};

class StreamHeader {
public:
	StreamHeader() {streamIndex = 0;};
	~StreamHeader() {index.clear();};
	
	void AddIndex(uint32 frameNo, bool keyFrame, bigtime_t pts, uint8 *data, uint32 size)
	{
		IndexEntry indexEntry;

		// Should be in a constructor	
		indexEntry.frameNo = frameNo;
		indexEntry.data = data;
		indexEntry.size = size;
		indexEntry.pts = pts;
		indexEntry.keyFrame = keyFrame;

		index.push_back(indexEntry);
	};
	
	IndexEntry GetIndex(uint32 frameNo)
	{
		IndexEntry indexEntry;
		
		for (std::vector<IndexEntry>::iterator itr = index.begin();
    		itr != index.end();
    		++itr) {
  				if (itr->frameNo == frameNo) {
  					indexEntry = *itr;
  					index.erase(itr);
  					break;
				}
		}
		return indexEntry;
	};
	
	bool HasIndex(uint32 frameNo)
	{
		if (!index.empty()) {
			for (std::vector<IndexEntry>::iterator itr = index.begin();
     			itr != index.end();
     			++itr) {
  					if (itr->frameNo == frameNo) {
  						return true;
					}
			}
		}
		
		return false;
	};
	
	uint16	streamIndex;
	std::vector<IndexEntry> index;
};

// ASF file reader 
class ASFFileReader {
private:
	off_t	StreamSize;
	BPositionIO	*theStream;
	asf_file_t *asfFile;
	asf_packet_t *packet;
	std::vector<StreamHeader> streams;

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
		
	// The first video track duration indexed by streamIndex
	bigtime_t	getVideoDuration(uint32 streamIndex);
	// the first audio track duration indexed by streamIndex
	bigtime_t	getAudioDuration(uint32 streamIndex);
	// the max of all active audio or video durations
	bigtime_t	getMaxDuration();

	bool getAudioFormat(uint32 streamIndex, ASFAudioFormat *format);
	bool getVideoFormat(uint32 streamIndex, ASFVideoFormat *format);
	
	// The no of frames in the video track indexed by streamIndex
	uint32		getFrameCount(uint32 streamIndex);
	// The no of chunks in the audio track indexed by streamIndex
	uint32		getAudioChunkCount(uint32 streamIndex);
	// Is stream (track) a video track
	bool		IsVideo(uint32 streamIndex);
	// Is stream (track) a audio track
	bool		IsAudio(uint32 streamIndex);

	BPositionIO *Source() {return theStream;};

	void AddIndex(uint32 streamIndex, uint32 frameNo, bool keyFrame, bigtime_t pts, uint8 *data, uint32 size);
	IndexEntry GetIndex(uint32 streamIndex, uint32 frameNo);
	bool HasIndex(uint32 streamIndex, uint32 frameNo);

	bool	GetNextChunkInfo(uint32 streamIndex, uint32 pFrameNo, char **buffer, uint32 *size, bool *keyframe, bigtime_t *pts);
	
	static	int32_t read(void *opaque, void *buffer, int32_t size);
	static	int32_t write(void *opaque, void *buffer, int32_t size);
	static	int64_t seek(void *opaque, int64_t offset);

};


#endif
