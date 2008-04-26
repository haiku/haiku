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

#include <List.h>
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

static int32 deviceCount = 0;
static BGameSoundDevice* theDevice = NULL;

BGameSoundDevice * GetDefaultDevice()
{
	if (!theDevice)
		theDevice = new BGameSoundDevice();
		
	deviceCount++;
	return theDevice;
}

void ReleaseDevice()
{
	deviceCount--;
	
	if (deviceCount <= 0) {
		delete theDevice;
		theDevice = NULL;
	}
}

// BGameSoundDevice -------------------------------------------------------
BGameSoundDevice::BGameSoundDevice()
	:   fIsConnected(false),
		fSoundCount(kInitSoundCount)
{
	fConnection = new Connection;
	memset(&fFormat, 0, sizeof(gs_audio_format));
	
	fInitError = Connect();
	
	fSounds = new GameSoundBuffer*[kInitSoundCount];
	for (int32 i = 0; i < kInitSoundCount; i++)
		fSounds[i] = NULL;	
}


BGameSoundDevice::~BGameSoundDevice()
{
	BMediaRoster* r = BMediaRoster::Roster();

	// We need to stop all the sounds before we stop the mixer
	for (int32 i = 0; i < fSoundCount; i++) {
		if (fSounds[i])
			fSounds[i]->StopPlaying();
		delete fSounds[i];
	}
	
	if (fIsConnected) {
		// stop the nodes if they are running
		r->StopNode(fConnection->producer, 0, true);		// synchronous stop
	
		// Ordinarily we'd stop *all* of the nodes in the chain at this point.  However,
		// one of the nodes is the System Mixer, and stopping the Mixer is a Bad Idea (tm).
		// So, we just disconnect from it, and release our references to the nodes that
		// we're using.  We *are* supposed to do that even for global nodes like the Mixer.
		r->Disconnect(fConnection->producer.node, fConnection->source,
							fConnection->consumer.node, fConnection->destination);
		
		r->ReleaseNode(fConnection->producer);
		r->ReleaseNode(fConnection->consumer);
	}
			
	delete [] fSounds;
	delete fConnection;
}


status_t
BGameSoundDevice::InitCheck() const
{
	return fInitError;
}


const gs_audio_format &
BGameSoundDevice::Format() const
{
	return fFormat;
}


const gs_audio_format &
BGameSoundDevice::Format(gs_id sound) const
{
	return fSounds[sound-1]->Format();
}


void
BGameSoundDevice::SetInitError(status_t error)
{
	fInitError = error;
}


status_t
BGameSoundDevice::CreateBuffer(gs_id * sound,
								const gs_audio_format * format,
								const void * data,
								int64 frames)
{
	if (frames <= 0 || !sound) 
		return B_BAD_VALUE;
	
	status_t err = B_MEDIA_TOO_MANY_BUFFERS;	
	int32 position = AllocateSound();
	
	if (position >= 0) {
		fSounds[position] = new SimpleSoundBuffer(format, data, frames);
		err = fSounds[position]->Connect(&fConnection->producer);
	}	
	
	*sound = gs_id(position+1);
	return err;		
}


status_t
BGameSoundDevice::CreateBuffer(gs_id * sound,
								const void * object,
								const gs_audio_format * format)
{
	if (!object || !sound) 
		return B_BAD_VALUE;
	
	status_t err = B_MEDIA_TOO_MANY_BUFFERS;	
	int32 position = AllocateSound();
	
	if (position >= 0) {
		fSounds[position] = new StreamingSoundBuffer(format, object);
		err = fSounds[position]->Connect(&fConnection->producer);
	}	
	
	*sound = gs_id(position+1);
	return err;			
}


void
BGameSoundDevice::ReleaseBuffer(gs_id sound)
{
	if (sound <= 0)
		return;

	if (fSounds[sound-1]) {
		// We must stop playback befor destroying the sound or else
		// we may recieve fatel errors.
		fSounds[sound-1]->StopPlaying();
		
		delete fSounds[sound-1];
		fSounds[sound-1] = NULL;
	}	
}
	

status_t
BGameSoundDevice::Buffer(gs_id sound,
						 gs_audio_format * format,
						 void * data)
{
	if (!format || sound <= 0)
		return B_BAD_VALUE;

	memcpy(format, &fSounds[sound-1]->Format(), sizeof(gs_audio_format));
	
	if (fSounds[sound-1]->Data()) {
		data = malloc(format->buffer_size);
		memcpy(data, fSounds[sound-1]->Data(), format->buffer_size);
	} else
		data = NULL;
	
	return B_OK;	
}

