/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <PushGameSound.h>

static const size_t default_in_buffer_frame_count = 4096; // XXX change this
static const gs_audio_format default_format = {}; // XXX change this
static const size_t default_in_buffer_count = 2; // XXX change this

BPushGameSound::BPushGameSound(size_t inBufferFrameCount,
							   const gs_audio_format *format,
							   size_t inBufferCount,
							   BGameSoundDevice *device)
 :	BStreamingGameSound(default_in_buffer_frame_count, &default_format, default_in_buffer_count, device)
{
}


BPushGameSound::~BPushGameSound()
{
}


BPushGameSound::lock_status
BPushGameSound::LockNextPage(void **out_pagePtr,
							 size_t *out_pageSize)
{
	return lock_failed;
}


status_t
BPushGameSound::UnlockPage(void *in_pagePtr)
{
	return B_ERROR;
}


BPushGameSound::lock_status
BPushGameSound::LockForCyclic(void **out_basePtr,
							  size_t *out_size)
{
	return lock_failed;
}


status_t
BPushGameSound::UnlockCyclic()
{
	return B_ERROR;
}


size_t
BPushGameSound::CurrentPosition()
{
	return 0;
}


BGameSound *
BPushGameSound::Clone() const
{
	return NULL;
}


status_t
BPushGameSound::Perform(int32 selector,
						void *data)
{
	return B_ERROR;
}


status_t
BPushGameSound::SetParameters(size_t inBufferFrameCount,
							  const gs_audio_format *format,
							  size_t inBufferCount)
{
	return B_ERROR;
}


status_t
BPushGameSound::SetStreamHook(void (*hook)(void * inCookie, void * inBuffer, size_t inByteCount, BStreamingGameSound * me),
							  void * cookie)
{
	return B_ERROR;
}


void
BPushGameSound::FillBuffer(void *inBuffer,
						   size_t inByteCount)
{
}


/* unimplemented for protection of the user:
 *
 * BPushGameSound::BPushGameSound()
 * BPushGameSound::BPushGameSound(const BPushGameSound &)
 * BPushGameSound &BPushGameSound::operator=(const BPushGameSound &)
 */


status_t
BPushGameSound::_Reserved_BPushGameSound_0(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_1(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_2(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_3(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_4(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_5(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_6(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_7(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_8(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_9(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_10(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_11(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_12(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_13(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_14(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_15(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_16(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_17(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_18(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_19(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_20(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_21(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_22(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BPushGameSound::_Reserved_BPushGameSound_23(int32 arg, ...)
{
	return B_ERROR;
}


