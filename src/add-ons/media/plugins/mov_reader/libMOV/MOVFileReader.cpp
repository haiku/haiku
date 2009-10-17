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


#include "MOVParser.h"
#include "MOVFileReader.h"

#include <DataIO.h>
#include <SupportKit.h>

#include <iostream>


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

bool MOVFileReader::IsEndOfData(off_t pPosition)
{
AtomBase	*aAtomBase;

	for (uint32 index=0;index<CountChildAtoms('mdat');index++) {
		aAtomBase = GetChildAtom(uint32('mdat'),index);
		if ((aAtomBase) && (aAtomBase->getAtomSize() > 8)) {
			MDATAtom *aMDATAtom = dynamic_cast<MDATAtom *>(aAtomBase);
			if (pPosition >= aMDATAtom->getAtomOffset() && pPosition <= aMDATAtom->getEOF()) {
				return false;
			}
		}
	}

	return true;
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
	return ((bigtime_t(getMVHDAtom()->getDuration()) * 1000000L) / getMovieTimeScale());
}

uint32	MOVFileReader::getStreamCount()
{
	// count the number of tracks in the file
	return (CountChildAtoms(uint32('trak')));
}

bigtime_t	MOVFileReader::getVideoDuration(uint32 stream_index)
{
AtomBase *aAtomBase;

	aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	
	if ((aAtomBase) && (dynamic_cast<TRAKAtom *>(aAtomBase)->IsVideo())) {
		return (dynamic_cast<TRAKAtom *>(aAtomBase)->Duration(1));
	}
	
	return 0;
}

bigtime_t	MOVFileReader::getAudioDuration(uint32 stream_index)
{
AtomBase *aAtomBase;

	aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	
	if ((aAtomBase) && (dynamic_cast<TRAKAtom *>(aAtomBase)->IsAudio())) {
		return (dynamic_cast<TRAKAtom *>(aAtomBase)->Duration(1));
	}
	
	return 0;
}

bigtime_t	MOVFileReader::getMaxDuration()
{
AtomBase *aAtomBase;
int32	video_index,audio_index;
	video_index = -1;
	audio_index = -1;

	// find the active video and audio tracks
	for (uint32 i=0;i<getStreamCount();i++) {
		aAtomBase = GetChildAtom(uint32('trak'),i);
		
		if ((aAtomBase) && (dynamic_cast<TRAKAtom *>(aAtomBase)->IsActive())) {
			if (dynamic_cast<TRAKAtom *>(aAtomBase)->IsAudio()) {
				audio_index = int32(i);
			}
			if (dynamic_cast<TRAKAtom *>(aAtomBase)->IsVideo()) {
				video_index = int32(i);
			}
		}
	}

	if ((video_index >= 0) && (audio_index >= 0)) {
		return MAX(getVideoDuration(video_index),getAudioDuration(audio_index));
	}
	if ((video_index < 0) && (audio_index >= 0)) {
		return getAudioDuration(audio_index);
	}
	if ((video_index >= 0) && (audio_index < 0)) {
		return getVideoDuration(video_index);
	}
	
	return 0;
}

uint32	MOVFileReader::getVideoFrameCount(uint32 stream_index)
{
AtomBase *aAtomBase;

	aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	
	if ((aAtomBase) && (dynamic_cast<TRAKAtom *>(aAtomBase)->IsVideo())) {

		return dynamic_cast<TRAKAtom *>(aAtomBase)->FrameCount();
	}
	
	return 1;
}

uint32	MOVFileReader::getAudioFrameCount(uint32 stream_index)
{
	if (IsAudio(stream_index)) {
		return uint32(((getAudioDuration(stream_index) * AudioFormat(stream_index)->SampleRate) / 1000000L) + 0.5);
	}
	
	return 0;
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

uint32	MOVFileReader::getFirstFrameInChunk(uint32 stream_index, uint32 pChunkID)
{
	// Find Track
	AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	if (aAtomBase) {
		TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);
		
		return aTrakAtom->getFirstSampleInChunk(pChunkID);
	}
	
	return 0;
}

uint32	MOVFileReader::getNoFramesInChunk(uint32 stream_index, uint32 pChunkID)
{
	// Find Track
	AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	if (aAtomBase) {
		TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);

		return aTrakAtom->getNoSamplesInChunk(pChunkID);
	}
	
	return 0;
}