status_t
BGameSoundDevice::StartPlaying(gs_id sound)
{
	if (sound <= 0)
		return B_BAD_VALUE;
		
	if (!fSounds[sound-1]->IsPlaying()) {
		// tell the producer to start playing the sound
		return fSounds[sound-1]->StartPlaying();
	}
	
	fSounds[sound-1]->Reset();
	return EALREADY;
}


status_t
BGameSoundDevice::StopPlaying(gs_id sound)
{
	if (sound <= 0)
		return B_BAD_VALUE;
	
	if (fSounds[sound-1]->IsPlaying()) {
		// Tell the producer to stop play this sound
		fSounds[sound-1]->Reset();  
		return fSounds[sound-1]->StopPlaying();
	}
	
	return EALREADY;
}


bool
BGameSoundDevice::IsPlaying(gs_id sound)
{
	if (sound <= 0)
		return false;
	return fSounds[sound-1]->IsPlaying();
}	


status_t
BGameSoundDevice::GetAttributes(gs_id sound,
								gs_attribute * attributes,
								size_t attributeCount)
{
	if (!fSounds[sound-1]) 
		return B_ERROR;
		
	return fSounds[sound-1]->GetAttributes(attributes, attributeCount); 
}
		

status_t
BGameSoundDevice::SetAttributes(gs_id sound,
								gs_attribute * attributes,
								size_t attributeCount)
{
	if (!fSounds[sound-1]) 
		return B_ERROR;
	
	return fSounds[sound-1]->SetAttributes(attributes, attributeCount);
}				


status_t
BGameSoundDevice::Connect()
{
	BMediaRoster* r = BMediaRoster::Roster();
	status_t err;
	
	// create your own audio mixer
	dormant_node_info mixer_dormant_info;
	int32 mixer_count = 1; // for now, we only care about the first  we find.
	err = r->GetDormantNodes(&mixer_dormant_info, &mixer_count, 0, 0, 0, B_SYSTEM_MIXER, 0);
	if (err != B_OK) 
		return err;
	
	//fMixer = new media_node;
	err = r->InstantiateDormantNode(mixer_dormant_info, &fConnection->producer);
	if (err != B_OK) 
		return err;
	
	// retieve the system's audio mixer
	err = r->GetAudioMixer(&fConnection->consumer);
	if (err != B_OK) 
		return err;
	
	int32 count = 1;
	media_input mixerInput;
	err = r->GetFreeInputsFor(fConnection->consumer, &mixerInput, 1, &count);
	if (err != B_OK) 
		return err;
	
	count = 1;
	media_output mixerOutput;
	err = r->GetFreeOutputsFor(fConnection->producer, &mixerOutput, 1, &count);
	if (err != B_OK) 
		return err;
	
	media_format format(mixerOutput.format);
	err = r->Connect(mixerOutput.source, mixerInput.destination, &format, &mixerOutput, &mixerInput);
	if (err != B_OK) 
		return err;	
	
	// set the producer's time source to be the "default" time source, which
	// the Mixer uses too.
	r->GetTimeSource(&fConnection->timeSource);
	r->SetTimeSourceFor(fConnection->producer.node, fConnection->timeSource.node);
	
	// Start our mixer's time source if need be. Chances are, it won't need to be, 
	// but if we forget to do this, our mixer might not do anything at all.
	BTimeSource* mixerTimeSource = r->MakeTimeSourceFor(fConnection->producer);
	if (!mixerTimeSource) 
		return B_ERROR;

	if (!mixerTimeSource->IsRunning()) {
		status_t err = r->StartNode(mixerTimeSource->Node(), BTimeSource::RealTime());
		if (err != B_OK) {
			mixerTimeSource->Release();
			return err;
		}
	}

	// Start up our mixer
	bigtime_t tpNow = mixerTimeSource->Now();
	err = r->StartNode(fConnection->producer, tpNow + 10000);
	mixerTimeSource->Release();
	if (err != B_OK) 
		return err;
	
	// the inputs and outputs might have been reassigned during the
	// nodes' negotiation of the Connect().  That's why we wait until
	// after Connect() finishes to save their contents.
	fConnection->format = format;
	fConnection->source = mixerOutput.source;
	fConnection->destination = mixerInput.destination;

	// Set an appropriate run mode for the producer
	r->SetRunModeNode(fConnection->producer, BMediaNode::B_INCREASE_LATENCY);

	media_to_gs_format(&fFormat, &format.u.raw_audio);
	fIsConnected = true;
	return B_OK;
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

