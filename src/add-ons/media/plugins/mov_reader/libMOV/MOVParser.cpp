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
#include <stdio.h>

#include <DataIO.h>
#include <SupportKit.h>
#include <MediaFormats.h>

#ifdef __HAIKU__
#include <libs/zlib/zlib.h>
#else
#include <zlib.h>
#endif

#include "MOVParser.h"

//static 
AtomBase *getAtom(BPositionIO *pStream)
{
	uint32 aAtomType;
	uint32 aAtomSize;
	uint64 aRealAtomSize;
	off_t aStreamOffset;
	
	aStreamOffset = pStream->Position();
	
	// Get Size uint32
	pStream->Read(&aAtomSize,4);
	aAtomSize = B_BENDIAN_TO_HOST_INT32(aAtomSize);
	// Get Type uint32
	pStream->Read(&aAtomType,4);
	aAtomType = B_BENDIAN_TO_HOST_INT32(aAtomType);
	// Check for extended size
	if (aAtomSize == 1) {
		// Handle extended size
		pStream->Read(&aRealAtomSize,4);
		aRealAtomSize = B_BENDIAN_TO_HOST_INT64(aRealAtomSize);
	} else {
		aRealAtomSize = aAtomSize;
	}

	if (aAtomType == uint32('moov')) {
		return new MOOVAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}
	
	if (aAtomType == uint32('mvhd')) {
		return new MVHDAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('trak')) {
		return new TRAKAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('tkhd')) {
		return new TKHDAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('free')) {
		return new FREEAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('skip')) {
		return new SKIPAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('wide')) {
		return new WIDEAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('mdat')) {
		return new MDATAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('mdia')) {
		return new MDIAAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('mdhd')) {
		return new MDHDAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('hdlr')) {
		return new HDLRAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('minf')) {
		return new MINFAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('vmhd')) {
		return new VMHDAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('smhd')) {
		return new SMHDAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('dinf')) {
		return new DINFAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('stbl')) {
		return new STBLAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('stsd')) {
		return new STSDAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('tmcd')) {
		return new TMCDAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('wave')) {
		return new WAVEAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('stts')) {
		return new STTSAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('pnot')) {
		return new PNOTAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('stsc')) {
		return new STSCAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('stco')) {
		return new STCOAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('stss')) {
		return new STSSAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('stsz')) {
		return new STSZAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('cmov')) {
		return new CMOVAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('dcom')) {
		return new DCOMAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('cmvd')) {
		return new CMVDAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('esds')) {
		return new ESDSAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('avcC')) {
		return new ESDSAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('ftyp')) {
		return new FTYPAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	return new AtomBase(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	
}

MOOVAtom::MOOVAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomContainer(pStream, pstreamOffset, patomType, patomSize)
{
	theMVHDAtom = NULL;
}

MOOVAtom::~MOOVAtom()
{
	theMVHDAtom = NULL;
}

void MOOVAtom::OnProcessMetaData()
{
}

char *MOOVAtom::OnGetAtomName()
{
	return "Quicktime Movie";
}

CMOVAtom::CMOVAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomContainer(pStream, pstreamOffset, patomType, patomSize)
{
	theUncompressedStream = NULL;
}

CMOVAtom::~CMOVAtom()
{
}

BPositionIO *CMOVAtom::OnGetStream()
{
	// Use the decompressed stream instead of file stream
	if (theUncompressedStream) {
		return theUncompressedStream;
	}
	
	return theStream;
}

void CMOVAtom::OnProcessMetaData()
{
	BMallocIO *theUncompressedData;
	uint8 *outBuffer;
	CMVDAtom *aCMVDAtom = NULL;
	uint32	compressionID = 0;
	uint64	descBytesLeft;
	uint32	Size;
	
	descBytesLeft = getAtomSize();
	
	// Check for Compression Type
	while (descBytesLeft > 0) {
		AtomBase *aAtomBase = getAtom(theStream);
	
		aAtomBase->OnProcessMetaData();
		printf("%s [%Ld]\n",aAtomBase->getAtomName(),aAtomBase->getAtomSize());

		if (aAtomBase->getAtomSize() > 0) {
			descBytesLeft = descBytesLeft - aAtomBase->getAtomSize();
		} else {
			printf("Invalid Atom found when reading Compressed Headers\n");
			descBytesLeft = 0;
		}

		if (dynamic_cast<DCOMAtom *>(aAtomBase)) {
			// DCOM atom
			compressionID = dynamic_cast<DCOMAtom *>(aAtomBase)->getCompressionID();
			delete aAtomBase;
		} else {
			if (dynamic_cast<CMVDAtom *>(aAtomBase)) {
				// CMVD atom
				aCMVDAtom = dynamic_cast<CMVDAtom *>(aAtomBase);
				descBytesLeft = 0;
			}
		}
	}

	// Decompress data
	if (compressionID == 'zlib') {
		Size = aCMVDAtom->getUncompressedSize();
		
		outBuffer = (uint8 *)(malloc(Size));
		
		printf("Decompressing %ld bytes to %ld bytes\n",aCMVDAtom->getBufferSize(),Size);
		int result = uncompress(outBuffer, &Size, aCMVDAtom->getCompressedData(), aCMVDAtom->getBufferSize());
		
		if (result != Z_OK) {
			printf("Failed to decompress headers uncompress returned ");
			switch (result) {
				case Z_MEM_ERROR:
					DEBUGGER("Lack of Memory Error\n");
					break;
				case Z_BUF_ERROR:
					DEBUGGER("Lack of Output buffer space Error\n");
					break;
				case Z_DATA_ERROR:
					DEBUGGER("Input Data is corrupt or not a compressed set Error\n");
					break;
			}
		}

		// Copy uncompressed data into BMAllocIO
		theUncompressedData = new BMallocIO();
		theUncompressedData->SetSize(Size);
		theUncompressedData->WriteAt(0L,outBuffer,Size);
		
		free(outBuffer);
		delete aCMVDAtom;
		
		// reset position on BMAllocIO
		theUncompressedData->Seek(SEEK_SET,0L);
		// Assign to Stream
		theUncompressedStream = theUncompressedData;
		
		// All subsequent reads should use theUncompressedStream
	}

}

void CMOVAtom::OnChildProcessingComplete()
{
	// revert back to file stream once all children have finished
	if (theUncompressedStream) {
		delete theUncompressedStream;
	}
	theUncompressedStream = NULL;
}

char *CMOVAtom::OnGetAtomName()
{
	return "Compressed Quicktime Movie";
}

DCOMAtom::DCOMAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

DCOMAtom::~DCOMAtom()
{
}

void DCOMAtom::OnProcessMetaData()
{
	Read(&compressionID);
}

char *DCOMAtom::OnGetAtomName()
{
	return "Decompression Atom";
}

CMVDAtom::CMVDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
	Buffer = NULL;
	UncompressedSize = 0;
	BufferSize = 0;
}

CMVDAtom::~CMVDAtom()
{
	if (Buffer) {
		free(Buffer);
	}
}

void CMVDAtom::OnProcessMetaData()
{
	Read(&UncompressedSize);

	if (UncompressedSize > 0) {
		BufferSize = getBytesRemaining();
		Buffer = (uint8 *)(malloc(BufferSize));
		Read(Buffer,BufferSize);
	}
}

char *CMVDAtom::OnGetAtomName()
{
	return "Compressed Movie Data";
}

MVHDAtom::MVHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

MVHDAtom::~MVHDAtom()
{
}

void MVHDAtom::OnProcessMetaData()
{
	Read(&theHeader.CreationTime);
	Read(&theHeader.ModificationTime);
	Read(&theHeader.TimeScale);
	Read(&theHeader.Duration);
	Read(&theHeader.PreferredRate);
	Read(&theHeader.PreferredVolume);
	Read(&theHeader.PreviewTime);
	Read(&theHeader.PreviewDuration);
	Read(&theHeader.PosterTime);
	Read(&theHeader.SelectionTime);
	Read(&theHeader.SelectionDuration);
	Read(&theHeader.CurrentTime);
	Read(&theHeader.NextTrackID);	
}

char *MVHDAtom::OnGetAtomName()
{
	return "Quicktime Movie Header";
}

MVHDAtom *MOOVAtom::getMVHDAtom()
{
AtomBase *aAtomBase;

	if (theMVHDAtom == NULL) {
		aAtomBase = GetChildAtom(uint32('mvhd'));
		theMVHDAtom = dynamic_cast<MVHDAtom *>(aAtomBase);
	}

	// Assert(theMVHDAtom != NULL,"Movie has no movie header atom");
	return theMVHDAtom;
}

STTSAtom::STTSAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
	theHeader.NoEntries = 0;
	SUMDurations = 0;
	SUMCounts = 0;
}

