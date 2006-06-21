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

#include "MP4Parser.h"

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
	} else if (aAtomSize == 0) {
		// aAtomSize extends to end of file.
		// TODO this is broken
		aRealAtomSize = 0;
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

	if (aAtomType == uint32('ctts')) {
		return new CTTSAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('stsz')) {
		return new STSZAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('stz2')) {
		return new STZ2Atom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
	}

	if (aAtomType == uint32('ftyp')) {
		return new FTYPAtom(pStream, aStreamOffset, aAtomType, aRealAtomSize);
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

MVHDAtom::MVHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
{
}

MVHDAtom::~MVHDAtom()
{
}

void MVHDAtom::OnProcessMetaData()
{
	FullAtom::OnProcessMetaData();

	if (getVersion() == 0) {
		mvhdV0 aHeader;

		Read(&aHeader.CreationTime);
		Read(&aHeader.ModificationTime);
		Read(&aHeader.TimeScale);
		Read(&aHeader.Duration);
		Read(&aHeader.PreferredRate);
		Read(&aHeader.PreferredVolume);
		Read(&aHeader.Reserved1);
		Read(&aHeader.Reserved2[0]);
		Read(&aHeader.Reserved2[1]);
		for (uint32 i=0;i<9;i++) {
			Read(&aHeader.Matrix[i]);
		}
		for (uint32 j=0;j<6;j++) {
			Read(&aHeader.pre_defined[j]);
		}
		Read(&aHeader.NextTrackID);

		// Upconvert to V1 header
		theHeader.CreationTime = (uint64)(aHeader.CreationTime);
		theHeader.ModificationTime = (uint64)(aHeader.ModificationTime);
		theHeader.TimeScale = aHeader.TimeScale;
		theHeader.Duration = (uint64)(aHeader.Duration);
		theHeader.PreferredRate = aHeader.PreferredRate;
		theHeader.PreferredVolume = aHeader.PreferredVolume;
		theHeader.NextTrackID = aHeader.NextTrackID;
	}
	
	if (getVersion() == 1) {
		// Read direct into V1 Header
		Read(&theHeader.CreationTime);
		Read(&theHeader.ModificationTime);
		Read(&theHeader.TimeScale);
		Read(&theHeader.Duration);
		Read(&theHeader.PreferredRate);
		Read(&theHeader.PreferredVolume);
		Read(&theHeader.Reserved1);
		Read(&theHeader.Reserved2[0]);
		Read(&theHeader.Reserved2[1]);
		for (uint32 i=0;i<9;i++) {
			Read(&theHeader.Matrix[i]);
		}
		for (uint32 j=0;j<6;j++) {
			Read(&theHeader.pre_defined[j]);
		}
		Read(&theHeader.NextTrackID);
	}
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

STTSAtom::STTSAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
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

	FullAtom::OnProcessMetaData();

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

CTTSAtom::CTTSAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
{
	theHeader.NoEntries = 0;
}

CTTSAtom::~CTTSAtom()
{
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		delete theCompTimeToSampleArray[i];
		theCompTimeToSampleArray[i] = NULL;
	}
}

void CTTSAtom::OnProcessMetaData()
{
CompTimeToSample	*aCompTimeToSample;

	FullAtom::OnProcessMetaData();

	ReadArrayHeader(&theHeader);

	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		aCompTimeToSample = new CompTimeToSample;
		
		Read(&aCompTimeToSample->Count);
		Read(&aCompTimeToSample->Offset);

		theCompTimeToSampleArray[i] = aCompTimeToSample;
	}
}

char *CTTSAtom::OnGetAtomName()
{
	return "Composition Time to Sample Atom";
}

STSCAtom::STSCAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
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

	FullAtom::OnProcessMetaData();

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

STSSAtom::STSSAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
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

	FullAtom::OnProcessMetaData();

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

STSZAtom::STSZAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
{
	SampleCount = 0;
}

STSZAtom::~STSZAtom()
{
	for (uint32 i=0;i<SampleCount;i++) {
		delete theSampleSizeArray[i];
		theSampleSizeArray[i] = NULL;
	}
}

void STSZAtom::OnProcessMetaData()
{
	FullAtom::OnProcessMetaData();

	Read(&SampleSize);
	Read(&SampleCount);

	// If the sample size is constant there is no array and NoEntries seems to contain bad values
	if (SampleSize == 0) {
		SampleSizePtr	aSampleSizePtr;
	
		for (uint32 i=0;i<SampleCount;i++) {
			aSampleSizePtr = new SampleSizeEntry;
		
			Read(&aSampleSizePtr->EntrySize);
	
			theSampleSizeArray[i] = aSampleSizePtr;
		}
	}
}

char *STSZAtom::OnGetAtomName()
{
	return "Sample Size Atom";
}

uint32	STSZAtom::getSizeForSample(uint32 pSampleNo)
{
	if (SampleSize > 0) {
		// All samples are the same size
		return SampleSize;
	}
	
	// Sample Array indexed by SampleNo
	return theSampleSizeArray[pSampleNo]->EntrySize;
}

bool	STSZAtom::IsSingleSampleSize()
{
	return (SampleSize > 0);
}

STZ2Atom::STZ2Atom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
{
	SampleCount = 0;
}

STZ2Atom::~STZ2Atom()
{
	for (uint32 i=0;i<SampleCount;i++) {
		delete theSampleSizeArray[i];
		theSampleSizeArray[i] = NULL;
	}
}

void STZ2Atom::OnProcessMetaData()
{
	FullAtom::OnProcessMetaData();

	uint8	reserved;
	Read(&reserved);
	Read(&reserved);
	Read(&reserved);

	Read(&FieldSize);
	Read(&SampleCount);

	SampleSizePtr	aSampleSizePtr;
	uint8	EntrySize8;
	uint16	EntrySize16;
	
	for (uint32 i=0;i<SampleCount;i++) {
		aSampleSizePtr = new SampleSizeEntry;
		
		switch (FieldSize) {
			case 4:
				Read(&EntrySize8);
				// 2 values per byte
				aSampleSizePtr->EntrySize = (uint32)(EntrySize8);
				break;
			case 8:
				Read(&EntrySize8);
				// 1 value per byte
				aSampleSizePtr->EntrySize = (uint32)(EntrySize8);
				break;
			case 16:
				Read(&EntrySize16);
				// 1 value per 2 bytes
				aSampleSizePtr->EntrySize = (uint32)(EntrySize16);
				break;
		}
		
		theSampleSizeArray[i] = aSampleSizePtr;
	}
}

char *STZ2Atom::OnGetAtomName()
{
	return "Compressed Sample Size Atom";
}

uint32	STZ2Atom::getSizeForSample(uint32 pSampleNo)
{
// THIS CODE NEEDS SOME TESTING, never seen a STZ2 atom

uint32	index;

	switch (FieldSize) {
		case 4:
			// 2 entries per array entry so Divide by 2
			index = pSampleNo / 2;
			if (index * 2 == pSampleNo) {
				// even so return low nibble
				return theSampleSizeArray[pSampleNo]->EntrySize && 0x0000000f;
			} else {
				// odd so return high nibble
				return theSampleSizeArray[pSampleNo]->EntrySize && 0x000000f0;
			}
			break;
		case 8:
			return theSampleSizeArray[pSampleNo]->EntrySize;
			break;
		case 16:
			return theSampleSizeArray[pSampleNo]->EntrySize;
			break;
	}

	// Sample Array indexed by SampleNo
	return theSampleSizeArray[pSampleNo]->EntrySize;
}

bool	STZ2Atom::IsSingleSampleSize()
{
	return false;
}

STCOAtom::STCOAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
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

	FullAtom::OnProcessMetaData();

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

STSDAtom::STSDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
{
	theHeader.NoEntries = 0;
	theAudioDescription.theVOL = NULL;
	theVideoDescription.theVOL = NULL;
}

STSDAtom::~STSDAtom()
{
	if (theAudioDescription.theVOL) {
		free(theAudioDescription.theVOL);
		theAudioDescription.theVOL = NULL;
	}
	
	if (theVideoDescription.theVOL) {
		free(theVideoDescription.theVOL);
		theVideoDescription.theVOL = NULL;
	}
}

uint32	STSDAtom::getMediaHandlerType()
{
	return dynamic_cast<STBLAtom *>(getParent())->getMediaHandlerType();
}

void STSDAtom::ReadESDS(uint8 **VOL, size_t *VOLSize)
{
	// Check for a esds and if it exists read it into the VOL buffer
	// MPEG-4 video/audio use the esds to store the VOL (STUPID COMMITTEE IDEA)
	
	// First make sure we have a esds
	if (getBytesRemaining() > 0) {
		// Well something is there so read it as an atom
		AtomBase *aAtomBase = getAtom(theStream);
	
		aAtomBase->ProcessMetaData();
		printf("%s [%Ld]\n",aAtomBase->getAtomName(),aAtomBase->getAtomSize());

		if (dynamic_cast<ESDSAtom *>(aAtomBase)) {
			// ESDS atom good
			*VOLSize = aAtomBase->getDataSize();
			*VOL = (uint8 *)(malloc(*VOLSize));
			memcpy(*VOL,dynamic_cast<ESDSAtom *>(aAtomBase)->getVOL(),*VOLSize);

			delete aAtomBase;
		} else {
			// Unknown atom bad
			delete aAtomBase;
		}
	}
}

void STSDAtom::ReadSoundDescription()
{
	Read(&theAudioDescription.theAudioSampleEntry.Reserved[1]);
	Read(&theAudioDescription.theAudioSampleEntry.Reserved[2]);
	Read(&theAudioDescription.theAudioSampleEntry.ChannelCount);
	Read(&theAudioDescription.theAudioSampleEntry.SampleSize);
	Read(&theAudioDescription.theAudioSampleEntry.pre_defined);
	Read(&theAudioDescription.theAudioSampleEntry.reserved);
	Read(&theAudioDescription.theAudioSampleEntry.SampleRate);

	ReadESDS(&theAudioDescription.theVOL,&theAudioDescription.VOLSize);
}

void STSDAtom::ReadVideoDescription()
{
	Read(&theVideoDescription.theVideoSampleEntry.pre_defined1);
	Read(&theVideoDescription.theVideoSampleEntry.reserved1);
	Read(&theVideoDescription.theVideoSampleEntry.pre_defined2[0]);
	Read(&theVideoDescription.theVideoSampleEntry.pre_defined2[1]);
	Read(&theVideoDescription.theVideoSampleEntry.pre_defined2[2]);
	Read(&theVideoDescription.theVideoSampleEntry.Width);
	Read(&theVideoDescription.theVideoSampleEntry.Height);
	Read(&theVideoDescription.theVideoSampleEntry.HorizontalResolution);
	Read(&theVideoDescription.theVideoSampleEntry.VerticalResolution);
	Read(&theVideoDescription.theVideoSampleEntry.reserved2);
	Read(&theVideoDescription.theVideoSampleEntry.FrameCount);
	
	Read(theVideoDescription.theVideoSampleEntry.CompressorName,32);
	
	// convert from pascal string (first bytes size) to C string (null terminated)
	uint8 size = (uint8)(theVideoDescription.theVideoSampleEntry.CompressorName[0]);
	if (size > 0) {
		memmove(&theVideoDescription.theVideoSampleEntry.CompressorName[0],&theVideoDescription.theVideoSampleEntry.CompressorName[1],size);
		theVideoDescription.theVideoSampleEntry.CompressorName[size] = '\0';
		printf("%s\n",theVideoDescription.theVideoSampleEntry.CompressorName);
	}
	
	Read(&theVideoDescription.theVideoSampleEntry.Depth);
	Read(&theVideoDescription.theVideoSampleEntry.pre_defined3);

	ReadESDS(&theVideoDescription.theVOL,&theVideoDescription.VOLSize);
}

void STSDAtom::OnProcessMetaData()
{

	FullAtom::OnProcessMetaData();

	ReadArrayHeader(&theHeader);

	// This Atom allows for multiple Desc Entries, we don't

	// Ok the committee was drunk when they designed this.
	// We are actually reading a Atom/Box where the Atom Type is the codec id.
	uint32	Size;
	
	Read(&Size);
	Read(&codecid);
	
	Read(theSampleEntry.Reserved,6);
	Read(&theSampleEntry.DataReference);

	switch (getMediaHandlerType()) {
		case 'soun':
			ReadSoundDescription();
			theAudioDescription.codecid = codecid;
			break;
		case 'vide':
			ReadVideoDescription();
			theVideoDescription.codecid = codecid;
			break;
		case 'hint':
			break;
		default:
			break;
	}
}

VideoDescription STSDAtom::getAsVideo()
{
	// Assert IsVideo
	return theVideoDescription;
}

AudioDescription STSDAtom::getAsAudio()
{
	// Assert IsAudio
	return theAudioDescription;
}

char *STSDAtom::OnGetAtomName()
{
	return "Sample Description Atom";
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

FTYPAtom::FTYPAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
	total_brands = 0;
}

FTYPAtom::~FTYPAtom()
{
}

void FTYPAtom::OnProcessMetaData()
{
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
	return "File type Atom";
}

bool	FTYPAtom::HasBrand(uint32 brand)
{

	if (major_brand == brand) {
		return true;
	}

	// return true if the specified brand is in the list
	for (uint32 i=0;i<total_brands;i++) {
		if (compatable_brands[i] == brand) {
			return true;
		}
	}
	
	return false;	
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
	printf("Offset %lld, Size %lld ",getStreamOffset(),getAtomSize());
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

uint32 MINFAtom::getMediaHandlerType()
{
	return dynamic_cast<MDIAAtom *>(getParent())->getMediaHandlerType();
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

uint32 STBLAtom::getMediaHandlerType()
{
	return dynamic_cast<MINFAtom *>(getParent())->getMediaHandlerType();
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

TKHDAtom::TKHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
{
}

TKHDAtom::~TKHDAtom()
{
}

void TKHDAtom::OnProcessMetaData()
{

	FullAtom::OnProcessMetaData();
	
	if (getVersion() == 0) {
		tkhdV0 aHeaderV0;

		Read(&aHeaderV0.CreationTime);
		Read(&aHeaderV0.ModificationTime);
		Read(&aHeaderV0.TrackID);
		Read(&aHeaderV0.Reserved1);
		Read(&aHeaderV0.Duration);
		Read(&aHeaderV0.Reserved2[0]);
		Read(&aHeaderV0.Reserved2[1]);
		Read(&aHeaderV0.Layer);
		Read(&aHeaderV0.AlternateGroup);
		Read(&aHeaderV0.Volume);
		Read(&aHeaderV0.Reserved3);

		for (uint32 i=0;i<9;i++) {
			Read(&theHeader.MatrixStructure[i]);
		}

		Read(&aHeaderV0.TrackWidth);
		Read(&aHeaderV0.TrackHeight);

		// upconvert to V1 header
		theHeader.CreationTime = (uint64)(aHeaderV0.CreationTime);
		theHeader.ModificationTime = (uint64)(aHeaderV0.ModificationTime);
		theHeader.TrackID = aHeaderV0.TrackID;
		theHeader.Duration = (uint64)(aHeaderV0.Duration);
		theHeader.Layer = aHeaderV0.Layer;
		theHeader.AlternateGroup = aHeaderV0.AlternateGroup;
		theHeader.Volume = aHeaderV0.Volume;
		theHeader.TrackWidth = aHeaderV0.TrackWidth;
		theHeader.TrackHeight = aHeaderV0.TrackHeight;

	} else {
		Read(&theHeader.CreationTime);
		Read(&theHeader.ModificationTime);
		Read(&theHeader.TrackID);
		Read(&theHeader.Reserved1);
		Read(&theHeader.Duration);
		Read(&theHeader.Reserved2[0]);
		Read(&theHeader.Reserved2[1]);
		Read(&theHeader.Layer);
		Read(&theHeader.AlternateGroup);
		Read(&theHeader.Volume);
		Read(&theHeader.Reserved3);

		for (uint32 i=0;i<9;i++) {
			Read(&theHeader.MatrixStructure[i]);
		}

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

uint32 MDIAAtom::getMediaHandlerType()
{
	// get child atom hdlr
	HDLRAtom *aHDLRAtom;
	aHDLRAtom = dynamic_cast<HDLRAtom *>(GetChildAtom(uint32('hdlr')));

	if (aHDLRAtom) {
		return aHDLRAtom->getMediaHandlerType();
	}
	
	return 0;
}

MDHDAtom::MDHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
{
}

MDHDAtom::~MDHDAtom()
{
}

void MDHDAtom::OnProcessMetaData()
{
	FullAtom::OnProcessMetaData();

	if (getVersion() == 0) {
		mdhdV0 aHeader;

		Read(&aHeader.CreationTime);
		Read(&aHeader.ModificationTime);
		Read(&aHeader.TimeScale);
		Read(&aHeader.Duration);
		Read(&aHeader.Language);
		Read(&aHeader.Reserved);

		theHeader.CreationTime = (uint64)(aHeader.CreationTime);
		theHeader.ModificationTime = (uint64)(aHeader.ModificationTime);
		theHeader.TimeScale = aHeader.TimeScale;
		theHeader.Duration = (uint64)(aHeader.Duration);
		theHeader.Language = aHeader.Language;
	} else {
		Read(&theHeader.CreationTime);
		Read(&theHeader.ModificationTime);
		Read(&theHeader.TimeScale);
		Read(&theHeader.Duration);
		Read(&theHeader.Language);
		Read(&theHeader.Reserved);
	}
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

HDLRAtom::HDLRAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
{
	name = NULL;
}

HDLRAtom::~HDLRAtom()
{
	if (name) {
		free(name);
	}
}

void HDLRAtom::OnProcessMetaData()
{
	FullAtom::OnProcessMetaData();

	Read(&theHeader.pre_defined);
	Read(&theHeader.handler_type);
	Read(&theHeader.Reserved[0]);
	Read(&theHeader.Reserved[1]);
	Read(&theHeader.Reserved[2]);

	name = (char *)(malloc(getBytesRemaining()));

	Read(name,getBytesRemaining());
}

char *HDLRAtom::OnGetAtomName()
{
	printf("%s %s:",(char *)&(theHeader.handler_type),name);
	return "Quicktime Handler Reference Atom ";
}

bool HDLRAtom::IsVideoHandler()
{
	return (theHeader.handler_type == 'vide');
}

bool HDLRAtom::IsAudioHandler()
{
	return (theHeader.handler_type == 'soun');
}

uint32 HDLRAtom::getMediaHandlerType()
{
	return theHeader.handler_type;
}

VMHDAtom::VMHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
{
}

VMHDAtom::~VMHDAtom()
{
}

void VMHDAtom::OnProcessMetaData()
{
	FullAtom::OnProcessMetaData();

	Read(&theHeader.GraphicsMode);
	Read(&theHeader.OpColour[0]);
	Read(&theHeader.OpColour[1]);
	Read(&theHeader.OpColour[2]);
}

char *VMHDAtom::OnGetAtomName()
{
	return "Quicktime Video Media Header";
}

SMHDAtom::SMHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : FullAtom(pStream, pstreamOffset, patomType, patomSize)
{
}

SMHDAtom::~SMHDAtom()
{
}

void SMHDAtom::OnProcessMetaData()
{
	FullAtom::OnProcessMetaData();

	Read(&theHeader.Balance);
	Read(&theHeader.Reserved);
}

char *SMHDAtom::OnGetAtomName()
{
	return "Quicktime Sound Media Header";
}
