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
#ifndef _MP4_PARSER_H
#define _MP4_PARSER_H

#include <File.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <SupportDefs.h>

// Std Headers
#include <map>

#include "MP4Atom.h"

typedef CompTimeToSample* CompTimeToSamplePtr;
typedef std::map<uint32, CompTimeToSamplePtr, std::less<uint32> > CompTimeToSampleArray;
typedef TimeToSample* TimeToSamplePtr;
typedef std::map<uint32, TimeToSamplePtr, std::less<uint32> > TimeToSampleArray;
typedef SampleToChunk* SampleToChunkPtr;
typedef std::map<uint32, SampleToChunkPtr, std::less<uint32> > SampleToChunkArray;
typedef ChunkToOffset* ChunkToOffsetPtr;
typedef std::map<uint32, ChunkToOffsetPtr, std::less<uint32> > ChunkToOffsetArray;
typedef SyncSample* SyncSamplePtr;
typedef std::map<uint32, SyncSamplePtr, std::less<uint32> > SyncSampleArray;
typedef SampleSizeEntry* SampleSizePtr;
typedef std::map<uint32, SampleSizePtr, std::less<uint32> > SampleSizeArray;

// Atom class for reading the movie header atom
class MVHDAtom : public FullAtom {
public:
			MVHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~MVHDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	uint32	getTimeScale() {return theHeader.TimeScale;};
	uint32	getDuration() {return theHeader.Duration;};
private:
	mvhdV1	theHeader;
};

// Atom class for reading the moov atom
class MOOVAtom : public AtomContainer {
public:
			MOOVAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~MOOVAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	MVHDAtom	*getMVHDAtom();
private:
	MVHDAtom	*theMVHDAtom;
};

// Atom class for reading the cmov atom
class CMOVAtom : public AtomContainer {
public:
			CMOVAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~CMOVAtom();

	BPositionIO *OnGetStream();
	void	OnProcessMetaData();
	void	OnChildProcessingComplete();

	char	*OnGetAtomName();
private:
	BMallocIO	*theUncompressedStream;
};

// Atom class for reading the dcom atom
class DCOMAtom : public AtomBase {
public:
			DCOMAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~DCOMAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	uint32	getCompressionID() {return compressionID;};
private:
	uint32	compressionID;
};

// Atom class for reading the cmvd atom
class CMVDAtom : public AtomBase {
public:
			CMVDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~CMVDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	uint32	getUncompressedSize() {return UncompressedSize;};
	uint8	*getCompressedData() {return Buffer;};
	uint32	getBufferSize() {return BufferSize;};
private:
	uint32	UncompressedSize;
	uint32	BufferSize;
	uint8	*Buffer;
};

// Atom class for reading the ftyp atom
class FTYPAtom : public AtomBase {
public:
			FTYPAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~FTYPAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	bool	HasBrand(uint32 brand);
private:
	uint32	major_brand;
	uint32	minor_version;
	uint32	compatable_brands[32];	// Should be infinite but we will settle for max 32
	uint32	total_brands;
};

// Atom class for reading the wide atom
class WIDEAtom : public AtomBase {
public:
			WIDEAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~WIDEAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
};

// Atom class for reading the free atom
class FREEAtom : public AtomBase {
public:
			FREEAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~FREEAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
};

// Atom class for reading the preview atom
class PNOTAtom : public AtomBase {
public:
			PNOTAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~PNOTAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
};

// Atom class for reading the time to sample atom
class STTSAtom : public FullAtom {
public:
			STTSAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STTSAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint64	getSUMCounts() {return SUMCounts;};
	uint64	getSUMDurations() {return SUMDurations;};
	uint32	getSampleForTime(uint32 pTime);
	uint32	getSampleForFrame(uint32 pFrame);
private:
	array_header		theHeader;
	TimeToSampleArray	theTimeToSampleArray;
	uint64	SUMDurations;
	uint64	SUMCounts;
};

// Atom class for reading the composition time to sample atom
class CTTSAtom : public FullAtom {
public:
			CTTSAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~CTTSAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
private:
	array_header			theHeader;
	CompTimeToSampleArray	theCompTimeToSampleArray;
};

// Atom class for reading the sample to chunk atom
class STSCAtom : public FullAtom {
public:
			STSCAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STSCAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint32	getChunkForSample(uint32 pSample, uint32 *pOffsetInChunk);
	uint32	getFirstSampleInChunk(uint32 pChunkID);
	uint32	getNoSamplesInChunk(uint32 pChunkID);
private:
	array_header		theHeader;
	SampleToChunkArray	theSampleToChunkArray;
};

// Atom class for reading the chunk to offset atom
class STCOAtom : public FullAtom {
public:
			STCOAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STCOAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint64	getOffsetForChunk(uint32 pChunkID);
	uint32	getTotalChunks() {return theHeader.NoEntries;};

protected:
	// Read a single chunk offset from Stream
	virtual uint64	OnGetChunkOffset();

private:
	array_header		theHeader;
	ChunkToOffsetArray	theChunkToOffsetArray;
};

// Atom class for reading the sync sample atom
class STSSAtom : public FullAtom {
public:
			STSSAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STSSAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	bool	IsSyncSample(uint32 pSampleNo);
private:
	array_header	theHeader;
	SyncSampleArray	theSyncSampleArray;
};

