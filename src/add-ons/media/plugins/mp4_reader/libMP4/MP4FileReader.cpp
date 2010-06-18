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
#include "MP4FileReader.h"

#include <DataIO.h>
#include <SupportKit.h>

#include <iostream>


extern AtomBase *GetAtom(BPositionIO *pStream);


MP4FileReader::MP4FileReader(BPositionIO *pStream)
{
	theStream = pStream;

	// Find Size of Stream, need to rethink this for non seekable streams
	theStream->Seek(0,SEEK_END);
	StreamSize = theStream->Position();
	theStream->Seek(0,SEEK_SET);
	TotalChildren = 0;

	theMVHDAtom = NULL;
}


MP4FileReader::~MP4FileReader()
{
	theStream = NULL;
	theMVHDAtom = NULL;
}


bool
MP4FileReader::IsEndOfData(off_t pPosition)
{
	AtomBase* aAtomBase;

	// check all mdat atoms to make sure pPosition is within one of them
	for (uint32 index=0; index < CountChildAtoms('mdat'); index++) {
		aAtomBase = GetChildAtom(uint32('mdat'),index);
		if ((aAtomBase) && (aAtomBase->GetAtomSize() > 8)) {
			MDATAtom *aMDATAtom = dynamic_cast<MDATAtom *>(aAtomBase);
			if (pPosition >= aMDATAtom->GetAtomOffset() && pPosition <= aMDATAtom->GetEOF()) {
				return false;
			}
		}
	}

	return true;
}


bool
MP4FileReader::IsEndOfFile(off_t position)
{
	return (position >= StreamSize);
}


bool
MP4FileReader::IsEndOfFile()
{
	return theStream->Position() >= StreamSize;
}


bool
MP4FileReader::AddChild(AtomBase *childAtom)
{
	if (childAtom) {
		atomChildren.push_back(childAtom);
		TotalChildren++;
		return true;
	}
	return false;
}


