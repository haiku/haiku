//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		BGameSoundDevice.cpp
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	Manages the game producer. The class may change with out
//					notice and was only inteneded for use by the GameKit at
//					this time. Use at your own risk.
//------------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <List.h>
#include <Locker.h>
#include <MediaRoster.h>
#include <MediaAddOn.h>
#include <TimeSource.h>
#include <MediaTheme.h>

#include "GSUtility.h"
#include "GameSoundDevice.h"
#include "GameSoundBuffer.h"
#include "GameProducer.h"

// BGameSoundDevice definitions ------------------------------------
const int32 kInitSoundCount = 32;
const int32 kGrowth = 16;

static int32 sDeviceCount = 0;
static BGameSoundDevice* sDevice = NULL;
static BLocker sDeviceRefCountLock = BLocker("GameSound device lock");


BGameSoundDevice*
GetDefaultDevice()
{
	BAutolock _(sDeviceRefCountLock);

	if (!sDevice)
		sDevice = new BGameSoundDevice();

	sDeviceCount++;
	return sDevice;
}


void
ReleaseDevice()
{
	BAutolock _(sDeviceRefCountLock);

	sDeviceCount--;

	if (sDeviceCount <= 0) {
		delete sDevice;
		sDevice = NULL;
	}
}


// BGameSoundDevice -------------------------------------------------------
BGameSoundDevice::BGameSoundDevice()
	:
	fIsConnected(false),
	fSoundCount(kInitSoundCount)
{
	memset(&fFormat, 0, sizeof(gs_audio_format));

	fInitError = B_OK;

	fSounds = new GameSoundBuffer*[kInitSoundCount];
	for (int32 i = 0; i < kInitSoundCount; i++)
		fSounds[i] = NULL;
}


BGameSoundDevice::~BGameSoundDevice()
{
	// We need to stop all the sounds before we stop the mixer
	for (int32 i = 0; i < fSoundCount; i++) {
		if (fSounds[i])
			fSounds[i]->StopPlaying();
		delete fSounds[i];
	}

	delete[] fSounds;
}


status_t
BGameSoundDevice::InitCheck() const
{
	return fInitError;
}


const gs_audio_format&
BGameSoundDevice::Format() const
{
	return fFormat;
}


const gs_audio_format&
BGameSoundDevice::Format(gs_id sound) const
{
	return fSounds[sound - 1]->Format();
}


void
BGameSoundDevice::SetInitError(status_t error)
{
	fInitError = error;
}


status_t
BGameSoundDevice::CreateBuffer(gs_id* sound, const gs_audio_format* format,
	const void* data, int64 frames)
{
	if (frames <= 0 || !sound)
		return B_BAD_VALUE;

	status_t err = B_MEDIA_TOO_MANY_BUFFERS;
	int32 position = AllocateSound();

	if (position >= 0) {
		media_node systemMixer;
		BMediaRoster::Roster()->GetAudioMixer(&systemMixer);
		fSounds[position] = new SimpleSoundBuffer(format, data, frames);
		err = fSounds[position]->Connect(&systemMixer);
	}

	if (err == B_OK)
		*sound = gs_id(position + 1);
	return err;
}


status_t
BGameSoundDevice::CreateBuffer(gs_id* sound, const void* object,
	const gs_audio_format* format, size_t inBufferFrameCount,
	size_t inBufferCount)
{
	if (!object || !sound)
		return B_BAD_VALUE;

	status_t err = B_MEDIA_TOO_MANY_BUFFERS;
	int32 position = AllocateSound();

	if (position >= 0) {
		media_node systemMixer;
		BMediaRoster::Roster()->GetAudioMixer(&systemMixer);
		fSounds[position] = new StreamingSoundBuffer(format, object,
			inBufferFrameCount, inBufferCount);
		err = fSounds[position]->Connect(&systemMixer);
	}

	if (err == B_OK)
		*sound = gs_id(position + 1);
	return err;
}


void
BGameSoundDevice::ReleaseBuffer(gs_id sound)
{
	if (sound <= 0)
		return;

	if (fSounds[sound - 1]) {
		// We must stop playback befor destroying the sound or else
		// we may receive fatel errors.
		fSounds[sound - 1]->StopPlaying();

		delete fSounds[sound - 1];
		fSounds[sound - 1] = NULL;
	}
}


status_t
BGameSoundDevice::Buffer(gs_id sound, gs_audio_format* format, void*& data)
{
	if (!format || sound <= 0)
		return B_BAD_VALUE;

	memcpy(format, &fSounds[sound - 1]->Format(), sizeof(gs_audio_format));
	if (fSounds[sound - 1]->Data()) {
		data = malloc(format->buffer_size);
		memcpy(data, fSounds[sound - 1]->Data(), format->buffer_size);
	} else
		data = NULL;

	return B_OK;
}


status_t
BGameSoundDevice::StartPlaying(gs_id sound)
{
	if (sound <= 0)
		return B_BAD_VALUE;

	if (!fSounds[sound - 1]->IsPlaying()) {
		// tell the producer to start playing the sound
		return fSounds[sound - 1]->StartPlaying();
	}

	fSounds[sound - 1]->Reset();
	return EALREADY;
}


status_t
BGameSoundDevice::StopPlaying(gs_id sound)
{
	if (sound <= 0)
		return B_BAD_VALUE;

	if (fSounds[sound - 1]->IsPlaying()) {
		// Tell the producer to stop play this sound
		fSounds[sound - 1]->Reset();
		return fSounds[sound - 1]->StopPlaying();
	}

	return EALREADY;
}


bool
BGameSoundDevice::IsPlaying(gs_id sound)
{
	if (sound <= 0)
		return false;
	return fSounds[sound - 1]->IsPlaying();
}


status_t
BGameSoundDevice::GetAttributes(gs_id sound, gs_attribute* attributes,
	size_t attributeCount)
{
	if (!fSounds[sound - 1])
		return B_ERROR;

	return fSounds[sound - 1]->GetAttributes(attributes, attributeCount);
}


status_t
BGameSoundDevice::SetAttributes(gs_id sound, gs_attribute* attributes,
	size_t attributeCount)
{
	if (!fSounds[sound - 1])
		return B_ERROR;

	return fSounds[sound - 1]->SetAttributes(attributes, attributeCount);
}


int32
BGameSoundDevice::AllocateSound()
{
	for (int32 i = 0; i < fSoundCount; i++)
		if (!fSounds[i])
			return i;

	// we need to allocate new space for the sound
	GameSoundBuffer ** sounds = new GameSoundBuffer*[fSoundCount + kGrowth];
	for (int32 i = 0; i < fSoundCount; i++)
		sounds[i] = fSounds[i];

	for (int32 i = fSoundCount; i < fSoundCount + kGrowth; i++)
		sounds[i] = NULL;

	// replace the old list
	delete [] fSounds;
	fSounds = sounds;
	fSoundCount += kGrowth;

	return fSoundCount - kGrowth;
}

