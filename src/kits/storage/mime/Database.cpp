/*
 * Copyright 2002-2014, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Axel Dörfler, axeld@pinc-software.de
 *		Rene Gollent, rene@gollent.com.
 */


#include <mime/Database.h>

#include <stdio.h>
#include <string>

#include <new>

#include <Application.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <Directory.h>
#include <Entry.h>
#include <fs_attr.h>
#include <Message.h>
#include <MimeType.h>
#include <Node.h>
#include <Path.h>
#include <String.h>
#include <TypeConstants.h>

#include <AutoLocker.h>
#include <mime/database_support.h>
#include <mime/DatabaseLocation.h>
#include <storage_support.h>


//#define DBG(x) x
#define DBG(x)
#define OUT printf


namespace BPrivate {
namespace Storage {
namespace Mime {


Database::NotificationListener::~NotificationListener()
{
}


/*!
	\class Database
	\brief Mime::Database is the master of the MIME data base.

	All write and non-atomic read accesses are carried out by this class.

	\note No error checking (other than checks for NULL pointers) is performed
	      by this class on the mime type strings passed to it. It's assumed
	      that this sort of checking has been done beforehand.
*/

// constructor
/*!	\brief Creates and initializes a Mime::Database object.
*/
Database::Database(DatabaseLocation* databaseLocation, MimeSniffer* mimeSniffer,
	NotificationListener* notificationListener)
	:
	fStatus(B_NO_INIT),
	fLocation(databaseLocation),
	fNotificationListener(notificationListener),
	fAssociatedTypes(databaseLocation, mimeSniffer),
	fInstalledTypes(databaseLocation),
	fSnifferRules(databaseLocation, mimeSniffer),
	fSupportingApps(databaseLocation),
	fDeferredInstallNotificationsLocker("deferred install notifications"),
	fDeferredInstallNotifications()
{
	// make sure the user's MIME DB directory exists
	fStatus = create_directory(fLocation->WritableDirectory(),
		S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

// destructor
/*!	\brief Frees all resources associated with this object.
*/
Database::~Database()
{
}

// InitCheck
/*! \brief Returns the initialization status of the object.
	\return
	- B_OK: success
	- "error code": failure
*/
status_t
Database::InitCheck() const
{
	return fStatus;
}

// Install
/*!	\brief Installs the given type in the database
	\note The R5 version of this call returned an unreliable result if the
	      MIME type was already installed. Ours simply returns B_OK.
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\param decsription Pointer to a NULL-terminated string containing the new long description
	\return
	- B_OK: success
	- B_FILE_EXISTS: the type is already installed
	- "error code": failure
*/
status_t
Database::Install(const char *type)
{
	if (type == NULL)
		return B_BAD_VALUE;

	BEntry entry;
	status_t err = entry.SetTo(fLocation->WritablePathForType(type));
	if (err == B_OK || err == B_ENTRY_NOT_FOUND) {
		if (entry.Exists())
			err = B_FILE_EXISTS;
		else {
			bool didCreate = false;
			BNode node;
			err = fLocation->OpenWritableType(type, node, true, &didCreate);
			if (!err && didCreate) {
				fInstalledTypes.AddType(type);
				_SendInstallNotification(type);
			}
		}
	}
	return err;
}

// Delete
/*!	\brief Removes the given type from the database
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\return
	- B_OK: success
	- "error code": failure
*/
status_t
Database::Delete(const char *type)
{
	if (type == NULL)
		return B_BAD_VALUE;

	// Open the type
	BEntry entry;
	status_t status = entry.SetTo(fLocation->WritablePathForType(type));
	if (status != B_OK)
		return status;

	// Remove it
	if (entry.IsDirectory()) {
		// We need to remove all files in this directory
		BDirectory directory(&entry);
		if (directory.InitCheck() == B_OK) {
			size_t length = strlen(type);
			char subType[B_PATH_NAME_LENGTH];
			memcpy(subType, type, length);
			subType[length++] = '/';

			BEntry subEntry;
			while (directory.GetNextEntry(&subEntry) == B_OK) {
				// Construct MIME type and remove it
				if (subEntry.GetName(subType + length) == B_OK) {
					status = Delete(subType);
					if (status != B_OK)
						return status;
				}
			}
		}
	}

	status = entry.Remove();

	if (status == B_OK) {
		// Notify the installed types database
		fInstalledTypes.RemoveType(type);
		// Notify the supporting apps database
		fSupportingApps.DeleteSupportedTypes(type, true);
		// Notify the monitor service
		_SendDeleteNotification(type);
	}

	return status;
}


status_t
Database::_SetStringValue(const char *type, int32 what, const char* attribute,
	type_code attributeType, size_t maxLength, const char *value)
{
	size_t length = value != NULL ? strlen(value) : 0;
	if (type == NULL || value == NULL || length >= maxLength)
		return B_BAD_VALUE;

	char oldValue[maxLength];
	status_t status = fLocation->ReadAttribute(type, attribute, oldValue,
		maxLength, attributeType);
	if (status >= B_OK && !strcmp(value, oldValue)) {
		// nothing has changed, no need to write back the data
		return B_OK;
	}

	bool didCreate = false;
	status = fLocation->WriteAttribute(type, attribute, value, length + 1,
		attributeType, &didCreate);

	if (status == B_OK) {
		if (didCreate)
			_SendInstallNotification(type);
		else
			_SendMonitorUpdate(what, type, B_META_MIME_MODIFIED);
	}

	return status;
}


// SetAppHint
/*!	\brief Sets the application hint for the given MIME type
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\param decsription Pointer to an entry_ref containing the location of an application
	       that should be used when launching an application with this signature.
*/
status_t
Database::SetAppHint(const char *type, const entry_ref *ref)
{
	DBG(OUT("Database::SetAppHint()\n"));

	if (type == NULL || ref == NULL)
		return B_BAD_VALUE;

	BPath path;
	status_t status = path.SetTo(ref);
	if (status < B_OK)
		return status;

	return _SetStringValue(type, B_APP_HINT_CHANGED, kAppHintAttr,
		kAppHintType, B_PATH_NAME_LENGTH, path.Path());
}

// SetAttrInfo
/*! \brief Stores a BMessage describing the format of attributes typically associated with
	files of the given MIME type

	See BMimeType::SetAttrInfo() for description of the expected message format.

	The \c BMessage::what value is ignored.

	\param info Pointer to a pre-allocated and properly formatted BMessage containing
	            information about the file attributes typically associated with the
	            MIME type.
	\return
	- \c B_OK: Success
	- "error code": Failure
*/
status_t
Database::SetAttrInfo(const char *type, const BMessage *info)
{
	DBG(OUT("Database::SetAttrInfo()\n"));

	if (type == NULL || info == NULL)
		return B_BAD_VALUE;

	bool didCreate = false;
	status_t status = fLocation->WriteMessageAttribute(type, kAttrInfoAttr,
		*info, &didCreate);
	if (status == B_OK) {
		if (didCreate)
			_SendInstallNotification(type);
		else
			_SendMonitorUpdate(B_ATTR_INFO_CHANGED, type, B_META_MIME_MODIFIED);
	}

	return status;
}


// SetShortDescription
/*!	\brief Sets the short description for the given MIME type
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\param decsription Pointer to a NULL-terminated string containing the new short description
*/
status_t
Database::SetShortDescription(const char *type, const char *description)
{
	DBG(OUT("Database::SetShortDescription()\n"));

	return _SetStringValue(type, B_SHORT_DESCRIPTION_CHANGED, kShortDescriptionAttr,
		kShortDescriptionType, B_MIME_TYPE_LENGTH, description);
}

// SetLongDescription
/*!	\brief Sets the long description for the given MIME type
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\param decsription Pointer to a NULL-terminated string containing the new long description
*/
status_t
Database::SetLongDescription(const char *type, const char *description)
{
	DBG(OUT("Database::SetLongDescription()\n"));

	size_t length = description != NULL ? strlen(description) : 0;
	if (type == NULL || description == NULL || length >= B_MIME_TYPE_LENGTH)
		return B_BAD_VALUE;

	return _SetStringValue(type, B_LONG_DESCRIPTION_CHANGED, kLongDescriptionAttr,
		kLongDescriptionType, B_MIME_TYPE_LENGTH, description);
}


/*!
	\brief Sets the list of filename extensions associated with the MIME type

	The list of extensions is given in a pre-allocated BMessage pointed to by
	the \c extensions parameter. Please see BMimeType::SetFileExtensions()
	for a description of the expected message format.

	\param extensions Pointer to a pre-allocated, properly formatted BMessage containing
	                  the new list of file extensions to associate with this MIME type.
	\return
	- \c B_OK: Success
	- "error code": Failure
*/
status_t
Database::SetFileExtensions(const char *type, const BMessage *extensions)
{
	DBG(OUT("Database::SetFileExtensions()\n"));

	if (type == NULL || extensions == NULL)
		return B_BAD_VALUE;

	bool didCreate = false;
	status_t status = fLocation->WriteMessageAttribute(type,
		kFileExtensionsAttr, *extensions, &didCreate);

	if (status == B_OK) {
		if (didCreate) {
			_SendInstallNotification(type);
		} else {
			_SendMonitorUpdate(B_FILE_EXTENSIONS_CHANGED, type,
				B_META_MIME_MODIFIED);
		}
	}

	return status;
}


/*!
	\brief Sets a bitmap icon for the given mime type
*/
status_t
Database::SetIcon(const char* type, const BBitmap* icon, icon_size which)
{
	if (icon != NULL)
		return SetIcon(type, icon->Bits(), icon->BitsLength(), which);
	return SetIcon(type, NULL, 0, which);
}


/*!
	\brief Sets a bitmap icon for the given mime type
*/
status_t
Database::SetIcon(const char *type, const void *data, size_t dataSize,
	icon_size which)
{
	return SetIconForType(type, NULL, data, dataSize, which);
}


/*!
	\brief Sets the vector icon for the given mime type
*/
status_t
Database::SetIcon(const char *type, const void *data, size_t dataSize)
{
	return SetIconForType(type, NULL, data, dataSize);
}


status_t
Database::SetIconForType(const char* type, const char* fileType,
	const BBitmap* icon, icon_size which)
{
	if (icon != NULL) {
		return SetIconForType(type, fileType, icon->Bits(),
			(size_t)icon->BitsLength(), which);
	}
	return SetIconForType(type, fileType, NULL, 0, which);
}


// SetIconForType
/*! \brief Sets the large or mini icon used by an application of this type for
	files of the given type.

	The type of the \c BMimeType object is not required to actually be a subtype of
	\c "application/"; that is the intended use however, and application-specific
	icons are not expected to be present for non-application types.

	The bitmap data pointed to by \c data must be of the proper size (\c 32x32
	for \c B_LARGE_ICON, \c 16x16 for \c B_MINI_ICON) and the proper color
	space (B_CMAP8).

	\param type The MIME type
	\param fileType The MIME type whose custom icon you wish to set.
	\param data Pointer to an array of bitmap data of proper dimensions and color depth
	\param dataSize The length of the array pointed to by \c data
	\param size The size icon you're expecting (\c B_LARGE_ICON or \c B_MINI_ICON)
	\return
	- \c B_OK: Success
	- "error code": Failure

*/
status_t
Database::SetIconForType(const char *type, const char *fileType,
	const void *data, size_t dataSize, icon_size which)
{
	DBG(OUT("Database::SetIconForType()\n"));

	if (type == NULL || data == NULL)
		return B_BAD_VALUE;

	int32 attrType = 0;

	// Figure out what kind of data we *should* have
	switch (which) {
		case B_MINI_ICON:
			attrType = kMiniIconType;
			break;
		case B_LARGE_ICON:
			attrType = kLargeIconType;
			break;

		default:
			return B_BAD_VALUE;
	}

	size_t attrSize = (size_t)which * (size_t)which;
	// Double check the data we've been given
	if (dataSize != attrSize)
		return B_BAD_VALUE;

	// Construct our attribute name
	std::string attr;
	if (fileType) {
		attr = (which == B_MINI_ICON
			? kMiniIconAttrPrefix : kLargeIconAttrPrefix)
			+ BPrivate::Storage::to_lower(fileType);
	} else
		attr = which == B_MINI_ICON ? kMiniIconAttr : kLargeIconAttr;

	// Write the icon data
	BNode node;
	bool didCreate = false;

	status_t err = fLocation->OpenWritableType(type, node, true, &didCreate);
	if (err != B_OK)
		return err;

	if (!err)
		err = node.WriteAttr(attr.c_str(), attrType, 0, data, attrSize);
	if (err >= 0)
		err = err == (ssize_t)attrSize ? (status_t)B_OK : (status_t)B_FILE_ERROR;
	if (didCreate) {
		_SendInstallNotification(type);
	} else if (!err) {
		if (fileType) {
			_SendMonitorUpdate(B_ICON_FOR_TYPE_CHANGED, type, fileType,
				which == B_LARGE_ICON, B_META_MIME_MODIFIED);
		} else {
			_SendMonitorUpdate(B_ICON_CHANGED, type,
				which == B_LARGE_ICON, B_META_MIME_MODIFIED);
		}
	}
	return err;
}

// SetIconForType
/*! \brief Sets the vector icon used by an application of this type for
	files of the given type.

	The type of the \c BMimeType object is not required to actually be a subtype of
	\c "application/"; that is the intended use however, and application-specific
	icons are not expected to be present for non-application types.

	\param type The MIME type
	\param fileType The MIME type whose custom icon you wish to set.
	\param data Pointer to an array of vector data
	\param dataSize The length of the array pointed to by \c data
	\return
	- \c B_OK: Success
	- "error code": Failure

*/
status_t
Database::SetIconForType(const char *type, const char *fileType,
	const void *data, size_t dataSize)
{
	DBG(OUT("Database::SetIconForType()\n"));

	if (type == NULL || data == NULL)
		return B_BAD_VALUE;

	int32 attrType = B_VECTOR_ICON_TYPE;

	// Construct our attribute name
	std::string attr;
	if (fileType) {
		attr = kIconAttrPrefix + BPrivate::Storage::to_lower(fileType);
	} else
		attr = kIconAttr;

	// Write the icon data
	BNode node;
	bool didCreate = false;

	status_t err = fLocation->OpenWritableType(type, node, true, &didCreate);
	if (err != B_OK)
		return err;

	if (!err)
		err = node.WriteAttr(attr.c_str(), attrType, 0, data, dataSize);
	if (err >= 0)
		err = err == (ssize_t)dataSize ? (status_t)B_OK : (status_t)B_FILE_ERROR;
	if (didCreate) {
		_SendInstallNotification(type);
	} else if (!err) {
		// TODO: extra notification for vector icons (currently
		// passing "true" for B_LARGE_ICON)?
		if (fileType) {
			_SendMonitorUpdate(B_ICON_FOR_TYPE_CHANGED, type, fileType,
				true, B_META_MIME_MODIFIED);
		} else {
			_SendMonitorUpdate(B_ICON_CHANGED, type, true,
				B_META_MIME_MODIFIED);
		}
	}
	return err;
}

// SetPreferredApp
/*!	\brief Sets the signature of the preferred application for the given app verb

	Currently, the only supported app verb is \c B_OPEN
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\param signature Pointer to a NULL-terminated string containing the MIME signature
	                 of the new preferred application
	\param verb \c app_verb action for which the new preferred application is applicable
*/
status_t
Database::SetPreferredApp(const char *type, const char *signature, app_verb verb)
{
	DBG(OUT("Database::SetPreferredApp()\n"));

	// TODO: use "verb" some day!

	return _SetStringValue(type, B_PREFERRED_APP_CHANGED, kPreferredAppAttr,
		kPreferredAppType, B_MIME_TYPE_LENGTH, signature);
}

// SetSnifferRule
/*! \brief Sets the mime sniffer rule for the given mime type
*/
status_t
Database::SetSnifferRule(const char *type, const char *rule)
{
	DBG(OUT("Database::SetSnifferRule()\n"));

	if (type == NULL || rule == NULL)
		return B_BAD_VALUE;

	bool didCreate = false;
	status_t status = fLocation->WriteAttribute(type, kSnifferRuleAttr, rule,
		strlen(rule) + 1, kSnifferRuleType, &didCreate);

	if (status == B_OK)
		status = fSnifferRules.SetSnifferRule(type, rule);

	if (didCreate) {
		_SendInstallNotification(type);
	} else if (status == B_OK) {
		_SendMonitorUpdate(B_SNIFFER_RULE_CHANGED, type,
			B_META_MIME_MODIFIED);
	}

	return status;
}

// SetSupportedTypes
/*!	\brief Sets the list of MIME types supported by the MIME type and
	syncs the internal supporting apps database either partially or
	completely.

	Please see BMimeType::SetSupportedTypes() for details.
	\param type The mime type of interest
	\param types The supported types to be assigned to the file.
	\param syncAll \c true to also synchronize the previously supported
		   types, \c false otherwise.
	\return
	- \c B_OK: success
	- other error codes: failure
*/
status_t
Database::SetSupportedTypes(const char *type, const BMessage *types, bool fullSync)
{
	DBG(OUT("Database::SetSupportedTypes()\n"));

	if (type == NULL || types == NULL)
		return B_BAD_VALUE;

	// Install the types
	const char *supportedType;
	for (int32 i = 0; types->FindString("types", i, &supportedType) == B_OK; i++) {
		if (!fLocation->IsInstalled(supportedType)) {
			if (Install(supportedType) != B_OK)
				break;

			// Since the type has been introduced by this application
			// we take the liberty and make it the preferred handler
			// for them, too.
			SetPreferredApp(supportedType, type, B_OPEN);
		}
	}

	// Write the attr
	bool didCreate = false;
	status_t status = fLocation->WriteMessageAttribute(type,
		kSupportedTypesAttr, *types, &didCreate);

	// Notify the monitor if we created the type when we opened it
	if (status != B_OK)
		return status;

	// Update the supporting apps map
	if (status == B_OK)
		status = fSupportingApps.SetSupportedTypes(type, types, fullSync);

	// Notify the monitor
	if (didCreate) {
		_SendInstallNotification(type);
	} else if (status == B_OK) {
		_SendMonitorUpdate(B_SUPPORTED_TYPES_CHANGED, type,
			B_META_MIME_MODIFIED);
	}

	return status;
}


// GetInstalledSupertypes
/*! \brief Fetches a BMessage listing all the MIME supertypes currently
	installed in the MIME database.

	The types are copied into the \c "super_types" field of the passed-in \c BMessage.
	The \c BMessage must be pre-allocated.

	\param super_types Pointer to a pre-allocated \c BMessage into which the
	                   MIME supertypes will be copied.
	\return
	- \c B_OK: Success
	- "error code": Failure
*/
status_t
Database::GetInstalledSupertypes(BMessage *supertypes)
{
	return fInstalledTypes.GetInstalledSupertypes(supertypes);
}

// GetInstalledTypes
/*! \brief Fetches a BMessage listing all the MIME types currently installed
	in the MIME database.

	The types are copied into the \c "types" field of the passed-in \c BMessage.
	The \c BMessage must be pre-allocated.

	\param types Pointer to a pre-allocated \c BMessage into which the
	             MIME types will be copied.
	\return
	- \c B_OK: Success
	- "error code": Failure
*/
status_t
Database::GetInstalledTypes(BMessage *types)
{
	return fInstalledTypes.GetInstalledTypes(types);
}

// GetInstalledTypes
/*! \brief Fetches a BMessage listing all the MIME subtypes of the given
	supertype currently installed in the MIME database.

	The types are copied into the \c "types" field of the passed-in \c BMessage.
	The \c BMessage must be pre-allocated.

	\param super_type Pointer to a string containing the MIME supertype whose
	                  subtypes you wish to retrieve.
	\param subtypes Pointer to a pre-allocated \c BMessage into which the appropriate
	                MIME subtypes will be copied.
	\return
	- \c B_OK: Success
	- "error code": Failure
*/
status_t
Database::GetInstalledTypes(const char *supertype, BMessage *subtypes)
{
	return fInstalledTypes.GetInstalledTypes(supertype, subtypes);
}

// GetSupportingApps
/*! \brief Fetches a \c BMessage containing a list of MIME signatures of
	applications that are able to handle files of this MIME type.

	Please see BMimeType::GetSupportingApps() for more details.
*/
status_t
Database::GetSupportingApps(const char *type, BMessage *signatures)
{
	return fSupportingApps.GetSupportingApps(type, signatures);
}

// GetAssociatedTypes
/*! \brief Returns a list of mime types associated with the given file extension

	Please see BMimeType::GetAssociatedTypes() for more details.
*/
status_t
Database::GetAssociatedTypes(const char *extension, BMessage *types)
{
	return B_ERROR;
}

// GuessMimeType
/*!	\brief Guesses a MIME type for the entry referred to by the given
	\c entry_ref.

	This version of GuessMimeType() combines the features of the other
	versions, plus adds a few tricks of its own:
	- If the entry is a meta mime entry (i.e. has a \c "META:TYPE" attribute),
	  the type returned is \c "application/x-vnd.be-meta-mime".
	- If the entry is a directory, the type returned is
	  \c "application/x-vnd.be-directory".
	- If the entry is a symlink, the type returned is
	  \c "application/x-vnd.be-symlink".
	- If the entry is a regular file, the file data is sniffed and, the
	  type returned is the mime type with the matching rule of highest
	  priority.
	- If sniffing fails, the filename is checked for known extensions.
	- If the extension check fails, the type returned is
	  \c "application/octet-stream".

	\param ref Pointer to the entry_ref referring to the entry.
	\param type Pointer to a pre-allocated BString which is set to the
		   resulting MIME type.
	\return
	- \c B_OK: success (even if the guess returned is "application/octet-stream")
	- other error code: failure
*/
status_t
Database::GuessMimeType(const entry_ref *ref, BString *result)
{
	if (ref == NULL || result == NULL)
		return B_BAD_VALUE;

	BNode node;
	struct stat statData;
	status_t status = node.SetTo(ref);
	if (status < B_OK)
		return status;

	attr_info info;
	if (node.GetAttrInfo(kTypeAttr, &info) == B_OK) {
		// Check for a META:TYPE attribute
		result->SetTo(kMetaMimeType);
		return B_OK;
	}

	// See if we have a directory, a symlink, or a vanilla file
	status = node.GetStat(&statData);
	if (status < B_OK)
		return status;

	if (S_ISDIR(statData.st_mode)) {
		// Directory
		result->SetTo(kDirectoryType);
	} else if (S_ISLNK(statData.st_mode)) {
		// Symlink
		result->SetTo(kSymlinkType);
	} else if (S_ISREG(statData.st_mode)) {
		// Vanilla file: sniff first
		status = fSnifferRules.GuessMimeType(ref, result);

		// If that fails, check extensions
		if (status == kMimeGuessFailureError)
			status = fAssociatedTypes.GuessMimeType(ref, result);

		// If that fails, return the generic file type
		if (status == kMimeGuessFailureError) {
			result->SetTo(kGenericFileType);
			status = B_OK;
		}
	} else {
		// TODO: we could filter out devices, ...
		return B_BAD_TYPE;
	}

	return status;
}

// GuessMimeType
/*!	\brief Guesses a MIME type for the supplied chunk of data.

	See \c SnifferRules::GuessMimeType(BPositionIO*, BString*)
	for more details.

	\param buffer Pointer to the data buffer.
	\param length Size of the buffer in bytes.
	\param type Pointer to a pre-allocated BString which is set to the
		   resulting MIME type.
	\return
	- \c B_OK: success
	- error code: failure
*/
status_t
Database::GuessMimeType(const void *buffer, int32 length, BString *result)
{
	if (buffer == NULL || result == NULL)
		return B_BAD_VALUE;

	status_t status = fSnifferRules.GuessMimeType(buffer, length, result);
	if (status == kMimeGuessFailureError) {
		result->SetTo(kGenericFileType);
		return B_OK;
	}

	return status;
}

// GuessMimeType
/*!	\brief Guesses a MIME type for the given filename.

	Only the filename itself is taken into consideration (in particular its
	name extension), not the entry or corresponding data it refers to (in fact,
	an entry with that name need not exist at all.

	\param filename The filename.
	\param type Pointer to a pre-allocated BString which is set to the
		   resulting MIME type.
	\return
	- \c B_OK: success
	- error code: failure
*/
status_t
Database::GuessMimeType(const char *filename, BString *result)
{
	if (filename == NULL || result == NULL)
		return B_BAD_VALUE;

	status_t status = fAssociatedTypes.GuessMimeType(filename, result);
	if (status == kMimeGuessFailureError) {
		result->SetTo(kGenericFileType);
		return B_OK;
	}

	return status;
}


/*!	\brief Subscribes the given BMessenger to the MIME monitor service

	Notification messages will be sent with a \c BMessage::what value
	of \c B_META_MIME_CHANGED. Notification messages have the following
	fields:

	<table>
		<tr>
			<td> Name </td>
			<td> Type </td>
			<td> Description </td>
		</tr>
		<tr>
			<td> \c be:type </td>
			<td> \c B_STRING_TYPE </td>
			<td> The MIME type that was changed </td>
		</tr>
		<tr>
			<td> \c be:which </td>
			<td> \c B_INT32_TYPE </td>
			<td> Bitmask describing which attributes were changed (see below) </td>
		</tr>
		<tr>
			<td> \c be:extra_type </td>
			<td> \c B_STRING_TYPE </td>
			<td> Additional MIME type string (applicable to B_ICON_FOR_TYPE_CHANGED notifications only)</td>
		</tr>
		<tr>
			<td> \c be:large_icon </td>
			<td> \c B_BOOL_TYPE </td>
			<td> \c true if the large icon was changed, \c false if the small icon
			     was changed (applicable to B_ICON_[FOR_TYPE_]CHANGED updates only) </td>
		</tr>
	</table>

	The \c be:which field of the message describes which attributes were updated, and
	may be the bitwise \c OR of any of the following values:

	<table>
		<tr>
			<td> Value </td>
			<td> Triggered By </td>
		</tr>
		<tr>
			<td> \c B_ICON_CHANGED </td>
			<td> \c BMimeType::SetIcon() </td>
		</tr>
		<tr>
			<td> \c B_PREFERRED_APP_CHANGED </td>
			<td> \c BMimeType::SetPreferredApp() </td>
		</tr>
		<tr>
			<td> \c B_ATTR_INFO_CHANGED </td>
			<td> \c BMimeType::SetAttrInfo() </td>
		</tr>
		<tr>
			<td> \c B_FILE_EXTENSIONS_CHANGED </td>
			<td> \c BMimeType::SetFileExtensions() </td>
		</tr>
		<tr>
			<td> \c B_SHORT_DESCRIPTION_CHANGED </td>
			<td> \c BMimeType::SetShortDescription() </td>
		</tr>
		<tr>
			<td> \c B_LONG_DESCRIPTION_CHANGED </td>
			<td> \c BMimeType::SetLongDescription() </td>
		</tr>
		<tr>
			<td> \c B_ICON_FOR_TYPE_CHANGED </td>
			<td> \c BMimeType::SetIconForType() </td>
		</tr>
		<tr>
			<td> \c B_APP_HINT_CHANGED </td>
			<td> \c BMimeType::SetAppHint() </td>
		</tr>
	</table>

	\param target The \c BMessenger to subscribe to the MIME monitor service
*/
status_t
Database::StartWatching(BMessenger target)
{
	DBG(OUT("Database::StartWatching()\n"));

	if (!target.IsValid())
		return B_BAD_VALUE;

	fMonitorMessengers.insert(target);
	return B_OK;
}


/*!
	Unsubscribes the given BMessenger from the MIME monitor service
	\param target The \c BMessenger to unsubscribe
*/
status_t
Database::StopWatching(BMessenger target)
{
	DBG(OUT("Database::StopWatching()\n"));

	if (!target.IsValid())
		return B_BAD_VALUE;

	status_t status = fMonitorMessengers.find(target) != fMonitorMessengers.end()
		? (status_t)B_OK : (status_t)B_ENTRY_NOT_FOUND;
	if (status == B_OK)
		fMonitorMessengers.erase(target);

	return status;
}


/*!	\brief Deletes the app hint attribute for the given type

	A \c B_APP_HINT_CHANGED notification is sent to the mime monitor service.
	\param type The mime type of interest
	\return
	- B_OK: success
	- B_ENTRY_NOT_FOUND: no such attribute existed
	- "error code": failure
*/
status_t
Database::DeleteAppHint(const char *type)
{
	status_t status = fLocation->DeleteAttribute(type, kAppHintAttr);
	if (status == B_OK)
		_SendMonitorUpdate(B_APP_HINT_CHANGED, type, B_META_MIME_DELETED);
	else if (status == B_ENTRY_NOT_FOUND)
		status = B_OK;

	return status;
}


/*!	\brief Deletes the attribute info attribute for the given type

	A \c B_ATTR_INFO_CHANGED notification is sent to the mime monitor service.
	\param type The mime type of interest
	\return
	- B_OK: success
	- B_ENTRY_NOT_FOUND: no such attribute existed
	- "error code": failure
*/
status_t
Database::DeleteAttrInfo(const char *type)
{
	status_t status = fLocation->DeleteAttribute(type, kAttrInfoAttr);
	if (status == B_OK)
		_SendMonitorUpdate(B_ATTR_INFO_CHANGED, type, B_META_MIME_DELETED);
	else if (status == B_ENTRY_NOT_FOUND)
		status = B_OK;

	return status;
}


/*!	\brief Deletes the short description attribute for the given type

	A \c B_SHORT_DESCRIPTION_CHANGED notification is sent to the mime monitor service.
	\param type The mime type of interest
	\return
	- B_OK: success
	- B_ENTRY_NOT_FOUND: no such attribute existed
	- "error code": failure
*/
status_t
Database::DeleteShortDescription(const char *type)
{
	status_t status = fLocation->DeleteAttribute(type, kShortDescriptionAttr);
	if (status == B_OK)
		_SendMonitorUpdate(B_SHORT_DESCRIPTION_CHANGED, type, B_META_MIME_DELETED);
	else if (status == B_ENTRY_NOT_FOUND)
		status = B_OK;

	return status;
}


/*!	\brief Deletes the long description attribute for the given type

	A \c B_LONG_DESCRIPTION_CHANGED notification is sent to the mime monitor service.
	\param type The mime type of interest
	\return
	- B_OK: success
	- B_ENTRY_NOT_FOUND: no such attribute existed
	- "error code": failure
*/
status_t
Database::DeleteLongDescription(const char *type)
{
	status_t status = fLocation->DeleteAttribute(type, kLongDescriptionAttr);
	if (status == B_OK)
		_SendMonitorUpdate(B_LONG_DESCRIPTION_CHANGED, type, B_META_MIME_DELETED);
	else if (status == B_ENTRY_NOT_FOUND)
		status = B_OK;

	return status;
}


/*!	\brief Deletes the associated file extensions attribute for the given type

	A \c B_FILE_EXTENSIONS_CHANGED notification is sent to the mime monitor service.
	\param type The mime type of interest
	\return
	- B_OK: success
	- B_ENTRY_NOT_FOUND: no such attribute existed
	- "error code": failure
*/
status_t
Database::DeleteFileExtensions(const char *type)
{
	status_t status = fLocation->DeleteAttribute(type, kFileExtensionsAttr);
	if (status == B_OK)
		_SendMonitorUpdate(B_FILE_EXTENSIONS_CHANGED, type, B_META_MIME_DELETED);
	else if (status == B_ENTRY_NOT_FOUND)
		status = B_OK;

	return status;
}


/*!	\brief Deletes the icon of the given size for the given type

	A \c B_ICON_CHANGED notification is sent to the mime monitor service.
	\param type The mime type of interest
	\param which The icon size of interest
	\return
	- B_OK: success
	- B_ENTRY_NOT_FOUND: no such attribute existed
	- "error code": failure
*/
status_t
Database::DeleteIcon(const char *type, icon_size which)
{
	const char *attr = which == B_MINI_ICON ? kMiniIconAttr : kLargeIconAttr;
	status_t status = fLocation->DeleteAttribute(type, attr);
	if (status == B_OK) {
		_SendMonitorUpdate(B_ICON_CHANGED, type, which == B_LARGE_ICON,
			B_META_MIME_DELETED);
	} else if (status == B_ENTRY_NOT_FOUND)
		status = B_OK;

	return status;
}


/*!	\brief Deletes the vector icon for the given type

	A \c B_ICON_CHANGED notification is sent to the mime monitor service.
	\param type The mime type of interest
	\return
	- B_OK: success
	- B_ENTRY_NOT_FOUND: no such attribute existed
	- "error code": failure
*/
status_t
Database::DeleteIcon(const char *type)
{
	// TODO: extra notification for vector icon (for now we notify a "large"
	// icon)
	status_t status = fLocation->DeleteAttribute(type, kIconAttr);
	if (status == B_OK) {
		_SendMonitorUpdate(B_ICON_CHANGED, type, true,
						   B_META_MIME_DELETED);
	} else if (status == B_ENTRY_NOT_FOUND)
		status = B_OK;

	return status;
}


/*!	\brief Deletes the icon of the given size associated with the given file
		type for the given application signature.

    (If this function seems confusing, please see BMimeType::GetIconForType() for a
    better description of what the *IconForType() functions are used for.)

	A \c B_ICON_FOR_TYPE_CHANGED notification is sent to the mime monitor service.
	\param type The mime type of the application whose custom icon you are deleting.
	\param fileType The mime type for which you no longer wish \c type to have a custom icon.
	\param which The icon size of interest
	\return
	- B_OK: success
	- B_ENTRY_NOT_FOUND: no such attribute existed
	- "error code": failure
*/
status_t
Database::DeleteIconForType(const char *type, const char *fileType, icon_size which)
{
	if (fileType == NULL)
		return B_BAD_VALUE;

	std::string attr = (which == B_MINI_ICON
		? kMiniIconAttrPrefix : kLargeIconAttrPrefix) + BPrivate::Storage::to_lower(fileType);

	status_t status = fLocation->DeleteAttribute(type, attr.c_str());
	if (status == B_OK) {
		_SendMonitorUpdate(B_ICON_FOR_TYPE_CHANGED, type, fileType,
			which == B_LARGE_ICON, B_META_MIME_DELETED);
	} else if (status == B_ENTRY_NOT_FOUND)
		status = B_OK;

	return status;
}


/*!	\brief Deletes the vector icon associated with the given file
		type for the given application signature.

    (If this function seems confusing, please see BMimeType::GetIconForType() for a
    better description of what the *IconForType() functions are used for.)

	A \c B_ICON_FOR_TYPE_CHANGED notification is sent to the mime monitor service.
	\param type The mime type of the application whose custom icon you are deleting.
	\param fileType The mime type for which you no longer wish \c type to have a custom icon.
	\return
	- B_OK: success
	- B_ENTRY_NOT_FOUND: no such attribute existed
	- "error code": failure
*/
status_t
Database::DeleteIconForType(const char *type, const char *fileType)
{
	if (fileType == NULL)
		return B_BAD_VALUE;

	std::string attr = kIconAttrPrefix + BPrivate::Storage::to_lower(fileType);

	// TODO: introduce extra notification for vector icons?
	// (uses B_LARGE_ICON now)
	status_t status = fLocation->DeleteAttribute(type, attr.c_str());
	if (status == B_OK) {
		_SendMonitorUpdate(B_ICON_FOR_TYPE_CHANGED, type, fileType,
			true, B_META_MIME_DELETED);
	} else if (status == B_ENTRY_NOT_FOUND)
		status = B_OK;

	return status;
}


// DeletePreferredApp
//! Deletes the preferred app for the given app verb for the given type
/*! A \c B_PREFERRED_APP_CHANGED notification is sent to the mime monitor service.
	\param type The mime type of interest
	\param which The app verb of interest
	\return
	- B_OK: success
	- B_ENTRY_NOT_FOUND: no such attribute existed
	- "error code": failure
*/
status_t
Database::DeletePreferredApp(const char *type, app_verb verb)
{
	status_t status;

	switch (verb) {
		case B_OPEN:
			status = fLocation->DeleteAttribute(type, kPreferredAppAttr);
			break;

		default:
			return B_BAD_VALUE;
	}

	/*! \todo The R5 monitor makes no note of which app_verb value was updated. If
		additional app_verb values besides \c B_OPEN are someday added, the format
		of the MIME monitor messages will need to be augmented.
	*/
	if (status == B_OK)
		_SendMonitorUpdate(B_PREFERRED_APP_CHANGED, type, B_META_MIME_DELETED);
	else if (status == B_ENTRY_NOT_FOUND)
		status = B_OK;

	return status;
}

// DeleteSnifferRule
//! Deletes the sniffer rule for the given type
/*! A \c B_SNIFFER_RULE_CHANGED notification is sent to the mime monitor service,
	and the corresponding rule is removed from the internal database of sniffer
	rules.
	\param type The mime type of interest
	\return
	- B_OK: success
	- B_ENTRY_NOT_FOUND: no such attribute existed
	- "error code": failure
*/
status_t
Database::DeleteSnifferRule(const char *type)
{
	status_t status = fLocation->DeleteAttribute(type, kSnifferRuleAttr);
	if (status == B_OK) {
		status = fSnifferRules.DeleteSnifferRule(type);
		if (status == B_OK) {
			_SendMonitorUpdate(B_SNIFFER_RULE_CHANGED, type,
				B_META_MIME_DELETED);
		}
	} else if (status == B_ENTRY_NOT_FOUND)
		status = B_OK;

	return status;
}

// DeleteSupportedTypes
//! Deletes the supported types list for the given type
/*! A \c B_SUPPORTED_TYPES_CHANGED notification is sent to the mime monitor service.
	If \c fullSync is \c true, the given type is removed from the internal list
	of supporting applictions for each previously supported type. If \c fullSync
	is \c false, the said removal will occur the next time SetSupportedTypes() or
	DeleteSupportedTypes() is called with a \c true \c fullSync paramter, or
	\c Delete() is called for the given type.
	\param type The mime type of interest
	\param fullSync Whether or not to remove the type as a supporting app for
	                all previously supported types
	\return
	- B_OK: success
	- B_ENTRY_NOT_FOUND: no such attribute existed
	- "error code": failure
*/
status_t
Database::DeleteSupportedTypes(const char *type, bool fullSync)
{
	status_t status = fLocation->DeleteAttribute(type, kSupportedTypesAttr);

	// Update the supporting apps database. If fullSync is specified,
	// do so even if the supported types attribute didn't exist, as
	// stranded types *may* exist in the database due to previous
	// calls to {Set,Delete}SupportedTypes() with fullSync == false.
	bool sendUpdate = true;
	if (status == B_OK)
		status = fSupportingApps.DeleteSupportedTypes(type, fullSync);
	else if (status == B_ENTRY_NOT_FOUND) {
		status = B_OK;
		if (fullSync)
			fSupportingApps.DeleteSupportedTypes(type, fullSync);
		else
			sendUpdate = false;
	}

	// Send a monitor notification
	if (status == B_OK && sendUpdate)
		_SendMonitorUpdate(B_SUPPORTED_TYPES_CHANGED, type, B_META_MIME_DELETED);

	return status;
}


void
Database::DeferInstallNotification(const char* type)
{
	AutoLocker<BLocker> _(fDeferredInstallNotificationsLocker);

	// check, if already deferred
	if (_FindDeferredInstallNotification(type))
		return;

	// add new
	DeferredInstallNotification* notification
		= new(std::nothrow) DeferredInstallNotification;
	if (notification == NULL)
		return;

	strlcpy(notification->type, type, sizeof(notification->type));
	notification->notify = false;

	if (!fDeferredInstallNotifications.AddItem(notification))
		delete notification;
}


void
Database::UndeferInstallNotification(const char* type)
{
	AutoLocker<BLocker> locker(fDeferredInstallNotificationsLocker);

	// check, if deferred at all
	DeferredInstallNotification* notification
		= _FindDeferredInstallNotification(type, true);

	locker.Unlock();

	if (notification == NULL)
		return;

	// notify, if requested
	if (notification->notify)
		_SendInstallNotification(notification->type);

	delete notification;
}


//! \brief Sends a \c B_MIME_TYPE_CREATED notification to the mime monitor service
status_t
Database::_SendInstallNotification(const char *type)
{
	return _SendMonitorUpdate(B_MIME_TYPE_CREATED, type, B_META_MIME_MODIFIED);
}


//! \brief Sends a \c B_MIME_TYPE_DELETED notification to the mime monitor service
status_t
Database::_SendDeleteNotification(const char *type)
{
	// Tell the backend first
	return _SendMonitorUpdate(B_MIME_TYPE_DELETED, type, B_META_MIME_MODIFIED);
}

// _SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have
	subscribed to the MIME Monitor service
	\param type The MIME type that was updated
	\param which Bitmask describing which attribute was updated
	\param extraType The MIME type to which the change is applies
	\param largeIcon \true if the the large icon was updated, \false if the
		   small icon was updated
*/
status_t
Database::_SendMonitorUpdate(int32 which, const char *type, const char *extraType,
	bool largeIcon, int32 action)
{
	BMessage msg(B_META_MIME_CHANGED);
	status_t err;

	if (_CheckDeferredInstallNotification(which, type))
		return B_OK;

	err = msg.AddInt32("be:which", which);
	if (!err)
		err = msg.AddString("be:type", type);
	if (!err)
		err = msg.AddString("be:extra_type", extraType);
	if (!err)
		err = msg.AddBool("be:large_icon", largeIcon);
	if (!err)
		err = msg.AddInt32("be:action", action);
	if (!err)
		err = _SendMonitorUpdate(msg);
	return err;
}

// _SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have
	subscribed to the MIME Monitor service
	\param type The MIME type that was updated
	\param which Bitmask describing which attribute was updated
	\param extraType The MIME type to which the change is applies
*/
status_t
Database::_SendMonitorUpdate(int32 which, const char *type, const char *extraType,
	int32 action)
{
	if (_CheckDeferredInstallNotification(which, type))
		return B_OK;

	BMessage msg(B_META_MIME_CHANGED);

	status_t err = msg.AddInt32("be:which", which);
	if (!err)
		err = msg.AddString("be:type", type);
	if (!err)
		err = msg.AddString("be:extra_type", extraType);
	if (!err)
		err = msg.AddInt32("be:action", action);
	if (!err)
		err = _SendMonitorUpdate(msg);
	return err;
}

// _SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have
	subscribed to the MIME Monitor service
	\param type The MIME type that was updated
	\param which Bitmask describing which attribute was updated
	\param largeIcon \true if the the large icon was updated, \false if the
		   small icon was updated
*/
status_t
Database::_SendMonitorUpdate(int32 which, const char *type, bool largeIcon, int32 action)
{
	if (_CheckDeferredInstallNotification(which, type))
		return B_OK;

	BMessage msg(B_META_MIME_CHANGED);

	status_t err = msg.AddInt32("be:which", which);
	if (!err)
		err = msg.AddString("be:type", type);
	if (!err)
		err = msg.AddBool("be:large_icon", largeIcon);
	if (!err)
		err = msg.AddInt32("be:action", action);
	if (!err)
		err = _SendMonitorUpdate(msg);
	return err;
}

// _SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have
	subscribed to the MIME Monitor service
	\param type The MIME type that was updated
	\param which Bitmask describing which attribute was updated
*/
status_t
Database::_SendMonitorUpdate(int32 which, const char *type, int32 action)
{
	if (_CheckDeferredInstallNotification(which, type))
		return B_OK;

	BMessage msg(B_META_MIME_CHANGED);

	status_t err = msg.AddInt32("be:which", which);
	if (!err)
		err = msg.AddString("be:type", type);
	if (!err)
		err = msg.AddInt32("be:action", action);
	if (!err)
		err = _SendMonitorUpdate(msg);
	return err;
}

// _SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have subscribed to
	the MIME Monitor service
	\param BMessage A preformatted MIME monitor message to be sent to all subscribers
*/
status_t
Database::_SendMonitorUpdate(BMessage &msg)
{
	if (fNotificationListener == NULL)
		return B_OK;

	status_t err;
	std::set<BMessenger>::const_iterator i;
	for (i = fMonitorMessengers.begin(); i != fMonitorMessengers.end(); i++) {
		status_t err = fNotificationListener->Notify(&msg, *i);
		if (err) {
			DBG(OUT("Database::_SendMonitorUpdate(BMessage&): DeliverMessage failed, 0x%lx\n", err));
		}
	}
	err = B_OK;
	return err;
}


Database::DeferredInstallNotification*
Database::_FindDeferredInstallNotification(const char* type, bool remove)
{
	for (int32 i = 0;
		DeferredInstallNotification* notification
			= (DeferredInstallNotification*)fDeferredInstallNotifications
				.ItemAt(i); i++) {
		if (strcmp(type, notification->type) == 0) {
			if (remove)
				fDeferredInstallNotifications.RemoveItem(i);
			return notification;
		}
	}

	return NULL;
}


bool
Database::_CheckDeferredInstallNotification(int32 which, const char* type)
{
	AutoLocker<BLocker> locker(fDeferredInstallNotificationsLocker);

	// check, if deferred at all
	DeferredInstallNotification* notification
		= _FindDeferredInstallNotification(type);
	if (notification == NULL)
		return false;

	if (which == B_MIME_TYPE_DELETED) {
		// MIME type deleted -- if the install notification had been
		// deferred, we don't send anything
		if (notification->notify) {
			fDeferredInstallNotifications.RemoveItem(notification);
			delete notification;
			return true;
		}
	} else if (which == B_MIME_TYPE_CREATED) {
		// MIME type created -- defer notification
		notification->notify = true;
		return true;
	} else {
		// MIME type update -- don't send update, if deferred
		if (notification->notify)
			return true;
	}

	return false;
}


} // namespace Mime
} // namespace Storage
} // namespace BPrivate