AtomBase *
MP4FileReader::GetChildAtom(uint32 patomType, uint32 offset)
{
	for (uint32 i = 0; i < TotalChildren; i++) {
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


uint32
MP4FileReader::CountChildAtoms(uint32 patomType)
{
	uint32 count = 0;

	while (GetChildAtom(patomType, count) != NULL) {
		count++;
	}
	return count;
}

TRAKAtom *
MP4FileReader::GetTrack(uint32 streamIndex) 
{
	if (tracks[streamIndex] == NULL) {
		AtomBase *aAtomBase = GetChildAtom(uint32('trak'), streamIndex);
		if (aAtomBase) {
			tracks[streamIndex] = (dynamic_cast<TRAKAtom *>(aAtomBase));
		}
	}
	
	if (tracks[streamIndex] != NULL) {
		return tracks[streamIndex];
	}

	#ifdef DEBUG
		char msg[100]; sprintf(msg, "Bad Stream Index %ld\n", streamIndex);
		DEBUGGER(msg);
	#endif

	TRESPASS();
	
	return NULL;	// Never to happen
}


MVHDAtom*
MP4FileReader::GetMVHDAtom()
{
	AtomBase *aAtomBase;

	if (theMVHDAtom == NULL) {
		aAtomBase = GetChildAtom(uint32('mvhd'));
		theMVHDAtom = dynamic_cast<MVHDAtom *>(aAtomBase);
	}

	// Assert(theMVHDAtom != NULL,"Movie has no movie header atom");
	return theMVHDAtom;
}


uint32
MP4FileReader::GetMovieTimeScale()
{
	return GetMVHDAtom()->GetTimeScale();
}


bigtime_t
MP4FileReader::GetMovieDuration()
{
	return bigtime_t((GetMVHDAtom()->GetDuration() * 1000000.0) / GetMovieTimeScale());
}


uint32
MP4FileReader::GetStreamCount()
{
	// count the number of tracks in the file
	return CountChildAtoms(uint32('trak'));
}


bigtime_t
MP4FileReader::GetVideoDuration(uint32 streamIndex)
{
	if (IsVideo(streamIndex)) {
		return GetTrack(streamIndex)->Duration(1);
	}
	
	return 0;
}


bigtime_t
MP4FileReader::GetAudioDuration(uint32 streamIndex)
{
	if (IsAudio(streamIndex)) {
		return GetTrack(streamIndex)->Duration(1);
	}

	return 0;
}


bigtime_t
MP4FileReader::GetMaxDuration()
{
	int32 videoIndex = -1;
	int32 audioIndex = -1;

	// find the active video and audio tracks
	for (uint32 i = 0; i < GetStreamCount(); i++) {
		if (GetTrack(i)->IsActive()) {
			if (GetTrack(i)->IsAudio()) {
				audioIndex = int32(i);
			}
			if (GetTrack(i)->IsVideo()) {
				videoIndex = int32(i);
			}
		}
	}

	if (videoIndex >= 0 && audioIndex >= 0) {
		return max_c(GetVideoDuration(videoIndex),
			GetAudioDuration(audioIndex));
	}
	if (videoIndex < 0 && audioIndex >= 0) {
		return GetAudioDuration(audioIndex);
	}
	if (videoIndex >= 0 && audioIndex < 0) {
		return GetVideoDuration(videoIndex);
	}

	return 0;
}


uint32
MP4FileReader::GetFrameCount(uint32 streamIndex)
{
	return GetTrack(streamIndex)->FrameCount();
}

uint32
MP4FileReader::GetAudioChunkCount(uint32 streamIndex)
{
	if (IsAudio(streamIndex)) {
		return GetTrack(streamIndex)->GetTotalChunks();
	}
	
	return 0;
}

bool
MP4FileReader::IsVideo(uint32 streamIndex)
{
	return GetTrack(streamIndex)->IsVideo();
}


bool
MP4FileReader::IsAudio(uint32 streamIndex)
{
	return GetTrack(streamIndex)->IsAudio();
}


uint32
MP4FileReader::GetFirstFrameInChunk(uint32 streamIndex, uint32 pChunkIndex)
{
	return GetTrack(streamIndex)->GetFirstSampleInChunk(pChunkIndex);
}


uint32
MP4FileReader::GetNoFramesInChunk(uint32 streamIndex, uint32 pChunkIndex)
{
	return GetTrack(streamIndex)->GetNoSamplesInChunk(pChunkIndex);
}

uint32
MP4FileReader::GetChunkForFrame(uint32 streamIndex, uint32 pFrameNo) {
	uint32 OffsetInChunk;
	return GetTrack(streamIndex)->GetChunkForSample(pFrameNo, &OffsetInChunk);	
}


uint64
MP4FileReader::GetOffsetForFrame(uint32 streamIndex, uint32 pFrameNo)
{
	TRAKAtom *aTrakAtom = GetTrack(streamIndex);
	
	if (pFrameNo < aTrakAtom->FrameCount()) {
		// Get time for Frame
		bigtime_t Time = aTrakAtom->GetTimeForFrame(pFrameNo);
		
		// Get Sample for Time
		uint32 SampleNo = aTrakAtom->GetSampleForTime(Time);
		
		// Get Chunk For Sample and the offset for the frame within that chunk
		uint32 OffsetInChunk;
		uint32 ChunkIndex = aTrakAtom->GetChunkForSample(SampleNo, &OffsetInChunk);
		// Get Offset For Chunk
		uint64 OffsetNo = aTrakAtom->GetOffsetForChunk(ChunkIndex);

		if (ChunkIndex != 0) {
			uint32 SizeForSample;
			// Adjust the Offset for the Offset in the chunk
			if (aTrakAtom->IsSingleSampleSize()) {
				SizeForSample = aTrakAtom->GetSizeForSample(SampleNo);
				OffsetNo = OffsetNo + (OffsetInChunk * SizeForSample);
			} else {
				// This is bad news performance wise
				for (uint32 i=1;i<=OffsetInChunk;i++) {
					SizeForSample = aTrakAtom->GetSizeForSample(SampleNo-i);
					OffsetNo = OffsetNo + SizeForSample;
				}
			}
		}
		
//		printf("frame %ld, time %Ld, sample %ld, Chunk %ld, OffsetInChunk %ld, Offset %Ld\n",pFrameNo, Time, SampleNo, ChunkNo, OffsetInChunk, OffsetNo);

		return OffsetNo;
	}

	return 0;
}

uint64
MP4FileReader::GetOffsetForChunk(uint32 streamIndex, uint32 pChunkIndex)
{
	return GetTrack(streamIndex)->GetOffsetForChunk(pChunkIndex);
}


status_t
MP4FileReader::ParseFile()
{
	AtomBase *aChild;
	while (IsEndOfFile() == false) {
		aChild = GetAtom(theStream);
		if (AddChild(aChild)) {
			aChild->ProcessMetaData();
		}
	}

#ifdef DEBUG
	// Debug info
	for (uint32 i=0;i<TotalChildren;i++) {
		atomChildren[i]->DisplayAtoms();
	}
#endif
	
	return B_OK;
}


const mp4_main_header*
MP4FileReader::MovMainHeader()
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

	theMainHeader.streams = GetStreamCount();
	theMainHeader.flags = 0;
	theMainHeader.initial_frames = 0;

	while (videoStream < theMainHeader.streams) {
		if (IsVideo(videoStream) && IsActive(videoStream))
			break;

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
		theMainHeader.total_frames = GetFrameCount(videoStream);
		theMainHeader.suggested_buffer_size = theMainHeader.width * theMainHeader.height * VideoFormat(videoStream)->bit_count / 8;
		theMainHeader.micro_sec_per_frame = uint32(1000000.0 / VideoFormat(videoStream)->FrameRate);
	}

	theMainHeader.padding_granularity = 0;
	theMainHeader.max_bytes_per_sec = 0;

	return &theMainHeader;
}


const AudioMetaData *
MP4FileReader::AudioFormat(uint32 streamIndex, size_t *size)
{
	if (IsAudio(streamIndex)) {
		AtomBase *aAtomBase = GetChildAtom(uint32('trak'),streamIndex);

		if (aAtomBase) {
			TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);
			
			aAtomBase = aTrakAtom->GetChildAtom(uint32('stsd'),0);
			if (aAtomBase) {
				STSDAtom *aSTSDAtom = dynamic_cast<STSDAtom *>(aAtomBase);

				// Fill in the AudioMetaData structure
				AudioDescription aAudioDescription = aSTSDAtom->GetAsAudio();

				theAudio.compression = aAudioDescription.codecid;
				theAudio.codecSubType = aAudioDescription.codecSubType;
				
				theAudio.NoOfChannels = aAudioDescription.theAudioSampleEntry.ChannelCount;
				
				// Fix for broken mp4's with 0 SampleSize, default to 16 bits
				if (aAudioDescription.theAudioSampleEntry.SampleSize == 0) {
					theAudio.SampleSize = 16;
				} else {
					theAudio.SampleSize = aAudioDescription.theAudioSampleEntry.SampleSize;
				}
				
				theAudio.SampleRate = aAudioDescription.theAudioSampleEntry.SampleRate;
				theAudio.FrameSize = aAudioDescription.FrameSize;
				if (aAudioDescription.BufferSize == 0) {
					theAudio.BufferSize = uint32((theAudio.SampleSize * theAudio.NoOfChannels * theAudio.FrameSize) / 8);
				} else {
					theAudio.BufferSize = aAudioDescription.BufferSize;
				}
				
				theAudio.BitRate = aAudioDescription.BitRate;

				theAudio.theDecoderConfig = aAudioDescription.theDecoderConfig;
				theAudio.DecoderConfigSize = aAudioDescription.DecoderConfigSize;

				return &theAudio;
			}
		}
	}
	
	return NULL;
}

