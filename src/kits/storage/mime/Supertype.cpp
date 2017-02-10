//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file Supertype.cpp
	Supertype class implementation
*/

#include <mime/Supertype.h>

#include <Message.h>
#include <mime/database_support.h>

#include <new>
#include <stdio.h>

#define DBG(x) x
//#define DBG(x)
#define OUT printf

namespace BPrivate {
namespace Storage {
namespace Mime {

/*!
	\class Supertype
	\brief Installed types information for a single supertype
*/

// Constructor
//! Constructs a new Supertype object
Supertype::Supertype(const char *super)
	: fCachedMessage(NULL)
	, fName(super ? super : "")
{
}

// Destructor
//! Destroys the Supertype object
Supertype::~Supertype()
{
	delete fCachedMessage;
}

// GetInstalledSubtypes
/*! \brief Returns a list of the installeds subtypes for this supertype
	in the pre-allocated \c BMessage pointed to by \c types.
*/
status_t
Supertype::GetInstalledSubtypes(BMessage *types)
{
	status_t err = types ? B_OK : B_BAD_VALUE;
	// See if we need to fill up a new message
	if (!err && !fCachedMessage) {
		err = CreateMessageWithTypes(&fCachedMessage);
	}
	// If we get this far, there's a cached message waiting
	if (!err) {
		*types = *fCachedMessage;
	}
	return err;
}

// AddSubtype
/*! \brief Adds the given subtype to the subtype list and the cached message,
	if one exists.
	\param sub The subtype to add (do not include the supertype)
	\return
	- B_OK: success
	- B_NAME_IN_USE: The subtype already exists in the subtype list
	- "error code": failure
*/
status_t
Supertype::AddSubtype(const char *sub)
{
	status_t err = sub ? B_OK : B_BAD_VALUE;
	if (!err) 
		err = fSubtypes.insert(sub).second ? B_OK : B_NAME_IN_USE;
	if (!err && fCachedMessage) {
		char type[B_PATH_NAME_LENGTH];
		sprintf(type, "%s/%s", fName.c_str(), sub);
		err = fCachedMessage->AddString("types", type);
	}
	return err;
}

// RemoveSubtype
/*! \brief Removes the given subtype from the subtype list and invalidates the
	cached message,	if one exists.
	\param sub The subtype to remove (do not include the supertype)
*/
status_t
Supertype::RemoveSubtype(const char *sub)
{
	status_t err = sub ? B_OK : B_BAD_VALUE;
	if (!err) 
		err = fSubtypes.erase(sub) == 1 ? B_OK : B_NAME_NOT_FOUND;
	if (!err && fCachedMessage) {
		delete fCachedMessage;
		fCachedMessage = NULL;
	}
	return err;
}

// SetName
//! Sets the supertype's name
void
Supertype::SetName(const char *super)
{
	if (super)
		fName = super;
}

// GetName
//! Returns the supertype's name
const char*
Supertype::GetName()
{
	return fName.c_str();
}

// FillMessageWithTypes
//! Adds the supertype's subtypes to the given message
/*! Each subtype is added as another item in the message's \c Mime::kTypesField
	field. The complete type ("supertype/subtype") is added. The supertype itself
	is not added to the message.
*/
status_t
Supertype::FillMessageWithTypes(BMessage &msg) const
{
	status_t err = B_OK;
	std::set<std::string>::const_iterator i;
	for (i = fSubtypes.begin(); i != fSubtypes.end() && !err; i++) {
		char type[B_PATH_NAME_LENGTH];
		sprintf(type, "%s/%s", fName.c_str(), (*i).c_str());
		err = msg.AddString(kTypesField, type);
	}
	return err;		
}

// CreateMessageWithTypes
/*! \brief Allocates a new BMessage into the BMessage pointer pointed to by \c result
	and fills it with the supertype's subtypes.
	
	See \c Supertype::FillMessageWithTypes() for more information.
*/
status_t
Supertype::CreateMessageWithTypes(BMessage **result) const
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
	if (!err)
		err = FillMessageWithTypes(**result);
	return err;		
}

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

