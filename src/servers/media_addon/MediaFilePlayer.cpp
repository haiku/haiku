/*
 * Copyright (c) 2004, Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 * Copyright (c) 2007, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Entry.h>
#include <FileGameSound.h>
#include <MediaFiles.h>
#include "MediaFilePlayer.h"
#include "debug.h"

void 
PlayMediaFile(const char *media_type, const char *media_name)
{
	TRACE("PlayMediaFile: type '%s', name '%s'\n", media_type, media_name);

	entry_ref ref;
	if (BMediaFiles().GetRefFor(media_type, media_name, &ref)!=B_OK
		|| !BEntry(&ref).Exists())
		return;

	BFileGameSound *sound = new BFileGameSound(&ref, false);
	if (sound->InitCheck() == B_OK) {
		sound->Preload();
		sound->StartPlaying();
	}
	// TODO reclaim sound
}

