/*
 * Controller.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#include <stdio.h>
#include <string.h>
#include <Debug.h>
#include <Entry.h>
#include <MediaFile.h>
#include <MediaTrack.h>

#include "Controller.h"
#include "ControllerView.h"
#include "VideoView.h"
#include "SoundOutput.h"


#define AUDIO_PRIORITY 10
#define VIDEO_PRIORITY 10

void 
HandleError(const char *text, status_t err)
{
	if (err != B_OK) { 
		printf("%s. error 0x%08x (%s)\n",text, (int)err, strerror(err));
		fflush(NULL);
		exit(1);
	}
}


Controller::Controller()
 :	fVideoView(NULL)
 ,	fControllerView(NULL)
 ,	fName()
 ,	fPaused(false)
 ,	fStopped(true)
 ,	fMediaFile(0)
 ,	fAudioTrack(0)
 ,	fVideoTrack(0)
 ,	fAudioTrackList(new BList)
 ,	fVideoTrackList(new BList)
 ,	fPosition(0)
 ,	fAudioPlaySem(create_sem(1, "audio playing"))
 ,	fVideoPlaySem(create_sem(1, "video playing"))
 ,	fAudioPlayThread(-1)
 ,	fVideoPlayThread(-1)
 ,	fStopAudioPlayback(false)
 ,	fStopVideoPlayback(false)
 ,	fSoundOutput(NULL)
 ,	fSeekAudio(false)
 ,	fSeekVideo(false)
 ,	fSeekPosition(0)
{
}


Controller::~Controller()
{
	StopAudioPlayback();
	StopVideoPlayback();
	
	if (fMediaFile)
		fMediaFile->ReleaseAllTracks();
	delete fMediaFile;
	delete fAudioTrackList;
	delete fVideoTrackList;
	delete_sem(fVideoPlaySem);
	delete_sem(fAudioPlaySem);
}


void
Controller::SetVideoView(VideoView *view)
{
	fVideoView = view;
}


void
Controller::SetControllerView(ControllerView *view)
{
	fControllerView = view;
}


status_t
Controller::SetTo(const entry_ref &ref)
{
	acquire_sem(fAudioPlaySem);
	acquire_sem(fVideoPlaySem);
	
	StopAudioPlayback();
	StopVideoPlayback();

	fAudioTrackList->MakeEmpty();
	fVideoTrackList->MakeEmpty();
	if (fMediaFile)
		fMediaFile->ReleaseAllTracks();
	delete fMediaFile;
	fAudioTrack = 0;
	fVideoTrack = 0;
	fMediaFile = 0;
	fPosition = 0;
	fPaused = false;
	fStopped = true;
	fName = ref.name;
	
	status_t err;
	
	BMediaFile *mf = new BMediaFile(&ref);
	err = mf->InitCheck();
	if (err != B_OK) {
		printf("Controller::SetTo: initcheck failed\n");
		delete mf;
		release_sem(fAudioPlaySem);
		release_sem(fVideoPlaySem);
		return err;
	}
	
	int trackcount = mf->CountTracks();
	if (trackcount <= 0) {
		printf("Controller::SetTo: trackcount %d\n", trackcount);
		delete mf;
		release_sem(fAudioPlaySem);
		release_sem(fVideoPlaySem);
		return B_MEDIA_NO_HANDLER;
	}
	
	for (int i = 0; i < trackcount; i++) {
		BMediaTrack *t = mf->TrackAt(i);
		media_format f;
		err = t->EncodedFormat(&f);
		if (err != B_OK) {
			printf("Controller::SetTo: EncodedFormat failed for track index %d, error 0x%08lx (%s)\n",
				i, err, strerror(err));
			mf->ReleaseTrack(t);
			continue;
		}
		if (f.IsAudio()) {
			fAudioTrackList->AddItem(t);
		} else if (f.IsVideo()) {
			fVideoTrackList->AddItem(t);
		} else {
			printf("Controller::SetTo: track index %d has unknown type\n", i);
			mf->ReleaseTrack(t);
		}
	}

	if (AudioTrackCount() == 0 && VideoTrackCount() == 0) {
		printf("Controller::SetTo: no audio or video tracks found\n");
		delete mf;
		release_sem(fAudioPlaySem);
		release_sem(fVideoPlaySem);
		return B_MEDIA_NO_HANDLER;
	}

	printf("Controller::SetTo: %d audio track, %d video track\n", AudioTrackCount(), VideoTrackCount());

	fMediaFile = mf;

	SelectAudioTrack(0);
	SelectVideoTrack(0);

	release_sem(fAudioPlaySem);
	release_sem(fVideoPlaySem);
	
	return B_OK;
}


int
Controller::VideoTrackCount()
{
	return fVideoTrackList->CountItems();
}


int
Controller::AudioTrackCount()
{
	return fAudioTrackList->CountItems();
}


status_t
Controller::SelectAudioTrack(int n)
{
	StopAudioPlayback();
	
	BMediaTrack *t = (BMediaTrack *)fAudioTrackList->ItemAt(n);
	if (!t)
		return B_ERROR;
	fAudioTrack = t;
	
	StartAudioPlayback();
	return B_OK;
}


status_t
Controller::SelectVideoTrack(int n)
{
	StopVideoPlayback();

	BMediaTrack *t = (BMediaTrack *)fVideoTrackList->ItemAt(n);
	if (!t)
		return B_ERROR;
	fVideoTrack = t;

	StartVideoPlayback();
	return B_OK;
}


bigtime_t
Controller::Duration()
{
	bigtime_t a = fAudioTrack ? fAudioTrack->Duration() : 0;
	bigtime_t v = fVideoTrack ? fVideoTrack->Duration() : 0;
//	printf("Controller::Duration: audio %.6f, video %.6f\n", a / 1000000.0, v / 1000000.0);
	return max_c(a, v);
}


bigtime_t
Controller::Position()
{
	return fPosition;
}


status_t
Controller::Seek(bigtime_t pos)
{
	return B_OK;
}


void
Controller::VolumeUp()
{
}


void
Controller::VolumeDown()
{
}


void
Controller::SetVolume(float value)
{
	printf("Controller::SetVolume %.4f\n", value);
	if (fSoundOutput) // hack...
		fSoundOutput->SetVolume(value);
}


void
Controller::SetPosition(float value)
{
	printf("Controller::SetPosition %.4f\n", value);
	fSeekPosition = bigtime_t(value * Duration());
	fSeekAudio = true;
	fSeekVideo = true;
}


void
Controller::UpdateVolume(float value)
{
	fControllerView->SetVolume(value);
}


void
Controller::UpdatePosition(float value)
{
	fControllerView->SetPosition(value);
}


bool
Controller::IsOverlayActive()
{
	return true;
}


void
Controller::LockBitmap()
{
}


void
Controller::UnlockBitmap()
{
}


BBitmap *
Controller::Bitmap()
{
	return NULL;
}


void
Controller::Stop()
{
	if (fStopped)
		return;

	acquire_sem(fAudioPlaySem);
	acquire_sem(fVideoPlaySem);
	
	fPaused = false;
	fStopped = true;
	fPosition = 0;
}


void
Controller::Play()
{
	if (!fPaused && !fStopped)
		return;

	fStopped =
	fPaused = false;

	release_sem(fAudioPlaySem);
	release_sem(fVideoPlaySem);
}


void
Controller::Pause()
{
	if (fStopped || fPaused)
		return;
		
	acquire_sem(fAudioPlaySem);
	acquire_sem(fVideoPlaySem);

	fPaused = true;
}


bool
Controller::IsPaused()
{
	return fPaused;
}


int32
Controller::audio_play_thread(void *self)
{
	static_cast<Controller *>(self)->AudioPlayThread();
	return 0;
}


int32
Controller::video_play_thread(void *self)
{
	static_cast<Controller *>(self)->VideoPlayThread();
	return 0;
}


void
Controller::AudioPlayThread()
{
	// this is a test...
	media_format fmt;
	fmt.type = B_MEDIA_RAW_AUDIO;
	fmt.u.raw_audio = media_multi_audio_format::wildcard;
#ifdef __HAIKU__
	fmt.u.raw_audio.format = media_multi_audio_format::B_AUDIO_FLOAT;
#endif
	fAudioTrack->DecodedFormat(&fmt);
	
	SoundOutput out(fName.String(), fmt.u.raw_audio);
	
	out.SetVolume(1.0);
	UpdateVolume(1.0);
	
	
	int frame_size = (fmt.u.raw_audio.format & 0xf) * fmt.u.raw_audio.channel_count;
	uint8 buffer[fmt.u.raw_audio.buffer_size];
	int64 frames;
	media_header mh;
	
	fSoundOutput = &out;
	while (!fStopAudioPlayback) {
		if (B_OK == fAudioTrack->ReadFrames(buffer, &frames, &mh)) {
			out.Play(buffer, frames * frame_size);
			UpdatePosition(mh.start_time / (float)Duration());
		} else {
			snooze(50000);
		}
		if (fSeekAudio) {
			bigtime_t pos = fSeekPosition;
			fAudioTrack->SeekToTime(&pos);
			fSeekAudio = false;
		}
	}
	fSoundOutput = NULL;
}


void
Controller::VideoPlayThread()
{
}


void
Controller::StartAudioPlayback()
{
	if (fAudioPlayThread > 0)
		return;
	fStopAudioPlayback = false;
	fAudioPlayThread = spawn_thread(audio_play_thread, "audio playback", AUDIO_PRIORITY, this);
	resume_thread(fAudioPlayThread);
}


void
Controller::StartVideoPlayback()
{
	if (fVideoPlayThread > 0)
		return;
	fStopVideoPlayback = false;
	fVideoPlayThread = spawn_thread(video_play_thread, "video playback", VIDEO_PRIORITY, this);
	resume_thread(fVideoPlayThread);
}


void
Controller::StopAudioPlayback()
{
	if (fAudioPlayThread < 0)
		return;
	fStopAudioPlayback = true;
	status_t dummy;
	wait_for_thread(fAudioPlayThread, &dummy);
	fAudioPlayThread = -1;
}


void
Controller::StopVideoPlayback()
{
	if (fVideoPlayThread < 0)
		return;
	fStopVideoPlayback = true;
	status_t dummy;
	wait_for_thread(fVideoPlayThread, &dummy);
	fVideoPlayThread = -1;
}