const VideoMetaData*
MP4FileReader::VideoFormat(uint32 streamIndex)
{
	if (IsVideo(streamIndex)) {
		AtomBase *aAtomBase = GetChildAtom(uint32('trak'),streamIndex);

		if (aAtomBase) {
			TRAKAtom *aTrakAtom = dynamic_cast<TRAKAtom *>(aAtomBase);

			aAtomBase = aTrakAtom->GetChildAtom(uint32('stsd'),0);
			if (aAtomBase) {
				STSDAtom *aSTSDAtom = dynamic_cast<STSDAtom *>(aAtomBase);
				VideoDescription aVideoDescription = aSTSDAtom->GetAsVideo();

				theVideo.compression = aVideoDescription.codecid;
				theVideo.codecSubType = aVideoDescription.codecSubType;

				theVideo.width = aVideoDescription.theVideoSampleEntry.Width;
				theVideo.height = aVideoDescription.theVideoSampleEntry.Height;
				theVideo.planes = aVideoDescription.theVideoSampleEntry.Depth;
				theVideo.BufferSize = aVideoDescription.theVideoSampleEntry.Width * aVideoDescription.theVideoSampleEntry.Height * aVideoDescription.theVideoSampleEntry.Depth / 8;
				theVideo.bit_count = aVideoDescription.theVideoSampleEntry.Depth;
				theVideo.image_size = aVideoDescription.theVideoSampleEntry.Height * aVideoDescription.theVideoSampleEntry.Width;
				theVideo.HorizontalResolution = aVideoDescription.theVideoSampleEntry.HorizontalResolution;
				theVideo.VerticalResolution = aVideoDescription.theVideoSampleEntry.VerticalResolution;
				theVideo.FrameCount = aVideoDescription.theVideoSampleEntry.FrameCount;

				theVideo.theDecoderConfig = aVideoDescription.theDecoderConfig;
				theVideo.DecoderConfigSize = aVideoDescription.DecoderConfigSize;

				aAtomBase = aTrakAtom->GetChildAtom(uint32('stts'),0);
				if (aAtomBase) {
					STTSAtom *aSTTSAtom = dynamic_cast<STTSAtom *>(aAtomBase);

					theVideo.FrameRate = ((aSTTSAtom->GetSUMCounts() * 1000000.0) / aTrakAtom->Duration(1));

					return &theVideo;
				}
			}
		}
	}

	return NULL;
}