STTSAtom::~STTSAtom()
{
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		delete theTimeToSampleArray[i];
		theTimeToSampleArray[i] = NULL;
	}
}

void STTSAtom::OnProcessMetaData()
{
TimeToSample	*aTimeToSample;

	ReadArrayHeader(&theHeader);

	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		aTimeToSample = new TimeToSample;
		
		Read(&aTimeToSample->Count);
		Read(&aTimeToSample->Duration);

		theTimeToSampleArray[i] = aTimeToSample;
		SUMDurations += (theTimeToSampleArray[i]->Duration * theTimeToSampleArray[i]->Count);
		SUMCounts += theTimeToSampleArray[i]->Count;
	}
}

char *STTSAtom::OnGetAtomName()
{
	return "Time to Sample Atom";
}

uint32	STTSAtom::getSampleForTime(uint32 pTime)
{
// TODO this is too slow.  PreCalc when loading this?
	uint64 Duration = 0;

	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		Duration += (theTimeToSampleArray[i]->Duration * theTimeToSampleArray[i]->Count);
		if (Duration > pTime) {
			return i;
		}
	}

	return 0;
}

uint32	STTSAtom::getSampleForFrame(uint32 pFrame)
{
// Hmm Sample is Frame really, this Atom is more usefull for time->sample calcs
	return pFrame;
}

