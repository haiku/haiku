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


#include "RecentEntries.h"

#include <new>
#include <map>

#include <AppFileInfo.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <Mime.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include <storage_support.h>

#include "Debug.h"


using namespace std;


/*!	\struct recent_entry

	\brief A recent entry, the corresponding signature of the application
	that launched/used/opened/viewed/whatevered it, and an index used for
	keeping track of orderings when loading/storing the recent entries list
	from/to disk.

*/

/*! \brief Creates a new recent_entry object.
*/
recent_entry::recent_entry(const entry_ref *ref, const char *appSig,
		uint32 index)
	:
	ref(ref ? *ref : entry_ref()),
	sig(appSig),
	index(index)
{
}


//	#pragma mark -


/*!	\class RecentEntries
	\brief Implements the common functionality used by the roster's recent
	folders and recent documents lists.

*/

/*!	\var std::list<std::string> RecentEntries::fEntryList
	\brief The list of entries and their corresponding app sigs, most recent first

	The signatures are expected to be stored all lowercase, as MIME
	signatures are case-independent.
*/


/*!	\brief Creates a new list.

	The list is initially empty.
*/
RecentEntries::RecentEntries()
{
}


/*!	\brief Frees all resources associated with the object.
*/
RecentEntries::~RecentEntries()
{
	Clear();
}


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
	if (ref == NULL || appSig == NULL)
		return B_BAD_VALUE;

	// Look for a previous instance of this entry
	std::list<recent_entry*>::iterator item;
	for (item = fEntryList.begin(); item != fEntryList.end(); item++) {
		if ((*item)->ref == *ref && !strcasecmp((*item)->sig.c_str(), appSig)) {
			fEntryList.erase(item);
			break;
		}
	}

	// Add this entry to the front of the list
	recent_entry *entry = new (nothrow) recent_entry(ref, appSig, 0);
	if (entry == NULL)
		return B_NO_MEMORY;

	try {
		fEntryList.push_front(entry);
	} catch (...) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


/*! \brief Returns the first \a maxCount recent apps in the \c BMessage
	pointed to by \a list.

	The message is cleared first, and \c entry_refs for the the apps are
	stored in the \c "refs" field of the message (\c B_REF_TYPE).

	If there are fewer than \a maxCount items in the list, the entire
	list is returned.

	Duplicate entries are never returned, i.e. if two instances of the
	same entry were added under different app sigs, and both instances
	match the given filter criterion, only the most recent instance is
	returned; the latter instance is ignored and not counted towards
	the \a maxCount number of entries to return.

	Since BRoster::GetRecentEntries() returns \c void, the message pointed
	to by \a list is simply cleared if maxCount is invalid (i.e. <= 0).

	\param fileTypes An array of file type filters. These file types are
	       expected to be all lowercase.
*/
status_t
RecentEntries::Get(int32 maxCount, const char *fileTypes[],
	int32 fileTypesCount, const char *appSig, BMessage *result)
{
	if (result == NULL
		|| fileTypesCount < 0
		|| (fileTypesCount > 0 && fileTypes == NULL))
		return B_BAD_VALUE;

	result->MakeEmpty();

	std::list<recent_entry*> duplicateList;
	std::list<recent_entry*>::iterator item;
	status_t error = B_OK;
	int count = 0;

	for (item = fEntryList.begin();
			error == B_OK && count < maxCount && item != fEntryList.end();
			item++) {
		// Filter by app sig
		if (appSig != NULL && strcasecmp((*item)->sig.c_str(), appSig))
			continue;

		// Filter by file type
		if (fileTypesCount > 0) {
			char type[B_MIME_TYPE_LENGTH];
			if (GetTypeForRef(&(*item)->ref, type) == B_OK) {
				bool match = false;
				for (int i = 0; i < fileTypesCount; i++) {
					if (!strcasecmp(type, fileTypes[i])) {
						match = true;
						break;
					}
				}
				if (!match)
					continue;
			}
		}

		// Check for duplicates
		bool duplicate = false;
		for (std::list<recent_entry*>::iterator dupItem = duplicateList.begin();
				dupItem != duplicateList.end(); dupItem++) {
			if ((*dupItem)->ref == (*item)->ref) {
				duplicate = true;
				break;
			}
		}
		if (duplicate)
			continue;

		// Add the ref to the list used to check
		// for duplicates, and then to the result
		try {
			duplicateList.push_back(*item);
		} catch (...) {
			error = B_NO_MEMORY;
		}
		if (error == B_OK)
			error = result->AddRef("refs", &(*item)->ref);
		if (error == B_OK)
			count++;
	}

	return error;
}


