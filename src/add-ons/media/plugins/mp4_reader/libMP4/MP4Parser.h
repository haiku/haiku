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
typedef std::vector<SampleToChunkPtr> SampleToChunkArray;
typedef ChunkToOffset* ChunkToOffsetPtr;
typedef std::map<uint32, ChunkToOffsetPtr, std::less<uint32> > ChunkToOffsetArray;
typedef SyncSample* SyncSamplePtr;
typedef std::vector<SyncSamplePtr> SyncSampleArray;
typedef SampleSizeEntry* SampleSizePtr;
typedef std::vector<SampleSizePtr> SampleSizeArray;


// Atom class for reading the movie header atom
class MVHDAtom : public FullAtom {
public:
						MVHDAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~MVHDAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
			uint32		GetTimeScale() {return theHeader.TimeScale;};
			uint32		GetDuration() {return theHeader.Duration;};

private:
			mvhdV1		theHeader;
};


// Atom class for reading the moov atom
class MOOVAtom : public AtomContainer {
public:
						MOOVAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~MOOVAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
	
			MVHDAtom*	GetMVHDAtom();

private:
			MVHDAtom*	theMVHDAtom;
};


// Atom class for reading the cmov atom
class CMOVAtom : public AtomContainer {
public:
						CMOVAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~CMOVAtom();

			BPositionIO	*OnGetStream();
			void		OnProcessMetaData();
			void		OnChildProcessingComplete();

			const char	*OnGetAtomName();

private:
			BMallocIO*	theUncompressedStream;
};


// Atom class for reading the dcom atom
class DCOMAtom : public AtomBase {
public:
						DCOMAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~DCOMAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
			uint32		GetCompressionID() {return compressionID;};

private:
			uint32		compressionID;
};


// Atom class for reading the cmvd atom
class CMVDAtom : public AtomBase {
public:
						CMVDAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~CMVDAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
			uint32		GetUncompressedSize() {return UncompressedSize;};
			uint8*		GetCompressedData() {return Buffer;};
			uint32		GetBufferSize() {return BufferSize;};

private:
			uint32		UncompressedSize;
			uint32		BufferSize;
			uint8*		Buffer;
};


// Atom class for reading the ftyp atom
class FTYPAtom : public AtomBase {
public:
						FTYPAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~FTYPAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
	
			bool		HasBrand(uint32 brand);

private:
			uint32		major_brand;
			uint32		minor_version;
			uint32		compatable_brands[32];
							// Should be infinite but we will settle for max 32
			uint32		total_brands;
};


// Atom class for reading the wide atom
class WIDEAtom : public AtomBase {
public:
						WIDEAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~WIDEAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
};


// Atom class for reading the free atom
class FREEAtom : public AtomBase {
public:
						FREEAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~FREEAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
};


// Atom class for reading the preview atom
class PNOTAtom : public AtomBase {
public:
						PNOTAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~PNOTAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
	
};


// Atom class for reading the time to sample atom
class STTSAtom : public FullAtom {
public:
						STTSAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~STTSAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
	
			uint64		GetSUMCounts() { return SUMCounts; };
			bigtime_t	GetSUMDurations() { return SUMDurations; };
			uint32		GetSampleForTime(bigtime_t pTime);
			uint32		GetSampleForFrame(uint32 pFrame);
			void		SetFrameRate(float pFrameRate)
							{ FrameRate = pFrameRate; };

private:
			array_header 		theHeader;
			TimeToSampleArray	theTimeToSampleArray;
			bigtime_t			SUMDurations;
			uint64				SUMCounts;
			float				FrameRate;
};


// Atom class for reading the composition time to sample atom
class CTTSAtom : public FullAtom {
public:
						CTTSAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~CTTSAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();

private:
			array_header 			theHeader;
			CompTimeToSampleArray	theCompTimeToSampleArray;
};


// Atom class for reading the sample to chunk atom
class STSCAtom : public FullAtom {
public:
						STSCAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~STSCAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
	
			uint32		GetChunkForSample(uint32 pSample,
							uint32 *pOffsetInChunk);
			uint32		GetFirstSampleInChunk(uint32 pChunkIndex);
			uint32		GetNoSamplesInChunk(uint32 pChunkIndex);

private:
	array_header		theHeader;
	SampleToChunkArray	theSampleToChunkArray;
};


// Atom class for reading the chunk to offset atom
class STCOAtom : public FullAtom {
public:
						STCOAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~STCOAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
	
			uint64		GetOffsetForChunk(uint32 pChunkIndex);
			uint32		GetTotalChunks() {return theHeader.NoEntries;};

protected:
	// Read a single chunk offset from Stream
	virtual	uint64		OnGetChunkOffset();

private:
			array_header		theHeader;
			ChunkToOffsetArray	theChunkToOffsetArray;
};


// Atom class for reading the sync sample atom
class STSSAtom : public FullAtom {
public:
						STSSAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~STSSAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
	