STSCAtom::STSCAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
	theHeader.NoEntries = 0;
}

STSCAtom::~STSCAtom()
{
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		delete theSampleToChunkArray[i];
		theSampleToChunkArray[i] = NULL;
	}
}

void STSCAtom::OnProcessMetaData()
{
SampleToChunk	*aSampleToChunk;

	ReadArrayHeader(&theHeader);
	
	uint32	TotalPrevSamples = 0;

	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		aSampleToChunk = new SampleToChunk;
		
		Read(&aSampleToChunk->FirstChunk);
		Read(&aSampleToChunk->SamplesPerChunk);
		Read(&aSampleToChunk->SampleDescriptionID);
		
		if (i > 0) {
			TotalPrevSamples = TotalPrevSamples + (aSampleToChunk->FirstChunk - theSampleToChunkArray[i-1]->FirstChunk) * theSampleToChunkArray[i-1]->SamplesPerChunk;
			aSampleToChunk->TotalPrevSamples = TotalPrevSamples;
		} else {
			aSampleToChunk->TotalPrevSamples = 0;
		}

		theSampleToChunkArray[i] = aSampleToChunk;
	}
}

char *STSCAtom::OnGetAtomName()
{
	return "Sample to Chunk Atom";
}

uint32	STSCAtom::getNoSamplesInChunk(uint32 pChunkID)
{
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		if (theSampleToChunkArray[i]->FirstChunk > pChunkID) {
			return theSampleToChunkArray[i-1]->SamplesPerChunk;
		}
	}
	
	return theSampleToChunkArray[theHeader.NoEntries-1]->SamplesPerChunk;
}

uint32	STSCAtom::getFirstSampleInChunk(uint32 pChunkID)
{
uint32 Diff;

	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		if (theSampleToChunkArray[i]->FirstChunk > pChunkID) {
			Diff = pChunkID - theSampleToChunkArray[i-1]->FirstChunk;
			return ((Diff * theSampleToChunkArray[i-1]->SamplesPerChunk) + theSampleToChunkArray[i-1]->TotalPrevSamples);
		}
	}
	
	Diff = pChunkID - theSampleToChunkArray[theHeader.NoEntries-1]->FirstChunk;
	return ((Diff * theSampleToChunkArray[theHeader.NoEntries-1]->SamplesPerChunk) + theSampleToChunkArray[theHeader.NoEntries-1]->TotalPrevSamples);
}

uint32	STSCAtom::getChunkForSample(uint32 pSample, uint32 *pOffsetInChunk)
{
	uint32 ChunkID = 0;
	uint32 OffsetInChunk = 0;

	for (int32 i=theHeader.NoEntries-1;i>=0;i--) {
		if (pSample >= theSampleToChunkArray[i]->TotalPrevSamples) {
			// Found chunk now calculate offset
			ChunkID = (pSample - theSampleToChunkArray[i]->TotalPrevSamples) / theSampleToChunkArray[i]->SamplesPerChunk;
			ChunkID += theSampleToChunkArray[i]->FirstChunk;

			OffsetInChunk = (pSample - theSampleToChunkArray[i]->TotalPrevSamples) % theSampleToChunkArray[i]->SamplesPerChunk;
			
			*pOffsetInChunk = OffsetInChunk;
			return ChunkID;
		}
	}
	
	*pOffsetInChunk = 0;
	return theSampleToChunkArray[theHeader.NoEntries-1]->FirstChunk;
}

STSSAtom::STSSAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
	theHeader.NoEntries = 0;
}

STSSAtom::~STSSAtom()
{
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		delete theSyncSampleArray[i];
		theSyncSampleArray[i] = NULL;
	}
}

void STSSAtom::OnProcessMetaData()
{
SyncSample	*aSyncSample;

	ReadArrayHeader(&theHeader);
	
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		aSyncSample = new SyncSample;
		
		Read(&aSyncSample->SyncSampleNo);
		theSyncSampleArray[i] = aSyncSample;
	}
}

char *STSSAtom::OnGetAtomName()
{
	return "Sync Sample Atom";
}

bool	STSSAtom::IsSyncSample(uint32 pSampleNo)
{

	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		if (theSyncSampleArray[i]->SyncSampleNo == pSampleNo) {
			return true;
		}
		
		if (pSampleNo > theSyncSampleArray[i]->SyncSampleNo) {
			return false;
		}
	}
	
	return false;
}

STSZAtom::STSZAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
	theHeader.NoEntries = 0;
}

STSZAtom::~STSZAtom()
{
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		delete theSampleSizeArray[i];
		theSampleSizeArray[i] = NULL;
	}
}

