/*
 * Copyright 2001-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@users.sf.net
 *		Jacob Secunda
 */

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

} // namespace BPrivate

