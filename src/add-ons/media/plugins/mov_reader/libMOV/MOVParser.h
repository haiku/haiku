#ifndef _MOV_PARSER_H
#define _MOV_PARSER_H

#include <File.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <SupportDefs.h>

// Std Headers
#include <map.h>

#include "MOVAtom.h"

typedef SoundDescriptionV1* SoundDescPtr;
typedef std::map<uint32, SoundDescPtr, std::less<uint32> > SoundDescArray;
typedef VideoDescriptionV0* VideoDescPtr;
typedef std::map<uint32, VideoDescPtr, std::less<uint32> > VideoDescArray;
typedef TimeToSample* TimeToSamplePtr;
typedef std::map<uint32, TimeToSamplePtr, std::less<uint32> > TimeToSampleArray;
typedef SampleToChunk* SampleToChunkPtr;
typedef std::map<uint32, SampleToChunkPtr, std::less<uint32> > SampleToChunkArray;
typedef ChunkToOffset* ChunkToOffsetPtr;
typedef std::map<uint32, ChunkToOffsetPtr, std::less<uint32> > ChunkToOffsetArray;
typedef SyncSample* SyncSamplePtr;
typedef std::map<uint32, SyncSamplePtr, std::less<uint32> > SyncSampleArray;
typedef SampleSize* SampleSizePtr;
typedef std::map<uint32, SampleSizePtr, std::less<uint32> > SampleSizeArray;

// Atom class for reading the movie header atom
class MVHDAtom : public AtomBase {
public:
			MVHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~MVHDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	uint32	getTimeScale() {return theHeader.TimeScale;};
	uint32	getDuration() {return theHeader.Duration;};
private:
	mvhd	theHeader;

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

// Atom class for reading the ftyp atom
class FTYPAtom : public AtomBase {
public:
			FTYPAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~FTYPAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
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
class STTSAtom : public AtomBase {
public:
			STTSAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STTSAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint64	getSUMCounts();
	uint64	getSUMDurations();
	uint32	getSampleForTime(uint32 pTime);
	uint32	getSampleForFrame(uint32 pFrame);
private:
	array_header		theHeader;
	TimeToSampleArray	theTimeToSampleArray;
};

// Atom class for reading the sample to chunk atom
class STSCAtom : public AtomBase {
public:
			STSCAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STSCAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint32	getChunkForSample(uint32 pSample, uint32 *pOffsetInChunk);
private:
	array_header		theHeader;
	SampleToChunkArray	theSampleToChunkArray;
};

// Atom class for reading the chunk to offset atom
class STCOAtom : public AtomBase {
public:
			STCOAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STCOAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint64	getOffsetForChunk(uint32 pChunkID);

protected:
	// Read a single chunk offset from Stream
	virtual uint64	OnGetChunkOffset();

private:
	array_header		theHeader;
	ChunkToOffsetArray	theChunkToOffsetArray;
};

// Atom class for reading the sync sample atom
class STSSAtom : public AtomBase {
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
class STSZAtom : public AtomBase {
public:
			STSZAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STSZAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint32	getSizeForSample(uint32 pSampleNo);
	
private:
	SampleSizeHeader	theHeader;
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
	
};

// Atom class for reading the sdst atom
class STSDAtom : public AtomBase {
public:
			STSDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STSDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	uint32	DataFormat;

	VideoDescriptionV0 getAsVideo();
	SoundDescriptionV1 getAsAudio();
	
private:
	bool	IsAudioSampleDesc(uint32 pDataFormat);
	bool	IsVideoSampleDesc(uint32 pDataFormat);

	array_header	theHeader;
	SoundDescArray	theAudioDescArray;
	VideoDescArray	theVideoDescArray;
};

// Atom class for reading the track header atom
class TKHDAtom : public AtomBase {
public:
			TKHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~TKHDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();

	uint32	getDuration() {return theHeader.Duration;};		// duration is in the scale of the media

private:
	tkhd	theHeader;
	
};

// Atom class for reading the media header atom
class MDHDAtom : public AtomBase {
public:
			MDHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~MDHDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();

	// return duration in ms
	bigtime_t	getDuration();
	uint32		getTimeScale();
	
private:
	mdhd	theHeader;
	
};

// Atom class for reading the video media header atom
class VMHDAtom : public AtomBase {
public:
			VMHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~VMHDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
};

// Atom class for reading the sound media header atom
class SMHDAtom : public AtomBase {
public:
			SMHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~SMHDAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
};

// Atom class for reading the track atom
class TRAKAtom : public AtomContainer {
public:
			TRAKAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~TRAKAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
	bigtime_t	Duration(uint32 TimeScale);	// Return duration of track
	uint32		FrameCount();
	bool		IsVideo();	// Is this a video track
	bool		IsAudio();	// Is this a audio track
	// GetAudioMetaData()	// If this is a audio track get the audio meta data
	// GetVideoMetaData()	// If this is a video track get the video meta data	
	
	uint32	getTimeForFrame(uint32 pFrame, uint32 pTimeScale);
	uint32	getSampleForTime(uint32 pTime);
	uint32	getSampleForFrame(uint32 pFrame);
	uint32	getChunkForSample(uint32 pSample, uint32 *pOffsetInChunk);
	uint64	getOffsetForChunk(uint32 pChunk);
	uint32	getSizeForSample(uint32 pSample);
	bool	IsSyncSample(uint32 pSampleNo);
		
	TKHDAtom	*getTKHDAtom();
	MDHDAtom	*getMDHDAtom();
private:
	TKHDAtom	*theTKHDAtom;
	MDHDAtom	*theMDHDAtom;
};

// Atom class for reading the media container atom
class MDIAAtom : public AtomContainer {
public:
			MDIAAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~MDIAAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
};

// Atom class for reading the media information atom
class MINFAtom : public AtomContainer {
public:
			MINFAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~MINFAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
};

// Atom class for reading the stbl atom
class STBLAtom : public AtomContainer {
public:
			STBLAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~STBLAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
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

// Atom class for reading the wave atom
class WAVEAtom : public AtomBase {
public:
			WAVEAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~WAVEAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	wave_format_ex getWaveFormat() {return theWaveFormat;};
private:
	wave_format_ex theWaveFormat;	
};

// Atom class for reading the hdlr atom
class HDLRAtom : public AtomBase {
public:
			HDLRAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize);
	virtual	~HDLRAtom();
	void	OnProcessMetaData();
	char	*OnGetAtomName();
	
};

#endif