uint64	MOVFileReader::getOffsetForFrame(uint32 stream_index, uint32 pFrameNo, uint32 *chunkFrameCount)
{
	// Find Track
	AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	if (aAtomBase) {
		TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);

		if (pFrameNo < aTrakAtom->FrameCount()) {
			// Get Sample for Frame
			uint32 SampleNo = aTrakAtom->getSampleForFrame(pFrameNo);
			// Get Chunk For Sample and the offset for the frame within that chunk
			uint32 OffsetInChunk;
			uint32 ChunkID = aTrakAtom->getChunkForSample(SampleNo, &OffsetInChunk);
			// Get Offset For Chunk
			uint64 OffsetNo = aTrakAtom->getOffsetForChunk(ChunkID);

			if (IsAudio(stream_index)) {
				// For audio we return all frames in Chunk less any offset
				*chunkFrameCount = aTrakAtom->getNoSamplesInChunk(ChunkID) - OffsetInChunk;
			} else {
				// For video we always return 1 frame at a time
				*chunkFrameCount = 1;
			}

			if (ChunkID != 0) {
				uint32 SampleSize;
				// Adjust the Offset for the Offset in the chunk
				if (aTrakAtom->IsSingleSampleSize()) {
					SampleSize = aTrakAtom->getSizeForSample(SampleNo);
					OffsetNo = OffsetNo + (OffsetInChunk * SampleSize);
				} else {
					// This is bad news performance wise
					for (uint32 i=1;i<=OffsetInChunk;i++) {
						SampleSize = aTrakAtom->getSizeForSample(SampleNo - i);
						OffsetNo = OffsetNo + SampleSize;
					}
				}
			}
		
			return OffsetNo;
		}
	}
	
	return 0;
}

void	MOVFileReader::BuildSuperIndex()
{
AtomBase *aAtomBase;

	for (uint32 stream=0;stream<getStreamCount();stream++) {
		aAtomBase = GetChildAtom(uint32('trak'),stream);
		if (aAtomBase) {
			TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);
			for (uint32 chunkid=1;chunkid<=aTrakAtom->getTotalChunks();chunkid++) {
				theChunkSuperIndex.AddChunkIndex(stream,chunkid,aTrakAtom->getOffsetForChunk(chunkid));
			}
		}
	}
	
	// Add end of file to index
	aAtomBase = GetChildAtom(uint32('mdat'),0);
	if (aAtomBase) {
		MDATAtom *aMdatAtom = dynamic_cast<MDATAtom *>(aAtomBase);
		theChunkSuperIndex.AddChunkIndex(0,0,aMdatAtom->getEOF());
	}
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
	
	BuildSuperIndex();
	
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
		if (IsVideo(videoStream) && IsActive(videoStream)) {
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
		theMainHeader.total_frames = getVideoFrameCount(videoStream);
		theMainHeader.suggested_buffer_size = theMainHeader.width * theMainHeader.height * VideoFormat(videoStream)->bit_count / 8;
		theMainHeader.micro_sec_per_frame = uint32(1000000.0 / VideoFormat(videoStream)->FrameRate);
	}
	
	theMainHeader.padding_granularity = 0;
	theMainHeader.max_bytes_per_sec = 0;

	return &theMainHeader;
}

const 	AudioMetaData 		*MOVFileReader::AudioFormat(uint32 stream_index)
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

				if (aSoundDescription.bytesPerFrame == 0) {
					// XXX probably not right
					theAudio.FrameSize = aSoundDescription.desc.SampleSize / 8;
				} else {
					theAudio.FrameSize = aSoundDescription.bytesPerFrame;
				}
				
				if (aSoundDescription.bytesPerPacket == 0) {
					theAudio.BufferSize = uint32((theAudio.SampleSize * theAudio.NoOfChannels * theAudio.FrameSize) / 8);
				} else {
					theAudio.BufferSize = aSoundDescription.bytesPerPacket;
				}
				
				theAudio.BitRate = theAudio.SampleSize * theAudio.NoOfChannels * theAudio.SampleRate;

				theAudio.theVOL = aSoundDescription.theVOL;
				theAudio.VOLSize = aSoundDescription.VOLSize;
			
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

				theVideo.theVOL = aVideoDescriptionV0.theVOL;
				theVideo.VOLSize = aVideoDescriptionV0.VOLSize;

				aAtomBase = aTrakAtom->GetChildAtom(uint32('stts'),0);
				if (aAtomBase) {
					STTSAtom *aSTTSAtom = dynamic_cast<STTSAtom *>(aAtomBase);

					theVideo.FrameRate = ((aSTTSAtom->getSUMCounts() * 1000000.0L) / aTrakAtom->Duration(1));
			
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
	
	if (IsActive(stream_index) == false) {
		return NULL;
	}

	if (IsVideo(stream_index)) {
		theStreamHeader.rate = uint32(1000000L*VideoFormat(stream_index)->FrameRate);
		theStreamHeader.scale = 1000000L;
		theStreamHeader.length = getVideoFrameCount(stream_index);
	}
	
	if (IsAudio(stream_index)) {
		theStreamHeader.rate = uint32(AudioFormat(stream_index)->SampleRate);
		theStreamHeader.scale = 1;
		theStreamHeader.length = getAudioFrameCount(stream_index);
		theStreamHeader.sample_size = AudioFormat(stream_index)->SampleSize;
		theStreamHeader.suggested_buffer_size =	theStreamHeader.rate * theStreamHeader.sample_size;
	}
	
	return &theStreamHeader;
}


