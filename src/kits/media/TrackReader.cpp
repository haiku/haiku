/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: TrackReader.cpp
 *  DESCR: The undocumented BTrackReader class, 
 *         used by BSound and the GameSound classes
 ***********************************************************************/
 
#include <MediaTrack.h>
#include <MediaFile.h>
#include <File.h>
#include <string.h>
#include "TrackReader.h"
#include "debug.h"

namespace BPrivate
{

BTrackReader::BTrackReader(BMediaTrack *track, media_raw_audio_format const &format) : 
	fFrameSize(0),
	fBuffer(0),
	fBufferOffset(0),
	fBufferUsedSize(0),
	fMediaFile(0),
	fMediaTrack(0),
	fFormat(format)
{
	CALLED();
	if (track == NULL)
		return;
	if (track->InitCheck() != B_OK)
		return;
		
	SetToTrack(track);

	// if the track was not set abort now
	if (fMediaTrack == 0)
		return;

	fBuffer = new uint8[fFormat.buffer_size];
	fFrameSize = fFormat.channel_count * (fFormat.format & media_raw_audio_format::B_AUDIO_SIZE_MASK);

	TRACE("BTrackReader::BTrackReader successful\n");
}

BTrackReader::BTrackReader(BFile *file, media_raw_audio_format const &format) : 
	fFrameSize(0),
	fBuffer(0),
	fBufferOffset(0),
	fBufferUsedSize(0),
	fMediaFile(0),
	fMediaTrack(0),
	fFormat(format)
{
	CALLED();
	if (file == NULL)
		return;
	if (file->InitCheck() != B_OK)
		return;
		
	fMediaFile = new BMediaFile(file);
	if (fMediaFile->InitCheck() != B_OK)
		return;
	
	int count = fMediaFile->CountTracks();
	if (count == 0) {
		TRACE("no tracks in file\n");
		return;
	}

	// find the first audio track
	BMediaTrack *track;
	BMediaTrack *audiotrack = 0;
	for (int tr = 0; tr < count; tr++) {
		track = fMediaFile->TrackAt(tr);
		if (track == 0 || track->InitCheck() != B_OK)
			continue;
		media_format fmt;
		if (track->EncodedFormat(&fmt) != B_OK)
			continue;
		if (fmt.type == B_MEDIA_RAW_AUDIO) {
			audiotrack = track;
			break;
		}
		fMediaFile->ReleaseTrack(track);
	}
	if (audiotrack == 0) {
		TRACE("no audio track in file\n");
		return;
	}
	
	SetToTrack(audiotrack);

	// if the track was not set, release it
	if (fMediaTrack == 0) {
		fMediaFile->ReleaseTrack(audiotrack);
		return;
	}

	fBuffer = new uint8[fFormat.buffer_size];
	fFrameSize = fFormat.channel_count * (fFormat.format & media_raw_audio_format::B_AUDIO_SIZE_MASK);

	TRACE("BTrackReader::BTrackReader successful\n");
}

void
BTrackReader::SetToTrack(BMediaTrack *track)
{	
	media_format fmt;
	memset(&fmt,0,sizeof(fmt)); //wildcard
	memcpy(&fmt.u.raw_audio,&fFormat,sizeof(fFormat));
	fmt.type = B_MEDIA_RAW_AUDIO;

	//try to find a output format
	if (B_OK == track->DecodedFormat(&fmt)) {
		memcpy(&fFormat,&fmt.u.raw_audio,sizeof(fFormat));
		fMediaTrack = track;
		return;
	}
	
	//try again
	fmt.u.raw_audio.buffer_size = 0;
	if (B_OK == track->DecodedFormat(&fmt)) {
		memcpy(&fFormat,&fmt.u.raw_audio,sizeof(fFormat));
		fMediaTrack = track;
		return;
	}
	
	//we have failed
	TRACE("BTrackReader::SetToTrack failed\n");
}

BTrackReader::~BTrackReader()
{
	CALLED();
	if (fMediaFile && fMediaTrack)
		fMediaFile->ReleaseTrack(fMediaTrack);
	delete fMediaFile;
	delete [] fBuffer;
}

status_t
BTrackReader::InitCheck()
{
	CALLED();
	return fMediaTrack ? fMediaTrack->InitCheck() : B_ERROR;
}

int64 
BTrackReader::CountFrames(void)
{
	CALLED();
	return fMediaTrack ? fMediaTrack->CountFrames() : 0;
}

const media_raw_audio_format & 
BTrackReader::Format(void) const
{
	CALLED();
	return fFormat;
}

int32 
BTrackReader::FrameSize(void)
{
	CALLED();
	return fFrameSize;
}

status_t 
BTrackReader::ReadFrames(void *in_buffer, int32 frame_count)
{
	CALLED();

	uint8 *buffer = (uint8 *)in_buffer;
	int32 bytes_to_read = frame_count * fFrameSize;

	status_t last_status = B_OK;
	while (bytes_to_read > 0) {
		int32 bytes_to_copy = min_c(fBufferUsedSize,bytes_to_read);
		if (bytes_to_copy > 0) {
			memcpy(buffer,fBuffer + fBufferOffset,bytes_to_copy);
			buffer += bytes_to_copy;
			bytes_to_read -= bytes_to_copy;
			fBufferOffset += bytes_to_copy;
			fBufferUsedSize -= bytes_to_copy;
		}
		if (last_status != B_OK)
			break;
		if (fBufferUsedSize == 0) {
			int64 outFrameCount;
			last_status = fMediaTrack->ReadFrames(fBuffer,&outFrameCount);
			fBufferOffset = 0;
			fBufferUsedSize = outFrameCount * fFrameSize;
		}
	}
	if (bytes_to_read > 0) {
		memset(buffer,0,bytes_to_read);
		return B_LAST_BUFFER_ERROR;
	} else {
		return B_OK;
	}
}

status_t 
BTrackReader::SeekToFrame(int64 *in_out_frame)
{
	CALLED();
	status_t s = fMediaTrack->SeekToFrame(in_out_frame, B_MEDIA_SEEK_CLOSEST_BACKWARD);
	if (s != B_OK)
		return s;
	fBufferUsedSize = 0;
	fBufferOffset = 0;
	return B_OK;
}

BMediaTrack * 
BTrackReader::Track(void)
{
	CALLED();
	return fMediaTrack;
}

}; //namespace BPrivate

