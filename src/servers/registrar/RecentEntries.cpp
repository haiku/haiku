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
//	File Name:		RecentEntries.cpp
//	Author:			Tyler Dauwalder (tyler@dauwalder.net)
//	Description:	Recently launched apps list
//------------------------------------------------------------------------------
/*! \file RecentEntries.cpp
	\brief RecentEntries class implementation
*/

#include "RecentEntries.h"

#include <AppFileInfo.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <Mime.h>
#include <Roster.h>
#include <String.h>
#include <storage_support.h>

#include <new>
#include <stdio.h>
#include <string>

#define DBG(x) (x)
//#define DBG(x)
#define OUT printf

//------------------------------------------------------------------------------
// recent_entry
//------------------------------------------------------------------------------

/*! \brief An recent entry and the corresponding signature of the application
	that launched/used/opened/viewed/whatevered it.
*/
struct recent_entry {
	recent_entry(const entry_ref *ref, const char *appSig);	
	entry_ref ref;
	std::string sig;
private:
	recent_entry();
};

recent_entry::recent_entry(const entry_ref *ref, const char *appSig)
	: ref(ref ? *ref : entry_ref())
	, sig(appSig)
{	
}

//------------------------------------------------------------------------------
// RecentEntries
//------------------------------------------------------------------------------

/*!	\class RecentEntries
	\brief Implements the common functionality used by the roster's recent
	folders and recent documents lists.

*/

/*!	\var std::list<std::string> RecentEntries::fEntryList
	\brief The list of entries and their corresponding app sigs, most recent first
	
	The signatures are expected to be stored all lowercase, as MIME
	signatures are case-independent.
*/

// constructor
/*!	\brief Creates a new list.
	
	The list is initially empty.
*/
RecentEntries::RecentEntries()
{
}

// destructor
/*!	\brief Frees all resources associated with the object.
*/
RecentEntries::~RecentEntries()
{
	Clear();
}

// Add
/*! \brief Places the given entry Places the app with the given signature at the front of
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
RecentEntries::Add(const entry_ref *ref, const char *appSig)
{
	std::string sig;
	status_t error = ref && appSig ? B_OK : B_BAD_VALUE;
	if (!error) {
		// Store all sigs as lowercase
		sig = BPrivate::Storage::to_lower(appSig);
		
		// Look for a previous instance of this entry
		std::list<recent_entry*>::iterator item;
		for (item = fEntryList.begin(); item != fEntryList.end(); item++) {
			if ((*item)->ref == *ref && (*item)->sig == sig) {
				fEntryList.erase(item);
				break;
			}
		}
		
		// Add this entry to the front of the list
		recent_entry *entry = new(nothrow) recent_entry(ref, appSig);
		error = entry ? B_OK : B_NO_MEMORY;
		if (!error)
			fEntryList.push_front(entry);	
	}
	return error;
}

// Get
/*! \brief Returns the first \a maxCount recent apps in the \c BMessage
	pointed to by \a list.
	
	The message is cleared first, and \c entry_refs for the the apps are
	stored in the \c "refs" field of the message (\c B_REF_TYPE).
	
	If there are fewer than \a maxCount items in the list, the entire
	list is returned.
	
	Since BRoster::GetRecentEntries() returns \c void, the message pointed
	to by \a list is simply cleared if maxCount is invalid (i.e. <= 0).
	
	\param fileTypes An array of file type filters. These file types are
	       expected to be all lowercase.
*/	
status_t
RecentEntries::Get(int32 maxCount, const char *fileTypes[], int32 fileTypesCount,
                   const char *appSig, BMessage *result)
{
	status_t error = result && (fileTypesCount == 0 || (fileTypesCount > 0 && fileTypes))
	               ? B_OK : B_BAD_VALUE;
	if (!error) {
		result->MakeEmpty();
		std::list<recent_entry*>::iterator item;
		int count = 0;
		for (item = fEntryList.begin();
		       count < maxCount && item != fEntryList.end();
		         item++)
		{
			bool match = true;
			// Filter if necessary
			if (fileTypesCount > 0 || appSig) {
				match = false;
				// Filter by app sig
				if (appSig)
					match = (*item)->sig == appSig;
				// Filter by file type
				if (!match && fileTypesCount > 0) {
					char type[B_MIME_TYPE_LENGTH];
					if (GetTypeForRef(&(*item)->ref, type) == B_OK) {
						for (int i = 0; i < fileTypesCount; i++) {						
							if (strcmp(type, fileTypes[i]) == 0) {
								match = true;
								break;
							}					
						}
					}
				}
			}
			if (match) {
				result->AddRef("refs", &(*item)->ref);
				count++;
			}
		}
	}
		
	return error;
}

// Clear
/*! \brief Clears the list of recently launched apps
*/
status_t
RecentEntries::Clear()
{
	std::list<recent_entry*>::iterator i;
	for (i = fEntryList.begin(); i != fEntryList.end(); i++)
		delete *i;
	fEntryList.clear();
	return B_OK;	
}

// Print
/*! \brief Dumps the the current list of entries to stdout.
*/
status_t
RecentEntries::Print()
{
	std::list<recent_entry*>::iterator item;
	int counter = 1;
	for (item = fEntryList.begin();
	       item != fEntryList.end();
	         item++)
	{
		printf("%d: device == '%ld', dir == '%lld', name == '%s', app == '%s'\n",
		       counter++, (*item)->ref.device, (*item)->ref.directory, (*item)->ref.name,
		       (*item)->sig.c_str());
	}
}

// RecentEntries
/*! \brief Fetches the file type of the given file.

	If the file has no type, an empty string is returned. The file
	is *not* sniffed.
*/
status_t
RecentEntries::GetTypeForRef(const entry_ref *ref, char *result)
{
	BNode node;
	status_t error = ref && result ? B_OK : B_BAD_VALUE;
	// Read the type and force to lowercase
	if (!error)
		error = node.SetTo(ref);
	if (!error) {
		ssize_t bytes = node.ReadAttr("BEOS:TYPE", B_MIME_STRING_TYPE,
		                0, result, B_MIME_TYPE_LENGTH-1);
		if (bytes < 0)
			error = bytes;
		else 
			result[bytes] = '\0';
	}
	if (!error)
		BPrivate::Storage::to_lower(result);
	return error;		
}
