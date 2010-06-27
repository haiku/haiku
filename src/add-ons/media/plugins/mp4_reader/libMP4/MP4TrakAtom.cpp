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


#include "MP4Parser.h"


TRAKAtom::TRAKAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType,
	uint64 patomSize)
	:
	AtomContainer(pStream, pstreamOffset, patomType, patomSize)
{
	theTKHDAtom = NULL;
	theMDHDAtom = NULL;
	frameCount = 0;
	bytesPerSample = 0;
	timeScale = 1;
}


TRAKAtom::~TRAKAtom()
{
}


void
TRAKAtom::OnProcessMetaData()
{
}


const char *
TRAKAtom::OnGetAtomName()
{
	return "MPEG-4 Track Atom";
}


TKHDAtom *
TRAKAtom::GetTKHDAtom()
{
	if (theTKHDAtom) {
		return theTKHDAtom;
	}
	
	theTKHDAtom = dynamic_cast<TKHDAtom *>(GetChildAtom(uint32('tkhd'),0));
	
	// Assert(theTKHDAtom != NULL,"Track has no Track Header");
	
	return theTKHDAtom;
}


MDHDAtom *
TRAKAtom::GetMDHDAtom()
{
	if (theMDHDAtom)
		return theMDHDAtom;
	
	theMDHDAtom = dynamic_cast<MDHDAtom *>(GetChildAtom(uint32('mdhd'),0));
	
	// Assert(theMDHDAtom != NULL,"Track has no Media Header");
	
	return theMDHDAtom;
}


bigtime_t
TRAKAtom::Duration(uint32 TimeScale)
{
	if (IsAudio() || IsVideo())
		return GetMDHDAtom()->GetDuration();
	
	return bigtime_t(GetTKHDAtom()->GetDuration() * 1000000.0) / TimeScale;
}


void
TRAKAtom::OnChildProcessingComplete()
{
	timeScale = GetMDHDAtom()->GetTimeScale();

	STTSAtom *aSTTSAtom = dynamic_cast<STTSAtom *>
		(GetChildAtom(uint32('stts'),0));

	if (aSTTSAtom) {
		frameCount = aSTTSAtom->GetSUMCounts();
		aSTTSAtom->SetFrameRate(GetFrameRate());
	}

}


bool
TRAKAtom::IsVideo()
{
	// video tracks have a vmhd atom
	return (GetChildAtom(uint32('vmhd'),0) != NULL);
}


bool
TRAKAtom::IsAudio()
{
	// audio tracks have a smhd atom
	return (GetChildAtom(uint32('smhd'),0) != NULL);
}


// frames per us
float
TRAKAtom::GetFrameRate() 
{
	if (IsAudio()) {
		AtomBase *aAtomBase = GetChildAtom(uint32('stsd'),0);
		if (aAtomBase) {
			STSDAtom *aSTSDAtom = dynamic_cast<STSDAtom *>(aAtomBase);

			AudioDescription aAudioDescription = aSTSDAtom->GetAsAudio();
			return (aAudioDescription.theAudioSampleEntry.SampleRate)
				/ aAudioDescription.FrameSize;
		}
	} else if (IsVideo()) {
		return (frameCount * 1000000.0) / Duration();
	}
	
	return 0.0;
}


float
TRAKAtom::GetSampleRate() 
{
	// Only valid for Audio
	if (IsAudio()) {
		AtomBase *aAtomBase = GetChildAtom(uint32('stsd'),0);
		if (aAtomBase) {
			STSDAtom *aSTSDAtom = dynamic_cast<STSDAtom *>(aAtomBase);

			AudioDescription aAudioDescription = aSTSDAtom->GetAsAudio();
			return aAudioDescription.theAudioSampleEntry.SampleRate;
		}
	}
	
	return 0.0;
}


uint32
TRAKAtom::GetFrameForTime(bigtime_t time) {
	// time / duration * frameCount?
	//return uint32(time * GetFrameRate() / 1000000LL);
	return time * frameCount / Duration();
}


bigtime_t
TRAKAtom::GetTimeForFrame(uint32 pFrame)
{
	return bigtime_t((pFrame * 1000000LL) / GetFrameRate());
}


uint32
TRAKAtom::GetSampleForTime(bigtime_t pTime)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stts'),0);
	
	if (aAtomBase) {
		return (dynamic_cast<STTSAtom *>(aAtomBase))->GetSampleForTime(pTime);
	}
	
	return 0;
}


