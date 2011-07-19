/*
 * Copyright 2001-2007, Haiku Inc.
 * Authors:
 *		Christopher ML Zumwalt May (zummy@users.sf.net)
 *		Jérôme Duval
 *
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Entry.h>
#include <FileGameSound.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <scheduler.h>

#include "GameSoundDevice.h"
#include "GSUtility.h"

const int32 kPages = 20;
struct _gs_media_tracker
{
	BMediaFile *	file;
	BMediaTrack * 	stream;
	int64			frames;
	size_t			position;
};

// Local utility functions -----------------------------------------------
bool FillBuffer(_gs_ramp * ramp, uint8 * data, uint8 * buffer, size_t * bytes)
{
	int32 samples = *bytes / sizeof(uint8);

	for (int32 byte = 0; byte < samples; byte++) {
		float gain = *ramp->value;
		data[byte] = uint8(float(buffer[byte]) * gain);

		if (ChangeRamp(ramp)) {
			*bytes = byte * sizeof(uint8);
			return true;
		}
	}

	return false;
}

bool FillBuffer(_gs_ramp * ramp, int16 * data, int16 * buffer, size_t * bytes)
{
	int32 samples = *bytes / sizeof(int16);

	for (int32 byte = 0; byte < samples; byte++) {
		float gain = *ramp->value;
		data[byte] = int16(float(buffer[byte]) * gain);

		if (ChangeRamp(ramp)) {
			*bytes = byte * sizeof(int16);
			return true;
		}
	}

	return false;
}

bool FillBuffer(_gs_ramp * ramp, int32 * data, int32 * buffer, size_t * bytes)
{
	size_t byte = 0;
	bool bytesAreReady = (*bytes > 0);

	while (bytesAreReady) {
		float gain = *ramp->value;
		data[byte] = int32(float(buffer[byte]) * gain);

		if (ChangeRamp(ramp)) {
			*bytes = byte;
			return true;
		}

		byte++;
		bytesAreReady = (byte >= *bytes);
	}

	return false;
}

bool FillBuffer(_gs_ramp * ramp, float * data, float * buffer, size_t * bytes)
{
	size_t byte = 0;
	bool bytesAreReady = (*bytes > 0);

	while (bytesAreReady) {
		float gain = *ramp->value;
		data[byte] = buffer[byte] * gain;

		if (ChangeRamp(ramp)) {
			*bytes = byte;
			return true;
		}

		byte++;
		bytesAreReady = (byte >= *bytes);
	}

	return false;
}

// BFileGameSound -------------------------------------------------------
BFileGameSound::BFileGameSound(const entry_ref *file,
							   bool looping,
							   BGameSoundDevice *device)
	:	BStreamingGameSound(device),
		fAudioStream(NULL),
		fStopping(false),
		fLooping(looping),
		fBuffer(NULL),
		fPlayPosition(0),
		fPausing(NULL),
		fPaused(false),
		fPauseGain(1.0)
{
	if (InitCheck() == B_OK)
		SetInitError(Init(file));
}


BFileGameSound::BFileGameSound(const char *file,
							   bool looping,
							   BGameSoundDevice *device)
	:	BStreamingGameSound(device),
		fAudioStream(NULL),
		fStopping(false),
		fLooping(looping),
		fBuffer(NULL),
		fPlayPosition(0),
		fPausing(NULL),
		fPaused(false),
		fPauseGain(1.0)
{
	if (InitCheck() == B_OK) {
		entry_ref node;

		if (get_ref_for_path(file, &node) != B_OK)
			SetInitError(B_ENTRY_NOT_FOUND);
		else
			SetInitError(Init(&node));
	}
}


BFileGameSound::~BFileGameSound()
{
	if (fReadThread >= 0) {
		// TODO: kill_thread() is very bad, since it will leak any resources
		// that the thread had allocated. It will also keep locks locked that
		// the thread holds! Set a flag to make the thread quit and use
		// wait_for_thread() here!
		kill_thread(fReadThread);
	}

	if (fAudioStream) {
		if (fAudioStream->stream)
			fAudioStream->file->ReleaseTrack(fAudioStream->stream);

		delete fAudioStream->file;
	}

	delete [] fBuffer;
	delete fAudioStream;
}


BGameSound *
BFileGameSound::Clone() const
{
	return NULL;
}


status_t
BFileGameSound::StartPlaying()
{
	// restart playback if needed
	if (IsPlaying())
		StopPlaying();

	// start playing the file
	return BStreamingGameSound::StartPlaying();
}


status_t
BFileGameSound::StopPlaying()
{
	status_t error = BStreamingGameSound::StopPlaying();

	if (!fAudioStream || !fAudioStream->stream)
		return B_OK;

	// start reading next time from the start of the file
	int64 frame = 0;
	fAudioStream->stream->SeekToFrame(&frame);

	fStopping = false;
	fAudioStream->position = 0;
	fPlayPosition = 0;

	return error;
}


status_t
BFileGameSound::Preload()
{
	if (!IsPlaying())
		Load();

	return B_OK;
}


void
BFileGameSound::FillBuffer(void *inBuffer,
						   size_t inByteCount)
{
	// Split or combine decoder buffers into mixer buffers
	// fPlayPosition is where we got up to in the input buffer after last call

	size_t out_offset = 0;

	while (inByteCount > 0 && !fPaused) {
		if (!fPaused || fPausing) {
			printf("mixout %ld, inByteCount %ld, decin %ld, BufferSize %ld\n",out_offset, inByteCount, fPlayPosition, fBufferSize);
			if (fPlayPosition == 0 || fPlayPosition >= fBufferSize) {
				Load();
			}

			if (fPausing) {
				Lock();

				bool rampDone = false;
				size_t bytes = fBufferSize - fPlayPosition;

				if (bytes > inByteCount) {
					bytes = inByteCount;
				}

				// Fill the requested buffer, stopping if the paused flag is set
				char * buffer = (char*)inBuffer;

				switch(Format().format) {
					case gs_audio_format::B_GS_U8:
						rampDone = ::FillBuffer(fPausing, (uint8*)&buffer[out_offset], (uint8*)&fBuffer[fPlayPosition], &bytes);
						break;

					case gs_audio_format::B_GS_S16:
						rampDone = ::FillBuffer(fPausing, (int16*)&buffer[out_offset], (int16*)&fBuffer[fPlayPosition], &bytes);
						break;

					case gs_audio_format::B_GS_S32:
						rampDone = ::FillBuffer(fPausing, (int32*)&buffer[out_offset], (int32*)&fBuffer[fPlayPosition], &bytes);
						break;

					case gs_audio_format::B_GS_F:
						rampDone = ::FillBuffer(fPausing, (float*)&buffer[out_offset], (float*)&fBuffer[fPlayPosition], &bytes);
						break;
				}

				inByteCount -= bytes;
				out_offset += bytes;
				fPlayPosition += bytes;

				// We finished ramping
				if (rampDone) {

					// We need to be able to stop asap when the pause flag is flipped.
					while(fPlayPosition < fBufferSize && (inByteCount > 0)) {
						buffer[out_offset++] = fBuffer[fPlayPosition++];
						inByteCount--;
					}

					delete fPausing;
					fPausing = NULL;
				}

				Unlock();
			} else {

				char * buffer = (char*)inBuffer;

				// We need to be able to stop asap when the pause flag is flipped.
				while(fPlayPosition < fBufferSize && (!fPaused || fPausing) && (inByteCount > 0)) {
					buffer[out_offset++] = fBuffer[fPlayPosition++];
					inByteCount--;
				}
			}
		}
	}
}


status_t
BFileGameSound::Perform(int32 selector,
						void *data)
{
	return B_ERROR;
}


status_t
BFileGameSound::SetPaused(bool isPaused,
						  bigtime_t rampTime)
{
	if (fPaused != isPaused) {
		Lock();

		// Clear any old ramping
		delete fPausing;
		fPausing = NULL;

		if (rampTime > 100000) {
			// Setup for ramping
			if (isPaused)
				fPausing = InitRamp(&fPauseGain, 0.0,
									Format().frame_rate, rampTime);
			else
				fPausing = InitRamp(&fPauseGain, 1.0,
									Format().frame_rate, rampTime);
		}

		fPaused = isPaused;
		Unlock();
	}

	return B_OK;
}


int32
BFileGameSound::IsPaused()
{
	if (fPausing)
		return B_PAUSE_IN_PROGRESS;

	if (fPaused)
		return B_PAUSED;

	return B_NOT_PAUSED;
}


status_t
BFileGameSound::Init(const entry_ref* file)
{
	fAudioStream = new(std::nothrow) _gs_media_tracker;
	if (!fAudioStream)
		return B_NO_MEMORY;

	memset(fAudioStream, 0, sizeof(_gs_media_tracker));
	fAudioStream->file = new(std::nothrow) BMediaFile(file);
	if (!fAudioStream->file) {
		delete fAudioStream;
		fAudioStream = NULL;
		return B_NO_MEMORY;
	}
	
	status_t error = fAudioStream->file->InitCheck();
	if (error != B_OK)
		return error;

	fAudioStream->stream = fAudioStream->file->TrackAt(0);

	// is this is an audio file?
	media_format playFormat;
	if ((error = fAudioStream->stream->EncodedFormat(&playFormat)) != B_OK) {
		fAudioStream->file->ReleaseTrack(fAudioStream->stream);
		fAudioStream->stream = NULL;
		return error;
	}

	if (!playFormat.IsAudio()) {
		fAudioStream->file->ReleaseTrack(fAudioStream->stream);
		fAudioStream->stream = NULL;
		return B_MEDIA_BAD_FORMAT;
	}

	gs_audio_format dformat = Device()->Format();

	// request the format we want the sound
	memset(&playFormat, 0, sizeof(media_format));
	playFormat.type = B_MEDIA_RAW_AUDIO;
	if (fAudioStream->stream->DecodedFormat(&playFormat) != B_OK) {
		fAudioStream->file->ReleaseTrack(fAudioStream->stream);
		fAudioStream->stream = NULL;
		return B_MEDIA_BAD_FORMAT;
	}

	// translate the format into a "GameKit" friendly one
	gs_audio_format gsformat;
	media_to_gs_format(&gsformat, &playFormat.u.raw_audio);

	// Since the buffer sized read from the file is most likely differnt
	// then the buffer used by the audio mixer, we must allocate a buffer
	// large enough to hold the largest request.
	fBufferSize = gsformat.buffer_size;
	if (fBufferSize < dformat.buffer_size)
		fBufferSize = dformat.buffer_size;

	// create the buffer
	fBuffer = new char[fBufferSize * 2];
	memset(fBuffer, 0, fBufferSize * 2);

	fFrameSize = gsformat.channel_count * get_sample_size(gsformat.format);
	fAudioStream->frames = fAudioStream->stream->CountFrames();

	// Ask the device to attach our sound to it
	gs_id sound;
	error = Device()->CreateBuffer(&sound, this, &gsformat);
	if (error != B_OK)
		return error;

	return BGameSound::Init(sound);
}


bool
BFileGameSound::Load()
{
	if (!fAudioStream || !fAudioStream->stream)
		return false;
  
	// read a new buffer
	int64 frames = 0;
	fAudioStream->stream->ReadFrames(fBuffer, &frames);
	fBufferSize = frames * fFrameSize;
	fPlayPosition = 0;

	if (fBufferSize <= 0) {
		// EOF
		if (fLooping) {
			// start reading next time from the start of the file
			int64 frame = 0;
			fAudioStream->stream->SeekToFrame(&frame);
		} else {
			StopPlaying();
		}
	}

	return true;
}


bool
BFileGameSound::Read(void * buffer, size_t bytes)
{
	return false;
}


/* unimplemented for protection of the user:
 *
 * BFileGameSound::BFileGameSound()
 * BFileGameSound::BFileGameSound(const BFileGameSound &)
 * BFileGameSound &BFileGameSound::operator=(const BFileGameSound &)
 */


status_t
BFileGameSound::_Reserved_BFileGameSound_0(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_1(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_2(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_3(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_4(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_5(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_6(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_7(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_8(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_9(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_10(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_11(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_12(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_13(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_14(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_15(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_16(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_17(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_18(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_19(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_20(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_21(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_22(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BFileGameSound::_Reserved_BFileGameSound_23(int32 arg, ...)
{
	return B_ERROR;
}


