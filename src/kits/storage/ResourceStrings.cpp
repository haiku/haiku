//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file ResourceStrings.cpp
	BResourceStrings implementation.
*/

#include <ResourceStrings.h>

#include <new>
#include <stdlib.h>
#include <string.h>

#include <Entry.h>
#include <File.h>
#include <Resources.h>
#include <String.h>

#include <AppMisc.h>

using namespace std;


// constructor
/*! \brief Creates an object initialized to the application's string resources.
*/
BResourceStrings::BResourceStrings()
				: _string_lock(),
				  _init_error(),
				  fFileRef(),
				  fResources(NULL),
				  fHashTable(NULL),
				  fHashTableSize(0),
				  fStringCount(0)
{
	SetStringFile(NULL);
}

// constructor
/*! \brief Creates an object initialized to the string resources of the
	file referred to by the supplied entry_ref.
	\param ref the entry_ref referring to the resource file
*/
BResourceStrings::BResourceStrings(const entry_ref &ref)
				: _string_lock(),
				  _init_error(),
				  fFileRef(),
				  fResources(NULL),
				  fHashTable(NULL),
				  fHashTableSize(0),
				  fStringCount(0)
{
	SetStringFile(&ref);
}

// destructor
/*!	\brief Frees all resources associated with the BResourceStrings object.
*/
BResourceStrings::~BResourceStrings()
{
	_string_lock.Lock();
	_Cleanup();
}

// InitCheck
/*!	\brief Returns the status of the last initialization via contructor or
	SetStringFile().
	\return \c B_OK, if the object is properly initialized, an error code
			otherwise.
*/
status_t
BResourceStrings::InitCheck()
{
	return _init_error;
}

// NewString
/*!	\brief Finds and returns a copy of the string identified by the supplied
	ID.
	The caller is responsible for deleting the returned BString object.
	\param id the ID of the requested string
	\return
	- A string object containing the requested string,
	- \c NULL, if the object is not properly initialized or there is no string
	  with ID \a id.
*/
BString *
BResourceStrings::NewString(int32 id)
{
//	_string_lock.Lock();
	BString *result = NULL;
	if (const char *str = FindString(id))
		result = new(nothrow) BString(str);
//	_string_lock.Unlock();
	return result;
}

// FindString
/*!	\brief Finds and returns the string identified by the supplied ID.
	The caller must not free the returned string. It belongs to the
	BResourceStrings object and is valid until the object is destroyed or set
	to another file.
	\param id the ID of the requested string
	\return
	- The requested string,
	- \c NULL, if the object is not properly initialized or there is no string
	  with ID \a id.
*/
const char *
BResourceStrings::FindString(int32 id)
{
	_string_lock.Lock();
	const char *result = NULL;
	if (InitCheck() == B_OK) {
		if (_string_id_hash *entry = _FindString(id))
			result = entry->data;
	}
	_string_lock.Unlock();
	return result;
}

// SetStringFile
/*!	\brief Re-initialized the BResourceStrings object to the file referred to
	by the supplied entry_ref.
	If the supplied entry_ref is \c NULL, the object is initialized to the
	application file.
	\param ref the entry_ref referring to the resource file
*/
status_t
BResourceStrings::SetStringFile(const entry_ref *ref)
{
	_string_lock.Lock();
	// cleanup
	_Cleanup();
	// get the ref (if NULL, take the application)
	status_t error = B_OK;
	entry_ref fileRef;
	if (ref) {
		fileRef = *ref;
		fFileRef = *ref;
	} else
		error = BPrivate::get_app_ref(&fileRef);
	// get the BResources
	if (error == B_OK) {
		BFile file(&fileRef, B_READ_ONLY);
		error = file.InitCheck();
		if (error == B_OK) {
			fResources = new(nothrow) BResources;
			if (fResources)
				error = fResources->SetTo(&file);
			else
				error = B_NO_MEMORY;
		}
	}
	// read the strings
	if (error == B_OK) {
		// count them first
		fStringCount = 0;
		int32 id;
		const char *name;
		size_t length;
		while (fResources->GetResourceInfo(RESOURCE_TYPE, fStringCount, &id,
										   &name, &length)) {
			fStringCount++;
		}
		// allocate a hash table with a nice size
		// I don't have a heuristic at hand, so let's simply take the count.
		error = _Rehash(fStringCount);
		// load the resources
		for (int32 i = 0; error == B_OK && i < fStringCount; i++) {
			if (!fResources->GetResourceInfo(RESOURCE_TYPE, i, &id, &name,
											 &length)) {
				error = B_ERROR;
			}
			if (error == B_OK) {
				const void *data
					= fResources->LoadResource(RESOURCE_TYPE, id, &length);
				if (data) {
					_string_id_hash *entry = NULL;
					if (length == 0)
						entry = _AddString(NULL, id, false);
					else
						entry = _AddString((char*)data, id, false);
					if (!entry)
						error = B_ERROR;
				} else
					error = B_ERROR;
			}
		}
	}
	// if something went wrong, cleanup the mess
	if (error != B_OK)
		_Cleanup();
	_init_error = error;
	_string_lock.Unlock();
	return error;
}

