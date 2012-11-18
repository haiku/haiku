/*
 * Copyright 2002-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "InstalledTypes.h"

#include <mime/database_support.h>
#include <storage_support.h>

#include <Directory.h>
#include <Entry.h>
#include <Message.h>
#include <MimeType.h>
#include <String.h>

#include <new>
#include <stdio.h>

#define DBG(x) x
//#define DBG(x)
#define OUT printf

namespace BPrivate {
namespace Storage {
namespace Mime {

/*!
	\class InstalledTypes
	\brief Installed types information for the entire database
*/

//! Constructs a new InstalledTypes object
InstalledTypes::InstalledTypes()
	:
	fCachedMessage(NULL),
	fCachedSupertypesMessage(NULL),
	fHaveDoneFullBuild(false)
{
}


//! Destroys the InstalledTypes object
InstalledTypes::~InstalledTypes()
{
	delete fCachedSupertypesMessage;
	delete fCachedMessage;
}


/*! \brief Returns a list of all currently installed types in the
	pre-allocated \c BMessage pointed to by \c types.

	See \c BMimeType::GetInstalledTypes(BMessage*) for more information.
*/
status_t
InstalledTypes::GetInstalledTypes(BMessage *types)
{
	status_t err = types ? B_OK : B_BAD_VALUE;
	// See if we need to do our initial build still
	if (!err && !fHaveDoneFullBuild)
		err = _BuildInstalledTypesList();

	// See if we need to fill up a new message
	if (!err && !fCachedMessage)
		err = _CreateMessageWithTypes(&fCachedMessage);

	// If we get this far, there a cached message waiting
	if (!err)
		*types = *fCachedMessage;

	return err;
}


/*! \brief Returns a list of all currently installed types of the given
	supertype in the pre-allocated \c BMessage pointed to by \c types.

	See \c BMimeType::GetInstalledTypes(const char*, BMessage*) for more
	information.
*/
status_t
InstalledTypes::GetInstalledTypes(const char *supertype, BMessage *types)
{
	if (supertype == NULL || types == NULL)
		return B_BAD_VALUE;

	// Verify the supertype is valid *and* is a supertype

	BMimeType mime;
	BMimeType super;
	// Make sure the supertype is valid
	status_t err = mime.SetTo(supertype);
	// Make sure it's really a supertype
	if (!err && !mime.IsSupertypeOnly())
		err = B_BAD_VALUE;
	// See if we need to do our initial build still
	if (!err && !fHaveDoneFullBuild)
		err = _BuildInstalledTypesList();

	// Ask the appropriate supertype for its list
	if (!err) {
		std::map<std::string, Supertype>::iterator i = fSupertypes.find(supertype);
		if (i != fSupertypes.end())
			err = i->second.GetInstalledSubtypes(types);
		else
			err = B_NAME_NOT_FOUND;
	}
	return err;
}


/*! \brief Returns a list of all currently installed supertypes in the
	pre-allocated \c BMessage pointed to by \c types.

	See \c BMimeType::GetInstalledSupertypes() for more information.
*/
status_t
InstalledTypes::GetInstalledSupertypes(BMessage *types)
{
	if (types == NULL)
		return B_BAD_VALUE;

	status_t err = B_OK;

	// See if we need to do our initial build still
	if (!fHaveDoneFullBuild)
		err = _BuildInstalledTypesList();

	// See if we need to fill up a new message
	if (!err && !fCachedSupertypesMessage)
		err = _CreateMessageWithSupertypes(&fCachedSupertypesMessage);

	// If we get this far, there's a cached message waiting
	if (!err)
		*types = *fCachedSupertypesMessage;

	return err;
}


/*! \brief Adds the given type to the appropriate lists of installed types.

	If cached messages exist, the type is simply appended to the end of
	the current type list.
*/
status_t
InstalledTypes::AddType(const char *type)
{
	if (!fHaveDoneFullBuild)
		return B_OK;

	BMimeType mime(type);
	if (type == NULL || mime.InitCheck() != B_OK)
		return B_BAD_VALUE;

	// Find the / in the string, if one exists
	uint i;
	size_t len = strlen(type);
	for (i = 0; i < len; i++) {
		if (type[i] == '/')
			break;
	}
	if (i == len) {
		// Supertype only
		std::map<std::string, Supertype>::iterator i;
		return _AddSupertype(type, i);
	}

	// Copy the supertype
	char super[B_PATH_NAME_LENGTH];
	strncpy(super, type, i);
	super[i] = 0;

	// Get a pointer to the subtype
	const char *sub = &(type[i+1]);

	// Add the subtype (which will add the supertype if necessary)
	return _AddSubtype(super, sub);
}