			bool		IsSyncSample(uint32 pSampleNo);

private:
			array_header	theHeader;
			SyncSampleArray	theSyncSampleArray;
};


// Atom class for reading the sample size atom
class STSZAtom : public FullAtom {
public:
						STSZAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~STSZAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
	
			uint32		GetSizeForSample(uint32 pSampleNo);
			bool		IsSingleSampleSize();	

private:
			uint32		SampleSize;
			uint32		SampleCount;

			SampleSizeArray		theSampleSizeArray;
};


// Atom class for reading the sample size 2 atom
class STZ2Atom : public FullAtom {
public:
						STZ2Atom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~STZ2Atom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();

			uint32		GetSizeForSample(uint32 pSampleNo);
			bool		IsSingleSampleSize();	
private:
			uint32		SampleSize;
			uint32		SampleCount;
			uint8		FieldSize;
	
			SampleSizeArray		theSampleSizeArray;
};


// Atom class for reading the skip atom
class SKIPAtom : public AtomBase {
public:
						SKIPAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~SKIPAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
};


// Atom class for reading the media data atom
class MDATAtom : public AtomBase {
public:
						MDATAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~MDATAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
			off_t		GetEOF();
};


// Subclass for handling special decoder config atoms like ESDS, ALAC, AVCC, AMR
// ESDS is actually a FullAtom but others are not :-(
class DecoderConfigAtom : public AtomBase {
public:
						DecoderConfigAtom(BPositionIO *pStream,
							off_t pStreamOffset, uint32 pAtomType,
							uint64 pAtomSize);
	virtual				~DecoderConfigAtom();
	virtual	void		OnProcessMetaData();
	virtual	const char	*OnGetAtomName();

	// The decoder config is what the decoder uses for it's setup
	// The values SHOULD mirror the Container values but often don't
	// So we have to parse the config and use the values as container overrides
			void		OverrideAudioDescription(
							AudioDescription *pAudioDescription);
	virtual void		OnOverrideAudioDescription(
							AudioDescription *pAudioDescription) {};
			void		OverrideVideoDescription(
							VideoDescription *pVideoDescription);
	virtual void		OnOverrideVideoDescription(
							VideoDescription *pVideoDescription) {};
			bool		SkipTag(uint8 *ESDS, uint8 Tag, uint32 *offset);
	
			uint8		*GetDecoderConfig();
			size_t		GetDecoderConfigSize() {return DecoderConfigSize;};

private:
			uint8		*theDecoderConfig;
			size_t		DecoderConfigSize;
};


// Atom class for reading the ESDS decoder config atom
class ESDSAtom : public DecoderConfigAtom {
public:
						ESDSAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~ESDSAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
			void		OnOverrideAudioDescription(
							AudioDescription *pAudioDescription);
			void		OnOverrideVideoDescription(
							VideoDescription *pVideoDescription);

private:
			uint8		ESDSType;
			uint8		StreamType;
			uint32		NeededBufferSize;
			uint32		MaxBitRate;
			uint32		AvgBitRate;
			AACHeader	theAACHeader;
};


// Atom class for reading the ALAC decoder config atom
class ALACAtom : public DecoderConfigAtom {
public:
						ALACAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~ALACAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
			void		OnOverrideAudioDescription(
							AudioDescription *pAudioDescription);
			void		OnOverrideVideoDescription(
							VideoDescription *pVideoDescription);
};


// Atom class for reading the WAVE atom for mp3 decoder config
class WAVEAtom : public DecoderConfigAtom {
public:
						WAVEAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~WAVEAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
			void		OnOverrideAudioDescription(
							AudioDescription *pAudioDescription);
			void		OnOverrideVideoDescription(
							VideoDescription *pVideoDescription);
};


// Atom class for reading the Sample Description atom
class STSDAtom : public FullAtom {
public:
						STSDAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~STSDAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
	
			VideoDescription GetAsVideo();
			AudioDescription GetAsAudio();

private:
			uint32		GetMediaHandlerType();

			void		ReadDecoderConfig(uint8 **pDecoderConfig,
							size_t *pDecoderConfigSize,
							AudioDescription *pAudioDescription,
							VideoDescription *pVideoDescription);
			void		ReadSoundDescription();
			void		ReadVideoDescription();
			void		ReadHintDescription();
	
			uint32		codecid;
	
			array_header		theHeader;
			SampleEntry			theSampleEntry;
			AudioDescription	theAudioDescription;
			VideoDescription	theVideoDescription;
};


// Atom class for reading the track header atom
class TKHDAtom : public FullAtom {
public:
						TKHDAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~TKHDAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();

			uint32		GetDuration() {return theHeader.Duration;};
							// duration is in the scale of the media
	
			// Flags3 should contain the following
			// 0x001 Track enabled
			// 0x002 Track in Movie
			// 0x004 Track in Preview
			// 0x008 Track in Poster
	
