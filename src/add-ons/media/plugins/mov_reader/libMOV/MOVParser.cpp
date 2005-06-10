#include <stdio.h>

#include <DataIO.h>
#include <SupportKit.h>
#include <MediaFormats.h>

#include "MOVParser.h"

struct mov_codec_table
{
	int id;
	media_type type;
	uint32	fourcc;
};

static const uint32 num_codecs = 27;
static const mov_codec_table theCodecTable[num_codecs] = 
{
1,B_MEDIA_ENCODED_VIDEO,'3IV1',
2,B_MEDIA_ENCODED_VIDEO,'3IV2',
3,B_MEDIA_ENCODED_VIDEO,'MP4V',
4,B_MEDIA_ENCODED_VIDEO,'mp4v',
5,B_MEDIA_ENCODED_VIDEO,'RMP4',
6,B_MEDIA_ENCODED_VIDEO,'dm4v',
7,B_MEDIA_ENCODED_VIDEO,'UMP4',
8,B_MEDIA_ENCODED_VIDEO,'MP4S',
9,B_MEDIA_ENCODED_AUDIO,AUDIO_NONE,
10,B_MEDIA_ENCODED_AUDIO,AUDIO_RAW,
11,B_MEDIA_ENCODED_AUDIO,AUDIO_TWOS1,
12,B_MEDIA_ENCODED_AUDIO,AUDIO_TWOS2,
13,B_MEDIA_ENCODED_AUDIO,'.mp3',
14,B_MEDIA_ENCODED_AUDIO,AUDIO_IMA4,
15,B_MEDIA_ENCODED_AUDIO,'fl32',
16,B_MEDIA_ENCODED_AUDIO,'fl64',
17,B_MEDIA_ENCODED_AUDIO,'in24',
18,B_MEDIA_ENCODED_AUDIO,'in32',
19,B_MEDIA_ENCODED_AUDIO,'ulaw',
20,B_MEDIA_ENCODED_AUDIO,'alaw',
21,B_MEDIA_ENCODED_AUDIO,'dvca',
22,B_MEDIA_ENCODED_AUDIO,'QDMC',
23,B_MEDIA_ENCODED_AUDIO,AUDIO_MS_PCM02,
24,B_MEDIA_ENCODED_AUDIO,AUDIO_INTEL_PCM17,
25,B_MEDIA_ENCODED_AUDIO,'mp4a',
26,B_MEDIA_ENCODED_AUDIO,AUDIO_MPEG3_CBR,
27,B_MEDIA_ENCODED_VIDEO,'avc1'
};

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

MVHDAtom::MVHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

MVHDAtom::~MVHDAtom()
{
}

void MVHDAtom::OnProcessMetaData()
{
	theStream->Read(&theHeader,sizeof(mvhd));

	theHeader.CreationTime = B_BENDIAN_TO_HOST_INT32(theHeader.CreationTime);
	theHeader.ModificationTime = B_BENDIAN_TO_HOST_INT32(theHeader.ModificationTime);
	theHeader.TimeScale = B_BENDIAN_TO_HOST_INT32(theHeader.TimeScale);
	theHeader.Duration = B_BENDIAN_TO_HOST_INT32(theHeader.Duration);
	theHeader.PreferredRate = B_BENDIAN_TO_HOST_INT32(theHeader.PreferredRate);
	theHeader.PreferredVolume = B_BENDIAN_TO_HOST_INT16(theHeader.PreferredVolume);
	theHeader.PreviewTime = B_BENDIAN_TO_HOST_INT32(theHeader.PreviewTime);
	theHeader.PreviewDuration = B_BENDIAN_TO_HOST_INT32(theHeader.PreviewDuration);
	theHeader.PosterTime = B_BENDIAN_TO_HOST_INT32(theHeader.PosterTime);
	theHeader.SelectionTime = B_BENDIAN_TO_HOST_INT32(theHeader.SelectionTime);
	theHeader.SelectionDuration = B_BENDIAN_TO_HOST_INT32(theHeader.SelectionDuration);
	theHeader.CurrentTime = B_BENDIAN_TO_HOST_INT32(theHeader.CurrentTime);
	theHeader.NextTrackID = B_BENDIAN_TO_HOST_INT32(theHeader.NextTrackID);
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
		
		theStream->Read(aTimeToSample,sizeof(TimeToSample));
		aTimeToSample->Count = B_BENDIAN_TO_HOST_INT32(aTimeToSample->Count);
		aTimeToSample->Duration = B_BENDIAN_TO_HOST_INT32(aTimeToSample->Duration);

		theTimeToSampleArray[i] = aTimeToSample;
	}
}

