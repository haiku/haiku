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

const bigtime_t kPulseRate = 500000;

PeriodicWatcher::PeriodicWatcher(int devicefd)
	:	Notifier(),
		devicefd(devicefd)
{
}

 
PeriodicWatcher::PeriodicWatcher(BMessage *)
	:	devicefd(-1)
{
	// not implemented yet
	PRINT(("under construction"));
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


PlayState::PlayState(int devicefd)
	:	PeriodicWatcher(devicefd),
		oldState(kNoCD)
{
}

bool
PlayState::UpdateState()
{
	// check the current CD play state and force a notification to
	// be sent if it changed from last time
	scsi_position pos;
	status_t media_status = B_DEV_NO_MEDIA;
	ioctl(devicefd, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status));
	if (media_status != B_NO_ERROR)
		return CurrentState(kNoCD);

	status_t result = ioctl(devicefd, B_SCSI_GET_POSITION, &pos);

	if (result != B_NO_ERROR)
		return CurrentState(kNoCD);
	else if ((!pos.position[1]) || (pos.position[1] >= 0x13) ||
	   ((pos.position[1] == 0x12) && (!pos.position[6])))
		return CurrentState(kStopped);
	else if (pos.position[1] == 0x11)
		return CurrentState(kPlaying);
	else
		return CurrentState(kPaused);
}

CDState
PlayState::GetState() const
{
	return oldState;
}

bool
PlayState::CurrentState(CDState newState)
{
	if (newState != oldState) {
		oldState = newState;
		return true;
	}
	return false;
}

TrackState::TrackState(int devicefd)
	:	PeriodicWatcher(devicefd),
		currentTrack(0)
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
	// check the current CD track number and force a notification to
	// be sent if it changed from last time
	scsi_position pos;

	status_t media_status = B_DEV_NO_MEDIA;
	ioctl(devicefd, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status));
	if (media_status != B_NO_ERROR)
		return CurrentState(-1);

	status_t result = ioctl(devicefd, B_SCSI_GET_POSITION, &pos);

	if (result != B_NO_ERROR)
		return CurrentState(-1);
		
	if (!pos.position[1] || pos.position[1] >= 0x13
		|| (pos.position[1] == 0x12 && !pos.position[6]))
		return CurrentState(0);
	else
		return CurrentState(pos.position[6]);
}

int32 
TrackState::GetNumTracks() const
{
	// get the number of tracks on the current CD

	scsi_toc toc;

	status_t result = ioctl(devicefd, B_SCSI_GET_TOC, &toc);

	if (result != B_NO_ERROR)
		return 0;
	
	return toc.toc_data[3];
}

bool
TrackState::CurrentState(int32 track)
{
	if (track != currentTrack) {
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
	scsi_position pos;

	status_t media_status = B_DEV_NO_MEDIA;
	ioctl(devicefd, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status));
	if (media_status != B_NO_ERROR)
		return CurrentState(-1, -1, -1, -1);
	
	status_t result = ioctl(devicefd, B_SCSI_GET_POSITION, &pos);
	
	if (result != B_NO_ERROR)
		return CurrentState(-1, -1, -1, -1);
	else 
	if ((!pos.position[1]) || (pos.position[1] >= 0x13) ||
		((pos.position[1] == 0x12) && (!pos.position[6])))
	{
		// This indicates that we have a CD, but we are stopped. We *could* simply
		// return all 0's, but because the GUI expects valid numbers or -1's, we
		// return -1's.
		return CurrentState(-1, -1, -1, -1);
	}
	else
		return CurrentState(pos.position[9], pos.position[10], pos.position[13], pos.position[14]);
}

bool 
TimeState::CurrentState(int32 dmin, int32 dsec, int32 tmin, int32 tsec)
{
	if (dmin == fDiscMinutes && dsec == fDiscSeconds)
		return false;
	
	fDiscMinutes = dmin;
	fDiscSeconds = dsec;
	fTrackMinutes = tmin;
	fTrackSeconds = tsec;
	
	return true;
}

void 
TimeState::GetDiscTime(int32 &minutes, int32 &seconds) const
{
	minutes = fDiscMinutes;
	seconds = fDiscSeconds;
}

void 
TimeState::GetTrackTime(int32 &minutes, int32 &seconds) const
{
	minutes = fTrackMinutes;
	seconds = fTrackSeconds;
}


CDContentWatcher::CDContentWatcher(int devicefd)
	:	PeriodicWatcher(devicefd),
		cddbQuery("us.freedb.org", 888, true),
		discID(-1),
		fReady(false)
{
}

