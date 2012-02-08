/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christopher ML Zumwalt May (zummy@users.sf.net)
 *
 */


#include <GameSound.h>

#include <stdio.h>
#include <string.h>

#include "GameSoundBuffer.h"
#include "GameSoundDevice.h"


using std::nothrow;


// Local Defines ---------------------------------------------------------------

// BGameSound class ------------------------------------------------------------
BGameSound::BGameSound(BGameSoundDevice *device)
	:
	fSound(-1)
{
	// TODO: device is ignored!
	// NOTE: BeBook documents that BGameSoundDevice must currently always
	// be NULL...
	fDevice = GetDefaultDevice();
	fInitError = fDevice->InitCheck();
}


BGameSound::BGameSound(const BGameSound &other)
	:
	fSound(-1)
{
	memcpy(&fFormat, &other.fFormat, sizeof(gs_audio_format));
	// TODO: device from other is ignored!
	fDevice = GetDefaultDevice();

	fInitError = fDevice->InitCheck();
}


BGameSound::~BGameSound()
{
	if (fSound >= 0)
		fDevice->ReleaseBuffer(fSound);

	ReleaseDevice();
}


status_t
BGameSound::InitCheck() const
{
	return fInitError;
}


BGameSoundDevice *
BGameSound::Device() const
{
	// TODO: Must return NULL if default device is being used!
	return fDevice;
}


gs_id
BGameSound::ID() const
{
	// TODO: Should be 0 if no sound has been selected! But fSound
	// is initialized with -1 in the constructors.
	return fSound;
}


const gs_audio_format &
BGameSound::Format() const
{
	return fDevice->Format(fSound);
}


status_t
BGameSound::StartPlaying()
{
	fDevice->StartPlaying(fSound);
	return B_OK;
}


bool
BGameSound::IsPlaying()
{
	return fDevice->IsPlaying(fSound);
}


status_t
BGameSound::StopPlaying()
{
	fDevice->StopPlaying(fSound);
	return B_OK;
}


status_t
BGameSound::SetGain(float gain, bigtime_t duration)
{
	gs_attribute attribute;

	attribute.attribute = B_GS_GAIN;
	attribute.value = gain;
	attribute.duration = duration;
	attribute.flags = 0;

	return fDevice->SetAttributes(fSound, &attribute, 1);
}


status_t
BGameSound::SetPan(float pan, bigtime_t duration)
{
	gs_attribute attribute;

	attribute.attribute = B_GS_PAN;
	attribute.value = pan;
	attribute.duration = duration;
	attribute.flags = 0;

	return fDevice->SetAttributes(fSound, &attribute, 1);
}


float
BGameSound::Gain()
{
	gs_attribute attribute;

	attribute.attribute = B_GS_GAIN;
	attribute.flags = 0;

	if (fDevice->GetAttributes(fSound, &attribute, 1) != B_OK)
		return 0.0;

	return attribute.value;
}


float
BGameSound::Pan()
{
	gs_attribute attribute;

	attribute.attribute = B_GS_PAN;
	attribute.flags = 0;

	if (fDevice->GetAttributes(fSound, &attribute, 1) != B_OK)
		return 0.0;

	return attribute.value;
}


status_t
BGameSound::SetAttributes(gs_attribute *inAttributes, size_t inAttributeCount)
{
	return fDevice->SetAttributes(fSound, inAttributes, inAttributeCount);
}


status_t
BGameSound::GetAttributes(gs_attribute *outAttributes, size_t inAttributeCount)
{
	return fDevice->GetAttributes(fSound, outAttributes, inAttributeCount);
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
	return ::operator new(size);
}


void *
BGameSound::operator new(size_t size, const std::nothrow_t &nt) throw()
{
	return ::operator new(size, nt);
}


void
BGameSound::operator delete(void *ptr)
{
	::operator delete(ptr);
}


#if !__MWERKS__
//	there's a bug in MWCC under R4.1 and earlier
void
BGameSound::operator delete(void *ptr, const std::nothrow_t &nt) throw()
{
	::operator delete(ptr, nt);
}
#endif


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
	return in_maxCount;
}


status_t
BGameSound::SetInitError(status_t in_initError)
{
	fInitError = in_initError;
	return B_OK;
}


status_t
BGameSound::Init(gs_id handle)
{
	if (fSound < 0)
		fSound = handle;

	return B_OK;
}


#if 0
BGameSound &
BGameSound::operator=(const BGameSound &other)
{
	if (fSound)
		fDevice->ReleaseBuffer(fSound);

	fSound = other.fSound;
	fInitError = other.fInitError;

	// TODO: This would need to acquire the sound another time!

	return this;
}
#endif


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