char *STTSAtom::OnGetAtomName()
{
	return "Time to Sample Atom";
}

uint64	STTSAtom::getSUMCounts()
{
	uint64 SUMCounts = 0;
	uint64 SUMDurations = 0;
	
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		SUMDurations += (theTimeToSampleArray[i]->Duration * theTimeToSampleArray[i]->Count);
		SUMCounts += theTimeToSampleArray[i]->Count;
	}
	
	return SUMCounts;
}

uint64	STTSAtom::getSUMDurations()
{
// this Duration is not scaled to timescale
	uint64 SUMDurations = 0;
	
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		SUMDurations += (theTimeToSampleArray[i]->Duration * theTimeToSampleArray[i]->Count);
	}
	
	return SUMDurations;
}

uint32	STTSAtom::getSampleForTime(uint32 pTime)
{
	uint64 SUMDurations = 0;

	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		SUMDurations += (theTimeToSampleArray[i]->Duration * theTimeToSampleArray[i]->Count);
		if (SUMDurations > pTime) {
			return i;
		}
	}

	return 0;
}

uint32	STTSAtom::getSampleForFrame(uint32 pFrame)
{
// Hmm Sample is Frame really.

	uint64 SUMCounts = 0;
	uint64 SUMDurations = 0;

	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		for (uint32 j=0;j<theTimeToSampleArray[i]->Count;j++) {
			SUMCounts ++;
			SUMDurations += theTimeToSampleArray[i]->Duration;
//			printf("(i=%ld j=%ld) Duration %ld, Count %ld, Duration*Count %ld, SUM(Count) %lld, SUM(Duration*Count) %lld\n",i,j,theTimeToSampleArray[i]->Duration,theTimeToSampleArray[i]->Count,theTimeToSampleArray[i]->Duration * theTimeToSampleArray[i]->Count,SUMCounts, SUMDurations);
			if (SUMCounts > pFrame) {
				return SUMCounts-1;
			}
		}
	}

	return theHeader.NoEntries;
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

	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		aSampleToChunk = new SampleToChunk;
		
		theStream->Read(aSampleToChunk,sizeof(SampleToChunk));
		aSampleToChunk->FirstChunk = B_BENDIAN_TO_HOST_INT32(aSampleToChunk->FirstChunk);
		aSampleToChunk->SamplesPerChunk = B_BENDIAN_TO_HOST_INT32(aSampleToChunk->SamplesPerChunk);
		aSampleToChunk->SampleDescriptionID = B_BENDIAN_TO_HOST_INT32(aSampleToChunk->SampleDescriptionID);

		theSampleToChunkArray[i] = aSampleToChunk;
	}
}

char *STSCAtom::OnGetAtomName()
{
	return "Sample to Chunk Atom";
}

