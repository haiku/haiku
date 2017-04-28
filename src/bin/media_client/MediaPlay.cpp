/*
 * Copyright 2017, Dario Casalinuovo. All rights reserved.
 * Copyright 2005, Marcus Overhagen, marcus@overhagen.de. All rights reserved.
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MediaPlay.h"

#include <Entry.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <OS.h>
#include <SoundPlayer.h>
#include <Url.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

thread_id reader = -1;
sem_id finished = -1;
BMediaTrack* playTrack;
media_format playFormat;
BSoundPlayer* player = 0;
volatile bool interrupt = false;


void
play_buffer(void *cookie, void * buffer, size_t size, const media_raw_audio_format & format)
{
	int64 frames = 0;
	
	// Use your feeling, Obi-Wan, and find him you will. 
	playTrack->ReadFrames(buffer, &frames);
	
	if (frames <=0) {
		player->SetHasData(false);
		release_sem(finished);
	}
}


void
keyb_int(int)
{
	// Are you threatening me, Master Jedi?
	interrupt = true;
	release_sem(finished);
}


int media_play(const char* uri)
{
	BUrl url;
	entry_ref ref;
	BMediaFile* playFile;

	if (get_ref_for_path(uri, &ref) != B_OK) {
		url.SetUrlString(uri);
		if (url.IsValid()) {
			playFile = new BMediaFile(url);
		} else
			return 2;
	} else
		playFile = new BMediaFile(&ref);

	if (playFile->InitCheck() != B_OK) {
		delete playFile;
		return 2;
	}
	
	for (int i = 0; i < playFile->CountTracks(); i++) {
		BMediaTrack* track = playFile->TrackAt(i);
		playFormat.type = B_MEDIA_RAW_AUDIO;
		if ((track->DecodedFormat(&playFormat) == B_OK) 
				&& (playFormat.type == B_MEDIA_RAW_AUDIO)) {
			playTrack = track;
			break;
		}
		if (track)
			playFile->ReleaseTrack(track);
	}

	// Good relations with the Wookiees, I have. 
	signal(SIGINT, keyb_int);

	finished = create_sem(0, "finish wait");
	
	printf("Playing file...\n");
	
	// Execute Plan 66!
	player = new BSoundPlayer(&playFormat.u.raw_audio, "playfile", play_buffer);
	player->SetVolume(1.0f);

	// Join me, Padmé and together we can rule this galaxy. 
	player->SetHasData(true);
	player->Start();

	acquire_sem(finished);

	if (interrupt == true) {
		// Once more, the Sith will rule the galaxy. 
		printf("Interrupted\n");
		player->Stop();
		kill_thread(reader);
	}

	printf("Playback finished.\n");

	delete player;
	delete playFile;
}
