#include <iostream.h>

#include <DataIO.h>
#include <SupportKit.h>

#include "MOVParser.h"
#include "MOVFileReader.h"

extern AtomBase *getAtom(BPositionIO *pStream);

MOVFileReader::MOVFileReader(BPositionIO *pStream)
{
	theStream = pStream;
	
	// Find Size of Stream, need to rethink this for non seekable streams
	theStream->Seek(0,SEEK_END);
	StreamSize = theStream->Position();
	theStream->Seek(0,SEEK_SET);
	TotalChildren = 0;
	
	theMVHDAtom = NULL;
}

MOVFileReader::~MOVFileReader()
{
	theStream = NULL;
	theMVHDAtom = NULL;
}

bool MOVFileReader::IsEndOfFile(off_t pPosition)
{
	return (pPosition >= StreamSize);
}

bool MOVFileReader::IsEndOfFile()
{
	return (theStream->Position() >= StreamSize);
}

bool MOVFileReader::AddChild(AtomBase *pChildAtom)
{
	if (pChildAtom) {
		atomChildren[TotalChildren++] = pChildAtom;
		return true;
	}
	return false;
}

AtomBase *MOVFileReader::GetChildAtom(uint32 patomType, uint32 offset)
{
	for (uint32 i=0;i<TotalChildren;i++) {
		if (atomChildren[i]->IsType(patomType)) {
			// found match, skip if offset non zero.
			if (offset == 0) {
				return atomChildren[i];
			} else {
				offset--;
			}
		} else {
			if (atomChildren[i]->IsContainer()) {
				// search container
				AtomBase *aAtomBase = (dynamic_cast<AtomContainer *>(atomChildren[i])->GetChildAtom(patomType, offset));
				if (aAtomBase) {
					// found in container
					return aAtomBase;
				}
				// not found
			}
		}
	}
	return NULL;
}

uint32	MOVFileReader::CountChildAtoms(uint32 patomType)
{
	uint32 count = 0;

	while (GetChildAtom(patomType, count) != NULL) {
		count++;
	}
	return count;
}

MVHDAtom	*MOVFileReader::getMVHDAtom()
{
AtomBase *aAtomBase;

	if (theMVHDAtom == NULL) {
		aAtomBase = GetChildAtom(uint32('mvhd'));
		theMVHDAtom = dynamic_cast<MVHDAtom *>(aAtomBase);
	}

	// Assert(theMVHDAtom != NULL,"Movie has no movie header atom");
	return theMVHDAtom;
}

uint32 MOVFileReader::getMovieTimeScale()
{
	return getMVHDAtom()->getTimeScale();
}

bigtime_t	MOVFileReader::getMovieDuration()
{
	return ((bigtime_t(getMVHDAtom()->getDuration()) * 1000000) / getMovieTimeScale());
}

uint32	MOVFileReader::getStreamCount()
{
	// count the number of tracks in the file
	return (CountChildAtoms(uint32('trak')));
}

bigtime_t	MOVFileReader::getVideoDuration()
{
AtomBase *aAtomBase;
uint32 TimeScale = getMovieTimeScale();

	for (uint32 i=0;i<getStreamCount();i++) {
		aAtomBase = GetChildAtom(uint32('trak'),i);
	
		if ((aAtomBase) && (dynamic_cast<TRAKAtom *>(aAtomBase)->IsVideo())) {
			return (dynamic_cast<TRAKAtom *>(aAtomBase)->Duration(TimeScale));
		}
	}
	
	return 0;
}

bigtime_t	MOVFileReader::getAudioDuration()
{
AtomBase *aAtomBase;
uint32 TimeScale = getMovieTimeScale();

	for (uint32 i=0;i<getStreamCount();i++) {
		aAtomBase = GetChildAtom(uint32('trak'),i);
	
		if ((aAtomBase) && (dynamic_cast<TRAKAtom *>(aAtomBase)->IsAudio())) {
			return (dynamic_cast<TRAKAtom *>(aAtomBase)->Duration(TimeScale));
		}
	}
	
	return 0;
}

bigtime_t	MOVFileReader::getMaxDuration()
{
	return MAX(getVideoDuration(),getAudioDuration());
}