void STSZAtom::OnProcessMetaData()
{
SampleSizeEntry	*aSampleSize;

	// Just to make things difficult this is not quite a standard array header
	Read(&theHeader.Version);
	Read(&theHeader.Flags1);
	Read(&theHeader.Flags2);
	Read(&theHeader.Flags3);
	Read(&theHeader.SampleSize);
	Read(&theHeader.NoEntries);
	
	// If the sample size is constant there is no array and NoEntries seems to contain bad values
	if (theHeader.SampleSize == 0) {
		for (uint32 i=0;i<theHeader.NoEntries;i++) {
			aSampleSize = new SampleSizeEntry;
		
			Read(&aSampleSize->EntrySize);
			theSampleSizeArray[i] = aSampleSize;
		}
	}
}

char *STSZAtom::OnGetAtomName()
{
	return "Sample Size Atom";
}

uint32	STSZAtom::getSizeForSample(uint32 pSampleNo)
{
	if (theHeader.SampleSize > 0) {
		// All samples are the same size
		return theHeader.SampleSize;
	}
	
	// Sample Array indexed by SampleNo
	return theSampleSizeArray[pSampleNo]->EntrySize;
}

bool	STSZAtom::IsSingleSampleSize()
{
	return (theHeader.SampleSize > 0);
}

STCOAtom::STCOAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
	theHeader.NoEntries = 0;
}

STCOAtom::~STCOAtom()
{
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		delete theChunkToOffsetArray[i];
		theChunkToOffsetArray[i] = NULL;
	}
}

uint64 STCOAtom::OnGetChunkOffset()
{
	uint32 Offset;

	Read(&Offset);

	// Upconvert to uint64	
	return uint64(Offset);
}

void STCOAtom::OnProcessMetaData()
{
ChunkToOffset	*aChunkToOffset;

	ReadArrayHeader(&theHeader);
	
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		aChunkToOffset = new ChunkToOffset;
		
		aChunkToOffset->Offset = OnGetChunkOffset();

		theChunkToOffsetArray[i] = aChunkToOffset;
	}
	
	PRINT(("Chunk to Offset Array has %ld entries\n",theHeader.NoEntries));
}

char *STCOAtom::OnGetAtomName()
{
	return "Chunk to Offset Atom";
}

uint64	STCOAtom::getOffsetForChunk(uint32 pChunkID)
{
	// First chunk is first array entry
	if ((pChunkID > 0) && (pChunkID <= theHeader.NoEntries)) {
		return theChunkToOffsetArray[pChunkID - 1]->Offset;
	}
	
	#if DEBUG
	  char msg[100]; sprintf(msg, "Bad Chunk ID %ld / %ld\n", pChunkID, theHeader.NoEntries);
	  DEBUGGER(msg);
	#endif

	TRESPASS();
	return 0LL;
}

ESDSAtom::ESDSAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
	theVOL = NULL;
}

ESDSAtom::~ESDSAtom()
{
	if (theVOL) {
		free(theVOL);
	}
}

void ESDSAtom::OnProcessMetaData()
{
	theVOL = (uint8 *)(malloc(getBytesRemaining()));
	Read(theVOL,getBytesRemaining());
}

uint8 *ESDSAtom::getVOL()
{
	return theVOL;
}

char *ESDSAtom::OnGetAtomName()
{
	return "Extended Sample Description Atom";
}

STSDAtom::STSDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
	theHeader.NoEntries = 0;
}

STSDAtom::~STSDAtom()
{
	if (getMediaComponentSubType() == 'soun') {
		for (uint32 i=0;i<theHeader.NoEntries;i++) {
			if (theAudioDescArray[i]->theVOL) {
				free(theAudioDescArray[i]->theVOL);
				theAudioDescArray[i]->theVOL = NULL;
			}
	
			delete theAudioDescArray[i];
			theAudioDescArray[i] = NULL;
		}
	} else if (getMediaComponentSubType() == 'vide') {
		for (uint32 i=0;i<theHeader.NoEntries;i++) {
			if (theVideoDescArray[i]->theVOL) {
				free(theVideoDescArray[i]->theVOL);
				theVideoDescArray[i]->theVOL = NULL;
			}

			delete theVideoDescArray[i];
			theVideoDescArray[i] = NULL;
		}
	}
}

uint32	STSDAtom::getMediaComponentSubType()
{
	return dynamic_cast<STBLAtom *>(getParent())->getMediaComponentSubType();
}