/*! \brief Removes the given type from the appropriate installed types lists.

	Any corresponding cached messages are invalidated.
*/
status_t
InstalledTypes::RemoveType(const char *type)
{
	if (!fHaveDoneFullBuild)
		return B_OK;

	BMimeType mime(type);
	if (type == NULL || mime.InitCheck() != B_OK)
		return B_BAD_VALUE;

	// Find the / in the string, if one exists
	uint i;
	size_t len = strlen(type);
	for (i = 0; i < len; i++) {
		if (type[i] == '/')
			break;
	}
	if (i == len) {
		// Supertype only
		return _RemoveSupertype(type);
	}

	// Copy the supertype
	char super[B_PATH_NAME_LENGTH];
	strncpy(super, type, i);
	super[i] = 0;

	// Get a pointer to the subtype
	const char *sub = &(type[i+1]);

	// Remove the subtype
	return _RemoveSubtype(super, sub);
}


/*! \brief Adds the given supertype to the supertype map.
	\return
	- B_OK: success, even if the supertype already existed in the map
	- "error code": failure
*/
status_t
InstalledTypes::_AddSupertype(const char *super,
	std::map<std::string, Supertype>::iterator &i)
{
	if (super == NULL)
		return B_BAD_VALUE;

	status_t err = B_OK;

	i = fSupertypes.find(super);
	if (i == fSupertypes.end()) {
		Supertype &supertype = fSupertypes[super];
		supertype.SetName(super);
		if (fCachedMessage)
			err = fCachedMessage->AddString(kTypesField, super);
		if (!err && fCachedSupertypesMessage)
			err = fCachedSupertypesMessage->AddString(kSupertypesField, super);
	}

	return err;
}


/*! \brief Adds the given subtype to the given supertype's lists of installed types.

	If the supertype does not yet exist, it is created.

	\param super The supertype
	\param sub The subtype (subtype only; no "supertype/subtype" types please)
	\return
	- B_OK: success
	- B_NAME_IN_USE: The subtype already exists in the subtype list
	- "error code": failure
*/
status_t
InstalledTypes::_AddSubtype(const char *super, const char *sub)
{
	if (super == NULL || sub == NULL)
		return B_BAD_VALUE;

	std::map<std::string, Supertype>::iterator i;
	status_t err = _AddSupertype(super, i);
	if (!err)
		err = _AddSubtype(i->second, sub);

	return err;
}


/*! \brief Adds the given subtype to the given supertype's lists of installed types.

	\param super The supertype object
	\param sub The subtype (subtype only; no "supertype/subtype" types please)
	\return
	- B_OK: success
	- B_NAME_IN_USE: The subtype already exists in the subtype list
	- "error code": failure
*/
status_t
InstalledTypes::_AddSubtype(Supertype &super, const char *sub)
{
	if (sub == NULL)
		return B_BAD_VALUE;

	status_t err = super.AddSubtype(sub);
	if (!err && fCachedMessage) {
		char type[B_PATH_NAME_LENGTH];
		sprintf(type, "%s/%s", super.GetName(), sub);
		err = fCachedMessage->AddString("types", type);
	}
	return err;
}


/*! \brief Removes the given supertype and any corresponding subtypes.
*/
status_t
InstalledTypes::_RemoveSupertype(const char *super)
{
	if (super == NULL)
		return B_BAD_VALUE;

	status_t err = fSupertypes.erase(super) == 1 ? B_OK : B_NAME_NOT_FOUND;
	if (!err)
		_ClearCachedMessages();
	return err;
}


/*! \brief Removes the given subtype from the given supertype.
*/
status_t
InstalledTypes::_RemoveSubtype(const char *super, const char *sub)
{
	if (super == NULL || sub == NULL)
		return B_BAD_VALUE;

	status_t err = B_NAME_NOT_FOUND;

	std::map<std::string, Supertype>::iterator i = fSupertypes.find(super);
	if (i != fSupertypes.end()) {
		err = i->second.RemoveSubtype(sub);
		if (!err)
			_ClearCachedMessages();
	}

	return err;

}


//! Clears any cached messages and empties the supertype map
void
InstalledTypes::_Unset()
{
	_ClearCachedMessages();
	fSupertypes.clear();
}


//! Frees any cached messages and sets their pointers to NULL
void
InstalledTypes::_ClearCachedMessages()
{
	delete fCachedSupertypesMessage;
	delete fCachedMessage;
	fCachedSupertypesMessage = NULL;
	fCachedMessage = NULL;
}