uint32	STSCAtom::getChunkForSample(uint32 pSample, uint32 *pOffsetInChunk)
{
	uint32 SampleNo = 0;
	
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		SampleNo += theSampleToChunkArray[i]->SamplesPerChunk;
		if (SampleNo > pSample) {
//			printf("*%ld %ld %ld %ld\n",SampleNo, pSample, theSampleToChunkArray[i]->SamplesPerChunk, theSampleToChunkArray[i]->FirstChunk);
			*pOffsetInChunk = theSampleToChunkArray[i]->SamplesPerChunk - (SampleNo - pSample);
			return theSampleToChunkArray[i]->FirstChunk;
		}
	}
	
	return 0;
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
		
		theStream->Read(aSyncSample,sizeof(SyncSample));
		aSyncSample->SyncSampleNo = B_BENDIAN_TO_HOST_INT32(aSyncSample->SyncSampleNo);

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
SampleSize	*aSampleSize;

	// Just to make things difficult this is not quite a standard array header
	theStream->Read(&theHeader,sizeof(SampleSizeHeader));

	theHeader.SampleSize = B_BENDIAN_TO_HOST_INT32(theHeader.SampleSize);
	theHeader.NoEntries = B_BENDIAN_TO_HOST_INT32(theHeader.NoEntries);
	
	// If the sample size is constant there is no array and NoEntries seems to contain bad values
	if (theHeader.SampleSize == 0) {
		for (uint32 i=0;i<theHeader.NoEntries;i++) {
			aSampleSize = new SampleSize;
		
			theStream->Read(aSampleSize,sizeof(SampleSize));
			aSampleSize->Size = B_BENDIAN_TO_HOST_INT32(aSampleSize->Size);
	
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
	return theSampleSizeArray[pSampleNo]->Size;
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

	theStream->Read(&Offset,sizeof(uint32));

	// Upconvert to uint64	
	return uint64(B_BENDIAN_TO_HOST_INT32(Offset));
}

void STCOAtom::OnProcessMetaData()
{
ChunkToOffset	*aChunkToOffset;

	ReadArrayHeader(&theHeader);
	
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		aChunkToOffset = new ChunkToOffset;
		
		aChunkToOffset->Offset = OnGetChunkOffset();

//		printf("%ld : %lld\n",i,aChunkToOffset->Offset);

		theChunkToOffsetArray[i] = aChunkToOffset;
	}
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
	
	printf("Bad Chunk ID %ld / %ld\n",pChunkID,theHeader.NoEntries);
	// TODO Yes this will seg fault.  But I get a very bad lock up if I don't :-(
	return theChunkToOffsetArray[pChunkID - 1]->Offset;
}

STSDAtom::STSDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
	theHeader.NoEntries = 0;
}

STSDAtom::~STSDAtom()
{
	if (IsAudioSampleDesc(DataFormat)) {
		for (uint32 i=0;i<theHeader.NoEntries;i++) {
			delete theAudioDescArray[i];
			theAudioDescArray[i] = NULL;
		}
	} else {
		for (uint32 i=0;i<theHeader.NoEntries;i++) {
			delete theVideoDescArray[i];
			theVideoDescArray[i] = NULL;
		}
	}
}

bool STSDAtom::IsAudioSampleDesc(uint32 pDataFormat)
{
	char dcc[5];

	dcc[0] = (char)((pDataFormat >> 24) & 0xff);
	dcc[1] = (char)((pDataFormat >> 16) & 0xff);
	dcc[2] = (char)((pDataFormat >> 8) & 0xff);
	dcc[3] = (char)((pDataFormat >> 0) & 0xff);
	dcc[4] = '\0';

	printf("%s\n",dcc);

	for (uint32 i=0;i<num_codecs;i++) {
		if (theCodecTable[i].fourcc == pDataFormat) {
			if (theCodecTable[i].type == B_MEDIA_ENCODED_AUDIO) {
				return true;
			}
		}
	}

	return false;
}

