/*
 * Copyright (c) 2004-2007, Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Entry.h>
#include <MediaFiles.h>
#include "MediaFilePlayer.h"
#include "debug.h"

void 
PlayMediaFile(const char *media_type, const char *media_name)
{
	TRACE("PlayMediaFile: type '%s', name '%s'\n", media_type, media_name);

	entry_ref ref;
	BMediaFiles().GetRefFor(media_type, media_name, &ref);
}

