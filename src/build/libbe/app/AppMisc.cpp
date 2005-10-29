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

// get_app_ref
/*!	\brief Returns an entry_ref referring to the application's executable.
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
//	return get_app_ref(B_CURRENT_TEAM, ref, traverse);
// TODO: Needed by BResourceStrings. We could implement the function by getting
// an entry_ref for the argv[0] at initialization time. Now it could be too
// late (in case the app has changed the cwd).
return B_ERROR;
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

// is_app_showing_modal_window
/*!	\brief Returns whether the application identified by the supplied
		   \c team_id is currently showing a modal window.
	\param team the ID of the application in question.
	\return \c true, if the application is showing a modal window, \c false
			otherwise.
*/
bool
is_app_showing_modal_window(team_id team)
{
	// TODO: Implement!
	return true;
}

} // namespace BPrivate