void STSDAtom::ReadSoundDescription()
{
	uint64 descBytesLeft;

	SoundDescriptionV1 *aSoundDescriptionV1;
			
	aSoundDescriptionV1 = new SoundDescriptionV1;
	Read(&aSoundDescriptionV1->basefields.Size);
	Read(&aSoundDescriptionV1->basefields.DataFormat);
	Read(aSoundDescriptionV1->basefields.Reserved,6);
	Read(&aSoundDescriptionV1->basefields.DataReference);

	aSoundDescriptionV1->VOLSize = 0;
	aSoundDescriptionV1->theVOL = NULL;

	// Read in Audio Sample Data
	// We place into a V1 description even though it might be a V0 or earlier

	Read(&aSoundDescriptionV1->desc.Version);
	Read(&aSoundDescriptionV1->desc.Revision);
	Read(&aSoundDescriptionV1->desc.Vendor);
	Read(&aSoundDescriptionV1->desc.NoOfChannels);
	Read(&aSoundDescriptionV1->desc.SampleSize);
	Read(&aSoundDescriptionV1->desc.CompressionID);
	Read(&aSoundDescriptionV1->desc.PacketSize);
	Read(&aSoundDescriptionV1->desc.SampleRate);

	if ((aSoundDescriptionV1->desc.Version == 1) && (aSoundDescriptionV1->basefields.DataFormat != AUDIO_IMA4)) {
		Read(&(aSoundDescriptionV1->samplesPerPacket));
		Read(&(aSoundDescriptionV1->bytesPerPacket));
		Read(&(aSoundDescriptionV1->bytesPerFrame));
		Read(&(aSoundDescriptionV1->bytesPerSample));
	
	} else {
		// Calculate?
		if (aSoundDescriptionV1->basefields.DataFormat == AUDIO_IMA4) {
			aSoundDescriptionV1->samplesPerPacket = 64;
			aSoundDescriptionV1->bytesPerFrame = aSoundDescriptionV1->desc.NoOfChannels * 34;

			aSoundDescriptionV1->bytesPerSample = aSoundDescriptionV1->desc.SampleSize / 8;
			aSoundDescriptionV1->bytesPerPacket = 64 * aSoundDescriptionV1->bytesPerFrame;
		} else {
			aSoundDescriptionV1->bytesPerSample = aSoundDescriptionV1->desc.SampleSize / 8;
			aSoundDescriptionV1->bytesPerFrame = aSoundDescriptionV1->desc.NoOfChannels * aSoundDescriptionV1->bytesPerSample;
			aSoundDescriptionV1->bytesPerPacket = aSoundDescriptionV1->desc.PacketSize;
			aSoundDescriptionV1->samplesPerPacket = aSoundDescriptionV1->desc.PacketSize / aSoundDescriptionV1->bytesPerFrame;
		}
	}

	// 0 means we dont have one
	aSoundDescriptionV1->theWaveFormat.format_tag = 0;

	descBytesLeft = getBytesRemaining();

	while (descBytesLeft > 0) {

		// More extended atoms
		AtomBase *aAtomBase = getAtom(theStream);
				
		aAtomBase->OnProcessMetaData();
		printf("%s\n",aAtomBase->getAtomName());

		if (dynamic_cast<WAVEAtom *>(aAtomBase)) {
			// WAVE atom
			aSoundDescriptionV1->theWaveFormat = dynamic_cast<WAVEAtom *>(aAtomBase)->getWaveFormat();
		}

		if (dynamic_cast<ESDSAtom *>(aAtomBase)) {
			// ESDS atom good
			aSoundDescriptionV1->VOLSize = aAtomBase->getDataSize();
			aSoundDescriptionV1->theVOL = (uint8 *)(malloc(aSoundDescriptionV1->VOLSize));
			memcpy(aSoundDescriptionV1->theVOL,dynamic_cast<ESDSAtom *>(aAtomBase)->getVOL(),aSoundDescriptionV1->VOLSize);
		}
		
		if ((aAtomBase->getAtomSize() > 0) && (descBytesLeft >= aAtomBase->getAtomSize())) {
			descBytesLeft = descBytesLeft - aAtomBase->getAtomSize();
		} else {
			DEBUGGER("Invalid Atom found when reading Sound Description\n");
			descBytesLeft = 0;
		}
				
		delete aAtomBase;
	}
			
	theAudioDescArray[0] = aSoundDescriptionV1;

	PRINT(("Size:Format=%ld:%ld %Ld\n",aSoundDescriptionV1->basefields.Size,aSoundDescriptionV1->basefields.DataFormat,descBytesLeft));
}

void STSDAtom::ReadVideoDescription()
{
	uint64 descBytesLeft;

	// read in Video Sample Data
	VideoDescriptionV0 *aVideoDescription;
				
	aVideoDescription = new VideoDescriptionV0;

	Read(&aVideoDescription->basefields.Size);
	Read(&aVideoDescription->basefields.DataFormat);
	Read(aVideoDescription->basefields.Reserved,6);
	Read(&aVideoDescription->basefields.DataReference);
	
	Read(&aVideoDescription->desc.Version);
	Read(&aVideoDescription->desc.Revision);
	Read(&aVideoDescription->desc.Vendor);
	Read(&aVideoDescription->desc.TemporaralQuality);
	Read(&aVideoDescription->desc.SpacialQuality);
	Read(&aVideoDescription->desc.Width);
	Read(&aVideoDescription->desc.Height);
	Read(&aVideoDescription->desc.HorizontalResolution);
	Read(&aVideoDescription->desc.VerticalResolution);
	Read(&aVideoDescription->desc.DataSize);
	// FrameCount is actually No of Frames per Sample which is usually 1
	Read(&aVideoDescription->desc.FrameCount);
	Read(aVideoDescription->desc.CompressorName,32);
	Read(&aVideoDescription->desc.Depth);
	Read(&aVideoDescription->desc.ColourTableID);

	aVideoDescription->VOLSize = 0;
	aVideoDescription->theVOL = NULL;

	theVideoDescArray[0] = aVideoDescription;

	descBytesLeft = getBytesRemaining();
	
	// May be a VOL
	// If not then seek back to where we are as it may be a complete new video description
	
	if (descBytesLeft > 0) {

		off_t pos = theStream->Position();
		// More extended atoms
		AtomBase *aAtomBase = getAtom(theStream);
				
		aAtomBase->OnProcessMetaData();
		printf("%s\n",aAtomBase->getAtomName());

		if (dynamic_cast<ESDSAtom *>(aAtomBase)) {
			// ESDS atom good
			aVideoDescription->VOLSize = aAtomBase->getDataSize();
			aVideoDescription->theVOL = (uint8 *)(malloc(aVideoDescription->VOLSize));
			memcpy(aVideoDescription->theVOL,dynamic_cast<ESDSAtom *>(aAtomBase)->getVOL(),aVideoDescription->VOLSize);
		} else {
			// Seek Back
			theStream->Seek(pos,SEEK_SET);
		}
		
		delete aAtomBase;
	}

	PRINT(("Size:Format=%ld:%ld %Ld\n",aVideoDescription->basefields.Size,aVideoDescription->basefields.DataFormat,getBytesRemaining()));
}