uint32
TRAKAtom::GetSampleForFrame(uint32 pFrame)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stts'),0);
	
	if (aAtomBase) {
		return (dynamic_cast<STTSAtom *>
			(aAtomBase))->GetSampleForTime(GetTimeForFrame(pFrame));
	}
	
	return 0;
}


uint32
TRAKAtom::GetChunkForSample(uint32 pSample, uint32 *pOffsetInChunk)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stsc'),0);
	
	if (aAtomBase)
		return (dynamic_cast<STSCAtom *>(aAtomBase))->GetChunkForSample(pSample,
			pOffsetInChunk);
	
	return 0;
}


uint32
TRAKAtom::GetNoSamplesInChunk(uint32 pChunkIndex)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stsc'),0);
	
	if (aAtomBase)
		return (dynamic_cast<STSCAtom *>
			(aAtomBase))->GetNoSamplesInChunk(pChunkIndex);
	
	return 0;
}


uint32
TRAKAtom::GetSizeForSample(uint32 pSample)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stsz'),0);
	
	if (aAtomBase) {
		return (dynamic_cast<STSZAtom *>(aAtomBase))->GetSizeForSample(pSample);
	}
	
	return 0;
}


uint64
TRAKAtom::GetOffsetForChunk(uint32 pChunkIndex)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stco'),0);
	
	if (aAtomBase) {
		return (dynamic_cast<STCOAtom *>
			(aAtomBase))->GetOffsetForChunk(pChunkIndex);
	}
	
	return 0;
}


uint32
TRAKAtom::ChunkCount()
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stco'),0);
	
	if (aAtomBase)
		return (dynamic_cast<STCOAtom *>(aAtomBase))->GetTotalChunks();

	return 0;
}


uint32
TRAKAtom::GetFirstSampleInChunk(uint32 pChunkIndex)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stsc'),0);
	
	if (aAtomBase)
		return (dynamic_cast<STSCAtom *>
			(aAtomBase))->GetFirstSampleInChunk(pChunkIndex);
	
	return 0;
}


bool
TRAKAtom::IsSyncSample(uint32 pSampleNo)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stss'),0);
	
	if (aAtomBase)
		return (dynamic_cast<STSSAtom *>(aAtomBase))->IsSyncSample(pSampleNo);
	
	return false;
}


bool
TRAKAtom::IsSingleSampleSize()
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stsz'),0);
	
	if (aAtomBase)
		return (dynamic_cast<STSZAtom *>(aAtomBase))->IsSingleSampleSize();
	
	return false;
}


bool
TRAKAtom::IsActive()
{
	return GetTKHDAtom()->IsActive();
}


uint32
TRAKAtom::GetBytesPerSample()
{
	AtomBase *aAtomBase;

	if (bytesPerSample > 0)
		return bytesPerSample;
	
	// calculate bytes per sample and cache it
	
	if (IsAudio()) {
		// only used by Audio
		aAtomBase = GetChildAtom(uint32('stsd'),0);
		if (aAtomBase) {
			STSDAtom *aSTSDAtom = dynamic_cast<STSDAtom *>(aAtomBase);

			AudioDescription aAudioDescription = aSTSDAtom->GetAsAudio();

			bytesPerSample = aAudioDescription.theAudioSampleEntry.SampleSize
				/ 8;
		}
	}
	
	return bytesPerSample;	
}


uint32
TRAKAtom::GetChunkSize(uint32 pChunkIndex)
{
	uint32 totalSamples = GetNoSamplesInChunk(pChunkIndex);
	
	if (IsSingleSampleSize()) {
		return totalSamples * GetBytesPerSample();
	} else {
		uint32 SampleNo = GetFirstSampleInChunk(pChunkIndex);
		uint32 ChunkSize = 0;
		
		if (SampleNo >= 0) {
			while (totalSamples-- > 0) {
				ChunkSize += GetSizeForSample(SampleNo++);
			}
		}
		
		return ChunkSize;
	}
	
	return 0;
}


uint32
TRAKAtom::GetTotalChunks()
{
	AtomBase *aAtomBase = GetChildAtom(uint32('stco'),0);
	
	if (aAtomBase)
		return (dynamic_cast<STCOAtom *>(aAtomBase))->GetTotalChunks();
	
	return 0;
}
