//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file InstalledTypes.cpp
	InstalledTypes class implementation
*/

#include "mime/InstalledTypes.h"

#include <Directory.h>
#include <Entry.h>
#include <Message.h>
#include <MimeType.h>
#include <mime/database_support.h>
#include <storage_support.h>

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

// Constructor
//! Constructs a new InstalledTypes object
InstalledTypes::InstalledTypes()
	: fCachedMessage(NULL)
	, fCachedSupertypesMessage(NULL)
	, fHaveDoneFullBuild(false)
{
}

// Destructor
//! Destroys the InstalledTypes object
InstalledTypes::~InstalledTypes()
{
	delete fCachedSupertypesMessage;
	delete fCachedMessage;
}

// GetInstalledTypes
/*! \brief Returns a list of all currently installed types in the
	pre-allocated \c BMessage pointed to by \c types.
	
	See \c BMimeType::GetInstalledTypes(BMessage*) for more information.
*/
status_t 
InstalledTypes::GetInstalledTypes(BMessage *types) 
{
	status_t err = types ? B_OK : B_BAD_VALUE;
	// See if we need to do our initial build still
	if (!err && !fHaveDoneFullBuild) {
		err = BuildInstalledTypesList();
	}
	// See if we need to fill up a new message
	if (!err && !fCachedMessage) {
		err = CreateMessageWithTypes(&fCachedMessage);
	}
	// If we get this far, there a cached message waiting
	if (!err) {
		*types = *fCachedMessage;
	}
	return err;
}

// GetInstalledTypes
/*! \brief Returns a list of all currently installed types of the given
	supertype in the pre-allocated \c BMessage pointed to by \c types.
	
	See \c BMimeType::GetInstalledTypes(const char*, BMessage*) for more
	information.
*/
status_t 
InstalledTypes::GetInstalledTypes(const char *supertype, BMessage *types)
{
	status_t err = supertype && types ? B_OK : B_BAD_VALUE;	
	// Verify the supertype is valid *and* is a supertype
	BMimeType mime;
	BMimeType super;
	// Make sure the supertype is valid
	if (!err)
		err = mime.SetTo(supertype);
	// Make sure it's really a supertype
	if (!err && !mime.IsSupertypeOnly())
		err = B_BAD_VALUE;
	// See if we need to do our initial build still
	if (!err && !fHaveDoneFullBuild) {
		err = BuildInstalledTypesList();
	}
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

// GetInstalledSupertypes
/*! \brief Returns a list of all currently installed supertypes in the
	pre-allocated \c BMessage pointed to by \c types.
	
	See \c BMimeType::GetInstalledSupertypes() for more information.
*/
status_t 
InstalledTypes::GetInstalledSupertypes(BMessage *types)
{
	status_t err = types ? B_OK : B_BAD_VALUE;
	// See if we need to do our initial build still
	if (!err && !fHaveDoneFullBuild) {
		err = BuildInstalledTypesList();
	}
	// See if we need to fill up a new message
	if (!err && !fCachedSupertypesMessage) {
		err = CreateMessageWithSupertypes(&fCachedSupertypesMessage);
	}
	// If we get this far, there's a cached message waiting
	if (!err) {
		*types = *fCachedSupertypesMessage;
	}
	return err;
}

// AddType
/*! \brief Adds the given type to the appropriate lists of installed types.
	
	If cached messages exist, the type is simply appended to the end of
	the current type list.
*/
status_t
InstalledTypes::AddType(const char *type)
{
	status_t err;
	if (fHaveDoneFullBuild) {
		BMimeType mime(type);
		BMimeType super;
		err = type ? mime.InitCheck() : B_BAD_VALUE;
		if (!err) {
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
				err = AddSupertype(type, i);
			} else {
				// Copy the supertype
				char super[B_PATH_NAME_LENGTH];
				strncpy(super, type, i);
				super[i] = 0;
				
				// Get a pointer to the subtype
				const char *sub = &(type[i+1]);
				
				// Add the subtype (which will add the supertype if necessary)
				err = AddSubtype(super, sub);
			}
		}
	} else {
		err = B_OK;
	}
	return err;
}

// RemoveType
/*! \brief Removes the given type from the appropriate installed types lists.

	Any corresponding cached messages are invalidated.
*/
status_t
InstalledTypes::RemoveType(const char *type)
{
	status_t err;
	if (fHaveDoneFullBuild) {
		BMimeType mime(type);
		BMimeType super;
		err = type ? mime.InitCheck() : B_BAD_VALUE;
		if (!err) {
			// Find the / in the string, if one exists
			uint i;
			size_t len = strlen(type);
			for (i = 0; i < len; i++) {
				if (type[i] == '/')
					break;
			}
			if (i == len) {
				// Supertype only
				err = RemoveSupertype(type);
			} else {
				// Copy the supertype
				char super[B_PATH_NAME_LENGTH];
				strncpy(super, type, i);
				super[i] = 0;
				
				// Get a pointer to the subtype
				const char *sub = &(type[i+1]);
				
				// Remove the subtype 
				err = RemoveSubtype(super, sub);
			}
		}
	} else
		err = B_OK;
	return err;
}

// AddSupertype
/*! \brief Adds the given supertype to the supertype map.
	\return
	- B_OK: success, even if the supertype already existed in the map
	- "error code": failure
*/
status_t
InstalledTypes::AddSupertype(const char *super, std::map<std::string, Supertype>::iterator &i)
{
	status_t err = super ? B_OK : B_BAD_VALUE;
	if (!err) {
		i = fSupertypes.find(super);
		if (i == fSupertypes.end()) {
			Supertype &supertype = fSupertypes[super];
			supertype.SetName(super);
			if (fCachedMessage)
				err = fCachedMessage->AddString(kTypesField, super);
			if (!err && fCachedSupertypesMessage)
				err = fCachedSupertypesMessage->AddString(kSupertypesField, super);
		}
	}
	return err;
}

