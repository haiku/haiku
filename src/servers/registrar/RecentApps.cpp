/*
 * Copyright 2001-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 */


//!	Recently launched apps list


#include "RecentApps.h"

#include <tracker_private.h>

#include <AppFileInfo.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <Mime.h>
#include <Roster.h>
#include <storage_support.h>

#include <string.h>

#include "Debug.h"


/*!	\class RecentApps
	\brief Manages the roster's list of recently launched applications

*/


/*!	\var std::list<std::string> RecentApps::fAppList
	\brief The list of app sigs, most recent first

	The signatures are expected to be stored all lowercase, as MIME
	signatures are case-independent.
*/


/*!	\brief Creates a new list.

	The list is initially empty.
*/
RecentApps::RecentApps()
{
}


/*!	\brief Frees all resources associated with the object.

	Currently does nothing.
*/
RecentApps::~RecentApps()
{
}


/*! \brief Places the app with the given signature at the front of
	the recent apps list.

	If the app already exists elsewhere in the list, that item is
	removed so only one instance exists in the list at any time.

	\param appSig The application's signature
	\param appFlags The application's flags. If \a appFlags contains
	                either \c B_ARGV_ONLY or \c B_BACKGROUND_APP, the
	                application is \b not added to the list (but \c B_OK
	                is still returned).
	\return
	- \c B_OK: success (even if the app was not added due to appFlags)
	- error code: failure
*/
status_t
RecentApps::Add(const char *appSig, int32 appFlags)
{
	if (appSig == NULL)
		return B_BAD_VALUE;

	// don't add background apps, as well as Tracker and Deskbar to the list
	// of recent apps
	if (!strcasecmp(appSig, kTrackerSignature)
		|| !strcasecmp(appSig, kDeskbarSignature)
		|| (appFlags & (B_ARGV_ONLY | B_BACKGROUND_APP)) != 0)
		return B_OK;

	// Remove any previous instance
	std::list<std::string>::iterator i;
	for (i = fAppList.begin(); i != fAppList.end(); i++) {
		if (!strcasecmp((*i).c_str(), appSig)) {
			fAppList.erase(i);
			break;
		}
	}

	try {
		// Add to the front
		fAppList.push_front(appSig);
	} catch (...) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


/*! \brief Adds the signature of the application referred to by \a ref at
	the front of the recent apps list.

	The entry is checked for a BEOS:APP_SIG attribute. If that fails, the
	app's resources are checked. If no signature can be found, the call
	fails.
*/
status_t
RecentApps::Add(const entry_ref *ref, int32 appFlags)
{
	if (ref == NULL)
		return B_BAD_VALUE;

	BFile file;
	BAppFileInfo info;
	char signature[B_MIME_TYPE_LENGTH];

	status_t err = file.SetTo(ref, B_READ_ONLY);
	if (!err)
		err = info.SetTo(&file);
	if (!err)
		err = info.GetSignature(signature);
	if (!err)
		err = Add(signature, appFlags);
	return err;
}


/*! \brief Returns the first \a maxCount recent apps in the \c BMessage
	pointed to by \a list.

	The message is cleared first, and \c entry_refs for the the apps are
	stored in the \c "refs" field of the message (\c B_REF_TYPE).

	If there are fewer than \a maxCount items in the list, the entire
	list is returned.

	Since BRoster::GetRecentApps() returns \c void, the message pointed
	to by \a list is simply cleared if maxCount is invalid (i.e. <= 0).
*/
status_t
RecentApps::Get(int32 maxCount, BMessage *list)
{
	if (list == NULL)
		return B_BAD_VALUE;

	// Clear
	list->MakeEmpty();

	// Fill
	std::list<std::string>::iterator item;
	status_t status = B_OK;
	int counter = 0;
	for (item = fAppList.begin();
			status == B_OK && counter < maxCount && item != fAppList.end();
			counter++, item++) {
		entry_ref ref;
		if (GetRefForApp(item->c_str(), &ref) == B_OK)
			status = list->AddRef("refs", &ref);
		else {
			D(PRINT("WARNING: RecentApps::Get(): No ref found for app '%s'\n",
				item->c_str()));
		}
	}

	return status;
}


/*! \brief Clears the list of recently launched apps
*/
status_t
RecentApps::Clear()
{
	fAppList.clear();
	return B_OK;
}


/*! \brief Dumps the the current list of apps to stdout.
*/
status_t
RecentApps::Print()
{
	std::list<std::string>::iterator item;
	int counter = 1;
	for (item = fAppList.begin(); item != fAppList.end(); item++) {
		printf("%d: '%s'\n", counter++, item->c_str());
	}
	return B_OK;
}


/*! \brief Outputs a textual representation of the current recent
	apps list to the given file stream.

*/
status_t
RecentApps::Save(FILE* file)
{
	status_t error = file ? B_OK : B_BAD_VALUE;
	if (!error) {
		fprintf(file, "# Recent applications\n");
		std::list<std::string>::iterator item;
		for (item = fAppList.begin(); item != fAppList.end(); item++) {
			fprintf(file, "RecentApp %s\n", item->c_str());
		}
		fprintf(file, "\n");
	}
	return error;
}


/*! \brief Fetches an \c entry_ref for the application with the
	given signature.

	First the MIME database is checked for a matching application type
	with a valid app hint attribute. If that fails, a query is established
	to track down such an application, if there is one available.
*/
status_t
RecentApps::GetRefForApp(const char *appSig, entry_ref *result)
{
	if (appSig == NULL || result == NULL)
		return B_BAD_VALUE;

	// We'll use BMimeType to check for the app hint, since I'm lazy
	// and Ingo's on vacation :-P :-)
	BMimeType mime(appSig);
	status_t err = mime.InitCheck();
	if (!err)
		err = mime.GetAppHint(result);
	return err;
}