uint32	MOVFileReader::getVideoFrameCount()
{
AtomBase *aAtomBase;

	for (uint32 i=0;i<getStreamCount();i++) {
		aAtomBase = GetChildAtom(uint32('trak'),i);
	
		if ((aAtomBase) && (dynamic_cast<TRAKAtom *>(aAtomBase)->IsVideo())) {

			aAtomBase = dynamic_cast<TRAKAtom *>(aAtomBase)->GetChildAtom(uint32('stts'),0);
			STTSAtom *aSTTSAtom = dynamic_cast<STTSAtom *>(aAtomBase);

			return aSTTSAtom->getSUMCounts();
		}
	}
	
	return 1;
}

uint32	MOVFileReader::getAudioFrameCount()
{

AtomBase *aAtomBase;

	for (uint32 i=0;i<getStreamCount();i++) {
		aAtomBase = GetChildAtom(uint32('trak'),i);
	
		if ((aAtomBase) && (dynamic_cast<TRAKAtom *>(aAtomBase)->IsAudio())) {
			return uint32(((getAudioDuration() * AudioFormat(i)->SampleRate) / 1000000L) + 0.5);
		}
	}
	
	return 1;
}

bool	MOVFileReader::IsVideo(uint32 stream_index)
{
	// Look for a trak with a vmhd atom

	AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	
	if (aAtomBase) {
		return (dynamic_cast<TRAKAtom *>(aAtomBase)->IsVideo());
	}
	
	// No trak
	return false;
}

bool	MOVFileReader::IsAudio(uint32 stream_index)
{
	// Look for a trak with a smhd atom

	AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	
	if (aAtomBase) {
		return (dynamic_cast<TRAKAtom *>(aAtomBase)->IsAudio());
	}
	
	// No trak
	return false;
}

uint32	MOVFileReader::getNoFramesInChunk(uint32 stream_index, uint32 pFrameNo)
{
	// Find Track
	AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	if (aAtomBase) {
		TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);

		uint32 SampleNo = aTrakAtom->getSampleForFrame(pFrameNo);

		if (IsAudio(stream_index)) {
			uint32 OffsetInChunk;
			uint32 ChunkNo = aTrakAtom->getChunkForSample(SampleNo, &OffsetInChunk);

			// Go back to first Sample in Chunk
			while (aTrakAtom->getChunkForSample(SampleNo-1, &OffsetInChunk) == ChunkNo) {
				SampleNo--;
			}

			// Add up all sample sizes in chunk
//			uint32 ChunkSize = 0;
		
			while (aTrakAtom->getChunkForSample(SampleNo, &OffsetInChunk) == ChunkNo) {
//				ChunkSize += aTrakAtom->getSizeForSample(SampleNo);
				SampleNo++;
			}
			return SampleNo ;
		}
		
		if (IsVideo(stream_index)) {
			return 1;
		}

	}
	
	return 0;
}

uint64	MOVFileReader::getOffsetForFrame(uint32 stream_index, uint32 pFrameNo)
{

	// Find Track
	AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	if (aAtomBase) {
		TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);

		// Convert Frame to Track Time
//		uint32 FrameTime = aTrakAtom->getTimeForFrame(pFrameNo, getMovieTimeScale());
		// Get Sample for Frame
		uint32 SampleNo = aTrakAtom->getSampleForFrame(pFrameNo);
		// Get Chunk For Sample and the offset for the frame within that chunk
		uint32 OffsetInChunk;
		uint32 ChunkNo = aTrakAtom->getChunkForSample(SampleNo, &OffsetInChunk);
		// Get Offset For Chunk
		uint64 OffsetNo = aTrakAtom->getOffsetForChunk(ChunkNo);

//		printf("Frame %ld {%lld} <%ld/%ld> (",pFrameNo, OffsetNo, ChunkNo, OffsetInChunk);

		uint32 SampleSize;
		// Adjust the Offset for the Offset in the chunk		
		for (uint32 i=1;i<=OffsetInChunk;i++) {
			SampleSize = aTrakAtom->getSizeForSample(SampleNo-i);
//			printf(" %ld ",SampleSize);
			OffsetNo = OffsetNo + SampleSize;
		}
//		printf(") %lld\n",OffsetNo);
		
//		printf("%ld:%ld:%ld:%lld\n",pFrameNo, SampleNo, ChunkNo, OffsetNo);
		
		return OffsetNo;
	}
	
	return 0;
}

