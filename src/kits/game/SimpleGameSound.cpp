/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <SimpleGameSound.h>

BSimpleGameSound::BSimpleGameSound(const entry_ref *inFile,
								   BGameSoundDevice *device)
 :	BGameSound(device)
{
}


BSimpleGameSound::BSimpleGameSound(const char *inFile,
								   BGameSoundDevice *device)
 :	BGameSound(device)
{
}


BSimpleGameSound::BSimpleGameSound(const void *inData,
								   size_t inFrameCount,
								   const gs_audio_format *format,
								   BGameSoundDevice *device)
 :	BGameSound(device)
{
}


BSimpleGameSound::BSimpleGameSound(const BSimpleGameSound &other)
 :	BGameSound(other.Device())
{
}


BSimpleGameSound::~BSimpleGameSound()
{
}


BGameSound *
BSimpleGameSound::Clone() const
{
	return NULL;
}


status_t
BSimpleGameSound::Perform(int32 selector,
						  void *data)
{
	return B_ERROR;
}


status_t
BSimpleGameSound::SetIsLooping(bool looping)
{
	return B_ERROR;
}


bool
BSimpleGameSound::IsLooping() const
{
	return false;
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


