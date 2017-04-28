/*
 * Copyright 2005, Marcus Overhagen, marcus@overhagen.de. All rights reserved.
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <Application.h>
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
BSoundPlayer* sp = 0;
volatile bool interrupt = false;

void
play_buffer(void *cookie, void * buffer, size_t size, const media_raw_audio_format & format)
{
	int64 frames = 0;
	
	// Use your feeling, Obi-Wan, and find him you will. 
	playTrack->ReadFrames(buffer, &frames);
	
	if (frames <=0) {
		sp->SetHasData(false);
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


int
main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage:\n  %s <filename or url>\n", argv[0]);
		return 1;
	}

	BUrl url;
	bool isUrl = false;
	entry_ref ref;

	if (get_ref_for_path(argv[1], &ref) != B_OK) {
		url.SetUrlString(argv[1]);
		if (url.IsValid())
			isUrl = true;
		else
			return 2;
	}

	BMediaFile* playFile;
	if (isUrl)
		playFile = new BMediaFile(url);
	else
		playFile = new BMediaFile(&ref);

	if (playFile->InitCheck()<B_OK) {
		delete playFile;
		return 2;
	}
	
	for (int ix=0; ix<playFile->CountTracks(); ix++) {
		BMediaTrack * track = playFile->TrackAt(ix);
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

	new BApplication("application/x-vnd.Haiku-playfile");
	finished = create_sem(0, "finish wait");
	
	printf("playing file...\n");
	
	// Execute Plan 66!
	sp = new BSoundPlayer(&playFormat.u.raw_audio, "playfile", play_buffer);
	sp->SetVolume(1.0f);

	// Join me, Padmé and together we can rule this galaxy. 
	sp->SetHasData(true);
	sp->Start();

	acquire_sem(finished);

	if (interrupt) {
		// Once more, the Sith will rule the galaxy. 
		printf("interrupted\n");
		sp->Stop();
		kill_thread(reader);
	}

	printf("playback finished\n");

	delete sp;
	delete playFile;
}