void STSDAtom::OnProcessMetaData()
{
	ReadArrayHeader(&theHeader);

	for (uint32 i=0;i<theHeader.NoEntries;i++) {

		switch (getMediaComponentSubType()) {
			case 'soun':
				ReadSoundDescription();
				break;
			case 'vide':
				ReadVideoDescription();
				break;
			default:
				// Skip
				SampleDescBase	aSampleDescBase;
				Read(&aSampleDescBase.Size);
				Read(&aSampleDescBase.DataFormat);
				Read(aSampleDescBase.Reserved,6);
				Read(&aSampleDescBase.DataReference);
				
				printf("%c%c%c%c\n",char(aSampleDescBase.DataFormat>>24),char(aSampleDescBase.DataFormat>>16),char(aSampleDescBase.DataFormat>>8),char(aSampleDescBase.DataFormat));

				theStream->Seek(aSampleDescBase.Size - SampleDescBaseSize, SEEK_CUR);
				break;
		}
	}
}

VideoDescriptionV0 STSDAtom::getAsVideo()
{
	// Assert IsVideo - how will we handle multiples
	return *theVideoDescArray[0];
}

SoundDescriptionV1 STSDAtom::getAsAudio()
{
	// Assert IsAudio - how will we handle multiples
	return *theAudioDescArray[0];
}

char *STSDAtom::OnGetAtomName()
{
	return "Sample Description Atom";
}

WAVEAtom::WAVEAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

WAVEAtom::~WAVEAtom()
{
}

void WAVEAtom::OnProcessMetaData()
{
	// This should be in LITTLE ENDIAN DATA
	theStream->Read(&theWaveFormat.format_tag,sizeof(uint16));
	theStream->Read(&theWaveFormat.channels,sizeof(uint16));
	theStream->Read(&theWaveFormat.frames_per_sec,sizeof(uint32));
	theStream->Read(&theWaveFormat.avg_bytes_per_sec,sizeof(uint32));
	theStream->Read(&theWaveFormat.block_align,sizeof(uint16));
	theStream->Read(&theWaveFormat.bits_per_sample,sizeof(uint16));
	theStream->Read(&theWaveFormat.extra_size,sizeof(uint16));

	theWaveFormat.format_tag = B_LENDIAN_TO_HOST_INT16(theWaveFormat.format_tag);
	theWaveFormat.channels = B_LENDIAN_TO_HOST_INT16(theWaveFormat.channels);
	theWaveFormat.frames_per_sec = B_LENDIAN_TO_HOST_INT32(theWaveFormat.frames_per_sec);
	theWaveFormat.avg_bytes_per_sec = B_LENDIAN_TO_HOST_INT32(theWaveFormat.avg_bytes_per_sec);
	theWaveFormat.block_align = B_LENDIAN_TO_HOST_INT16(theWaveFormat.block_align);
	theWaveFormat.bits_per_sample = B_LENDIAN_TO_HOST_INT16(theWaveFormat.bits_per_sample);
	theWaveFormat.extra_size = B_LENDIAN_TO_HOST_INT16(theWaveFormat.extra_size);
}

char *WAVEAtom::OnGetAtomName()
{
	return "WAVE structure Atom";
}

TMCDAtom::TMCDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

TMCDAtom::~TMCDAtom()
{
}

void TMCDAtom::OnProcessMetaData()
{
}

char *TMCDAtom::OnGetAtomName()
{
	return "TimeCode Atom";
}

WIDEAtom::WIDEAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

WIDEAtom::~WIDEAtom()
{
}

void WIDEAtom::OnProcessMetaData()
{
}

char *WIDEAtom::OnGetAtomName()
{
	return "WIDE Atom";
}

FREEAtom::FREEAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

FREEAtom::~FREEAtom()
{
}

void FREEAtom::OnProcessMetaData()
{
}