// Atom class for reading the sample size atom
class STSZAtom : public FullAtom {
public:
			STSZAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STSZAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint32	getSizeForSample(uint32 pSampleNo);
	bool	IsSingleSampleSize();	
private:
	uint32	SampleSize;
	uint32	SampleCount;
	
	SampleSizeArray		theSampleSizeArray;
};

// Atom class for reading the sample size 2 atom
class STZ2Atom : public FullAtom {
public:
			STZ2Atom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STZ2Atom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint32	getSizeForSample(uint32 pSampleNo);
	bool	IsSingleSampleSize();	
private:
	uint32	SampleSize;
	uint32	SampleCount;
	uint8	FieldSize;
	
	SampleSizeArray		theSampleSizeArray;
};

// Atom class for reading the skip atom
class SKIPAtom : public AtomBase {
public:
			SKIPAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~SKIPAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
};

// Atom class for reading the media data atom
class MDATAtom : public AtomBase {
public:
			MDATAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~MDATAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	off_t	getEOF();
};

class ESDSAtom : public AtomBase {
public:
			ESDSAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~ESDSAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint8	*getVOL();
private:
	uint8	*theVOL;
};

// Atom class for reading the sdst atom
class STSDAtom : public FullAtom {
public:
			STSDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STSDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	VideoDescription getAsVideo();
	AudioDescription getAsAudio();
	
private:
	uint32	getMediaHandlerType();

	void	ReadESDS(uint8 **VOL, size_t *VOLSize);
	void	ReadSoundDescription();
	void	ReadVideoDescription();

	
	uint32	codecid;
	
	array_header		theHeader;
	SampleEntry			theSampleEntry;
	AudioDescription	theAudioDescription;
	VideoDescription	theVideoDescription;
};

// Atom class for reading the track header atom
class TKHDAtom : public FullAtom {
public:
			TKHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~TKHDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();

	uint32	getDuration() {return theHeader.Duration;};		// duration is in the scale of the media
	
	// Flags3 should contain the following
	// 0x001 Track enabled
	// 0x002 Track in Movie
	// 0x004 Track in Preview
	// 0x008 Track in Poster
	
	bool	IsActive() {return ((getFlags3() && 0x01) || (getFlags3() && 0x0f));};
	
private:
	tkhdV1	theHeader;
	
};

// Atom class for reading the media header atom
class MDHDAtom : public FullAtom {
public:
			MDHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~MDHDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();

	// return duration in ms
	bigtime_t	getDuration();
	uint32		getTimeScale();
	
private:
	mdhdV1	theHeader;
	
};

// Atom class for reading the video media header atom
class VMHDAtom : public FullAtom {
public:
			VMHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~VMHDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	bool	IsCopyMode() {return theHeader.GraphicsMode == 0;};
private:
	vmhd	theHeader ;
};

// Atom class for reading the sound media header atom
class SMHDAtom : public FullAtom {
public:
			SMHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~SMHDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
private:
	smhd	theHeader;	
};

// Atom class for reading the track atom
class TRAKAtom : public AtomContainer {
public:
			TRAKAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~TRAKAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	void	OnChildProcessingComplete();

	bigtime_t	Duration(uint32 TimeScale);	// Return duration of track
	uint32		FrameCount() {return framecount;};
	bool		IsVideo();	// Is this a video track
	bool		IsAudio();	// Is this a audio track
	// GetAudioMetaData()	// If this is a audio track get the audio meta data
	// GetVideoMetaData()	// If this is a video track get the video meta data	
	
	uint32	getTimeForFrame(uint32 pFrame, uint32 pTimeScale);
	uint32	getSampleForTime(uint32 pTime);
	uint32	getSampleForFrame(uint32 pFrame);
	uint32	getChunkForSample(uint32 pSample, uint32 *pOffsetInChunk);
	uint64	getOffsetForChunk(uint32 pChunkID);
	uint32	getFirstSampleInChunk(uint32 pChunkID);
	uint32	getSizeForSample(uint32 pSample);
	uint32	getNoSamplesInChunk(uint32 pChunkID);
	uint32	getTotalChunks();
	
	bool	IsSyncSample(uint32 pSampleNo);
	bool	IsSingleSampleSize();
	bool	IsActive();
	
	uint32	getBytesPerSample();

	TKHDAtom	*getTKHDAtom();
	MDHDAtom	*getMDHDAtom();
private:
	TKHDAtom	*theTKHDAtom;
	MDHDAtom	*theMDHDAtom;
	
	uint32		framecount;
	uint32		bytespersample;
};

// Atom class for reading the media container atom
class MDIAAtom : public AtomContainer {
public:
			MDIAAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~MDIAAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();

	uint32	getMediaHandlerType();
};

// Atom class for reading the media information atom
class MINFAtom : public AtomContainer {
public:
			MINFAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~MINFAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint32	getMediaHandlerType();
};

// Atom class for reading the stbl atom
class STBLAtom : public AtomContainer {
public:
			STBLAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STBLAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint32	getMediaHandlerType();
};

// Atom class for reading the dinf atom
class DINFAtom : public AtomContainer {
public:
			DINFAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~DINFAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
};

// Atom class for reading the tmcd atom
class TMCDAtom : public AtomBase {
public:
			TMCDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~TMCDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
};

// Atom class for reading the Media Handler atom
class HDLRAtom : public FullAtom {
public:
			HDLRAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~HDLRAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();

	bool	IsVideoHandler();
	bool	IsAudioHandler();
	uint32	getMediaHandlerType();

private:
	hdlr	theHeader;
	char	*name;
};

#endif
