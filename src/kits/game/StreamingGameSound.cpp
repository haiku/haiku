/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <StreamingGameSound.h>

BStreamingGameSound::BStreamingGameSound(size_t inBufferFrameCount,
										 const gs_audio_format *format,
										 size_t inBufferCount,
										 BGameSoundDevice *device)
 :	BGameSound(device)
{
}


BStreamingGameSound::~BStreamingGameSound()
{
}


BGameSound *
BStreamingGameSound::Clone() const
{
	return NULL;
}


status_t
BStreamingGameSound::SetStreamHook(void (*hook)(void * inCookie, void * inBuffer, size_t inByteCount, BStreamingGameSound * me),
								   void * cookie)
{
	return B_ERROR;
}


void
BStreamingGameSound::FillBuffer(void *inBuffer,
								size_t inByteCount)
{
}


status_t
BStreamingGameSound::SetAttributes(gs_attribute *inAttributes,
								   size_t inAttributeCount)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::Perform(int32 selector,
							 void *data)
{
	return B_ERROR;
}


BStreamingGameSound::BStreamingGameSound(BGameSoundDevice *device)
 :	BGameSound(device)
{
}


status_t
BStreamingGameSound::SetParameters(size_t inBufferFrameCount,
								   const gs_audio_format *format,
								   size_t inBufferCount)
{
	return B_ERROR;
}


bool
BStreamingGameSound::Lock()
{
	return false;
}


void
BStreamingGameSound::Unlock()
{
}


void
BStreamingGameSound::stream_callback(void *cookie,
									 gs_id handle,
									 void *buffer,
									 int32 offset,
									 int32 size,
									 size_t bufSize)
{
}


/* unimplemented for protection of the user:
 *
 * BStreamingGameSound::BStreamingGameSound()
 * BStreamingGameSound::BStreamingGameSound(const BStreamingGameSound &)
 * BStreamingGameSound &BStreamingGameSound::operator=(const BStreamingGameSound &)
 */


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_0(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_1(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_2(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_3(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_4(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_5(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_6(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_7(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_8(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_9(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_10(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_11(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_12(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_13(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_14(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_15(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_16(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_17(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_18(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_19(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_20(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_21(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_22(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::_Reserved_BStreamingGameSound_23(int32 arg, ...)
{
	return B_ERROR;
}


