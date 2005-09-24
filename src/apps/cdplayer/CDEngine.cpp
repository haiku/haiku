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
CDAudioDevice gCDDevice;

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
	if(UpdateState())
		Notify();
}

void 
PeriodicWatcher::UpdateNow()
{
	if(UpdateState())
		Notify();
}


PlayState::PlayState(CDEngine *engine)
 :	oldState(kInit),
 	fEngine(engine)
{
}

bool
PlayState::UpdateState(void)
{
	CDState state = gCDDevice.GetState();
	if(state == kStopped)
	{
		if(fEngine->GetState() == kPlaying)
		{
			// this means we have come to the end of a song, but probably not
			// the last song in the playlist
			int16 next = sPlayList.GetNextTrack();
			if(next > 0)
			{
				gCDDevice.Play(next);
				return CurrentState(kPlaying);
			}
		}
		else
		{
			int16 count = gCDDevice.CountTracks();
			if(count != sPlayList.TrackCount())
				sPlayList.SetTrackCount(count);
			
			// TODO: Is this correct?
			
			/*
			int16 start = gCDDevice.GetTrack();
			if(start != sPlayList.StartingTrack())
				sPlayList.SetStartingTrack(start);
			*/
			
			return CurrentState(kStopped);
		}
	}
	
	if(state == kPlaying && fEngine->GetState() == kStopped)
	{
		// This happens when the CD drive is started by R5's player
		// or this app is started while the drive is playing. We should
		// reset the to start at the current track and finish at the
		// last one and send a notification.
		sPlayList.SetTrackCount(gCDDevice.CountTracks());
		sPlayList.SetStartingTrack(gCDDevice.GetTrack());
		return CurrentState(kPlaying);
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
	// It's possible that the state of the cd drive was changed by outside means
	// As a result, we want to make sure this hasn't happened. If it has, then we
	// need to update the playlist's position.
	int16 cdTrack, count;
	
	if(gCDDevice.GetState() == kPlaying)
	{
		cdTrack = gCDDevice.GetTrack();
		if(cdTrack != sPlayList.GetCurrentTrack())
			sPlayList.SetCurrentTrack(cdTrack);
		return CurrentState(cdTrack, trackCount);
	}

	// If we're not playing, just monitor the current track in the playlist
	count = gCDDevice.CountTracks();
	if(count>0)
	{
		cdTrack = sPlayList.GetCurrentTrack();
	}
	else
	{
		sPlayList.SetTrackCount(0);
		cdTrack=-1;
	}
	return CurrentState(cdTrack,count);
}

int32 
TrackState::GetNumTracks() const
{
	return gCDDevice.CountTracks();
}

bool
TrackState::CurrentState(int32 track, int32 count)
{
	if( (track != currentTrack) || (count != trackCount) )
	{
		currentTrack = track;
		trackCount = count;
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
	
	if(gCDDevice.GetTime(track,disc))
	{
		cdaudio_time ttrack;
		cdaudio_time tdisc;
		int16 ctrack = gCDDevice.GetTrack();
		
		gCDDevice.GetTimeForDisc(tdisc);
		gCDDevice.GetTimeForTrack(ctrack,ttrack);
		
		return CurrentState(track,ttrack,disc,tdisc);
	}
	else
	{
		track.minutes = -1;
		track.seconds = -1;
		disc.minutes = -1;
		disc.seconds = -1;
		return CurrentState(disc,disc,track,track);
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
	:	cddbQuery("us.freedb.org", 888),
		discID(-1)
{
}

bool 
CDContentWatcher::GetContent(BString *title, vector<BString> *tracks)
{
	if(discID == -1)
	{
		title->SetTo("");
		tracks->empty();
		tracks->push_back("");
		return true;
	}
	
	return cddbQuery.GetTitles(title, tracks, 1000000);
}

bool 
CDContentWatcher::UpdateState()
{
	int32 newDiscID = -1;
	
	if (engine->PlayStateWatcher()->GetState() == kNoCD) 
	{
		if(discID != -1)
		{
			discID = -1;
			return true;
		}
		else
			return false;
	}
	
	// Check the table of contents to see if the new one is different
	// from the old one whenever there is a CD in the drive
	newDiscID = gCDDevice.GetDiscID();
	
	if (discID == newDiscID)
		return false;
	
	// We have changed CDs, so we are not ready until the CDDB lookup finishes
	cddbQuery.SetToCD(gCDDevice.GetDrivePath());
	
	// Notify when the query is ready
	if(cddbQuery.Ready())
	{
		discID = newDiscID;
		return true;
	}
	
	return false;
}

VolumeState::VolumeState(void)
 : 	fVolume(-1)
{
}

bool
VolumeState::UpdateState(void)
{
	uint8 volume = gCDDevice.GetVolume();
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
	sPlayList.SetTrackCount(gCDDevice.CountTracks());
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
	gCDDevice.Pause();
	fEngineState = gCDDevice.GetState();
}

void 
CDEngine::Play()
{
	if(fEngineState == kPaused)
	{
		gCDDevice.Resume();
		fEngineState = gCDDevice.GetState();
	}
	else
	if(fEngineState == kPlaying)
	{
		Pause();
	}
	else
	{
		gCDDevice.Play(sPlayList.GetCurrentTrack());
		fEngineState = gCDDevice.GetState();
	}
}

void 
CDEngine::Stop()
{
	fEngineState = kStopped;
	gCDDevice.Stop();
}

void 
CDEngine::Eject()
{
	gCDDevice.Eject();
	fEngineState = gCDDevice.GetState();
}

void 
CDEngine::SkipOneForward()
{
	int16 track = sPlayList.GetNextTrack();
	
	if(track <= 0)
	{
		// force a "wrap around" when possible. This makes it
		// possible for the user to be able to, for example, jump
		// back to the first track from the last one with 1 button push
		track = sPlayList.GetFirstTrack();
		
		if(track <= 0)
			return;
	}
	
	CDState state = gCDDevice.GetState();
	if(state == kPlaying)
		gCDDevice.Play(track);
	
	if(state == kPaused)
	{
		gCDDevice.Play(track);
		gCDDevice.Pause();
	}
	trackState.UpdateNow();
}

void 
CDEngine::SkipOneBackward()
{
	int16 track = sPlayList.GetPreviousTrack();

	if(track <= 0)
	{
		// force a "wrap around" when possible. This way the user
		// can search backwards to get to a later track
		track = sPlayList.GetLastTrack();
		
		if(track <= 0)
			return;
	}
	
	CDState state = gCDDevice.GetState();
	
	if(state == kPlaying)
		gCDDevice.Play(track);
	
	if(state == kPaused)
	{
		gCDDevice.Play(track);
		gCDDevice.Pause();
	}
	trackState.UpdateNow();
}

void 
CDEngine::StartSkippingBackward()
{
	gCDDevice.StartRewind();
}

void 
CDEngine::StartSkippingForward()
{
	gCDDevice.StartFastFwd();
}

void 
CDEngine::StopSkipping()
{
	gCDDevice.StopFastFwd();
}

void 
CDEngine::SelectTrack(int32 trackNumber)
{
	sPlayList.SetCurrentTrack(trackNumber);
	if(GetState() == kPlaying)
		gCDDevice.Play(trackNumber);
	trackState.UpdateNow();
}

void
CDEngine::SetVolume(uint8 value)
{
	gCDDevice.SetVolume(value);
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
		sPlayList.SetTrackCount(gCDDevice.CountTracks());
	}
	else
	{
		// Not shuffled, so we will play the entire CD and randomly pick
		sPlayList.SetTrackCount(gCDDevice.CountTracks());
		sPlayList.SetShuffle(true);
	}
}

bool
CDEngine::IsShuffled(void)
{
	return sPlayList.IsShuffled();
}

void
CDEngine::ToggleRepeat(void)
{
	if(sPlayList.IsLoop())
		sPlayList.SetLoop(false);
	else
		sPlayList.SetLoop(true);
}

bool
CDEngine::IsRepeated(void)
{
	return sPlayList.IsLoop();
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
