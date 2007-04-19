/*
 * Copyright (c) 2004, Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_FILE_PLAYER_H
#define _MEDIA_FILE_PLAYER_H

#include <Entry.h>
#include <SoundPlayer.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <MediaDefs.h>


void PlayMediaFile(const char *media_type, const char *media_name);


class MediaFilePlayer
{
public:
	MediaFilePlayer(const char *media_type, const char *media_name,
		entry_ref *ref);
	~MediaFilePlayer();

	bool IsPlaying();
	void Restart();
	void Stop();
	status_t InitCheck();
	const char *Name() { return fName; };
	const entry_ref *Ref() { return &fRef; };
	
	static void PlayFunction(void *cookie, void * buffer, 
		size_t size, const media_raw_audio_format & format);

private:
	char 		*fName;
	status_t	fInitCheck;
	entry_ref	fRef;
	BSoundPlayer *fSoundPlayer;
	BMediaFile *fPlayFile;
	BMediaTrack	*fPlayTrack;
	media_format fPlayFormat;
};

#endif // _MEDIA_FILE_PLAYER_H
