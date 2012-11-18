/*
 * Copyright 2002-2007, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@users.sf.net
 */


#include <new>
#include <set>
#include <stdlib.h>
#include <string>

#include <AppFileInfo.h>
#include <Bitmap.h>
#include <File.h>
#include <fs_attr.h>
#include <IconUtils.h>
#include <MimeType.h>
#include <RegistrarDefs.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>

using namespace std;

// attributes
static const char* kTypeAttribute				= "BEOS:TYPE";
static const char* kSignatureAttribute			= "BEOS:APP_SIG";
static const char* kAppFlagsAttribute			= "BEOS:APP_FLAGS";
static const char* kSupportedTypesAttribute		= "BEOS:FILE_TYPES";
static const char* kVersionInfoAttribute		= "BEOS:APP_VERSION";
static const char* kMiniIconAttribute			= "BEOS:M:";
static const char* kLargeIconAttribute			= "BEOS:L:";
static const char* kIconAttribute				= "BEOS:";
static const char* kStandardIconType			= "STD_ICON";
static const char* kIconType					= "ICON";
static const char* kCatalogEntryAttribute		= "SYS:NAME";

// resource IDs
static const int32 kTypeResourceID				= 2;
static const int32 kSignatureResourceID			= 1;
static const int32 kAppFlagsResourceID			= 1;
static const int32 kSupportedTypesResourceID	= 1;
static const int32 kMiniIconResourceID			= 101;
static const int32 kLargeIconResourceID			= 101;
static const int32 kIconResourceID				= 101;
static const int32 kVersionInfoResourceID		= 1;
static const int32 kMiniIconForTypeResourceID	= 0;
static const int32 kLargeIconForTypeResourceID	= 0;
static const int32 kIconForTypeResourceID		= 0;
static const int32 kCatalogEntryResourceID		= 1;

// type codes
enum {
	B_APP_FLAGS_TYPE	= 'APPF',
	B_VERSION_INFO_TYPE	= 'APPV',
};

// R5 also exports these (Tracker is using them):
// (maybe we better want to drop them silently and declare 
// the above in a public Haiku header - and use that one in
// Tracker when compiled for Haiku)
extern const uint32 MINI_ICON_TYPE, LARGE_ICON_TYPE;
const uint32 MINI_ICON_TYPE = 'MICN';
const uint32 LARGE_ICON_TYPE = 'ICON';

// debugging
//#define DBG(x) x
#define DBG(x)
#define OUT	printf

// constructor
/*!	\brief Creates an uninitialized BAppFileInfo object.
*/
BAppFileInfo::BAppFileInfo()
	:
	fResources(NULL),
	fWhere(B_USE_BOTH_LOCATIONS)
{
}


// constructor
/*!	\brief Creates an BAppFileInfo object and initializes it to the supplied
		   file.

	The caller retains ownership of the supplied BFile object. It must not
	be deleted during the life time of the BAppFileInfo. It is not deleted
	when the BAppFileInfo is destroyed.

	\param file The file the object shall be initialized to.
*/
BAppFileInfo::BAppFileInfo(BFile* file)
	:
	fResources(NULL),
	fWhere(B_USE_BOTH_LOCATIONS)
{
	SetTo(file);
}


// destructor
/*!	\brief Frees all resources associated with this object.

	The BFile the object is set to is not deleted.
*/
BAppFileInfo::~BAppFileInfo()
{
	delete fResources;
}


// SetTo
/*!	\brief Initializes the BAppFileInfo to the supplied file.

	The caller retains ownership of the supplied BFile object. It must not
	be deleted during the life time of the BAppFileInfo. It is not deleted
	when the BAppFileInfo is destroyed.

	\param file The file the object shall be initialized to.

	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a file or \a file is not properly initialized.
*/
status_t
BAppFileInfo::SetTo(BFile *file)
{
	// unset the old file
	BNodeInfo::SetTo(NULL);
	if (fResources) {
		delete fResources;
		fResources = NULL;
	}

	// check param
	status_t error = (file && file->InitCheck() == B_OK ? B_OK : B_BAD_VALUE);

	info_location where = B_USE_BOTH_LOCATIONS;

	// create resources
	if (error == B_OK) {
		fResources = new(nothrow) BResources();
		if (fResources) {
			error = fResources->SetTo(file);
			if (error != B_OK) {
				// no resources - this is no critical error, we'll just use
				// attributes only, then
				where = B_USE_ATTRIBUTES;
				error = B_OK;
			}
		} else
			error = B_NO_MEMORY;
	}

	// set node info
	if (error == B_OK)
		error = BNodeInfo::SetTo(file);

	if (error != B_OK || (where & B_USE_RESOURCES) == 0) {
		delete fResources;
		fResources = NULL;
	}

	// clean up on error
	if (error != B_OK) {
		if (InitCheck() == B_OK)
			BNodeInfo::SetTo(NULL);
	}

	// set data location
	if (error == B_OK)
		SetInfoLocation(where);

	// set error
	fCStatus = error;
	return error;
}