CDContentWatcher::CDContentWatcher(BMessage *message)
	:	PeriodicWatcher(message),
		cddbQuery("us.freedb.org", 888, true),
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
	bool newDiscID = -1;
	
	// Check the table of contents to see if the new one is different
	// from the old one whenever there is a CD in the drive
	if (engine->PlayStateWatcher()->GetState() != kNoCD) 
	{
		scsi_toc toc;
		ioctl(devicefd, B_SCSI_GET_TOC, &toc);
		newDiscID = cddbQuery.GetDiscID(&toc);
		
		if (discID == newDiscID)
			return false;
		
		// We have changed CDs, so we are not ready until the CDDB lookup finishes
		cddbQuery.SetToCD(&toc);
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

VolumeState::VolumeState(int devicefd)
 : 	PeriodicWatcher(devicefd),
 	fVolume(-1)
{
}

bool
VolumeState::UpdateState(void)
{
	scsi_volume vol;
	ioctl(devicefd, B_SCSI_GET_VOLUME, &vol);
	if(vol.port0_volume == fVolume)
		return false;
	
	fVolume = vol.port0_volume;
	return true;
}

int32
VolumeState::GetVolume(void) const
{
	return fVolume;
}

CDEngine::CDEngine(int devicefd)
	:	BHandler("CDEngine"),
		devicefd(devicefd),
		playState(devicefd),
		trackState(devicefd),
		timeState(devicefd),
		volumeState(devicefd),
		contentWatcher(devicefd)
{
}

CDEngine::CDEngine(BMessage *message)
	:	BHandler(message),
		devicefd(-1),
		playState(message),
		trackState(message),
		timeState(message),
		volumeState(message),
		contentWatcher(devicefd)
{
}

CDEngine::~CDEngine()
{
	if(devicefd >= 0)
		close(devicefd);
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
CDEngine::PlayOrPause()
{
	// if paused, or stopped, plays, if playing, pauses
	playState.UpdateNow();
	switch (playState.GetState()) {
		case kNoCD:
			Play();
			return;
		case kStopped:
			Play();
			break;
		case kPaused:
			PlayContinue();
			break;
		case kPlaying:
			Pause();
			break;
		default:
			break;
	}
}

void 
CDEngine::Pause()
{
	// pause the CD
	status_t result = ioctl(devicefd, B_SCSI_PAUSE_AUDIO);
	if (result != B_NO_ERROR) {
		PRINT(("error %s pausing\n", strerror(errno)));
		return;
	}
}

void 
CDEngine::Play()
{
	// play the CD
	if (playState.GetState() == kNoCD) 
	{
		// no CD available, bail out
		ioctl(devicefd, B_LOAD_MEDIA, 0, 0);
		return;
	}
	
	scsi_play_track track;

	track.start_track = 1;
	track.start_index = 1;
	track.end_track = 99;
	track.end_index = 1;

	status_t result = ioctl(devicefd, B_SCSI_PLAY_TRACK, &track);
	if (result != B_NO_ERROR) 
	{
		PRINT(("error %s playing track\n", strerror(errno)));
		return;
	}
}

void
CDEngine::PlayContinue()
{
	// continue after a pause
	status_t result = ioctl(devicefd, B_SCSI_RESUME_AUDIO);
	if (result != B_NO_ERROR) 
	{
		PRINT(("error %s resuming\n", strerror(errno)));
		return;
	}
}

void 
CDEngine::Stop()
{
	// stop a playing CD
	status_t result = ioctl(devicefd, B_SCSI_STOP_AUDIO);
	if (result != B_OK) 
	{
		PRINT(("error %s stopping\n", strerror(errno)));
		return;
	}
}

void 
CDEngine::Eject()
{
	// eject or load a CD
	status_t media_status = B_DEV_NO_MEDIA;
	
	// get the status first
	ioctl(devicefd, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status));
	
	// if door open, load the media, else eject the CD
	status_t result = ioctl(devicefd,
							media_status == B_DEV_DOOR_OPEN ? B_LOAD_MEDIA : B_EJECT_DEVICE);
	
	if (result != B_OK) 
	{
		PRINT(("error %s ejecting\n", strerror(errno)));
		return;
	}
}

void 
CDEngine::SkipOneForward()
{
	// skip forward by one track
	CDState state = playState.GetState();
	
	if (state == kNoCD)
		return;

	bool wasPaused = state == kPaused
		|| state == kStopped;

	SelectTrack(trackState.GetTrack() + 1);
	if (wasPaused)
		// make sure we don't start playing if we were paused before
		Pause();
}

void 
CDEngine::SkipOneBackward()
{
	// skip backward by one track
	CDState state = playState.GetState();
	
	if (state == kNoCD)
		return;

	bool wasPaused = state == kPaused
		|| state == kStopped;

	int32 track = trackState.GetTrack();
	
	if (track > 1)
		track--;
	
	SelectTrack(track);

	if (wasPaused)
		// make sure we don't start playing if we were paused before
		Pause();
}

void 
CDEngine::StartSkippingBackward()
{
	// start skipping
	CDState state = playState.GetState();
	
	if (state == kNoCD)
		return;

	scsi_scan scan;
	scan.direction = -1;
	scan.speed = 1;
	status_t result = ioctl(devicefd, B_SCSI_SCAN, &scan);
	if (result != B_NO_ERROR) {
		PRINT(("error %s skipping backward\n", strerror(errno)));
		return;
	}
}

void 
CDEngine::StartSkippingForward()
{
	// start skipping
	CDState state = playState.GetState();
	
	if (state == kNoCD)
		return;

	scsi_scan scan;
	scan.direction = 1;
	scan.speed = 1;
	status_t result = ioctl(devicefd, B_SCSI_SCAN, &scan);
	if (result != B_NO_ERROR) {
		PRINT(("error %s skipping forward\n", strerror(errno)));
		return;
	}
}

void 
CDEngine::StopSkipping()
{
	// stop skipping
	CDState state = playState.GetState();
	
	if (state == kNoCD)
		return;

	scsi_scan scan;
	scan.direction = 0;
	scan.speed = 1;
	status_t result = ioctl(devicefd, B_SCSI_SCAN, &scan);
	if (result != B_NO_ERROR) {
		PRINT(("error %s in stop skipping\n", strerror(errno)));
		return;
	}

	result = ioctl(devicefd, B_SCSI_RESUME_AUDIO);
	if (result != B_NO_ERROR) {
		PRINT(("error %s resuming\n", strerror(errno)));
		return;
	}

}

void 
CDEngine::SelectTrack(int32 trackNumber)
{
	// go to a selected track
	if (playState.GetState() == kNoCD)
		return;

	scsi_play_track track;

	track.start_track = trackNumber;
	track.start_index = 1;
	track.end_track = 99;
	track.end_index = 1;

	status_t result = ioctl(devicefd, B_SCSI_PLAY_TRACK, &track);
	if (result != B_NO_ERROR) {
		PRINT(("error %s playing track\n", strerror(errno)));
		return;
	}
}

void
CDEngine::SetVolume(uint8 value)
{
	scsi_volume vol;
	
	// change only port0's volume
	vol.flags=2;
	vol.port0_volume=value;
	
	ioctl(devicefd,B_SCSI_SET_VOLUME,&vol);
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
	if (message->what == 'slTk') {
		// handle message from menu selection
		int32 track;
		if (message->FindInt32("track", &track) == B_OK)
			SelectTrack(track);
			
	} else
		// handle observing
		if (!Notifier::HandleObservingMessages(message)
		&& !CDEngineFunctorFactory::DispatchIfFunctionObject(message))
		BHandler::MessageReceived(message);
}


static int
try_dir(const char *directory)
{ 
	BDirectory dir; 
	dir.SetTo(directory); 
	if(dir.InitCheck() != B_NO_ERROR) { 
		return false; 
	} 
	dir.Rewind(); 
	BEntry entry; 
	while(dir.GetNextEntry(&entry) >= 0) { 
		BPath path; 
		const char *name; 
		entry_ref e; 
		
		if(entry.GetPath(&path) != B_NO_ERROR) 
			continue; 
		name = path.Path(); 
		
		if(entry.GetRef(&e) != B_NO_ERROR) 
			continue; 

		if(entry.IsDirectory()) { 
			if(strcmp(e.name, "floppy") == 0) 
				continue; // ignore floppy (it is not silent) 
			int devfd = try_dir(name);
			if(devfd >= 0)
				return devfd;
		} 
		else { 
			int devfd; 
			device_geometry g; 

			if(strcmp(e.name, "raw") != 0) 
				continue; // ignore partitions 

			devfd = open(name, O_RDONLY); 
			if(devfd < 0) 
				continue; 

			if(ioctl(devfd, B_GET_GEOMETRY, &g, sizeof(g)) >= 0) {
				if(g.device_type == B_CD)
				{ 
					return devfd;
				}
			}
			close(devfd);
		} 
	}
	return B_ERROR;
}

int
CDEngine::FindCDPlayerDevice()
{
	return try_dir("/dev/disk");
}