const mp4_stream_header*
MP4FileReader::StreamFormat(uint32 streamIndex)
{
	if (IsActive(streamIndex) == false) {
		return NULL;
	}

	// Fill In a Stream Header
	theStreamHeader.length = 0;

	if (IsVideo(streamIndex)) {
		theStreamHeader.rate = uint32(1000000.0*VideoFormat(streamIndex)->FrameRate);
		theStreamHeader.scale = 1000000L;
		theStreamHeader.length = GetFrameCount(streamIndex);
	}

	if (IsAudio(streamIndex)) {
		theStreamHeader.rate = uint32(AudioFormat(streamIndex)->SampleRate);
		theStreamHeader.scale = 1;
		theStreamHeader.length = GetFrameCount(streamIndex);
		theStreamHeader.sample_size = AudioFormat(streamIndex)->SampleSize;
		theStreamHeader.suggested_buffer_size =	AudioFormat(streamIndex)->BufferSize;
	}

	return &theStreamHeader;
}


uint32
MP4FileReader::GetFrameSize(uint32 streamIndex, uint32 pFrameNo)
{
	if (pFrameNo < GetTrack(streamIndex)->FrameCount()) {
		uint32 SampleNo = GetTrack(streamIndex)->GetSampleForFrame(pFrameNo);
		return GetTrack(streamIndex)->GetSizeForSample(SampleNo);
	}

	return 0;
}

bool
MP4FileReader::isValidFrame(uint32 streamIndex, uint32 pFrameNo) {
	return (pFrameNo < GetTrack(streamIndex)->FrameCount());
}

bool
MP4FileReader::isValidChunkIndex(uint32 streamIndex, uint32 pChunkIndex) {
	return (pChunkIndex > 0 && pChunkIndex <= GetTrack(streamIndex)->ChunkCount());
}


uint32
MP4FileReader::GetChunkSize(uint32 streamIndex, uint32 pChunkIndex)
{
	return GetTrack(streamIndex)->GetChunkSize(pChunkIndex);
}


bool
MP4FileReader::IsKeyFrame(uint32 streamIndex, uint32 pFrameNo)
{
	if (IsAudio(streamIndex)) {
		return true;
	}
	
	return GetTrack(streamIndex)->IsSyncSample(pFrameNo);
}

bigtime_t
MP4FileReader::GetTimeForFrame(uint32 streamIndex, uint32 pFrameNo) {
	// Calculate PTS
	return GetTrack(streamIndex)->GetTimeForFrame(pFrameNo);
}


uint32
MP4FileReader::GetFrameForTime(uint32 streamIndex, bigtime_t time) {
	return GetTrack(streamIndex)->GetFrameForTime(time);
}