// GetType
/*!	\brief Gets the file's MIME type.

	\param type A pointer to a pre-allocated character buffer of size
		   \c B_MIME_TYPE_LENGTH or larger into which the MIME type of the
		   file shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a type or the type string stored in the
	  attribute/resources is longer than \c B_MIME_TYPE_LENGTH.
	- \c B_BAD_TYPE: The attribute/resources the type string is stored in have
	  the wrong type.
	- \c B_ENTRY_NOT_FOUND: No type is set on the file.
	- other error codes
*/
status_t
BAppFileInfo::GetType(char *type) const
{
	// check param and initialization
	status_t error = (type ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// read the data
	size_t read = 0;
	if (error == B_OK) {
		error = _ReadData(kTypeAttribute, kTypeResourceID, B_MIME_STRING_TYPE,
						  type, B_MIME_TYPE_LENGTH, read);
	}
	// check the read data -- null terminate the string
	if (error == B_OK && type[read - 1] != '\0') {
		if (read == B_MIME_TYPE_LENGTH)
			error = B_ERROR;
		else
			type[read] = '\0';
	}
	return error;
}


// SetType
/*!	\brief Sets the file's MIME type.

	If \a type is \c NULL the file's MIME type is unset.

	\param type The MIME type to be assigned to the file. Must not be longer
		   than \c B_MIME_TYPE_LENGTH (including the terminating null).
		   May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \a type is longer than \c B_MIME_TYPE_LENGTH.
	- other error codes
*/
status_t
BAppFileInfo::SetType(const char* type)
{
	// check initialization
	status_t error = B_OK;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	if (error == B_OK) {
		if (type) {
			// check param
			size_t typeLen = strlen(type);
			if (error == B_OK && typeLen >= B_MIME_TYPE_LENGTH)
				error = B_BAD_VALUE;
			// write the data
			if (error == B_OK) {
				error = _WriteData(kTypeAttribute, kTypeResourceID,
								   B_MIME_STRING_TYPE, type, typeLen + 1);
			}
		} else
			error = _RemoveData(kTypeAttribute, B_MIME_STRING_TYPE);
	}
	return error;
}


// GetSignature
/*!	\brief Gets the file's application signature.

	\param signature A pointer to a pre-allocated character buffer of size
		   \c B_MIME_TYPE_LENGTH or larger into which the application
		   signature of the file shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a signature or the signature stored in the
	  attribute/resources is longer than \c B_MIME_TYPE_LENGTH.
	- \c B_BAD_TYPE: The attribute/resources the signature is stored in have
	  the wrong type.
	- \c B_ENTRY_NOT_FOUND: No signature is set on the file.
	- other error codes
*/
status_t
BAppFileInfo::GetSignature(char* signature) const
{
	// check param and initialization
	status_t error = (signature ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// read the data
	size_t read = 0;
	if (error == B_OK) {
		error = _ReadData(kSignatureAttribute, kSignatureResourceID,
						  B_MIME_STRING_TYPE, signature,
						  B_MIME_TYPE_LENGTH, read);
	}
	// check the read data -- null terminate the string
	if (error == B_OK && signature[read - 1] != '\0') {
		if (read == B_MIME_TYPE_LENGTH)
			error = B_ERROR;
		else
			signature[read] = '\0';
	}
	return error;
}


// SetSignature
/*!	\brief Sets the file's application signature.

	If \a signature is \c NULL the file's application signature is unset.

	\param signature The application signature to be assigned to the file.
		   Must not be longer than \c B_MIME_TYPE_LENGTH (including the
		   terminating null). May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \a signature is longer than \c B_MIME_TYPE_LENGTH.
	- other error codes
*/
status_t
BAppFileInfo::SetSignature(const char* signature)
{
	// check initialization
	status_t error = B_OK;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	if (error == B_OK) {
		if (signature) {
			// check param
			size_t signatureLen = strlen(signature);
			if (error == B_OK && signatureLen >= B_MIME_TYPE_LENGTH)
				error = B_BAD_VALUE;
			// write the data
			if (error == B_OK) {
				error = _WriteData(kSignatureAttribute, kSignatureResourceID,
								   B_MIME_STRING_TYPE, signature,
								   signatureLen + 1);
			}
		} else
			error = _RemoveData(kSignatureAttribute, B_MIME_STRING_TYPE);
	}
	return error;
}


// GetCatalogEntry
/*!	\brief Gets the file's catalog entry. (localization)

	\param catalogEntry A pointer to a pre-allocated character buffer of size
		   \c B_MIME_TYPE_LENGTH * 3 or larger into which the catalog entry
		   of the file shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a catalogEntry or the entry stored in the
	  attribute/resources is longer than \c B_MIME_TYPE_LENGTH * 3.
	- \c B_BAD_TYPE: The attribute/resources the entry is stored in have
	  the wrong type.
	- \c B_ENTRY_NOT_FOUND: No catalog entry is set on the file.
	- other error codes
*/
status_t
BAppFileInfo::GetCatalogEntry(char *catalogEntry) const
{
	if (catalogEntry == NULL)
		return B_BAD_VALUE;

	if (InitCheck() != B_OK)
		return B_NO_INIT;

	size_t read = 0;
	status_t error = _ReadData(kCatalogEntryAttribute, kCatalogEntryResourceID,
		B_STRING_TYPE, catalogEntry, B_MIME_TYPE_LENGTH * 3, read);

	if (error != B_OK)
		return error;

	if (read >= B_MIME_TYPE_LENGTH * 3)
		return B_ERROR;

	catalogEntry[read] = '\0';

	return B_OK;
}


// SetCatalogEntry
/*!	\brief Sets the file's catalog entry. (localization)

	If \a catalogEntry is \c NULL the file's catalog entry is unset.

	\param catalogEntry The catalog entry to be assigned to the file.
		Of the form "x-vnd.Haiku-app:context:name".
		Must not be longer than \c B_MIME_TYPE_LENGTH * 3
		(including the terminating null). May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \a catalogEntry is longer than \c B_MIME_TYPE_LENGTH * 3.
	- other error codes
*/
status_t
BAppFileInfo::SetCatalogEntry(const char* catalogEntry)
{
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	if (catalogEntry == NULL)
		return _RemoveData(kCatalogEntryAttribute, B_STRING_TYPE);

	size_t nameLength = strlen(catalogEntry);
	if (nameLength > B_MIME_TYPE_LENGTH * 3)
		return B_BAD_VALUE;

	return _WriteData(kCatalogEntryAttribute, kCatalogEntryResourceID,
		B_STRING_TYPE, catalogEntry, nameLength + 1);
}


// GetAppFlags
/*!	\brief Gets the file's application flags.

	\param flags A pointer to a pre-allocated uint32 into which the application
		   flags of the file shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a flags.
	- \c B_BAD_TYPE: The attribute/resources the flags are stored in have
	  the wrong type.
	- \c B_ENTRY_NOT_FOUND: No application flags are set on the file.
	- other error codes
*/
status_t
BAppFileInfo::GetAppFlags(uint32* flags) const
{
	// check param and initialization
	status_t error = (flags ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// read the data
	size_t read = 0;
	if (error == B_OK) {
		error = _ReadData(kAppFlagsAttribute, kAppFlagsResourceID,
						  B_APP_FLAGS_TYPE, flags, sizeof(uint32),
						  read);
	}
	// check the read data
	if (error == B_OK && read != sizeof(uint32))
		error = B_ERROR;
	return error;
}


// SetAppFlags
/*!	\brief Sets the file's application flags.
	\param flags The application flags to be assigned to the file.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- other error codes
*/
status_t
BAppFileInfo::SetAppFlags(uint32 flags)
{
	// check initialization
	status_t error = B_OK;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	if (error == B_OK) {
		// write the data
		error = _WriteData(kAppFlagsAttribute, kAppFlagsResourceID,
						   B_APP_FLAGS_TYPE, &flags, sizeof(uint32));
	}
	return error;
}


// RemoveAppFlags
/*!	\brief Removes the file's application flags.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- other error codes
*/
status_t
BAppFileInfo::RemoveAppFlags()
{
	// check initialization
	status_t error = B_OK;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	if (error == B_OK) {
		// remove the data
		error = _RemoveData(kAppFlagsAttribute, B_APP_FLAGS_TYPE);
	}
	return error;
}


// GetSupportedTypes
/*!	\brief Gets the MIME types supported by the application.

	The supported MIME types are added to a field "types" of type
	\c B_STRING_TYPE in \a types.

	\param types A pointer to a pre-allocated BMessage into which the
		   MIME types supported by the appplication shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a types.
	- \c B_BAD_TYPE: The attribute/resources the supported types are stored in
	  have the wrong type.
	- \c B_ENTRY_NOT_FOUND: No supported types are set on the file.
	- other error codes
*/
status_t
BAppFileInfo::GetSupportedTypes(BMessage* types) const
{
	// check param and initialization
	status_t error = (types ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// read the data
	size_t read = 0;
	void *buffer = NULL;
	if (error == B_OK) {
		error = _ReadData(kSupportedTypesAttribute, kSupportedTypesResourceID,
						  B_MESSAGE_TYPE, NULL, 0, read, &buffer);
	}
	// unflatten the buffer
	if (error == B_OK)
		error = types->Unflatten((const char*)buffer);
	// clean up
	free(buffer);
	return error;
}


// SetSupportedTypes
/*!	\brief Sets the MIME types supported by the application.

	If \a types is \c NULL the application's supported types are unset.

	The supported MIME types must be stored in a field "types" of type
	\c B_STRING_TYPE in \a types.

	The method informs the registrar about this news.
	For each supported type the result of BMimeType::GetSupportingApps() will
	afterwards include the signature of this application. That is, the
	application file needs to have a signature set.

	\a syncAll specifies whether the not longer supported types shall be
	updated as well, i.e. whether this application shall be remove from the
	lists of supporting applications.

	\param types The supported types to be assigned to the file.
		   May be \c NULL.
	\param syncAll \c true to also synchronize the not longer supported
		   types, \c false otherwise.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- other error codes
*/
status_t
BAppFileInfo::SetSupportedTypes(const BMessage* types, bool syncAll)
{
	// check initialization
	status_t error = B_OK;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	BMimeType mimeType;
	if (error == B_OK)
		error = GetMetaMime(&mimeType);
	if (error == B_OK || error == B_ENTRY_NOT_FOUND) {
		error = B_OK;
		if (types) {
			// check param -- supported types must be valid
			const char* type;
			for (int32 i = 0;
				 error == B_OK && types->FindString("types", i, &type) == B_OK;
				 i++) {
				if (!BMimeType::IsValid(type))
					error = B_BAD_VALUE;
			}
			// get flattened size
			ssize_t size = 0;
			if (error == B_OK) {
				size = types->FlattenedSize();
				if (size < 0)
					error = size;
			}
			// allocate a buffer for the flattened data
			char* buffer = NULL;
			if (error == B_OK) {
				buffer = new(nothrow) char[size];
				if (!buffer)
					error = B_NO_MEMORY;
			}
			// flatten the message
			if (error == B_OK)
				error = types->Flatten(buffer, size);
			// write the data
			if (error == B_OK) {
				error = _WriteData(kSupportedTypesAttribute,
								   kSupportedTypesResourceID, B_MESSAGE_TYPE,
								   buffer, size);
			}
			// clean up
			delete[] buffer;
		} else
			error = _RemoveData(kSupportedTypesAttribute, B_MESSAGE_TYPE);
		// update the MIME database, if the app signature is installed
		if (error == B_OK && mimeType.IsInstalled())
			error = mimeType.SetSupportedTypes(types, syncAll);
	}
	return error;
}


// SetSupportedTypes
/*!	\brief Sets the MIME types supported by the application.

	This method is a short-hand for SetSupportedTypes(types, false).
	\see SetSupportedType(const BMessage*, bool) for detailed information.

	\param types The supported types to be assigned to the file.
		   May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- other error codes
*/
status_t
BAppFileInfo::SetSupportedTypes(const BMessage* types)
{
	return SetSupportedTypes(types, false);
}


// IsSupportedType
/*!	\brief Returns whether the application supports the supplied MIME type.

	If the application supports the wildcard type "application/octet-stream"
	any this method returns \c true for any MIME type.

	\param type The MIME type in question.
	\return \c true, if \a type is a valid MIME type and it is supported by
			the application, \c false otherwise.
*/
bool
BAppFileInfo::IsSupportedType(const char* type) const
{
	status_t error = (type ? B_OK : B_BAD_VALUE);
	// get the supported types
	BMessage types;
	if (error == B_OK)
		error = GetSupportedTypes(&types);
	// turn type into a BMimeType
	BMimeType mimeType;
	if (error == B_OK)
		error = mimeType.SetTo(type);
	// iterate through the supported types
	bool found = false;
	if (error == B_OK) {
		const char* supportedType;
		for (int32 i = 0;
			 !found && types.FindString("types", i, &supportedType) == B_OK;
			 i++) {
			found = !strcmp(supportedType, "application/octet-stream")
					|| BMimeType(supportedType).Contains(&mimeType);
		}
	}
	return found;
}


// Supports
/*!	\brief Returns whether the application supports the supplied MIME type
		   explicitly.

	Unlike IsSupportedType(), this method returns \c true, only if the type
	is explicitly supported, regardless of whether it supports
	"application/octet-stream".

	\param type The MIME type in question.
	\return \c true, if \a type is a valid MIME type and it is explicitly
			supported by the application, \c false otherwise.
*/
bool
BAppFileInfo::Supports(BMimeType* type) const
{
	status_t error = (type && type->InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	// get the supported types
	BMessage types;
	if (error == B_OK)
		error = GetSupportedTypes(&types);
	// iterate through the supported types
	bool found = false;
	if (error == B_OK) {
		const char* supportedType;
		for (int32 i = 0;
			 !found && types.FindString("types", i, &supportedType) == B_OK;
			 i++) {
			found = BMimeType(supportedType).Contains(type);
		}
	}
	return found;
}


// GetIcon
/*!	\brief Gets the file's icon.
	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param which Specifies the size of the icon to be retrieved:
		   \c B_MINI_ICON for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a icon, unsupported icon size \a which or bitmap
		 dimensions (\a icon) and icon size (\a which) do not match.
	- other error codes
*/
status_t
BAppFileInfo::GetIcon(BBitmap* icon, icon_size which) const
{
	return GetIconForType(NULL, icon, which);
}


// GetIcon
/*!	\brief Gets the file's icon.
	\param data The pointer in which the flat icon data will be returned.
	\param size The pointer in which the size of the data found will be returned.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a data or \c NULL size.
	- other error codes
*/
status_t
BAppFileInfo::GetIcon(uint8** data, size_t* size) const
{
	return GetIconForType(NULL, data, size);
}


// SetIcon
/*!	\brief Sets the file's icon.

	If \a icon is \c NULL the file's icon is unset.

	\param icon A pointer to the BBitmap containing the icon to be set.
		   May be \c NULL.
	\param which Specifies the size of the icon to be set: \c B_MINI_ICON
		   for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: Unknown icon size \a which or bitmap dimensions (\a icon)
		 and icon size (\a which) do not match.
	- other error codes
*/
status_t
BAppFileInfo::SetIcon(const BBitmap* icon, icon_size which)
{
	return SetIconForType(NULL, icon, which);
}


// SetIcon
/*!	\brief Sets the file's icon.

	If \a icon is \c NULL the file's icon is unset.

	\param data A pointer to the data buffer containing the vector icon
		   to be set. May be \c NULL.
	\param size Specifies the size of buffer pointed to by \a data.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL data.
	- other error codes
*/
status_t
BAppFileInfo::SetIcon(const uint8* data, size_t size)
{
	return SetIconForType(NULL, data, size);
}


// GetVersionInfo
/*!	\brief Gets the file's version info.
	\param info A pointer to a pre-allocated version_info structure into which
		   the version info should be written.
	\param kind Specifies the kind of the version info to be retrieved:
		   \c B_APP_VERSION_KIND for the application's version info and
		   \c B_SYSTEM_VERSION_KIND for the suite's info the application
		   belongs to.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a info.
	- other error codes
*/
status_t
BAppFileInfo::GetVersionInfo(version_info* info, version_kind kind) const
{
	// check params and initialization
	if (!info)
		return B_BAD_VALUE;

	int32 index = 0;
	switch (kind) {
		case B_APP_VERSION_KIND:
			index = 0;
			break;
		case B_SYSTEM_VERSION_KIND:
			index = 1;
			break;
		default:
			return B_BAD_VALUE;
	}

	if (InitCheck() != B_OK)
		return B_NO_INIT;

	// read the data
	size_t read = 0;
	version_info infos[2];
	status_t error = _ReadData(kVersionInfoAttribute, kVersionInfoResourceID,
		B_VERSION_INFO_TYPE, infos, 2 * sizeof(version_info), read);
	if (error != B_OK)
		return error;

	// check the read data
	if (read == sizeof(version_info)) {
		// only the app version info is there -- return a cleared system info
		if (index == 0)
			*info = infos[index];
		else if (index == 1)
			memset(info, 0, sizeof(version_info));
	} else if (read == 2 * sizeof(version_info)) {
		*info = infos[index];
	} else
		return B_ERROR;

	// return result	
	return B_OK;
}


// SetVersionInfo
/*!	\brief Sets the file's version info.

	If \a info is \c NULL the file's version info is unset.

	\param info The version info to be set. May be \c NULL.
	\param kind Specifies kind of version info to be set:
		   \c B_APP_VERSION_KIND for the application's version info and
		   \c B_SYSTEM_VERSION_KIND for the suite's info the application
		   belongs to.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- other error codes
*/
status_t
BAppFileInfo::SetVersionInfo(const version_info* info, version_kind kind)
{
	// check initialization
	status_t error = B_OK;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	if (error == B_OK) {
		if (info) {
			// check param
			int32 index = 0;
			if (error == B_OK) {
				switch (kind) {
					case B_APP_VERSION_KIND:
						index = 0;
						break;
					case B_SYSTEM_VERSION_KIND:
						index = 1;
						break;
					default:
						error = B_BAD_VALUE;
						break;
				}
			}
			// read both infos
			version_info infos[2];
			if (error == B_OK) {
				size_t read;
				if (_ReadData(kVersionInfoAttribute, kVersionInfoResourceID,
						B_VERSION_INFO_TYPE, infos, 2 * sizeof(version_info),
						read) == B_OK) {
					// clear the part that hasn't been read
					if (read < sizeof(infos))
						memset((char*)infos + read, 0, sizeof(infos) - read);
				} else {
					// failed to read -- clear
					memset(infos, 0, sizeof(infos));
				}
			}
			infos[index] = *info;
			// write the data
			if (error == B_OK) {
				error = _WriteData(kVersionInfoAttribute,
								   kVersionInfoResourceID,
								   B_VERSION_INFO_TYPE, infos,
								   2 * sizeof(version_info));
			}
		} else
			error = _RemoveData(kVersionInfoAttribute, B_VERSION_INFO_TYPE);
	}
	return error;
}


// GetIconForType
/*!	\brief Gets the icon the application provides for a given MIME type.

	If \a type is \c NULL, the application's icon is retrieved.

	\param type The MIME type in question. May be \c NULL.
	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param which Specifies the size of the icon to be retrieved:
		   \c B_MINI_ICON for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a icon, unsupported icon size
		 \a which or bitmap dimensions (\a icon) and icon size (\a which) do
		 not match.
	- other error codes
*/
status_t
BAppFileInfo::GetIconForType(const char* type, BBitmap* icon,
							 icon_size size) const
{
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	if (!icon || icon->InitCheck() != B_OK)
		return B_BAD_VALUE;

	// TODO: for consistency with attribute based icon reading, we
	// could also prefer B_CMAP8 icons here if the provided bitmap
	// is in that format. Right now, an existing B_CMAP8 icon resource
	// would be ignored as soon as a vector icon is present. On the other
	// hand, maybe this still results in a more consistent user interface,
	// since Tracker/Deskbar would surely show the vector icon.

	// try vector icon first
	BString vectorAttributeName(kIconAttribute);

	// check type param
	if (type) {
		if (BMimeType::IsValid(type))
			vectorAttributeName += type;
		else
			return B_BAD_VALUE;
	} else {
		vectorAttributeName += kIconType;
	}
	const char* attribute = vectorAttributeName.String();

	size_t bytesRead;
	void* allocatedBuffer;
	status_t error = _ReadData(attribute, -1, B_VECTOR_ICON_TYPE, NULL, 0,
							   bytesRead, &allocatedBuffer);
	if (error == B_OK) {
		error = BIconUtils::GetVectorIcon((uint8*)allocatedBuffer,
										  bytesRead, icon);
		free(allocatedBuffer);
		return error;
	}

	// no vector icon if we got this far,
	// align size argument just in case
	if (size < B_LARGE_ICON)
		size = B_MINI_ICON;
	else
		size = B_LARGE_ICON;

	error = B_OK;
	// set some icon size related variables
	BString attributeString;
	BRect bounds;
	uint32 attrType = 0;
	size_t attrSize = 0;
	switch (size) {
		case B_MINI_ICON:
			attributeString = kMiniIconAttribute;
			bounds.Set(0, 0, 15, 15);
			attrType = B_MINI_ICON_TYPE;
			attrSize = 16 * 16;
			break;
		case B_LARGE_ICON:
			attributeString = kLargeIconAttribute;
			bounds.Set(0, 0, 31, 31);
			attrType = B_LARGE_ICON_TYPE;
			attrSize = 32 * 32;
			break;
		default:
			return B_BAD_VALUE;
	}
	// check type param
	if (type) {
		if (BMimeType::IsValid(type))
			attributeString += type;
		else
			return B_BAD_VALUE;
	} else
		attributeString += kStandardIconType;

	attribute = attributeString.String();

	// check parameters
	// currently, scaling B_CMAP8 icons is not supported
	if (icon->ColorSpace() == B_CMAP8 && icon->Bounds() != bounds)
		return B_BAD_VALUE;

	// read the data
	if (error == B_OK) {
		bool tempBuffer = (icon->ColorSpace() != B_CMAP8
						   || icon->Bounds() != bounds);
		uint8* buffer = NULL;
		size_t read;
		if (tempBuffer) {
			// other color space or bitmap size than stored in attribute
			buffer = new(nothrow) uint8[attrSize];
			if (!buffer) {
				error = B_NO_MEMORY;
			} else {
				error = _ReadData(attribute, -1, attrType, buffer, attrSize,
								  read);
			}
		} else {
			error = _ReadData(attribute, -1, attrType, icon->Bits(), attrSize,
							  read);
		}
		if (error == B_OK && read != attrSize)
			error = B_ERROR;
		if (tempBuffer) {
			// other color space than stored in attribute
			if (error == B_OK) {
				error = BIconUtils::ConvertFromCMAP8(buffer,
													 (uint32)size,
													 (uint32)size,
													 (uint32)size,
													 icon);
			}
			delete[] buffer;
		}
	}
	return error;
}


// GetIconForType
/*!	\brief Gets the icon the application provides for a given MIME type.

	If \a type is \c NULL, the application's icon is retrieved.

	\param type The MIME type in question. May be \c NULL.
	\param data A pointer in which the icon data will be returned. When you
	are done with the data, you should use free() to deallocate it.
	\param size A pointer in which the size of the retrieved data is returned.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a data and/or \a size. Or the supplied
	\a type is not a valid MIME type.
	- other error codes
*/
status_t
BAppFileInfo::GetIconForType(const char* type, uint8** data,
							 size_t* size) const
{
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	if (!data || !size)
		return B_BAD_VALUE;

	// get vector icon
	BString attributeName(kIconAttribute);

	// check type param
	if (type) {
		if (BMimeType::IsValid(type))
			attributeName += type;
		else
			return B_BAD_VALUE;
	} else {
		attributeName += kIconType;
	}

	void* allocatedBuffer = NULL;
	status_t ret = _ReadData(attributeName.String(), -1,
							 B_VECTOR_ICON_TYPE, NULL, 0, *size, &allocatedBuffer);

	if (ret < B_OK)
		return ret;

	*data = (uint8*)allocatedBuffer;
	return B_OK;
}


// SetIconForType
/*!	\brief Sets the icon the application provides for a given MIME type.

	If \a type is \c NULL, the application's icon is set.
	If \a icon is \c NULL the icon is unset.

	If the file has a signature, then the icon is also set on the MIME type.
	If the type for the signature has not been installed yet, it is installed
	before.

	\param type The MIME type in question. May be \c NULL.
	\param icon A pointer to the BBitmap containing the icon to be set.
		   May be \c NULL.
	\param which Specifies the size of the icon to be set: \c B_MINI_ICON
		   for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: Either the icon size \a which is unkown, bitmap dimensions (\a icon)
		 and icon size (\a which) do not match, or the provided \a type is
		 not a valid MIME type. 
	- other error codes
*/
status_t
BAppFileInfo::SetIconForType(const char* type, const BBitmap* icon,
							 icon_size which)
{
	status_t error = B_OK;
	// set some icon size related variables
	BString attributeString;
	BRect bounds;
	uint32 attrType = 0;
	size_t attrSize = 0;
	int32 resourceID = 0;
	switch (which) {
		case B_MINI_ICON:
			attributeString = kMiniIconAttribute;
			bounds.Set(0, 0, 15, 15);
			attrType = B_MINI_ICON_TYPE;
			attrSize = 16 * 16;
			resourceID = (type ? kMiniIconForTypeResourceID
							   : kMiniIconResourceID);
			break;
		case B_LARGE_ICON:
			attributeString = kLargeIconAttribute;
			bounds.Set(0, 0, 31, 31);
			attrType = B_LARGE_ICON_TYPE;
			attrSize = 32 * 32;
			resourceID = (type ? kLargeIconForTypeResourceID
							   : kLargeIconResourceID);
			break;
		default:
			error = B_BAD_VALUE;
			break;
	}
	// check type param
	if (error == B_OK) {
		if (type) {
			if (BMimeType::IsValid(type))
				attributeString += type;
			else
				error = B_BAD_VALUE;
		} else
			attributeString += kStandardIconType;
	}
	const char* attribute = attributeString.String();
	// check parameter and initialization
	if (error == B_OK && icon
		&& (icon->InitCheck() != B_OK || icon->Bounds() != bounds)) {
		error = B_BAD_VALUE;
	}
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// write/remove the attribute
	if (error == B_OK) {
		if (icon) {
			bool otherColorSpace = (icon->ColorSpace() != B_CMAP8);
			if (otherColorSpace) {
				BBitmap bitmap(bounds, B_BITMAP_NO_SERVER_LINK, B_CMAP8);
				error = bitmap.InitCheck();
				if (error == B_OK)
					error = bitmap.ImportBits(icon);
				if (error == B_OK) {
					error = _WriteData(attribute, resourceID, attrType,
									   bitmap.Bits(), attrSize, true);
				}
			} else {
				error = _WriteData(attribute, resourceID, attrType,
								   icon->Bits(), attrSize, true);
			}
		} else	// no icon given => remove
			error = _RemoveData(attribute, attrType);
	}
	// set the attribute on the MIME type, if the file has a signature
	BMimeType mimeType;
	if (error == B_OK && GetMetaMime(&mimeType) == B_OK) {
		if (!mimeType.IsInstalled())
			error = mimeType.Install();
		if (error == B_OK)
			error = mimeType.SetIconForType(type, icon, which);
	}
	return error;
}


// SetIconForType
/*!	\brief Sets the icon the application provides for a given MIME type.

	If \a type is \c NULL, the application's icon is set.
	If \a data is \c NULL the icon is unset.

	If the file has a signature, then the icon is also set on the MIME type.
	If the type for the signature has not been installed yet, it is installed
	before.

	\param type The MIME type in question. May be \c NULL.
	\param data A pointer to the data containing the icon to be set.
		   May be \c NULL.
	\param size Specifies the size of buffer provided in \a data.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: The provided \a type is not a valid MIME type.
	- other error codes
*/
status_t
BAppFileInfo::SetIconForType(const char* type, const uint8* data,
							 size_t size)
{
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	// set some icon related variables
	BString attributeString = kIconAttribute;
	int32 resourceID = type ? kIconForTypeResourceID : kIconResourceID;
	uint32 attrType = B_VECTOR_ICON_TYPE;

	// check type param
	if (type) {
		if (BMimeType::IsValid(type))
			attributeString += type;
		else
			return B_BAD_VALUE;
	} else
		attributeString += kIconType;

	const char* attribute = attributeString.String();

	status_t error;
	// write/remove the attribute
	if (data)
		error = _WriteData(attribute, resourceID, attrType, data, size, true);
	else	// no icon given => remove
		error = _RemoveData(attribute, attrType);

	// set the attribute on the MIME type, if the file has a signature
	BMimeType mimeType;
	if (error == B_OK && GetMetaMime(&mimeType) == B_OK) {
		if (!mimeType.IsInstalled())
			error = mimeType.Install();
		if (error == B_OK)
			error = mimeType.SetIconForType(type, data, size);
	}
	return error;
}


// SetInfoLocation
/*!	\brief Specifies the location where the meta data shall be stored.

	The options for \a location are:
	- \c B_USE_ATTRIBUTES: Store the data in the attributes.
	- \c B_USE_RESOURCES: Store the data in the resources.
	- \c B_USE_BOTH_LOCATIONS: Store the data in attributes and resources.

	\param location The location where the meta data shall be stored.
*/
void
BAppFileInfo::SetInfoLocation(info_location location)
{
	// if the resources failed to initialize, we must not use them
	if (fResources == NULL)
		location = info_location(location & ~B_USE_RESOURCES);

	fWhere = location;
}

// IsUsingAttributes
/*!	\brief Returns whether the object stores the meta data (also) in the
		   file's attributes.
	\return \c true, if the meta data are (also) stored in the file's
			attributes, \c false otherwise.
*/
bool
BAppFileInfo::IsUsingAttributes() const
{
	return (fWhere & B_USE_ATTRIBUTES) != 0;
}


// IsUsingResources
/*!	\brief Returns whether the object stores the meta data (also) in the
		   file's resources.
	\return \c true, if the meta data are (also) stored in the file's
			resources, \c false otherwise.
*/
bool
BAppFileInfo::IsUsingResources() const
{
	return (fWhere & B_USE_RESOURCES) != 0;
}


// FBC
void BAppFileInfo::_ReservedAppFileInfo1() {}
void BAppFileInfo::_ReservedAppFileInfo2() {}
void BAppFileInfo::_ReservedAppFileInfo3() {}


// =
/*!	\brief Privatized assignment operator to prevent usage.
*/
BAppFileInfo &
BAppFileInfo::operator=(const BAppFileInfo &)
{
	return *this;
}


// copy constructor
/*!	\brief Privatized copy constructor to prevent usage.
*/
BAppFileInfo::BAppFileInfo(const BAppFileInfo &)
{
}


// GetMetaMime
/*!	\brief Initializes a BMimeType to the file's signature.

	The parameter \a meta is not checked.

	\param meta A pointer to a pre-allocated BMimeType that shall be
		   initialized to the file's signature.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a meta
	- \c B_ENTRY_NOT_FOUND: The file has not signature or the signature is
(	  not installed in the MIME database.)
	  no valid MIME string.
	- other error codes
*/
status_t
BAppFileInfo::GetMetaMime(BMimeType* meta) const
{
	char signature[B_MIME_TYPE_LENGTH];
	status_t error = GetSignature(signature);
	if (error == B_OK)
		error = meta->SetTo(signature);
	else if (error == B_BAD_VALUE)
		error = B_ENTRY_NOT_FOUND;
	if (error == B_OK && !meta->IsValid())
		error = B_BAD_VALUE;
	return error;
}


// _ReadData
/*!	\brief Reads data from an attribute or resource.

	The data are read from the location specified by \a fWhere.

	The object must be properly initialized. The parameters are NOT checked.

	\param name The name of the attribute/resource to be read.
	\param id The resource ID of the resource to be read. Is ignored, when
		   < 0.
	\param type The type of the attribute/resource to be read.
	\param buffer A pre-allocated buffer for the data to be read.
	\param bufferSize The size of the supplied buffer.
	\param bytesRead A reference parameter, set to the number of bytes
		   actually read.
	\param allocatedBuffer If not \c NULL, the method allocates a buffer
		   large enough too store the whole data and writes a pointer to it
		   into this variable. If \c NULL, the supplied buffer is used.
	\return
	- \c B_OK: Everything went fine.
	- error code
*/
status_t
BAppFileInfo::_ReadData(const char* name, int32 id, type_code type,
						void* buffer, size_t bufferSize,
						size_t &bytesRead, void** allocatedBuffer) const
{
	status_t error = B_OK;
	
	if (allocatedBuffer)
		buffer = NULL;

	bool foundData = false;

	if (IsUsingAttributes()) {
		// get an attribute info
		attr_info info;
		if (error == B_OK)
			error = fNode->GetAttrInfo(name, &info);

		// check type and size, allocate a buffer, if required
		if (error == B_OK && info.type != type)
			error = B_BAD_VALUE;
		if (error == B_OK && allocatedBuffer) {
			buffer = malloc(info.size);
			if (!buffer)
				error = B_NO_MEMORY;
			bufferSize = info.size;
		}
		if (error == B_OK && (off_t)bufferSize < info.size)
			error = B_BAD_VALUE;

		// read the data
		if (error == B_OK) {
			ssize_t read = fNode->ReadAttr(name, type, 0, buffer, info.size);
			if (read < 0)
				error = read;
			else if (read != info.size)
				error = B_ERROR;
			else
				bytesRead = read;
		}

		foundData = (error == B_OK);

		// free the allocated buffer on error
		if (!foundData && allocatedBuffer && buffer) {
			free(buffer);
			buffer = NULL;
		}
	}

	if (!foundData && IsUsingResources()) {
		// get a resource info
		error = B_OK;
		int32 idFound;
		size_t sizeFound;
		if (error == B_OK) {
			if (!fResources->GetResourceInfo(type, name, &idFound, &sizeFound))
				error = B_ENTRY_NOT_FOUND;
		}

		// check id and size, allocate a buffer, if required
		if (error == B_OK && id >= 0 && idFound != id)
			error = B_ENTRY_NOT_FOUND;
		if (error == B_OK && allocatedBuffer) {
			buffer = malloc(sizeFound);
			if (!buffer)
				error = B_NO_MEMORY;
			bufferSize = sizeFound;
		}
		if (error == B_OK && bufferSize < sizeFound)
			error = B_BAD_VALUE;

		// load resource
		const void* resourceData = NULL;
		if (error == B_OK) {
			resourceData = fResources->LoadResource(type, name, &bytesRead);
			if (resourceData && sizeFound == bytesRead)
				memcpy(buffer, resourceData, bytesRead);
			else
				error = B_ERROR;
		}
	} else if (!foundData)
		error = B_BAD_VALUE;

	// return the allocated buffer, or free it on error
	if (allocatedBuffer) {
		if (error == B_OK)
			*allocatedBuffer = buffer;
		else
			free(buffer);
	}

	return error;
}


// _WriteData
/*!	\brief Writes data to an attribute or resource.

	The data are written to the location(s) specified by \a fWhere.

	The object must be properly initialized. The parameters are NOT checked.

	\param name The name of the attribute/resource to be written.
	\param id The resource ID of the resource to be written.
	\param type The type of the attribute/resource to be written.
	\param buffer A buffer containing the data to be written.
	\param bufferSize The size of the supplied buffer.
	\param findID If set to \c true use the ID that is already assigned to the
		   \a name / \a type pair or take the first unused ID >= \a id.
		   If \c false, \a id is used.
	If \a id is already in use and .
	\return
	- \c B_OK: Everything went fine.
	- error code
*/
status_t
BAppFileInfo::_WriteData(const char* name, int32 id, type_code type,
						 const void* buffer, size_t bufferSize, bool findID)
{
	if (!IsUsingAttributes() && !IsUsingResources())
		return B_NO_INIT;

	status_t error = B_OK;

	// write to attribute
	if (IsUsingAttributes()) {
		ssize_t written = fNode->WriteAttr(name, type, 0, buffer, bufferSize);
		if (written < 0)
			error = written;
		else if (written != (ssize_t)bufferSize)
			error = B_ERROR;
	}
	// write to resource
	if (IsUsingResources() && error == B_OK) {
		if (findID) {
			// get the resource info
			int32 idFound;
			size_t sizeFound;
			if (fResources->GetResourceInfo(type, name, &idFound, &sizeFound))
				id = idFound;
			else {
				// type-name pair doesn't exist yet -- find unused ID
				while (fResources->HasResource(type, id))
					id++;
			}
		}
		error = fResources->AddResource(type, id, buffer, bufferSize, name);
	}
	return error;
}

// _RemoveData
/*!	\brief Removes an attribute or resource.

	The removal location is specified by \a fWhere.

	The object must be properly initialized. The parameters are NOT checked.

	\param name The name of the attribute/resource to be remove.
	\param type The type of the attribute/resource to be removed.
	\return
	- \c B_OK: Everything went fine.
	- error code
*/
status_t
BAppFileInfo::_RemoveData(const char* name, type_code type)
{
	if (!IsUsingAttributes() && !IsUsingResources())
		return B_NO_INIT;

	status_t error = B_OK;

	// remove the attribute
	if (IsUsingAttributes()) {
		error = fNode->RemoveAttr(name);
		// It's no error, if there has been no attribute.
		if (error == B_ENTRY_NOT_FOUND)
			error = B_OK;
	}
	// remove the resource
	if (IsUsingResources() && error == B_OK) {
		// get a resource info
		int32 idFound;
		size_t sizeFound;
		if (fResources->GetResourceInfo(type, name, &idFound, &sizeFound))
			error = fResources->RemoveResource(type, idFound);
	}
	return error;
}

