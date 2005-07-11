/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Debug.h>
#include <Entry.h>
#include <Directory.h>
#include <Path.h>
#include <errno.h>
#include "scsi.h"
#include "CDEngine.h"
#include "PlayList.h"

static PlayList sPlayList;
static CDAudioDevice sCDDevice;

const bigtime_t kPulseRate = 500000;

PeriodicWatcher::PeriodicWatcher(void)
{
}

 
BHandler *
PeriodicWatcher::RecipientHandler() const
{
	 return engine;
}

void
PeriodicWatcher::DoPulse()
{
	// control the period here
	if (UpdateState())
		Notify();
}

void 
PeriodicWatcher::UpdateNow()
{
	UpdateState();
}


PlayState::PlayState(CDEngine *engine)
 :	oldState(kNoCD),
 	fEngine(engine)
{
}

bool
PlayState::UpdateState(void)
{
	CDState state = sCDDevice.GetState();
	if( state == kStopped && fEngine->GetState() == kPlaying)
	{
		// this means we have come to the end of a song, but probably not
		// the last song in the playlist
		int16 next = sPlayList.GetNextTrack();
		if(next > 0)
		{
			sCDDevice.Play(next);
			return CurrentState(kPlaying);
		}
	}
	
	return CurrentState(state);
}

CDState
PlayState::GetState() const
{
	return oldState;
}

bool
PlayState::CurrentState(CDState newState)
{
	if (newState != oldState) 
	{
		oldState = newState;
		return true;
	}
	return false;
}

TrackState::TrackState(void)
 : currentTrack(0)
{
}

int32
TrackState::GetTrack() const
{
	return currentTrack;
}

bool
TrackState::UpdateState()
{
	return CurrentState(sPlayList.GetCurrentTrack());
}

int32 
TrackState::GetNumTracks() const
{
	return sCDDevice.CountTracks();
}

bool
TrackState::CurrentState(int32 track)
{
	if (track != currentTrack) 
	{
		currentTrack = track;
		return true;
	}
	return false;
}

bool 
TimeState::UpdateState()
{
	// check the current CD time and force a notification to
	// be sent if it changed from last time
	// currently only supports global time
	
	cdaudio_time track;
	cdaudio_time disc;
	
	if(sCDDevice.GetTime(track,disc))
	{
		cdaudio_time ttrack;
		cdaudio_time tdisc;
		int16 ctrack = sCDDevice.GetTrack();
		
		sCDDevice.GetTimeForDisc(tdisc);
		sCDDevice.GetTimeForTrack(ctrack,ttrack);
		
		return CurrentState(track,ttrack,disc,tdisc);
	}
	else
	{
		track.minutes = -1;
		track.seconds = -1;
		disc.minutes = -1;
		disc.seconds = -1;
		CurrentState(disc,disc,track,track);
		return false;
	}
}

bool
TimeState::CurrentState(cdaudio_time tracktime, cdaudio_time totaltracktime,
						cdaudio_time disctime,	cdaudio_time totaldisctime)
{
	if (disctime.minutes == fDiscTime.minutes && disctime.seconds == fDiscTime.seconds)
		return false;
	
	fDiscTime = disctime;
	fTotalDiscTime = totaldisctime;
	
	fTrackTime = tracktime;
	fTotalTrackTime = totaltracktime;
	
	return true;
}

void 
TimeState::GetDiscTime(int32 &minutes, int32 &seconds) const
{
	minutes = fDiscTime.minutes;
	seconds = fDiscTime.seconds;
}

void
TimeState::GetTotalDiscTime(int32 &minutes, int32 &seconds) const
{
	minutes = fTotalDiscTime.minutes;
	seconds = fTotalDiscTime.seconds;
}

void 
TimeState::GetTrackTime(int32 &minutes, int32 &seconds) const
{
	minutes = fTrackTime.minutes;
	seconds = fTrackTime.seconds;
}

void
TimeState::GetTotalTrackTime(int32 &minutes, int32 &seconds) const
{
	minutes = fTotalTrackTime.minutes;
	seconds = fTotalTrackTime.seconds;
}


CDContentWatcher::CDContentWatcher(void)
	:	cddbQuery("us.freedb.org", 888, true),
		discID(-1),
		fReady(false)
{
}

bool 
CDContentWatcher::GetContent(BString *title, vector<BString> *tracks)
{
	return cddbQuery.GetTitles(title, tracks, 1000000);
}

bool 
CDContentWatcher::UpdateState()
{
	int32 newDiscID = -1;
	
	// Check the table of contents to see if the new one is different
	// from the old one whenever there is a CD in the drive
	if (engine->PlayStateWatcher()->GetState() != kNoCD) 
	{
		newDiscID = sCDDevice.GetDiscID();
		
		if (discID == newDiscID)
			return false;
		
		// We have changed CDs, so we are not ready until the CDDB lookup finishes
		cddbQuery.SetToCD(sCDDevice.GetDrivePath());
		fReady=false;
	}
	
	// If the CD has changed and the CDDB query is ready, we set to true so that
	// when UpdateState returns, a notification is sent
	bool result = ( (fReady != cddbQuery.Ready()) && (newDiscID != discID) );
	
	if(result)
	{
		fReady = true;
		discID = newDiscID;
	}

	return result;
}

