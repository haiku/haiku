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
//	File Name:		RecentApps.cpp
//	Author:			Tyler Dauwalder (tyler@dauwalder.net)
//	Description:	Recently launched apps list
//------------------------------------------------------------------------------
/*! \file RecentApps.cpp
	\brief RecentApps class implementation
*/

#include "RecentApps.h"

#include <AppFileInfo.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <Mime.h>
#include <Roster.h>
#include <storage_support.h>

#define DBG(x) (x)
//#define DBG(x)
#define OUT printf

/*!	\class RecentApps
	\brief Manages the roster's list of recently launched applications

*/

/*!	\var std::list<std::string> RecentApps::fAppList
	\brief The list of app sigs, most recent first
	
	The signatures are expected to be stored all lowercase, as MIME
	signatures are case-independent.
*/

// constructor
/*!	\brief Creates a new list.
	
	The list is initially empty.
*/
RecentApps::RecentApps()
{
}

// destructor
/*!	\brief Frees all resources associated with the object.

	Currently does nothing.
*/
RecentApps::~RecentApps()
{
}

// Add
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
	status_t err = appSig ? B_OK : B_BAD_VALUE;
	if (!err && !(appFlags & B_ARGV_ONLY) && !(appFlags & B_BACKGROUND_APP)) {
		// Store all sigs as lowercase
		std::string sig = BPrivate::Storage::to_lower(appSig);
		
		// Remove any previous instance
		std::list<std::string>::iterator i;
		for (i = fAppList.begin(); i != fAppList.end(); i++) {
			if (*i == sig) {
				fAppList.erase(i);
				break;
			}
		}
		// Add to the front
		fAppList.push_front(sig);
	}
		
	return err;
}

// Add
/*! \brief Adds the signature of the application referred to by \a ref at
	the front of the recent apps list.
	
	The entry is checked for a BEOS:APP_SIG attribute. If that fails, the
	app's resources are checked. If no signature can be found, the call
	fails.
*/
status_t
RecentApps::Add(const entry_ref *ref, int32 appFlags)
{
	BFile file;
	BAppFileInfo info;
	char signature[B_MIME_TYPE_LENGTH];
	
	status_t err = ref ? B_OK : B_BAD_VALUE;
	if (!err)
		err = file.SetTo(ref, B_READ_ONLY);
	if (!err)
		err = info.SetTo(&file);
	if (!err)
		err = info.GetSignature(signature);
	if (!err)
		err = Add(signature, appFlags);
	return err;
}

// Get
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
	status_t err = list ? B_OK : B_BAD_VALUE;
	if (!err) {
		// Clear
		list->MakeEmpty();
		
		// Fill
		std::list<std::string>::iterator item;
		int counter = 0;
		for (item = fAppList.begin();
		       counter < maxCount && item != fAppList.end();
		         counter++, item++)
		{
			entry_ref ref;
			status_t error = GetRefForApp(item->c_str(), &ref);
			if (!error)
				list->AddRef("refs", &ref);
			else
				DBG(OUT("WARNING: RecentApps::Get(): No ref found for app '%s'\n", item->c_str()));
		}
	}
		
	return err;
}

// Clear
/*! \brief Clears the list of recently launched apps
*/
status_t
RecentApps::Clear()
{
	fAppList.clear();
	return B_OK;	
}

// Print
/*! \brief Dumps the the current list of apps to stdout.
*/
status_t
RecentApps::Print()
{
	std::list<std::string>::iterator item;
	int counter = 1;
	for (item = fAppList.begin();
	       item != fAppList.end();
	         item++)
	{
		printf("%d: '%s'\n", counter++, item->c_str());
	}
}

// Save
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
		for (item = fAppList.begin();
		       item != fAppList.end();
		         item++)
		{
			fprintf(file, "RecentApp %s\n", item->c_str());
		}
		fprintf(file, "\n");
	}
	return error;
}

// GetRefForApp
/*! \brief Fetches an \c entry_ref for the application with the
	given signature.
	
	First the MIME database is checked for a matching application type
	with a valid app hint attribute. If that fails, a query is established
	to track down such an application, if there is one available.
*/
status_t
RecentApps::GetRefForApp(const char *appSig, entry_ref *result)
{
	status_t err = appSig && result ? B_OK : B_BAD_VALUE;
	
	// We'll use BMimeType to check for the app hint, since I'm lazy
	// and Ingo's on vacation :-P :-)
	BMimeType mime(appSig);
	err = mime.InitCheck();
	if (!err) 
		err = mime.GetAppHint(result);
	return err;	
}