/*! \brief Clears the list of recently launched apps
*/
status_t
RecentEntries::Clear()
{
	std::list<recent_entry*>::iterator i;
	for (i = fEntryList.begin(); i != fEntryList.end(); i++) {
		delete *i;
	}
	fEntryList.clear();
	return B_OK;
}


/*! \brief Dumps the the current list of entries to stdout.
*/
status_t
RecentEntries::Print()
{
	std::list<recent_entry*>::iterator item;
	int counter = 1;
	for (item = fEntryList.begin(); item != fEntryList.end(); item++) {
		printf("%d: device == '%" B_PRIdDEV "', dir == '%" B_PRIdINO "', "
			"name == '%s', app == '%s', index == %" B_PRId32 "\n", counter++,
			(*item)->ref.device, (*item)->ref.directory, (*item)->ref.name,
			(*item)->sig.c_str(), (*item)->index);
	}
	return B_OK;
}


status_t
RecentEntries::Save(FILE* file, const char *description, const char *tag)
{
	if (file == NULL || description == NULL || tag == NULL)
		return B_BAD_VALUE;

	fprintf(file, "# %s\n", description);

	/*	In order to write our entries out in the format used by the
		Roster settings file, we need to collect all the signatures
		for each entry in one place, while at the same time updating
		the index values for each entry/sig pair to reflect the current
		ordering of the list. I believe this is the data structure
		R5 actually maintains all the time, as their indices do not
		change over time (whereas ours will). If our implementation
		proves to be slower that R5, we may want to consider using
		the data structure pervasively.
	*/
	std::map<entry_ref, std::list<recent_entry*> > map;
	uint32 count = fEntryList.size();

	try {
		for (std::list<recent_entry*>::iterator item = fEntryList.begin();
				item != fEntryList.end(); count--, item++) {
			recent_entry *entry = *item;
			if (entry) {
				entry->index = count;
				map[entry->ref].push_back(entry);
			} else {
				D(PRINT("WARNING: RecentEntries::Save(): The entry %ld entries "
					"from the front of fEntryList was found to be NULL\n",
					fEntryList.size() - count));
			}
		}
	} catch (...) {
		return B_NO_MEMORY;
	}

	for (std::map<entry_ref, std::list<recent_entry*> >::iterator mapItem = map.begin();
			mapItem != map.end(); mapItem++) {
		// We're going to need to properly escape the path name we
		// get, which will at absolute worst double the length of
		// the string.
		BPath path;
		char escapedPath[B_PATH_NAME_LENGTH*2];
		status_t outputError = path.SetTo(&mapItem->first);
		if (!outputError) {
			BPrivate::Storage::escape_path(path.Path(), escapedPath);
			fprintf(file, "%s %s", tag, escapedPath);
			std::list<recent_entry*> &list = mapItem->second;
			int32 i = 0;
			for (std::list<recent_entry*>::iterator item = list.begin();
					item != list.end(); i++, item++) {
				recent_entry *entry = *item;
				if (entry) {
					fprintf(file, " \"%s\" %" B_PRId32, entry->sig.c_str(),
						entry->index);
				} else {
					D(PRINT("WARNING: RecentEntries::Save(): The entry %ld "
						"entries from the front of the compiled recent_entry* "
						"list for the entry ref (%ld, %lld, '%s') was found to "
						"be NULL\n", i, mapItem->first.device,
						mapItem->first.directory, mapItem->first.name));
				}
			}
			fprintf(file, "\n");
		} else {
			D(PRINT("WARNING: RecentEntries::Save(): entry_ref_to_path() "
				"failed on the entry_ref (%ld, %lld, '%s') with error 0x%lx\n",
				mapItem->first.device, mapItem->first.directory,
				mapItem->first.name, outputError));
		}
	}

	fprintf(file, "\n");
	return B_OK;
}


// GetTypeForRef
/*! \brief Fetches the file type of the given file.

	If the file has no type, an empty string is returned. The file
	is *not* sniffed.
*/
status_t
RecentEntries::GetTypeForRef(const entry_ref *ref, char *result)
{
	if (ref == NULL || result == NULL)
		return B_BAD_VALUE;

	// Read the type
	BNode node;
	status_t error = node.SetTo(ref);
	if (error == B_OK) {
		ssize_t bytes = node.ReadAttr("BEOS:TYPE", B_MIME_STRING_TYPE,
			0, result, B_MIME_TYPE_LENGTH - 1);
		if (bytes < B_OK)
			error = bytes;
		else
			result[bytes] = '\0';
	}

	return error;
}
