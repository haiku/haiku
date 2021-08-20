//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		StreamingGameSound.cpp
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	BStreamingGameSound is a class for all kinds of streaming
//					(data not known beforehand) game sounds.
//------------------------------------------------------------------------------


#include "StreamingGameSound.h"

#include "GameSoundDevice.h"


BStreamingGameSound::BStreamingGameSound(size_t inBufferFrameCount,
	const gs_audio_format *format, size_t inBufferCount,
	BGameSoundDevice *device)
	:
	BGameSound(device),
	fStreamHook(NULL),
	fStreamCookie(NULL)
{
	if (InitCheck() == B_OK) {
		status_t error = SetParameters(inBufferFrameCount, format,
			inBufferCount);
		SetInitError(error);
	}
}


BStreamingGameSound::BStreamingGameSound(BGameSoundDevice *device)
	:
	BGameSound(device),
	fStreamHook(NULL),
	fStreamCookie(NULL)
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
BStreamingGameSound::SetStreamHook(void (*hook)(void* inCookie, void* inBuffer,
	size_t inByteCount, BStreamingGameSound * me), void * cookie)
{
	fStreamHook = hook;
	fStreamCookie = cookie;

	return B_OK;
}


void
BStreamingGameSound::FillBuffer(void *inBuffer,
								size_t inByteCount)
{
	if (fStreamHook)
		(fStreamHook)(fStreamCookie, inBuffer, inByteCount, this);
}


status_t
BStreamingGameSound::Perform(int32 selector, void *data)
{
	return B_ERROR;
}


status_t
BStreamingGameSound::SetAttributes(gs_attribute * inAttributes,
									size_t inAttributeCount)
{
	return BGameSound::SetAttributes(inAttributes, inAttributeCount);
}


status_t
BStreamingGameSound::SetParameters(size_t inBufferFrameCount,
	const gs_audio_format *format, size_t inBufferCount)
{
	gs_id sound;
	status_t error = Device()->CreateBuffer(&sound, this, format,
		inBufferFrameCount, inBufferCount);
	if (error != B_OK) return error;

	return BGameSound::Init(sound);
}


bool
BStreamingGameSound::Lock()
{
	return fLock.Lock();
}


void
BStreamingGameSound::Unlock()
{
	fLock.Unlock();
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