// GetStringFile
/*!	\brief Returns an entry_ref referring to the resource file, the object is
	currently initialized to.
	\param outRef a pointer to an entry_ref variable to be initialized to the
		   requested entry_ref
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a outRef.
	- other error codes
*/
status_t
BResourceStrings::GetStringFile(entry_ref *outRef)
{
	status_t error = (outRef ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = InitCheck();
	if (error == B_OK) {
		if (fFileRef == entry_ref())
			error = B_ENTRY_NOT_FOUND;
		else
			*outRef = fFileRef;
	}
	return error;
}


// _Cleanup
/*!	\brief Frees all resources associated with this object and sets all
	member variables to harmless values.
*/
void
BResourceStrings::_Cleanup()
{
//	_string_lock.Lock();
	_MakeEmpty();
	delete[] fHashTable;
	fHashTable = NULL;
	delete fResources;
	fResources = NULL;
	fFileRef = entry_ref();
	fHashTableSize = 0;
	fStringCount = 0;
	_init_error = B_OK;
//	_string_lock.Unlock();
}

// _MakeEmpty
/*!	\brief Empties the id->string hash table.
*/
void
BResourceStrings::_MakeEmpty()
{
	if (fHashTable) {
		for (int32 i = 0; i < fHashTableSize; i++) {
			while (_string_id_hash *entry = fHashTable[i]) {
				fHashTable[i] = entry->next;
				delete entry;
			}
		}
		fStringCount = 0;
	}
}

// _Rehash
/*!	\brief Resizes the id->string hash table to the supplied size.
	\param newSize the new hash table size
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Insuffient memory.
*/
status_t
BResourceStrings::_Rehash(int32 newSize)
{
	status_t error = B_OK;
	if (newSize > 0 && newSize != fHashTableSize) {
		// alloc a new table and fill it with NULL
		_string_id_hash **newHashTable
			= new(nothrow) _string_id_hash*[newSize];
		if (newHashTable) {
			memset(newHashTable, 0, sizeof(_string_id_hash*) * newSize);
			// move the entries to the new table
			if (fHashTable && fHashTableSize > 0 && fStringCount > 0) {
				for (int32 i = 0; i < fHashTableSize; i++) {
					while (_string_id_hash *entry = fHashTable[i]) {
						fHashTable[i] = entry->next;
						int32 newPos = entry->id % newSize;
						entry->next = newHashTable[newPos];
						newHashTable[newPos] = entry;
					}
				}
			}
			// set the new table
			delete[] fHashTable;
			fHashTable = newHashTable;
			fHashTableSize = newSize;
		} else
			error = B_NO_MEMORY;
	}
	return error;
}

// _AddString
/*!	\brief Adds an entry to the id->string hash table.
	If there is already a string with the given ID, it will be replaced.
	\param str the string
	\param id the id of the string
	\param wasMalloced if \c true, the object will be responsible for
		   free()ing the supplied string
	\return the hash table entry or \c NULL, if something went wrong
*/
BResourceStrings::_string_id_hash *
BResourceStrings::_AddString(char *str, int32 id, bool wasMalloced)
{
	_string_id_hash *entry = NULL;
	if (fHashTable && fHashTableSize > 0)
		entry = new(nothrow) _string_id_hash;
	if (entry) {
		entry->assign_string(str, false);
		entry->id = id;
		entry->data_alloced = wasMalloced;
		int32 pos = id % fHashTableSize;
		entry->next = fHashTable[pos];
		fHashTable[pos] = entry;
	}
	return entry;
}

// _FindString
/*!	\brief Returns the hash table entry for a given ID.
	\param id the ID
	\return the hash table entry or \c NULL, if there is no entry with this ID
*/
BResourceStrings::_string_id_hash *
BResourceStrings::_FindString(int32 id)
{
	_string_id_hash *entry = NULL;
	if (fHashTable && fHashTableSize > 0) {
		int32 pos = id % fHashTableSize;
		entry = fHashTable[pos];
		while (entry != NULL && entry->id != id)
			entry = entry->next;
	}
	return entry;
}


// FBC
status_t BResourceStrings::_Reserved_ResourceStrings_0(void *) { return 0; }
status_t BResourceStrings::_Reserved_ResourceStrings_1(void *) { return 0; }
status_t BResourceStrings::_Reserved_ResourceStrings_2(void *) { return 0; }
status_t BResourceStrings::_Reserved_ResourceStrings_3(void *) { return 0; }
status_t BResourceStrings::_Reserved_ResourceStrings_4(void *) { return 0; }
status_t BResourceStrings::_Reserved_ResourceStrings_5(void *) { return 0; }


// _string_id_hash

// constructor
/*!	\brief Creates an uninitialized hash table entry.
*/
BResourceStrings::_string_id_hash::_string_id_hash()
	: next(NULL),
	  id(0),
	  data(NULL),
	  data_alloced(false)
{
}

// destructor
/*!	\brief Frees all resources associated with this object.
	Only if \c data_alloced is \c true, the string will be free()d.
*/
BResourceStrings::_string_id_hash::~_string_id_hash()
{
	if (data_alloced)
		free(data);
}

// assign_string
/*!	\brief Sets the string of the hash table entry.
	\param str the string
	\param makeCopy If \c true, the supplied string is copied and the copy
		   will be freed on destruction. If \c false, the entry points to the
		   supplied string. It will not be freed() on destruction.
*/
void
BResourceStrings::_string_id_hash::assign_string(const char *str,
												 bool makeCopy)
{
	if (data_alloced)
		free(data);
	data = NULL;
	data_alloced = false;
	if (str) {
		if (makeCopy) {
			data = strdup(str);
			data_alloced = true;
		} else
			data = const_cast<char*>(str);
	}
}