uint32
MOVFileReader::getChunkSize(uint32 streamIndex, uint32 frameNo)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('trak'), streamIndex);
	if (aAtomBase == NULL)
		return 0;

	TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);

	if (frameNo < aTrakAtom->FrameCount()) {
		uint32 SampleNo = aTrakAtom->getSampleForFrame(frameNo);
		
		if (IsAudio(streamIndex)) {
			// Audio Chunk Size is all samples in the chunk
			// Get Chunk For Sample and the offset for the frame within that chunk
			uint32 OffsetInChunk;
			uint32 ChunkID = aTrakAtom->getChunkForSample(SampleNo, &OffsetInChunk);

			off_t chunkStart = aTrakAtom->getOffsetForChunk(ChunkID);
			return theChunkSuperIndex.getChunkSize(streamIndex, ChunkID, chunkStart);
		}

		if (IsVideo(streamIndex)) {
			// We read video in Sample by Sample so chunk size is Sample Size
			return aTrakAtom->getSizeForSample(SampleNo);
		}
	}

	return 0;
}


bool
MOVFileReader::IsKeyFrame(uint32 stream_index, uint32 pFrameNo)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	if (aAtomBase) {
		TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);
		
		uint32 SampleNo = aTrakAtom->getSampleForFrame(pFrameNo);
		return aTrakAtom->IsSyncSample(SampleNo);
	}

	return false;
}

bool	MOVFileReader::GetNextChunkInfo(uint32 stream_index, uint32 pFrameNo, off_t *start, uint32 *size, bool *keyframe, uint32 *chunkFrameCount)
{
	*start = getOffsetForFrame(stream_index, pFrameNo, chunkFrameCount);
	*size = getChunkSize(stream_index, pFrameNo);
	
	if ((*start > 0) && (*size > 0)) {
		*keyframe = IsKeyFrame(stream_index, pFrameNo);
	}

	printf("(%ld) frame %ld start %Ld, size %ld, eof %s, eod %s\n",stream_index, pFrameNo,*start,*size, IsEndOfFile(*start + *size) ? "true" : "false", IsEndOfData(*start + *size) ? "true" : "false");

	// TODO need a better method for detecting End of Data
	if (IsEndOfFile(*start + *size) || IsEndOfData(*start + *size)) {
		return false;
	}
	
	return *start > 0 && *size > 0;
}

bool	MOVFileReader::IsActive(uint32 stream_index)
{
	AtomBase *aAtomBase = GetChildAtom(uint32('trak'),stream_index);
	if (aAtomBase) {
		TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);
		return aTrakAtom->IsActive();
	}
	
	return false;
}

/* static */
bool	MOVFileReader::IsSupported(BPositionIO *source)
{
	// MOV files normally do not have ftyp atoms
	// But when they do we need to check if they have a qt brand
	// No qt brand means the file is likely to be a MP4 file
	
	AtomBase *aAtom;
	
	aAtom = getAtom(source);

	if (aAtom) {
		if (dynamic_cast<FTYPAtom *>(aAtom)) {
			printf("ftyp atom found checking for qt brand\n");
			aAtom->ProcessMetaData();
			// MP4 files start with a ftyp atom that contains an isom brand
			// MOV files with a ftyp atom contain the qt brand
			return dynamic_cast<FTYPAtom *>(aAtom)->HasBrand(uint32('qt  '));
		} else {
			// no ftyp atom so just see if we know the atom we have
			return (aAtom->IsKnown());
		}
	}
	
	return false;
}
