#include "MOVParser.h"

TRAKAtom::TRAKAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomContainer(pStream, pstreamOffset, patomType, patomSize)
{
	theTKHDAtom = NULL;
	theMDHDAtom = NULL;
}

TRAKAtom::~TRAKAtom()
{
}

void TRAKAtom::OnProcessMetaData()
{
}

char *TRAKAtom::OnGetAtomName()
{
	return "Quicktime Track Atom";
}

TKHDAtom	*TRAKAtom::getTKHDAtom()
{
	if (theTKHDAtom) {
		return theTKHDAtom;
	}
	
	theTKHDAtom = dynamic_cast<TKHDAtom *>(GetChildAtom(uint32('tkhd'),0));
	
	// Assert(theTKHDAtom != NULL,"Track has no Track Header");
	
	return theTKHDAtom;
}

MDHDAtom	*TRAKAtom::getMDHDAtom()
{
	if (theMDHDAtom) {
		return theMDHDAtom;
	}
	
	theMDHDAtom = dynamic_cast<MDHDAtom *>(GetChildAtom(uint32('mdhd'),0));
	
	// Assert(theMDHDAtom != NULL,"Track has no Media Header");
	
	return theMDHDAtom;
}

// Return duration of track
bigtime_t	TRAKAtom::Duration(uint32 TimeScale)
{
	if (IsAudio() || IsVideo()) {
		return getMDHDAtom()->getDuration();
	}
	
	return bigtime_t(getTKHDAtom()->getDuration() * 1000000) / TimeScale;
}

// Is this a video track
bool TRAKAtom::IsVideo()
{
	// video tracks have a vmhd atom
	return (GetChildAtom(uint32('vmhd'),0) != NULL);
}

// Is this a audio track
bool TRAKAtom::IsAudio()
{
	// audio tracks have a smhd atom
	return (GetChildAtom(uint32('smhd'),0) != NULL);
}

uint32	TRAKAtom::getTimeForFrame(uint32 pFrame, uint32 pTimeScale)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stts'),0);
	
	if (aAtomBase) {
		STTSAtom *aSTTSAtom = dynamic_cast<STTSAtom *>(aAtomBase);

		if (IsAudio()) {
			// Frame * SampleRate = Time
		} else if (IsVideo()) {
			// Frame * fps = Time
			return pFrame * ((aSTTSAtom->getSUMCounts() * 1000000L) / Duration(pTimeScale));
		}
	}
	
	return 0;
}

uint32	TRAKAtom::getSampleForTime(uint32 pTime)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stts'),0);
	
	if (aAtomBase) {
		return (dynamic_cast<STTSAtom *>(aAtomBase))->getSampleForTime(pTime);
	}
	
	return 0;
}

uint32	TRAKAtom::getSampleForFrame(uint32 pFrame)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stts'),0);
	
	if (aAtomBase) {
		return (dynamic_cast<STTSAtom *>(aAtomBase))->getSampleForFrame(pFrame);
	}
	
	return 0;
}

uint32	TRAKAtom::getChunkForSample(uint32 pSample, uint32 *pOffsetInChunk)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stsc'),0);
	
	if (aAtomBase) {
		return (dynamic_cast<STSCAtom *>(aAtomBase))->getChunkForSample(pSample, pOffsetInChunk);
	}
	
	return 0;
}

uint32	TRAKAtom::getSizeForSample(uint32 pSample)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stsz'),0);
	
	if (aAtomBase) {
		return (dynamic_cast<STSZAtom *>(aAtomBase))->getSizeForSample(pSample);
	}
	
	return 0;
}

uint64	TRAKAtom::getOffsetForChunk(uint32 pChunk)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stco'),0);
	
	if (aAtomBase) {
		return (dynamic_cast<STCOAtom *>(aAtomBase))->getOffsetForChunk(pChunk);
	}
	
	return 0;
}

bool	TRAKAtom::IsSyncSample(uint32 pSampleNo)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stss'),0);
	
	if (aAtomBase) {
		return (dynamic_cast<STSSAtom *>(aAtomBase))->IsSyncSample(pSampleNo);
	}
	
	return false;
}

// GetAudioMetaData()	// If this is a audio track get the audio meta data
// GetVideoMetaData()	// If this is a video track get the video meta data	