			bool		IsActive() {return ((GetFlags3() && 0x01) 
							|| (GetFlags3() && 0x0f));};

private:
			tkhdV1		theHeader;
	
};


// Atom class for reading the media header atom
class MDHDAtom : public FullAtom {
public:
						MDHDAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~MDHDAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();

			// return duration in ms
			bigtime_t	GetDuration();
			uint32		GetTimeScale();

private:
			mdhdV1		theHeader;
	
};


// Atom class for reading the video media header atom
class VMHDAtom : public FullAtom {
public:
						VMHDAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~VMHDAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
	
			bool		IsCopyMode() {return theHeader.GraphicsMode == 0;};
private:
			vmhd		theHeader ;
};


// Atom class for reading the sound media header atom
class SMHDAtom : public FullAtom {
public:
						SMHDAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~SMHDAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();

private:
			smhd		theHeader;	
};


// Atom class for reading the track atom
class TRAKAtom : public AtomContainer {
public:
						TRAKAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~TRAKAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
			void	OnChildProcessingComplete();

			bigtime_t	Duration(uint32 TimeScale);	// Return duration of track
			bigtime_t	Duration() { return Duration(timeScale); }
			uint32		FrameCount() { return frameCount; };
			bool		IsVideo();	// Is this a video track
			bool		IsAudio();	// Is this a audio track
	// GetAudioMetaData() // If this is a audio track Get the audio meta data
	// GetVideoMetaData() // If this is a video track Get the video meta data	
	
			bigtime_t	GetTimeForFrame(uint32 pFrame);
			uint32		GetFrameForTime(bigtime_t time);
			uint32		GetSampleForTime(bigtime_t pTime);
			uint32		GetSampleForFrame(uint32 pFrame);
			uint32		GetChunkForSample(uint32 pSample,
							uint32 *pOffsetInChunk);
			uint64		GetOffsetForChunk(uint32 pChunkIndex);
			uint32		GetFirstSampleInChunk(uint32 pChunkIndex);
			uint32		GetSizeForSample(uint32 pSample);
			uint32		GetNoSamplesInChunk(uint32 pChunkIndex);
			uint32		GetTotalChunks();
			uint32		GetChunkSize(uint32 pChunkIndex);
			uint32		ChunkCount();
	
			float		GetSampleRate();
			float		GetFrameRate();
	
			bool		IsSyncSample(uint32 pSampleNo);
			bool		IsSingleSampleSize();
			bool		IsActive();
	
			uint32		GetBytesPerSample();

			TKHDAtom	*GetTKHDAtom();
			MDHDAtom	*GetMDHDAtom();
private:
			TKHDAtom	*theTKHDAtom;
			MDHDAtom	*theMDHDAtom;

			uint32		frameCount;
			uint32		bytesPerSample;
			uint32		timeScale;
};


// Atom class for reading the media container atom
class MDIAAtom : public AtomContainer {
public:
						MDIAAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~MDIAAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();

			uint32		GetMediaHandlerType();
};


// Atom class for reading the media information atom
class MINFAtom : public AtomContainer {
public:
						MINFAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~MINFAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
	
			uint32		GetMediaHandlerType();
};


// Atom class for reading the stbl atom
class STBLAtom : public AtomContainer {
public:
						STBLAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~STBLAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
	
			uint32		GetMediaHandlerType();
};


// Atom class for reading the dinf atom
class DINFAtom : public AtomContainer {
public:
						DINFAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~DINFAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
};


// Atom class for reading the tmcd atom
class TMCDAtom : public AtomBase {
public:
						TMCDAtom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~TMCDAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
};


// Atom class for reading the dac3 atom
class DAC3Atom : public DecoderConfigAtom {
public:
						DAC3Atom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~DAC3Atom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
			void		OnOverrideAudioDescription(
							AudioDescription *pAudioDescription);
			void		OnOverrideVideoDescription(
							VideoDescription *pVideoDescription);
};


// Atom class for reading the dec3 atom
class DEC3Atom : public DecoderConfigAtom {
public:
						DEC3Atom(BPositionIO *pStream, off_t pStreamOffset,
							uint32 pAtomType, uint64 pAtomSize);
	virtual				~DEC3Atom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();
			void		OnOverrideAudioDescription(
							AudioDescription *pAudioDescription);
			void		OnOverrideVideoDescription(
							VideoDescription *pVideoDescription);
};


// Atom class for reading the Media Handler atom
class HDLRAtom : public FullAtom {
public:
						HDLRAtom(BPositionIO *pStream, off_t pStreamOffset, uint32 pAtomType, uint64 pAtomSize);
	virtual				~HDLRAtom();
			void		OnProcessMetaData();
			const char	*OnGetAtomName();

			bool		IsVideoHandler();
			bool		IsAudioHandler();
			uint32		GetMediaHandlerType();

private:
			hdlr		theHeader;
			char		*name;
};


#endif