bool STSDAtom::IsVideoSampleDesc(uint32 pDataFormat)
{
	char dcc[5];

	dcc[0] = (char)((pDataFormat >> 24) & 0xff);
	dcc[1] = (char)((pDataFormat >> 16) & 0xff);
	dcc[2] = (char)((pDataFormat >> 8) & 0xff);
	dcc[3] = (char)((pDataFormat >> 0) & 0xff);
	dcc[4] = '\0';

	printf("%s\n",dcc);

	for (uint32 i=0;i<num_codecs;i++) {
		if (theCodecTable[i].fourcc == pDataFormat) {
			if (theCodecTable[i].type == B_MEDIA_ENCODED_VIDEO) {
				return true;
			}
		}
	}
	
	return false;
}

void STSDAtom::OnProcessMetaData()
{
	uint32 descBytesLeft;

	ReadArrayHeader(&theHeader);

	SampleDescBase aSampleDesc;
	
	printf("Entries=%ld\n",theHeader.NoEntries);
	
	for (uint32 i=0;i<theHeader.NoEntries;i++) {
		theStream->Read(&aSampleDesc,sizeof(SampleDescBase));
		aSampleDesc.Size = B_BENDIAN_TO_HOST_INT32(aSampleDesc.Size);
		aSampleDesc.DataFormat = B_BENDIAN_TO_HOST_INT32(aSampleDesc.DataFormat);
		aSampleDesc.DataReference = B_BENDIAN_TO_HOST_INT16(aSampleDesc.DataReference);
		
		descBytesLeft = aSampleDesc.Size - sizeof(SampleDescBase);
		
		// Hmm this should be ok if each stsd describes only a single type
		DataFormat = aSampleDesc.DataFormat;
		
		if (IsAudioSampleDesc(aSampleDesc.DataFormat)) {
			// Read in Audio Sample Data
			// We place into a V1 description even though it might be a V0 or earlier
			SoundDescriptionV1 *aSoundDescriptionV1;
			
			aSoundDescriptionV1 = new SoundDescriptionV1;
			aSoundDescriptionV1->basefields.Size = aSampleDesc.Size;
			aSoundDescriptionV1->basefields.DataFormat = aSampleDesc.DataFormat;
			aSoundDescriptionV1->basefields.DataReference = aSampleDesc.DataReference;
			
			theStream->Read(&aSoundDescriptionV1->desc,sizeof(SoundDescription));
			descBytesLeft = descBytesLeft - sizeof(SoundDescription);
			
			aSoundDescriptionV1->desc.Version = B_BENDIAN_TO_HOST_INT16(aSoundDescriptionV1->desc.Version);
			aSoundDescriptionV1->desc.Revision = B_BENDIAN_TO_HOST_INT16(aSoundDescriptionV1->desc.Revision);
			aSoundDescriptionV1->desc.Vendor = B_BENDIAN_TO_HOST_INT32(aSoundDescriptionV1->desc.Vendor);
			aSoundDescriptionV1->desc.NoOfChannels = B_BENDIAN_TO_HOST_INT16(aSoundDescriptionV1->desc.NoOfChannels);
			aSoundDescriptionV1->desc.SampleSize = B_BENDIAN_TO_HOST_INT16(aSoundDescriptionV1->desc.SampleSize);
			aSoundDescriptionV1->desc.CompressionID = B_BENDIAN_TO_HOST_INT16(aSoundDescriptionV1->desc.CompressionID);
			aSoundDescriptionV1->desc.PacketSize = B_BENDIAN_TO_HOST_INT16(aSoundDescriptionV1->desc.PacketSize);
			aSoundDescriptionV1->desc.SampleRate = B_BENDIAN_TO_HOST_INT32(aSoundDescriptionV1->desc.SampleRate);

			if (aSoundDescriptionV1->desc.Version == 1) {
				theStream->Read(&(aSoundDescriptionV1->samplesPerPacket),4);
				theStream->Read(&(aSoundDescriptionV1->bytesPerPacket),4);
				theStream->Read(&(aSoundDescriptionV1->bytesPerFrame),4);
				theStream->Read(&(aSoundDescriptionV1->bytesPerSample),4);
				
				aSoundDescriptionV1->samplesPerPacket = B_BENDIAN_TO_HOST_INT32(aSoundDescriptionV1->samplesPerPacket);
				aSoundDescriptionV1->bytesPerPacket = B_BENDIAN_TO_HOST_INT32(aSoundDescriptionV1->bytesPerPacket);
				aSoundDescriptionV1->bytesPerFrame = B_BENDIAN_TO_HOST_INT32(aSoundDescriptionV1->bytesPerFrame);
				aSoundDescriptionV1->bytesPerSample = B_BENDIAN_TO_HOST_INT32(aSoundDescriptionV1->bytesPerSample);
				
				descBytesLeft = descBytesLeft - 16;
			} else {
				// Calculate?
				aSoundDescriptionV1->samplesPerPacket = 0;
				aSoundDescriptionV1->bytesPerPacket = 0;
				aSoundDescriptionV1->bytesPerFrame = 0;
				aSoundDescriptionV1->bytesPerSample = 0;
			}

			// 0 means we dont have one
			aSoundDescriptionV1->theWaveFormat.format_tag = 0;

			while (descBytesLeft > 0) {

				// More extended atoms
				AtomBase *aAtomBase = getAtom(theStream);
				
				aAtomBase->OnProcessMetaData();
				printf("%s\n",aAtomBase->getAtomName());

				if (dynamic_cast<WAVEAtom *>(aAtomBase)) {
					// WAVE atom
					aSoundDescriptionV1->theWaveFormat = dynamic_cast<WAVEAtom *>(aAtomBase)->getWaveFormat();
				}
				
				if (aAtomBase->getAtomSize() > 0) {
					descBytesLeft = descBytesLeft - aAtomBase->getAtomSize();
				} else {
					printf("Invalid Atom found when reading Sound Description\n");
					descBytesLeft = 0;
				}
				
				delete aAtomBase;
				
			}
			
			theAudioDescArray[i] = aSoundDescriptionV1;

			theStream->Seek(descBytesLeft,0);
			
		} else {
			if (IsVideoSampleDesc(aSampleDesc.DataFormat)) {
				// read in Video Sample Data
				VideoDescriptionV0 *aVideoDescription;
				
				aVideoDescription = new VideoDescriptionV0;

				aVideoDescription->basefields.Size = aSampleDesc.Size;
				aVideoDescription->basefields.DataFormat = aSampleDesc.DataFormat;
				aVideoDescription->basefields.DataReference = aSampleDesc.DataReference;

				theStream->Read(&(aVideoDescription->desc),sizeof(VideoDescription));
				
				aVideoDescription->desc.Version = B_BENDIAN_TO_HOST_INT16(aVideoDescription->desc.Version);
				aVideoDescription->desc.Vendor = B_BENDIAN_TO_HOST_INT32(aVideoDescription->desc.Vendor);
				aVideoDescription->desc.TemporaralQuality = B_BENDIAN_TO_HOST_INT32(aVideoDescription->desc.TemporaralQuality);
				aVideoDescription->desc.SpacialQuality = B_BENDIAN_TO_HOST_INT32(aVideoDescription->desc.SpacialQuality);
				aVideoDescription->desc.Width = B_BENDIAN_TO_HOST_INT16(aVideoDescription->desc.Width);
				aVideoDescription->desc.Height = B_BENDIAN_TO_HOST_INT16(aVideoDescription->desc.Height);
				aVideoDescription->desc.HorizontalResolution = B_BENDIAN_TO_HOST_INT32(aVideoDescription->desc.HorizontalResolution);
				aVideoDescription->desc.VerticalResolution = B_BENDIAN_TO_HOST_INT32(aVideoDescription->desc.VerticalResolution);
				aVideoDescription->desc.DataSize = B_BENDIAN_TO_HOST_INT32(aVideoDescription->desc.DataSize);
				// This framecount never seems to be right
				aVideoDescription->desc.FrameCount = B_BENDIAN_TO_HOST_INT16(aVideoDescription->desc.FrameCount);
				aVideoDescription->desc.Depth = B_BENDIAN_TO_HOST_INT16(aVideoDescription->desc.Depth);
				aVideoDescription->desc.ColourTableID = B_BENDIAN_TO_HOST_INT16(aVideoDescription->desc.ColourTableID);
				
				descBytesLeft = descBytesLeft - sizeof(VideoDescription);

				theVideoDescArray[i] = aVideoDescription;

				// We seem to have read 2 bytes too many???
				theStream->Seek(descBytesLeft,0);
			}
		}

		printf("Size:Format=%ld:%ld %ld\n",aSampleDesc.Size,aSampleDesc.DataFormat,descBytesLeft);
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
	theStream->Read(&theWaveFormat,sizeof(wave_format_ex));

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

FTYPAtom::FTYPAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

FTYPAtom::~FTYPAtom()
{
}

void FTYPAtom::OnProcessMetaData()
{
}

char *FTYPAtom::OnGetAtomName()
{
	return "Graphics export file type Atom";
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
	theStream->Read(&theHeader,sizeof(tkhd));

	theHeader.CreationTime = B_BENDIAN_TO_HOST_INT32(theHeader.CreationTime);
	theHeader.ModificationTime = B_BENDIAN_TO_HOST_INT32(theHeader.ModificationTime);
	theHeader.TrackID = B_BENDIAN_TO_HOST_INT32(theHeader.TrackID);
	theHeader.Duration = B_BENDIAN_TO_HOST_INT32(theHeader.Duration);
	theHeader.Layer = B_BENDIAN_TO_HOST_INT16(theHeader.Layer);
	theHeader.AlternateGroup = B_BENDIAN_TO_HOST_INT16(theHeader.AlternateGroup);
	theHeader.Volume = B_BENDIAN_TO_HOST_INT16(theHeader.Volume);
	theHeader.TrackWidth = B_BENDIAN_TO_HOST_INT32(theHeader.TrackWidth);
	theHeader.TrackHeight = B_BENDIAN_TO_HOST_INT32(theHeader.TrackHeight);
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

MDHDAtom::MDHDAtom(BPositionIO *pStream, off_t pstreamOffset, uint32 patomType, uint64 patomSize) : AtomBase(pStream, pstreamOffset, patomType, patomSize)
{
}

MDHDAtom::~MDHDAtom()
{
}

void MDHDAtom::OnProcessMetaData()
{
	theStream->Read(&theHeader,sizeof(mdhd));
	
	theHeader.CreationTime = B_BENDIAN_TO_HOST_INT32(theHeader.CreationTime);
	theHeader.ModificationTime = B_BENDIAN_TO_HOST_INT32(theHeader.ModificationTime);
	theHeader.TimeScale = B_BENDIAN_TO_HOST_INT32(theHeader.TimeScale);
	theHeader.Duration = B_BENDIAN_TO_HOST_INT32(theHeader.Duration);
	theHeader.Language = B_BENDIAN_TO_HOST_INT16(theHeader.Language);
	theHeader.Quality = B_BENDIAN_TO_HOST_INT16(theHeader.Quality);
}

char *MDHDAtom::OnGetAtomName()
{
	return "Quicktime Media Header";
}

bigtime_t	MDHDAtom::getDuration() 
{
	return (bigtime_t(theHeader.Duration) * 1000000) / (theHeader.TimeScale);
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
	hdlr aHeader;
	theStream->Read(&aHeader,sizeof(hdlr));
	// Read Array of Strings?
}

char *HDLRAtom::OnGetAtomName()
{
	return "Quicktime Handler Reference Atom ";
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
	theStream->Read(&aHeader,sizeof(vmhd));
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
	theStream->Read(&aHeader,sizeof(smhd));
}

char *SMHDAtom::OnGetAtomName()
{
	return "Quicktime Sound Media Header";
}
