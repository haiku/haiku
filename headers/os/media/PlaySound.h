/******************************************************************************

	File:			PlaySound.h

	Description:	Interface for a simple beep sound.

	Copyright 1995-97, Be Incorporated

******************************************************************************/
#ifndef _PLAY_SOUND_H
#define _PLAY_SOUND_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <OS.h>
#include <Entry.h>

typedef sem_id sound_handle;

_IMPEXP_MEDIA
sound_handle play_sound(const entry_ref *soundRef,
						bool mix,
						bool queue,
						bool background
						);

_IMPEXP_MEDIA
status_t stop_sound(sound_handle handle);

_IMPEXP_MEDIA
status_t wait_for_sound(sound_handle handle);

#endif			/* #ifndef _PLAY_SOUND_H*/
