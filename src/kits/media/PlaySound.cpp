/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: PlaySound.cpp
 *  DESCR: 
 ***********************************************************************/

#include <BeBuild.h>
#include <OS.h>
#include <Entry.h>

#include <PlaySound.h>
#include "MediaDebug.h"

sound_handle play_sound(const entry_ref *soundRef,
						bool mix,
						bool queue,
						bool background
						)
{
	UNIMPLEMENTED();
	return (sound_handle)1;
}

status_t stop_sound(sound_handle handle)
{
	UNIMPLEMENTED();
	return B_OK;
}

status_t wait_for_sound(sound_handle handle)
{
	UNIMPLEMENTED();
	return B_OK;
}
