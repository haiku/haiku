/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <GameSound.h>

BGameSound::BGameSound(BGameSoundDevice *device)
{
}


BGameSound::~BGameSound()
{
}


status_t
BGameSound::InitCheck() const
{
	return B_ERROR;
}


BGameSoundDevice *
BGameSound::Device() const
{
	return NULL;
}


gs_id
BGameSound::ID() const
{
	return 0;
}


const gs_audio_format &
BGameSound::Format() const
{
}


status_t
BGameSound::StartPlaying()
{
	return B_ERROR;
}


bool
BGameSound::IsPlaying()
{
	return false;
}


status_t
BGameSound::StopPlaying()
{
	return B_ERROR;
}


status_t
BGameSound::SetGain(float gain,
					bigtime_t duration)
{
	return B_ERROR;
}


status_t
BGameSound::SetPan(float pan,
				   bigtime_t duration)
{
	return B_ERROR;
}


float
BGameSound::Gain()
{
	return 0.0f;
}


float
BGameSound::Pan()
{
	return 0.0f;
}


status_t
BGameSound::SetAttributes(gs_attribute *inAttributes,
						  size_t inAttributeCount)
{
	return B_ERROR;
}


status_t
BGameSound::GetAttributes(gs_attribute *outAttributes,
						  size_t inAttributeCount)
{
	return B_ERROR;
}


status_t
BGameSound::Perform(int32 selector,
					void *data)
{
	return B_ERROR;
}


void *
BGameSound::operator new(size_t size)
{
	return NULL;
}


void
BGameSound::operator delete(void *ptr)
{
}


status_t
BGameSound::SetMemoryPoolSize(size_t in_poolSize)
{
	return B_ERROR;
}


status_t
BGameSound::LockMemoryPool(bool in_lockInCore)
{
	return B_ERROR;
}


int32
BGameSound::SetMaxSoundCount(int32 in_maxCount)
{
	return 0;
}


status_t
BGameSound::SetInitError(status_t in_initError)
{
	return B_ERROR;
}


status_t
BGameSound::Init(gs_id handle)
{
	return B_ERROR;
}


BGameSound::BGameSound(const BGameSound &other)
{
}


BGameSound &
BGameSound::operator=(const BGameSound &other)
{
}


/* unimplemented for protection of the user:
 *
 * BGameSound::BGameSound()
 */
 

status_t
BGameSound::_Reserved_BGameSound_0(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_1(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_2(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_3(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_4(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_5(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_6(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_7(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_8(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_9(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_10(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_11(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_12(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_13(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_14(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_15(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_16(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_17(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_18(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_19(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_20(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_21(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_22(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_23(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_24(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_25(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_26(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_27(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_28(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_29(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_30(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_31(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_32(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_33(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_34(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_35(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_36(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_37(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_38(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_39(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_40(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_41(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_42(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_43(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_44(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_45(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_46(int32 arg, ...)
{
	return B_ERROR;
}


status_t
BGameSound::_Reserved_BGameSound_47(int32 arg, ...)
{
	return B_ERROR;
}