status_t	MOVFileReader::ParseFile()
{
	AtomBase *aChild;
	while (IsEndOfFile() == false) {
		aChild = getAtom(theStream);
		if (AddChild(aChild)) {
			aChild->ProcessMetaData();
		}
	}

	// Debug info
	for (uint32 i=0;i<TotalChildren;i++) {
		atomChildren[i]->DisplayAtoms();
	}
	
	return B_OK;
}

const 	mov_main_header		*MOVFileReader::MovMainHeader()
{
	// Fill In theMainHeader
//	uint32 micro_sec_per_frame;
//	uint32 max_bytes_per_sec;
//	uint32 padding_granularity;
//	uint32 flags;
//	uint32 total_frames;
//	uint32 initial_frames;
//	uint32 streams;
//	uint32 suggested_buffer_size;
//	uint32 width;
//	uint32 height;

	uint32 videoStream = 0;

	theMainHeader.streams = getStreamCount();
	theMainHeader.flags = 0;
	theMainHeader.initial_frames = 0;

	while (	videoStream < theMainHeader.streams ) {
		if (IsVideo(videoStream)) {
			break;
		}
		videoStream++;
	}

	if (videoStream >= theMainHeader.streams) {
		theMainHeader.width = 0;
		theMainHeader.height = 0;
		theMainHeader.total_frames = 0;
		theMainHeader.suggested_buffer_size = 0;
		theMainHeader.micro_sec_per_frame = 0;
	} else {
		theMainHeader.width = VideoFormat(videoStream)->width;
		theMainHeader.height = VideoFormat(videoStream)->height;
		theMainHeader.total_frames = getVideoFrameCount();
		theMainHeader.suggested_buffer_size = theMainHeader.width * theMainHeader.height * VideoFormat(videoStream)->bit_count / 8;
		theMainHeader.micro_sec_per_frame = uint32(1000000.0 / VideoFormat(videoStream)->FrameRate);
	}
	
	theMainHeader.padding_granularity = 0;
	theMainHeader.max_bytes_per_sec = 0;

	return &theMainHeader;
}

const 	AudioMetaData 		*MOVFileReader::AudioFormat(uint32 stream_index, size_t *size = 0)
{
	if (IsAudio(stream_index)) {

		AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);

		if (aAtomBase) {
			TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);
			
			aAtomBase = aTrakAtom->GetChildAtom(uint32('stsd'),0);
			if (aAtomBase) {
				STSDAtom *aSTSDAtom = dynamic_cast<STSDAtom *>(aAtomBase);

				// Fill In a wave_format_ex structure
				SoundDescriptionV1 aSoundDescription = aSTSDAtom->getAsAudio();

				// Hmm 16bit format 32 bit FourCC.
				theAudio.compression = aSoundDescription.basefields.DataFormat;

				theAudio.NoOfChannels = aSoundDescription.desc.NoOfChannels;
				theAudio.SampleSize = aSoundDescription.desc.SampleSize;
				theAudio.SampleRate = aSoundDescription.desc.SampleRate / 65536;	// Convert from fixed point decimal to float
				theAudio.PacketSize = uint32((theAudio.SampleRate * theAudio.NoOfChannels * theAudio.SampleSize) / 8);
			
//			// Do we have a wave structure
//			if ((aSoundDescription.samplesPerPacket == 0) && (aSoundDescription.theWaveFormat.format_tag != 0)) {
//					theAudio.PacketSize = aSoundDescription.theWaveFormat.frames_per_sec;
//			} else {
//				// no wave structure need to calculate these
//				theAudio.PacketSize = uint32((theAudio.SampleRate * theAudio.NoOfChannels * theAudio.SampleSize) / 8);
//			}
			
				return &theAudio;
			}
		}
	}
	
	return NULL;
}