VolumeState::VolumeState(void)
 : 	fVolume(-1)
{
}

bool
VolumeState::UpdateState(void)
{
	uint8 volume = sCDDevice.GetVolume();
	if(fVolume == volume)
		return false;
	
	fVolume = volume;
	return true;
}

int32
VolumeState::GetVolume(void) const
{
	return fVolume;
}

CDEngine::CDEngine(void)
	:	BHandler("CDEngine"),
		playState(this),
		fEngineState(kStopped)
{
	sPlayList.SetTrackCount(sCDDevice.CountTracks());
}

CDEngine::~CDEngine()
{
}

void 
CDEngine::AttachedToLooper(BLooper *looper)
{
	looper->AddHandler(this);
	playState.AttachedToLooper(this);
	trackState.AttachedToLooper(this);
	timeState.AttachedToLooper(this);
	volumeState.AttachedToLooper(this);
	contentWatcher.AttachedToLooper(this);
}

void 
CDEngine::Pause()
{
	sCDDevice.Pause();
	fEngineState = sCDDevice.GetState();
}

void 
CDEngine::Play()
{
	if(fEngineState == kPaused)
	{
		sCDDevice.Resume();
		fEngineState = sCDDevice.GetState();
	}
	else
	if(fEngineState == kPlaying)
	{
		Pause();
	}
	else
	{
		sCDDevice.Play(sPlayList.GetCurrentTrack());
		fEngineState = sCDDevice.GetState();
	}
}

void 
CDEngine::Stop()
{
	fEngineState = kStopped;
	sCDDevice.Stop();
}

void 
CDEngine::Eject()
{
	sCDDevice.Eject();
	fEngineState = sCDDevice.GetState();
}

void 
CDEngine::SkipOneForward()
{
	int16 track = sPlayList.GetNextTrack();
	if(track <= 0)
		return;
	
	CDState state = sCDDevice.GetState();
	if(state == kPlaying)
		sCDDevice.Play(track);
	
//	if(state == kPaused)
//		sCDDevice.Pause();
}

void 
CDEngine::SkipOneBackward()
{
	int16 track = sPlayList.GetPreviousTrack();
	if(track <= 0)
		return;
	
	CDState state = sCDDevice.GetState();
	
	if(state == kPlaying)
		sCDDevice.Play(track);
	
//	if(state == kPaused)
//		sCDDevice.Pause();
}

void 
CDEngine::StartSkippingBackward()
{
	sCDDevice.StartRewind();
}

void 
CDEngine::StartSkippingForward()
{
	sCDDevice.StartFastFwd();
}

void 
CDEngine::StopSkipping()
{
	sCDDevice.StopFastFwd();
}

void 
CDEngine::SelectTrack(int32 trackNumber)
{
	sCDDevice.Play(trackNumber);
}

void
CDEngine::SetVolume(uint8 value)
{
	sCDDevice.SetVolume(value);
}

void
CDEngine::ToggleShuffle(void)
{
	if(sPlayList.IsShuffled())
	{
		// If already in random mode, we will play to the end of the cd
		int16 track = sPlayList.GetCurrentTrack();
		sPlayList.SetShuffle(false);
		sPlayList.SetStartingTrack(track);
		sPlayList.SetTrackCount(sCDDevice.CountTracks());
	}
	else
	{
		// Not shuffled, so we will play the entire CD and randomly pick
		sPlayList.SetTrackCount(sCDDevice.CountTracks());
		sPlayList.SetShuffle(true);
	}
}

void
CDEngine::ToggleRepeat(void)
{
	if(sPlayList.IsLoop())
		sPlayList.SetLoop(false);
	else
		sPlayList.SetLoop(true);
}


void
CDEngine::DoPulse()
{
	// this is the CDEngine's heartbeat; Since it is a Notifier, it checks if
	// any values changed since the last hearbeat and sends notices to observers

	bigtime_t time = system_time();
	if (time > lastPulse && time < lastPulse + kPulseRate)
		return;
	
	// every pulse rate have all the different state watchers check the
	// curent state and send notifications if anything changed
	
	lastPulse = time;

	playState.DoPulse();
	trackState.DoPulse();
	timeState.DoPulse();
	volumeState.DoPulse();
	contentWatcher.DoPulse();
}

void 
CDEngine::MessageReceived(BMessage *message)
{
	// handle observing
	if (!Notifier::HandleObservingMessages(message)	&& 
		!CDEngineFunctorFactory::DispatchIfFunctionObject(message))
		BHandler::MessageReceived(message);
}