// AddSubtype
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
InstalledTypes::AddSubtype(const char *super, const char *sub)
{
	status_t err = super && sub ? B_OK : B_BAD_VALUE;
	if (!err) {
		std::map<std::string, Supertype>::iterator i;
		err = AddSupertype(super, i);
		if (!err)
			err = AddSubtype(i->second, sub);
	}
	return err;
}

// AddSubtype
/*! \brief Adds the given subtype to the given supertype's lists of installed types.

	\param super The supertype object
	\param sub The subtype (subtype only; no "supertype/subtype" types please)
	\return
	- B_OK: success
	- B_NAME_IN_USE: The subtype already exists in the subtype list
	- "error code": failure
*/
status_t
InstalledTypes::AddSubtype(Supertype &super, const char *sub)
{
	status_t err = sub ? B_OK : B_BAD_VALUE;
	if (!err) 
		err = super.AddSubtype(sub);
	if (!err && fCachedMessage) {
		char type[B_PATH_NAME_LENGTH];
		sprintf(type, "%s/%s", super.GetName(), sub);
		err = fCachedMessage->AddString("types", type);
	}
	return err;
}

// RemoveSupertype
/*! \brief Removes the given supertype and any corresponding subtypes.
*/
status_t
InstalledTypes::RemoveSupertype(const char *super)
{
	status_t err = super ? B_OK : B_BAD_VALUE;
	if (!err) 
		err = fSupertypes.erase(super) == 1 ? B_OK : B_NAME_NOT_FOUND;
	if (!err) 
		ClearCachedMessages();
	return err;
}

// RemoveSubtype
/*! \brief Removes the given subtype from the given supertype.
*/
status_t
InstalledTypes::RemoveSubtype(const char *super, const char *sub)
{
	status_t err = super && sub ? B_OK : B_BAD_VALUE;
	if (!err) {
		std::map<std::string, Supertype>::iterator i = fSupertypes.find(super);
		if (i != fSupertypes.end()) {
			err = i->second.RemoveSubtype(sub);
			if (!err)
				ClearCachedMessages();		
		} else
			err = B_NAME_NOT_FOUND;
	}
	return err;

}

// Unset
// Clears any cached messages and empties the supertype map
void
InstalledTypes::Unset()
{
	ClearCachedMessages();
	fSupertypes.clear();
}

// ClearCachedMessages
//! Frees any cached messages and sets their pointers to NULL
void
InstalledTypes::ClearCachedMessages()
{
	delete fCachedSupertypesMessage;
	delete fCachedMessage;
	fCachedSupertypesMessage = NULL;
	fCachedMessage = NULL;
}

// BuildInstalledTypesList
/*! \brief Reads through the database and builds a complete set of installed types lists.

	An initial set of cached messages are also created.
*/
status_t
InstalledTypes::BuildInstalledTypesList()
{
	status_t err = B_OK;	
	Unset();
	
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
		err = root.SetTo(kDatabaseDir.c_str());
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
					if (AddSupertype(supertype, i) != B_OK)
						DBG(OUT("Mime::InstalledTypes::BuildInstalledTypesList() -- Error adding supertype '%s': 0x%lx\n",
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
								// Get the subtype's name
								char subtype[B_PATH_NAME_LENGTH];
								if (subEntry.GetName(subtype) == B_OK) {
									BPrivate::Storage::to_lower(subtype);
								
									// Add the subtype
									if (AddSubtype(supertypeRef, subtype) != B_OK) {
										DBG(OUT("Mime::InstalledTypes::BuildInstalledTypesList() -- Error adding subtype '%s/%s': 0x%lx\n",
													supertype, subtype, err));
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
		            kDatabaseDir.c_str()));
	}
	fHaveDoneFullBuild = true;
	return err;
		
}

// CreateMessageWithTypes
/*! \brief Allocates a new BMessage into the BMessage pointer pointed to by \c result
	and fills it with a complete list of installed types.
*/
status_t
InstalledTypes::CreateMessageWithTypes(BMessage **result) const
{
	status_t err = result ? B_OK : B_BAD_VALUE;
	// Alloc the message
	if (!err) {
		try {
			*result = new BMessage();
		} catch (std::bad_alloc) {
			err = B_NO_MEMORY;
		}
	}
	// Fill with types
	if (!err) {
		BMessage &msg = **result;
		std::map<std::string, Supertype>::const_iterator i;
		for (i = fSupertypes.begin(); i != fSupertypes.end() && !err; i++) {
			err = msg.AddString(kTypesField, i->first.c_str());
			if (!err)
				err = i->second.FillMessageWithTypes(msg);
		}
	}
	return err;		
}

// CreateMessageWithSupertypes
/*! \brief Allocates a new BMessage into the BMessage pointer pointed to by \c result
	and fills it with a complete list of installed supertypes.
*/
status_t
InstalledTypes::CreateMessageWithSupertypes(BMessage **result) const
{
	status_t err = result ? B_OK : B_BAD_VALUE;
	// Alloc the message
	if (!err) {
		try {
			*result = new BMessage();
		} catch (std::bad_alloc) {
			err = B_NO_MEMORY;
		}
	}
	// Fill with types
	if (!err) {
		BMessage &msg = **result;
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