char *FREEAtom::OnGetAtomName()
{
	return "Free Atom";
}

PNOTAtom::PNOTAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

PNOTAtom::~PNOTAtom()
{
}

void PNOTAtom::OnProcessMetaData()
{
}

char *PNOTAtom::OnGetAtomName()
{
	return "Preview Atom";
}

SKIPAtom::SKIPAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

SKIPAtom::~SKIPAtom()
{
}

void SKIPAtom::OnProcessMetaData()
{
}

char *SKIPAtom::OnGetAtomName()
{
	return "Skip Atom";
}

MDATAtom::MDATAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

MDATAtom::~MDATAtom()
{
}

void MDATAtom::OnProcessMetaData()
{
}

char *MDATAtom::OnGetAtomName()
{
	return "Media Data Atom";
}

off_t	MDATAtom::getEOF()
{
	return getStreamOffset() + getAtomSize();
}

MINFAtom::MINFAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomContainer(pStream, pstreamOffset, patomType, patomSize)
{
}

MINFAtom::~MINFAtom()
{
}

void MINFAtom::OnProcessMetaData()
{
}

char *MINFAtom::OnGetAtomName()
{
	return "Quicktime Media Information Atom";
}

uint32 MINFAtom::getMediaComponentSubType()
{
	return dynamic_cast<MDIAAtom *>(getParent())->getMediaComponentSubType();
}

STBLAtom::STBLAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomContainer(pStream, pstreamOffset, patomType, patomSize)
{
}

STBLAtom::~STBLAtom()
{
}

void STBLAtom::OnProcessMetaData()
{
}

char *STBLAtom::OnGetAtomName()
{
	return "Quicktime Sample Table Atom";
}

uint32 STBLAtom::getMediaComponentSubType()
{
	return dynamic_cast<MINFAtom *>(getParent())->getMediaComponentSubType();
}

DINFAtom::DINFAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomContainer(pStream, pstreamOffset, patomType, patomSize)
{
}

DINFAtom::~DINFAtom()
{
}

void DINFAtom::OnProcessMetaData()
{
}

char *DINFAtom::OnGetAtomName()
{
	return "Quicktime Data Information Atom";
}

TKHDAtom::TKHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

TKHDAtom::~TKHDAtom()
{
}

void TKHDAtom::OnProcessMetaData()
{
	Read(&Version);
	
	if (Version == 0) {
		// Read into V0 header and convert to V1 Header
		tkhdV0 aHeaderV0;
		
		Read(&aHeaderV0.Flags1);
		Read(&aHeaderV0.Flags2);
		Read(&aHeaderV0.Flags3);
		Read(&aHeaderV0.CreationTime);
		Read(&aHeaderV0.ModificationTime);
		Read(&aHeaderV0.TrackID);
		Read(&aHeaderV0.Reserved1);
		Read(&aHeaderV0.Duration);
		Read(&aHeaderV0.Reserved2);
		Read(&aHeaderV0.Layer);
		Read(&aHeaderV0.AlternateGroup);
		Read(&aHeaderV0.Volume);
		Read(&aHeaderV0.Reserved3);
		Read(aHeaderV0.MatrixStructure,36);
		Read(&aHeaderV0.TrackWidth);
		Read(&aHeaderV0.TrackHeight);

		theHeader.Flags1 = aHeaderV0.Flags1;
		theHeader.Flags2 = aHeaderV0.Flags2;
		theHeader.Flags3 = aHeaderV0.Flags3;
		theHeader.Reserved1 = aHeaderV0.Reserved1;
		theHeader.Reserved2 = aHeaderV0.Reserved2;
		theHeader.Reserved3 = aHeaderV0.Reserved3;
		
		for (uint32 i=0;i<36;i++) {
			theHeader.MatrixStructure[i] =  aHeaderV0.MatrixStructure[i];
		}

		// upconvert to V1 header
		theHeader.CreationTime = uint64(aHeaderV0.CreationTime);
		theHeader.ModificationTime = uint64(aHeaderV0.ModificationTime);
		theHeader.TrackID = aHeaderV0.TrackID;
		theHeader.Duration = aHeaderV0.Duration;
		theHeader.Layer = aHeaderV0.Layer;
		theHeader.AlternateGroup = aHeaderV0.AlternateGroup;
		theHeader.Volume = aHeaderV0.Volume;
		theHeader.TrackWidth = aHeaderV0.TrackWidth;
		theHeader.TrackHeight = aHeaderV0.TrackHeight;

	} else {
		// Read direct into V1 Header
		Read(&theHeader.Flags1);
		Read(&theHeader.Flags2);
		Read(&theHeader.Flags3);
		Read(&theHeader.CreationTime);
		Read(&theHeader.ModificationTime);
		Read(&theHeader.TrackID);
		Read(&theHeader.Reserved1);
		Read(&theHeader.Duration);
		Read(&theHeader.Reserved2);
		Read(&theHeader.Layer);
		Read(&theHeader.AlternateGroup);
		Read(&theHeader.Volume);
		Read(&theHeader.Reserved3);
		Read(theHeader.MatrixStructure,36);
		Read(&theHeader.TrackWidth);
		Read(&theHeader.TrackHeight);
	}

}