uint32
MP4FileReader::GetFrameForSample(uint32 streamIndex, uint32 sample) {
	if (IsVideo(streamIndex)) {
		return sample;		// frame = sample for video
	}
	
	if (IsAudio(streamIndex)) {
		bigtime_t time = bigtime_t((sample * 1000000.0) / GetTrack(streamIndex)->GetSampleRate());
		return GetTrack(streamIndex)->GetFrameForTime(time);
	}
	
	return 0;
}


uint32
MP4FileReader::GetSampleForTime(uint32 streamIndex, bigtime_t time) {
	return GetTrack(streamIndex)->GetSampleForTime(time);
}


uint32
MP4FileReader::GetTimeForSample(uint32 streamIndex, uint32 sample) {
	return GetTimeForFrame(streamIndex, GetFrameForSample(streamIndex, sample));
}


bool
MP4FileReader::GetBufferForChunk(uint32 streamIndex, uint32 pChunkIndex, off_t *start, uint32 *size, bool *keyframe, bigtime_t *time, uint32 *framesInBuffer) {
	
	if (isValidChunkIndex(streamIndex, pChunkIndex) == false) {
		*start = 0;
		*size = 0;
		*keyframe = false;
		*time = 0;
		*framesInBuffer = 0;
		return false;
	}

	*start = GetOffsetForChunk(streamIndex, pChunkIndex);
	*size = GetChunkSize(streamIndex, pChunkIndex);
	
	uint32 frameNo = GetFirstFrameInChunk(streamIndex, pChunkIndex);
	
	if ((*start > 0) && (*size > 0)) {
		*keyframe = IsKeyFrame(streamIndex, frameNo);
	}
	
	*framesInBuffer = GetNoFramesInChunk(streamIndex, pChunkIndex);
	*time = GetTimeForFrame(streamIndex, frameNo);
		
	if (IsEndOfFile(*start + *size) || IsEndOfData(*start + *size)) {
		return false;
	}
	
	return *start > 0 && *size > 0;
}


bool
MP4FileReader::GetBufferForFrame(uint32 streamIndex, uint32 pFrameNo, off_t *start, uint32 *size, bool *keyframe, bigtime_t *time, uint32 *framesInBuffer) {
	// Get the offset for a given frame and the size of the remaining bytes in the chunk
	// framesInBuffer is how many frames there are in the buffer.

	if (isValidFrame(streamIndex, pFrameNo) == false) {
		*start = 0;
		*size = 0;
		*keyframe = false;
		*time = 0;
		*framesInBuffer = 0;
		return false;
	}
	
	*start = GetOffsetForFrame(streamIndex, pFrameNo);
	*size = GetFrameSize(streamIndex, pFrameNo);

	if ((*start > 0) && (*size > 0)) {
		*keyframe = IsKeyFrame(streamIndex, pFrameNo);
	}

	*framesInBuffer = 1;
	*time = GetTimeForFrame(streamIndex, pFrameNo);
	
	if (IsEndOfFile(*start + *size) || IsEndOfData(*start + *size)) {
		return false;
	}
	
	return *start > 0 && *size > 0;
}


bool
MP4FileReader::IsActive(uint32 streamIndex)
{
	// Don't use helper method streamIndex is likely invalid
	AtomBase *aAtomBase = GetChildAtom(uint32('trak'), streamIndex);
	if (aAtomBase) {
		return dynamic_cast<TRAKAtom *>(aAtomBase)->IsActive();
	}
	
	return false;
}


/* static */
bool
MP4FileReader::IsSupported(BPositionIO *source)
{
	AtomBase *aAtom = GetAtom(source);
	if (aAtom) {
		if (dynamic_cast<FTYPAtom *>(aAtom)) {
			aAtom->ProcessMetaData();
			printf("ftyp atom found checking brands...");
			// MP4 files start with a ftyp atom that does not contain a qt brand
			if (!dynamic_cast<FTYPAtom *>(aAtom)->HasBrand(uint32('qt  '))) {
				printf("no quicktime brand found must be mp4\n");
				return true;
			} else {
				printf("quicktime brand found\n");
			}
		}
	}

	printf("NO ftyp atom found, cannot be mp4\n");
	
	return false;
}
