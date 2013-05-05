/*
 * Copyright 2001-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christopher ML Zumwalt May (zummy@users.sf.net)
 */


#include <SimpleGameSound.h>

#include <Entry.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <stdlib.h>
#include <string.h>

#include "GameSoundBuffer.h"
#include "GameSoundDefs.h"
#include "GameSoundDevice.h"
#include "GSUtility.h"


BSimpleGameSound::BSimpleGameSound(const entry_ref *inFile,
	BGameSoundDevice *device)
	:
	BGameSound(device)
{
	if (InitCheck() == B_OK)
		SetInitError(Init(inFile));
}


BSimpleGameSound::BSimpleGameSound(const char *inFile, BGameSoundDevice *device)
	:
	BGameSound(device)
{
	if (InitCheck() == B_OK) {
		entry_ref file;

		if (get_ref_for_path(inFile, &file) != B_OK)
			SetInitError(B_ENTRY_NOT_FOUND);
		else
			SetInitError(Init(&file));
	}
}


BSimpleGameSound::BSimpleGameSound(const void *inData, size_t inFrameCount,
	const gs_audio_format *format, BGameSoundDevice *device)
	:
	BGameSound(device)
{
	if (InitCheck() == B_OK)
		SetInitError(Init(inData, inFrameCount, format));
}


BSimpleGameSound::BSimpleGameSound(const BSimpleGameSound &other)
	:
	BGameSound(other)
{
	gs_audio_format format;
	void *data = NULL;

	status_t error = other.Device()->Buffer(other.ID(), &format, data);
	if (error != B_OK)
		SetInitError(error);

	Init(data, 0, &format);
	free(data);
}


BSimpleGameSound::~BSimpleGameSound()
{
}


BGameSound *
BSimpleGameSound::Clone() const
{
	gs_audio_format format;
	void *data = NULL;

	status_t error = Device()->Buffer(ID(), &format, data);
	if (error != B_OK)
		return NULL;

	BSimpleGameSound *clone = new BSimpleGameSound(data, 0, &format, Device());
	free(data);

	return clone;
}


/* virtual */ status_t
BSimpleGameSound::Perform(int32 selector, void * data)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::SetIsLooping(bool looping)
{
	gs_attribute attribute;

	attribute.attribute = B_GS_LOOPING;
	attribute.value = (looping) ? -1.0 : 0.0;
	attribute.duration = bigtime_t(0);
	attribute.flags = 0;

	return Device()->SetAttributes(ID(), &attribute, 1);
}


bool
BSimpleGameSound::IsLooping() const
{
	gs_attribute attribute;

	attribute.attribute = B_GS_LOOPING;
	attribute.flags = 0;

	if (Device()->GetAttributes(ID(), &attribute, 1) != B_OK)
		return false;

	return bool(attribute.value);
}


status_t
BSimpleGameSound::Init(const entry_ref* inFile)
{
	BMediaFile file(inFile);
	gs_audio_format gsformat;
	media_format mformat;
	int64 framesRead, framesTotal = 0;

	if (file.InitCheck() != B_OK)
		return file.InitCheck();

	BMediaTrack* audioStream = file.TrackAt(0);
	audioStream->EncodedFormat(&mformat);
	if (!mformat.IsAudio())
		return B_ERROR;

	int64 frames = audioStream->CountFrames();

	memset(&mformat, 0, sizeof(media_format));
	mformat.type = B_MEDIA_RAW_AUDIO;
//	mformat.u.raw_audio.byte_order
//		= (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	status_t error = audioStream->DecodedFormat(&mformat);
	if (error != B_OK)
		return error;

	memset(&gsformat, 0, sizeof(gs_audio_format));
	media_to_gs_format(&gsformat, &mformat.u.raw_audio);

	if (mformat.u.raw_audio.format == media_raw_audio_format::B_AUDIO_CHAR) {
		// The GameKit doesnt support this format so we will have to reformat
		// the data into something the GameKit does support.
		char * buffer = new char[gsformat.buffer_size];
		uchar * data = new uchar[frames * gsformat.channel_count];

		while (framesTotal < frames) {
			// read the next chunck from the stream
			memset(buffer, 0, gsformat.buffer_size);
			audioStream->ReadFrames(buffer, &framesRead);

			// refomat the buffer from
			int64 position = framesTotal * gsformat.channel_count;
			for (int32 i = 0; i < (int32)gsformat.buffer_size; i++)
				data[i + position] = buffer[i] + 128;

			framesTotal += framesRead;
		}

		gsformat.format = gs_audio_format::B_GS_U8;

		error = Init(data, frames, &gsformat);

		// free the buffers we no longer need
		delete [] buffer;
		delete [] data;
	} else {
		// We need to determine the size, in bytes, of a single sample.
		// At the same time, we will store the format of the audio buffer
		size_t frameSize
			= get_sample_size(gsformat.format) * gsformat.channel_count;
		char * data = new char[frames * frameSize];
		gsformat.buffer_size = frames * frameSize;

		while (framesTotal < frames) {
			char * position = &data[framesTotal * frameSize];
			audioStream->ReadFrames(position, &framesRead);

			framesTotal += framesRead;
		}

		error = Init(data, frames, &gsformat);

		delete [] data;
	}

	file.ReleaseTrack(audioStream);
	return error;
}


status_t
BSimpleGameSound::Init(const void* inData, int64 inFrameCount,
	const gs_audio_format* format)
{
	gs_id sound;

	status_t error
		= Device()->CreateBuffer(&sound, format, inData, inFrameCount);
	if (error != B_OK)
		return error;

	BGameSound::Init(sound);

	return B_OK;
}


/* unimplemented for protection of the user:
 *
 * BSimpleGameSound::BSimpleGameSound()
 * BSimpleGameSound &BSimpleGameSound::operator=(const BSimpleGameSound &)
 */


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_0(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_1(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_2(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_3(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_4(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_5(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_6(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_7(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_8(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_9(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_10(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_11(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_12(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_13(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_14(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_15(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_16(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_17(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_18(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_19(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_20(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_21(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_22(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::_Reserved_BSimpleGameSound_23(int32 arg, ...)
{
	return B_ERROR;
}
