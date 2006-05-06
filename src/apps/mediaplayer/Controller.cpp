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
#include <MediaFile.h>
#include <MediaTrack.h>

#include "Controller.h"
#include "VideoView.h"

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
{
}


Controller::~Controller()
{
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


status_t
Controller::SetTo(const entry_ref &ref)
{
	acquire_sem(fAudioPlaySem);
	acquire_sem(fVideoPlaySem);

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
	BMediaTrack *t = (BMediaTrack *)fAudioTrackList->ItemAt(n);
	if (!t)
		return B_ERROR;
	fAudioTrack = t;
	
	return B_OK;
}


status_t
Controller::SelectVideoTrack(int n)
{
	BMediaTrack *t = (BMediaTrack *)fVideoTrackList->ItemAt(n);
	if (!t)
		return B_ERROR;
	fVideoTrack = t;

	return B_OK;
}


bigtime_t
Controller::Duration()
{
	bigtime_t a = fAudioTrack ? fAudioTrack->Duration() : 0;
	bigtime_t v = fVideoTrack ? fVideoTrack->Duration() : 0;
	printf("Controller::Duration: audio %.6f, video %.6f", a / 1000000.0, v / 1000000.0);
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