/*! \brief Reads through the database and builds a complete set of installed types lists.

	An initial set of cached messages are also created.
*/
status_t
InstalledTypes::_BuildInstalledTypesList()
{
	status_t err = B_OK;
	_Unset();

	// Create empty "cached messages" so proper messages
	// will be built up as we add new types
	try {
		fCachedMessage = new BMessage();
		fCachedSupertypesMessage = new BMessage();
	} catch (std::bad_alloc) {
		err = B_NO_MEMORY;
	}

	BDirectory root;
	if (!err)
		err = root.SetTo(get_database_directory().c_str());
	if (!err) {
		root.Rewind();
		while (true) {
			BEntry entry;
			err = root.GetNextEntry(&entry);
			if (err) {
				// If we've come to the end of list, it's not an error
				if (err == B_ENTRY_NOT_FOUND)
					err = B_OK;
				break;
			} else {
				// Check that this entry is both a directory and a valid MIME string
				char supertype[B_PATH_NAME_LENGTH];
				if (entry.IsDirectory()
				      && entry.GetName(supertype) == B_OK
				         && BMimeType::IsValid(supertype))
				{
					// Make sure our string is all lowercase
					BPrivate::Storage::to_lower(supertype);

					// Add this supertype
					std::map<std::string, Supertype>::iterator i;
					if (_AddSupertype(supertype, i) != B_OK)
						DBG(OUT("Mime::InstalledTypes::BuildInstalledTypesList()"
							" -- Error adding supertype '%s': 0x%" B_PRIx32 "\n",
							supertype, err));
					Supertype &supertypeRef = fSupertypes[supertype];

					// Now iterate through this supertype directory and add
					// all of its subtypes
					BDirectory dir;
					if (dir.SetTo(&entry) == B_OK) {
						dir.Rewind();
						while (true) {
							BEntry subEntry;
							err = dir.GetNextEntry(&subEntry);
							if (err) {
								// If we've come to the end of list, it's not an error
								if (err == B_ENTRY_NOT_FOUND)
									err = B_OK;
								break;
							} else {
								// We need to preserve the case of the type name for
								// queries, so we can't use the file name directly
								BString type;
								int32 subStart;
								BNode node(&subEntry);
								if (node.InitCheck() == B_OK
									&& node.ReadAttrString(kTypeAttr, &type) >= B_OK
									&& (subStart = type.FindFirst('/')) > 0) {
									// Add the subtype
									if (_AddSubtype(supertypeRef, type.String()
											+ subStart + 1) != B_OK) {
										DBG(OUT("Mime::InstalledTypes::BuildInstalledTypesList()"
											" -- Error adding subtype '%s/%s': 0x%" B_PRIx32 "\n",
											supertype, type.String() + subStart + 1, err));
									}
								}
							}
						}
					} else {
						DBG(OUT("Mime::InstalledTypes::BuildInstalledTypesList(): "
						          "Failed opening supertype directory '%s'\n",
						            supertype));
					}
				}
			}
		}
	} else {
		DBG(OUT("Mime::InstalledTypes::BuildInstalledTypesList(): "
		          "Failed opening mime database directory '%s'\n",
		            get_database_directory().c_str()));
	}
	fHaveDoneFullBuild = true;
	return err;

}


/*! \brief Allocates a new BMessage into the BMessage pointer pointed to by \c result
	and fills it with a complete list of installed types.
*/
status_t
InstalledTypes::_CreateMessageWithTypes(BMessage **_result) const
{
	if (_result == NULL)
		return B_BAD_VALUE;

	status_t err = B_OK;

	// Alloc the message
	try {
		*_result = new BMessage();
	} catch (std::bad_alloc) {
		err = B_NO_MEMORY;
	}

	// Fill with types
	if (!err) {
		BMessage &msg = **_result;
		std::map<std::string, Supertype>::const_iterator i;
		for (i = fSupertypes.begin(); i != fSupertypes.end() && !err; i++) {
			err = msg.AddString(kTypesField, i->first.c_str());
			if (!err)
				err = i->second.FillMessageWithTypes(msg);
		}
	}
	return err;
}


/*! \brief Allocates a new BMessage into the BMessage pointer pointed to by \c result
	and fills it with a complete list of installed supertypes.
*/
status_t
InstalledTypes::_CreateMessageWithSupertypes(BMessage **_result) const
{
	if (_result == NULL)
		return B_BAD_VALUE;

	status_t err = B_OK;

	// Alloc the message
	try {
		*_result = new BMessage();
	} catch (std::bad_alloc) {
		err = B_NO_MEMORY;
	}

	// Fill with types
	if (!err) {
		BMessage &msg = **_result;
		std::map<std::string, Supertype>::const_iterator i;
		for (i = fSupertypes.begin(); i != fSupertypes.end() && !err; i++) {
			err = msg.AddString(kSupertypesField, i->first.c_str());
		}
	}
	return err;
}

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

