/*
 * Copyright 2002-2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _APP_MISC_H
#define _APP_MISC_H

#include <Handler.h>
#include <Locker.h>
#include <OS.h>
#include <SupportDefs.h>

struct entry_ref;

namespace BPrivate {

// Global lock that can be used e.g. to initialize singletons.
extern BLocker gInitializationLock;

status_t get_app_path(team_id team, char *buffer);
status_t get_app_path(char *buffer);
status_t get_app_ref(team_id team, entry_ref *ref, bool traverse = true);
status_t get_app_ref(entry_ref *ref, bool traverse = true);

team_id current_team();
thread_id main_thread_for(team_id team);

bool is_running_on_haiku();

bool is_app_showing_modal_window(team_id team);

} // namespace BPrivate

// _get_object_token_
/*!	Return the token of a BHandler.

	\param handler The BHandler.
	\return the token.

*/
inline int32 _get_object_token_(const BHandler* object)
	{ return object->fToken; }

#endif	// _APP_MISC_H