char *TKHDAtom::OnGetAtomName()
{
	return "Quicktime Track Header";
}

MDIAAtom::MDIAAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomContainer(pStream, pstreamOffset, patomType, patomSize)
{
}

MDIAAtom::~MDIAAtom()
{
}

void MDIAAtom::OnProcessMetaData()
{
}

char *MDIAAtom::OnGetAtomName()
{
	return "Quicktime Media Atom";
}

uint32 MDIAAtom::getMediaComponentSubType()
{
	// get child atom hdlr
	HDLRAtom *aHDLRAtom;
	aHDLRAtom = dynamic_cast<HDLRAtom *>(GetChildAtom(uint32('hdlr')));

	if (aHDLRAtom) {
		return aHDLRAtom->getMediaComponentSubType();
	}
	
	return 0;
}

MDHDAtom::MDHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

MDHDAtom::~MDHDAtom()
{
}

void MDHDAtom::OnProcessMetaData()
{
	Read(&theHeader.Version);
	Read(&theHeader.Flags1);
	Read(&theHeader.Flags2);
	Read(&theHeader.Flags3);
	Read(&theHeader.CreationTime);
	Read(&theHeader.ModificationTime);
	Read(&theHeader.TimeScale);
	Read(&theHeader.Duration);
	Read(&theHeader.Language);
	Read(&theHeader.Quality);
}

char *MDHDAtom::OnGetAtomName()
{
	return "Quicktime Media Header";
}

bigtime_t	MDHDAtom::getDuration() 
{
	return bigtime_t((uint64(theHeader.Duration) * 1000000L) / theHeader.TimeScale);
}

uint32		MDHDAtom::getTimeScale()
{
	return theHeader.TimeScale;
}

HDLRAtom::HDLRAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

HDLRAtom::~HDLRAtom()
{
}

void HDLRAtom::OnProcessMetaData()
{
	Read(&theHeader.Version);
	Read(&theHeader.Flags1);
	Read(&theHeader.Flags2);
	Read(&theHeader.Flags3);
	Read(&theHeader.ComponentType);
	Read(&theHeader.ComponentSubType);
	Read(&theHeader.ComponentManufacturer);
	Read(&theHeader.ComponentFlags);
	Read(&theHeader.ComponentFlagsMask);
	
	// Read Array of Strings?
}

char *HDLRAtom::OnGetAtomName()
{
	return "Quicktime Handler Reference Atom ";
}

bool HDLRAtom::IsVideoHandler()
{
	return (theHeader.ComponentSubType == 'vide');
}

bool HDLRAtom::IsAudioHandler()
{
	return (theHeader.ComponentSubType == 'soun');
}

uint32 HDLRAtom::getMediaComponentSubType()
{
	return theHeader.ComponentSubType;
}

VMHDAtom::VMHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

VMHDAtom::~VMHDAtom()
{
}

void VMHDAtom::OnProcessMetaData()
{
	vmhd aHeader;
	Read(&aHeader.Version);
	Read(&aHeader.Flags1);
	Read(&aHeader.Flags2);
	Read(&aHeader.Flags3);
	Read(&aHeader.GraphicsMode);
	Read(&aHeader.OpColourRed);
	Read(&aHeader.OpColourGreen);
	Read(&aHeader.OpColourBlue);
}

char *VMHDAtom::OnGetAtomName()
{
	return "Quicktime Video Media Header";
}

SMHDAtom::SMHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

SMHDAtom::~SMHDAtom()
{
}

void SMHDAtom::OnProcessMetaData()
{
	smhd aHeader;
	Read(&aHeader.Version);
	Read(&aHeader.Flags1);
	Read(&aHeader.Flags2);
	Read(&aHeader.Flags3);
	Read(&aHeader.Balance);
	Read(&aHeader.Reserved);
}

char *SMHDAtom::OnGetAtomName()
{
	return "Quicktime Sound Media Header";
}

FTYPAtom::FTYPAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

FTYPAtom::~FTYPAtom()
{
}

void FTYPAtom::OnProcessMetaData()
{
// ftyp is really an mp4 thing, but some mov encoders are adding it anyway.
// but a mov file with an ftyp should define a brand of qt (I think)

	Read(&major_brand);
	Read(&minor_version);

	total_brands = getBytesRemaining() / sizeof(uint32);
	
	if (total_brands > 32) {
		total_brands = 32;	// restrict to 32
	}
	
	for (uint32 i=0;i<total_brands;i++) {
		Read(&compatable_brands[i]);
	}
}

char *FTYPAtom::OnGetAtomName()
{
	return "Quicktime File Type";
}

bool	FTYPAtom::HasBrand(uint32 brand)
{
	// return true if the specified brand is in the list

	if (major_brand == brand) {
		return true;
	}

	for (uint32 i=0;i<total_brands;i++) {
		if (compatable_brands[i] == brand) {
			return true;
		}
	}
	
	return false;	
}