const 	VideoMetaData	*MOVFileReader::VideoFormat(uint32 stream_index)
{
	if (IsVideo(stream_index)) {

		AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);

		if (aAtomBase) {
			TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);
			
			aAtomBase = aTrakAtom->GetChildAtom(uint32('stsd'),0);
			
			if (aAtomBase) {
				STSDAtom *aSTSDAtom = dynamic_cast<STSDAtom *>(aAtomBase);
			
				VideoDescriptionV0 aVideoDescriptionV0 = aSTSDAtom->getAsVideo();
	
				theVideo.width = aVideoDescriptionV0.desc.Width;
				theVideo.height = aVideoDescriptionV0.desc.Height;
				theVideo.size = aVideoDescriptionV0.desc.DataSize;
				theVideo.planes = aVideoDescriptionV0.desc.Depth;
				theVideo.bit_count = aVideoDescriptionV0.desc.Depth;
				theVideo.compression = aVideoDescriptionV0.basefields.DataFormat;
				theVideo.image_size = aVideoDescriptionV0.desc.Height * aVideoDescriptionV0.desc.Width;
				theVideo.HorizontalResolution = aVideoDescriptionV0.desc.HorizontalResolution;
				theVideo.VerticalResolution = aVideoDescriptionV0.desc.VerticalResolution;

				aAtomBase = aTrakAtom->GetChildAtom(uint32('stts'),0);
				if (aAtomBase) {
					STTSAtom *aSTTSAtom = dynamic_cast<STTSAtom *>(aAtomBase);

					theVideo.FrameRate = ((aSTTSAtom->getSUMCounts() * 1000000.0L) / getVideoDuration());
			
					return &theVideo;
				}
			}
		}
	}
	
	return NULL;
}

const 	mov_stream_header	*MOVFileReader::StreamFormat(uint32 stream_index)
{
	// Fill In a Stream Header
	theStreamHeader.length = 0;

	if (IsVideo(stream_index)) {
		theStreamHeader.rate = 0;
		theStreamHeader.scale = 1000000;
		theStreamHeader.length = getVideoFrameCount();
	}
	
	if (IsAudio(stream_index)) {
		theStreamHeader.rate = AudioFormat(stream_index)->SampleRate;
		theStreamHeader.scale = 1;
		theStreamHeader.length = getAudioFrameCount();
		theStreamHeader.sample_size = AudioFormat(stream_index)->SampleSize;
		theStreamHeader.suggested_buffer_size =	theStreamHeader.rate * theStreamHeader.sample_size;
	}
	
	return &theStreamHeader;
}

uint32	MOVFileReader::getChunkSize(uint32 stream_index, uint32 pFrameNo)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	if (aAtomBase) {
		TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);

		uint32 SampleNo = aTrakAtom->getSampleForFrame(pFrameNo);

		if (IsAudio(stream_index)) {
			uint32 OffsetInChunk;
			uint32 ChunkNo = aTrakAtom->getChunkForSample(SampleNo, &OffsetInChunk);

			// Go back to first Sample in Chunk
			while (aTrakAtom->getChunkForSample(SampleNo-1, &OffsetInChunk) == ChunkNo) {
				SampleNo--;
			}

			// Add up all sample sizes in chunk
			uint32 ChunkSize = 0;
		
			while (aTrakAtom->getChunkForSample(SampleNo, &OffsetInChunk) == ChunkNo) {
				ChunkSize += aTrakAtom->getSizeForSample(SampleNo);
				SampleNo++;
			}
			return ChunkSize;
		}
		
		if (IsVideo(stream_index)) {
			return aTrakAtom->getSizeForSample(SampleNo);
		}

	}

	return 0;
}

bool	MOVFileReader::IsKeyFrame(uint32 stream_index, uint32 pFrameNo)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	if (aAtomBase) {
		TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);
		
		uint32 SampleNo = aTrakAtom->getSampleForFrame(pFrameNo);
		return aTrakAtom->IsSyncSample(SampleNo);
	}

	return false;
}

bool	MOVFileReader::GetNextChunkInfo(uint32 stream_index, uint32 pFrameNo, off_t *start, uint32 *size, bool *keyframe)
{

	*start = getOffsetForFrame(stream_index, pFrameNo);
	*size = getChunkSize(stream_index, pFrameNo);
	*keyframe = IsKeyFrame(stream_index, pFrameNo);

	return !(IsEndOfFile(*start) || *start < 0);
}

/* static */
bool	MOVFileReader::IsSupported(BPositionIO *source)
{
	AtomBase *aAtom;
	
	aAtom = getAtom(source);
	if (aAtom) {
		return (aAtom->IsKnown());
	}
	return false;
}
