/*
 * Copyright 2002-2009, Ingo Weinhold, bonefish@users.sf.net.
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

class ServerLink;


status_t get_app_path(team_id team, char *buffer);
status_t get_app_path(char *buffer);
status_t get_app_ref(team_id team, entry_ref *ref, bool traverse = true);
status_t get_app_ref(entry_ref *ref, bool traverse = true);

team_id current_team();
thread_id main_thread_for(team_id team);

bool is_app_showing_modal_window(team_id team);

void invalidate_server_port();
port_id get_app_server_port();
status_t create_desktop_connection(ServerLink* link, const char* name,
	int32 capacity);

} // namespace BPrivate

// _get_object_token_
/*!	Return the token of a BHandler.

	\param handler The BHandler.
	\return the token.

*/
inline int32 _get_object_token_(const BHandler* object)
	{ return object->fToken; }

#endif	// _APP_MISC_H
