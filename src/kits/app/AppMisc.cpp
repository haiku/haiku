//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		AppMisc.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Miscellaneous private functionality.
//------------------------------------------------------------------------------

#include <string.h>
#include <sys/utsname.h>

#include <AppMisc.h>
#include <Entry.h>
#include <image.h>
#include <OS.h>

namespace BPrivate {

// get_app_path
/*!	\brief Returns the path to the application's executable.
	\param buffer A pointer to a pre-allocated character array of at least
		   size B_PATH_NAME_LENGTH to be filled in by this function.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a buffer.
	- another error code
*/
status_t
get_app_path(char *buffer)
{
	// The only way to get the path to the application's executable seems to
	// be to get an image_info of its image, which also contains a path.
	// Several images may belong to the team (libraries, add-ons), but only
	// the one in question should be typed B_APP_IMAGE.
	status_t error = (buffer ? B_OK : B_BAD_VALUE);
	image_info info;
	int32 cookie = 0;
	bool found = false;
	if (error == B_OK) {
		while (!found && get_next_image_info(0, &cookie, &info) == B_OK) {
			if (info.type == B_APP_IMAGE) {
				strncpy(buffer, info.name, B_PATH_NAME_LENGTH - 1);
				buffer[B_PATH_NAME_LENGTH - 1] = 0;
				found = true;
			}
		}
	}
	if (error == B_OK && !found)
		error = B_ENTRY_NOT_FOUND;
	return error;
}

// get_app_ref
/*!	\brief Returns an entry_ref referring to the application's exectable.
	\param ref A pointer to a pre-allocated entry_ref to be initialized
		   to an entry_ref referring to the application's executable.
	\param traverse If \c true, the function traverses symbolic links.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- another error code
*/
status_t
get_app_ref(entry_ref *ref, bool traverse)
{
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	char appFilePath[B_PATH_NAME_LENGTH];
	if (error == B_OK)
		error = get_app_path(appFilePath);
	if (error == B_OK) {
		BEntry entry(appFilePath, traverse);
		error = entry.GetRef(ref);
	}
	return error;
}

// current_team
/*!	\brief Returns the ID of the current team.
	\return The ID of the current team.
*/
team_id
current_team()
{
	team_id team = -1;
	thread_info info;
	if (get_thread_info(find_thread(NULL), &info) == B_OK)
		team = info.team;
	return team;
}

// main_thread_for
/*!	Returns the ID of the supplied team's main thread.
	\param team The team.
	\return
	- The thread ID of the supplied team's main thread
	- \c B_BAD_TEAM_ID: The supplied team ID does not identify a running team.
	- another error code
*/
thread_id
main_thread_for(team_id team)
{
	// For I can't find any trace of how to explicitly get the main thread,
	// I assume the main thread is the one with the least thread ID.
	thread_id thread = B_BAD_TEAM_ID;
	int32 cookie = 0;
	thread_info info;
	while (get_next_thread_info(team, &cookie, &info) == B_OK) {
		if (thread < 0 || info.thread < thread)
			thread = info.thread;
	}
	return thread;
}

// is_running_on_haiku
/*!	Returns whether we're running under Haiku natively.

	This is a runtime check for components compiled only once for both
	BeOS and Haiku and nevertheless need to behave differently on the two
	systems, like the registrar, which uses another MIME database directory
	under BeOS.

	\return \c true, if we're running under Haiku, \c false otherwise.
*/
bool
is_running_on_haiku()
{
	struct utsname info;
	return (uname(&info) == 0 && strcmp(info.sysname, "Haiku") == 0);
}

} // namespace BPrivate

