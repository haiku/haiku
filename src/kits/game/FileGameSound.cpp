/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <FileGameSound.h>

static const size_t default_in_buffer_frame_count = 4096; // XXX change this
static const gs_audio_format default_format = {}; // XXX change this
static const size_t default_in_buffer_count = 2; // XXX change this

BFileGameSound::BFileGameSound(const entry_ref *file,
							   bool looping,
							   BGameSoundDevice *device)
 :	BStreamingGameSound(default_in_buffer_frame_count, &default_format, default_in_buffer_count, device)
{
}


BFileGameSound::BFileGameSound(const char *file,
							   bool looping,
							   BGameSoundDevice *device)
 :	BStreamingGameSound(default_in_buffer_frame_count, &default_format, default_in_buffer_count, device)
{
}


BFileGameSound::~BFileGameSound()
{
}


BGameSound *
BFileGameSound::Clone() const
{
	return NULL;
}


status_t
BFileGameSound::StartPlaying()
{
	return B_ERROR;
}


status_t
BFileGameSound::StopPlaying()
{
	return B_ERROR;
}


status_t
BFileGameSound::Preload()
{
	return B_ERROR;
}


void
BFileGameSound::FillBuffer(void *inBuffer,
						   size_t inByteCount)
{
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
	return B_ERROR;
}


int32
BFileGameSound::IsPaused()
{
	return 0;
}


status_t
BFileGameSound::stream_thread(void *that)
{
	return B_ERROR;
}


void
BFileGameSound::LoadFunc()
{
}


size_t
BFileGameSound::Load(size_t &offset,
					 size_t bytes)
{
	return 0;
}


void
BFileGameSound::read_data(char *dest,
						  size_t bytes)
{
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


