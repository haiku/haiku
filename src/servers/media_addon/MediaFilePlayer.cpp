/*
 * Copyright (c) 2004, Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 * Copyright (c) 2007, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MediaFilePlayer.h"

#include <MediaFiles.h>
#include <ObjectList.h>

#include <stdlib.h>


BObjectList<MediaFilePlayer> list;


MediaFilePlayer*
FindMediaFilePlayer(MediaFilePlayer* player, void* media_name)
{
	if (strcmp(player->Name(), (const char*)media_name) == 0)
		return player;
	return NULL;
}


void
PlayMediaFile(const char* media_type, const char* media_name)
{
	entry_ref ref;
	if (BMediaFiles().GetRefFor(media_type, media_name, &ref) != B_OK
		|| !BEntry(&ref).Exists())
		return;

	MediaFilePlayer* player = list.EachElement(FindMediaFilePlayer,
		(void*)media_name);

	if (player != NULL) {
		if (*(player->Ref()) == ref) {
			player->Restart();
			return;
		}

		list.RemoveItem(player);
		delete player;
		player = NULL;
	}

	if (player == NULL) {
		player = new MediaFilePlayer(media_type, media_name, &ref);
		if (player->InitCheck() == B_OK)
			list.AddItem(player);
		else
			delete player;
	}
}



MediaFilePlayer::MediaFilePlayer(const char* media_type,
	const char* media_name, entry_ref* ref)
	:
	fName(media_name),
	fInitCheck(B_ERROR),
	fRef(*ref),
	fSoundPlayer(NULL),
	fPlayTrack(NULL)
{
	fPlayFile = new BMediaFile(&fRef);
	fInitCheck = fPlayFile->InitCheck();
	if (fInitCheck != B_OK)
		return;

	memset(&fPlayFormat, 0, sizeof(fPlayFormat));

	for (int i=0; i < fPlayFile->CountTracks(); i++) {
		BMediaTrack* track = fPlayFile->TrackAt(i);
		if (track == NULL)
			continue;
		fPlayFormat.type = B_MEDIA_RAW_AUDIO;
		fPlayFormat.u.raw_audio.buffer_size = 256;
		if ((track->DecodedFormat(&fPlayFormat) == B_OK)
			&& (fPlayFormat.type == B_MEDIA_RAW_AUDIO)) {
			fPlayTrack = track;
			break;
		}
		fPlayFile->ReleaseTrack(track);
	}

	if (fPlayTrack == NULL) {
		fInitCheck = B_BAD_VALUE;
		return;
	}

	fSoundPlayer = new BSoundPlayer(&fPlayFormat.u.raw_audio,
		media_name, PlayFunction, NULL, this);

	fInitCheck = fSoundPlayer->InitCheck();
	if (fInitCheck != B_OK)
		return;

	fSoundPlayer->SetHasData(true);
	fSoundPlayer->Start();
}


MediaFilePlayer::~MediaFilePlayer()
{
	delete fSoundPlayer;
	delete fPlayFile;
}


status_t
MediaFilePlayer::InitCheck()
{
	return fInitCheck;
}


const char*
MediaFilePlayer::Name()
{
	return fName;
}


const entry_ref*
MediaFilePlayer::Ref()
{
	return &fRef;
}


bool
MediaFilePlayer::IsPlaying()
{
	return (fSoundPlayer != NULL && fSoundPlayer->HasData());
}

void
MediaFilePlayer::Restart()
{
	fSoundPlayer->Stop();
	int64 frame = 0;
	fPlayTrack->SeekToFrame(&frame);
	fSoundPlayer->SetHasData(true);
	fSoundPlayer->Start();
}


void
MediaFilePlayer::Stop()
{
	fSoundPlayer->Stop();
}


void
MediaFilePlayer::PlayFunction(void* cookie, void* buffer,
	size_t size, const media_raw_audio_format& format)
{
	MediaFilePlayer* player = (MediaFilePlayer*)cookie;
	int64 frames = 0;
	player->fPlayTrack->ReadFrames(buffer, &frames);

	if (frames <= 0)
		player->fSoundPlayer->SetHasData(false);
}
