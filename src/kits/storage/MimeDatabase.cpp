// MimeDatabase.cpp

#include <Application.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <Directory.h>
#include <Entry.h>
#include <Message.h>
#include <MimeType.h>
#include <Node.h>
#include <Path.h>
#include <TypeConstants.h>

#include <ctype.h>		// For tolower()
#include <fs_attr.h>	// For struct attr_info
#include <iostream>
#include <new>			// For new(nothrow)
#include <stdio.h>
#include <string>

#include "MimeDatabase.h"

//#define DBG(x) x
#define DBG(x)
#define OUT printf

// icon types
enum {
	B_MINI_ICON_TYPE	= 'MICN',
	B_LARGE_ICON_TYPE	= 'ICON',
};

namespace BPrivate {

//const char* MimeDatabase::kDefaultDatabaseDir = "/boot/home/config/settings/beos_mime";
const char* MimeDatabase::kDefaultDatabaseDir = "/boot/home/config/settings/obos_mime";

#define ATTR_PREFIX "META:"
#define MINI_ICON_ATTR_PREFIX ATTR_PREFIX "M:"
#define LARGE_ICON_ATTR_PREFIX ATTR_PREFIX "L:"

const char *MimeDatabase::kMiniIconAttrPrefix	= MINI_ICON_ATTR_PREFIX;
const char *MimeDatabase::kLargeIconAttrPrefix	= LARGE_ICON_ATTR_PREFIX; 

// attribute names
const char *MimeDatabase::kFileTypeAttr			= "BEOS:TYPE";
const char *MimeDatabase::kTypeAttr				= ATTR_PREFIX "TYPE";
const char *MimeDatabase::kAppHintAttr			= ATTR_PREFIX "PPATH";
const char *MimeDatabase::kAttrInfoAttr			= ATTR_PREFIX "ATTR_INFO";
const char *MimeDatabase::kShortDescriptionAttr	= ATTR_PREFIX "S:DESC";
const char *MimeDatabase::kLongDescriptionAttr	= ATTR_PREFIX "L:DESC";
const char *MimeDatabase::kFileExtensionsAttr	= ATTR_PREFIX "EXTENS";
const char *MimeDatabase::kMiniIconAttr			= MINI_ICON_ATTR_PREFIX "STD_ICON";
const char *MimeDatabase::kLargeIconAttr		= LARGE_ICON_ATTR_PREFIX "STD_ICON";
const char *MimeDatabase::kPreferredAppAttr		= ATTR_PREFIX "PREF_APP";
const char *MimeDatabase::kSnifferRuleAttr		= ATTR_PREFIX "SNIFF_RULE";
const char *MimeDatabase::kSupportedTypesAttr	= ATTR_PREFIX "FILE_TYPES";

// attribute data types (as used in the R5 database)
const int32 MimeDatabase::kFileTypeType			= 'MIMS';	// B_MIME_STRING_TYPE
const int32 MimeDatabase::kTypeType				= B_STRING_TYPE;
const int32 MimeDatabase::kAppHintType			= 'MPTH';
const int32 MimeDatabase::kAttrInfoType			= B_MESSAGE_TYPE;
const int32 MimeDatabase::kShortDescriptionType	= 'MSDC';
const int32 MimeDatabase::kLongDescriptionType	= 'MLDC';
const int32 MimeDatabase::kFileExtensionsType	= B_MESSAGE_TYPE;
const int32 MimeDatabase::kMiniIconType			= B_MINI_ICON_TYPE;
const int32 MimeDatabase::kLargeIconType		= B_LARGE_ICON_TYPE;
const int32 MimeDatabase::kPreferredAppType		= 'MSIG';
const int32 MimeDatabase::kSnifferRuleType		= B_STRING_TYPE;
const int32 MimeDatabase::kSupportedTypesType	= B_MESSAGE_TYPE;


#undef ATTR_PREFIX 
#undef MINI_ICON_ATTR_PREFIX
#undef LARGE_ICON_ATTR_PREFIX

//! Converts every character in \c str to lowercase and returns the result
std::string
MimeDatabase::ToLower(const char *str)
{
	if (str) {
		std::string result = "";
		for (int i = 0; i < strlen(str); i++)
			result += tolower(str[i]);
		return result;
	} else
		return "(null)";
}

/*!
	\class MimeDatabase
	\brief MimeDatabase is the master of the MIME data base.

	All write and non-atomic read accesses are carried out by this class.
	
	\note No error checking (other than checks for NULL pointers) is performed
	      by this class on the mime type strings passed to it. It's assumed
	      that this sort of checking has been done beforehand.
*/

// constructor
/*!	\brief Creates and initializes a MimeDatabase.
*/
MimeDatabase::MimeDatabase(const char *databaseDir)
	: fDatabaseDir(databaseDir)
	, fCStatus(B_NO_INIT)
{
	// Do some really minor error checking
	BEntry entry(fDatabaseDir.c_str());
	fCStatus = entry.Exists() ? B_OK : B_BAD_VALUE; 
}

// destructor
/*!	\brief Frees all resources associated with this object.
*/
MimeDatabase::~MimeDatabase()
{
}

// InitCheck
/*! \brief Returns the initialization status of the object.
	\return
	- B_OK: success
	- "error code": failure
*/
status_t
MimeDatabase::InitCheck()
{
	return fCStatus;
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
MimeDatabase::Install(const char *type)
{
	BEntry entry;
	status_t err = (type ? B_OK : B_BAD_VALUE);
	if (!err) 
		err = entry.SetTo(TypeToFilename(type).c_str());
	if (!err) {
		if (entry.Exists())
			err = B_FILE_EXISTS;
		else {
			BNode node;
			err = OpenOrCreateType(type, &node);
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
MimeDatabase::Delete(const char *type)
{
	BEntry entry;
	status_t err = (type ? B_OK : B_BAD_VALUE);
	if (!err) 
		err = entry.SetTo(TypeToFilename(type).c_str());		 
	if (!err)
		err = entry.Remove();
	return err;
}

// IsInstalled
//! Checks if the given MIME type is present in the database
bool
MimeDatabase::IsInstalled(const char *type) const
{
	BNode node;
	return OpenType(type, &node) == B_OK;
}

// GetIcon
//! Fetches the icon of given size associated with the given MIME type
/*! The bitmap pointed to by \c icon must be of the proper size (\c 32x32
	for \c B_LARGE_ICON, \c 16x16 for \c B_MINI_ICON) and color depth
	(\c B_CMAP8).
	
	\param type The mime type
	\param icon Pointer to a pre-allocated bitmap of proper dimensions and color depth
	\param size The size icon you're interested in (\c B_LARGE_ICON or \c B_MINI_ICON)
*/
status_t
MimeDatabase::GetIcon(const char *type, BBitmap *icon, icon_size which) const
{
	const char *attr = (which == B_MINI_ICON) ? kMiniIconAttr : kLargeIconAttr;
	return GetIcon(type, attr, icon, which);
}



// GetPreferredApp
//!	Fetches signature of the MIME type's preferred application for the given action.
/*!	The string pointed to by \c signature must be long enough to
	hold the short description; a length of \c B_MIME_TYPE_LENGTH+1 is
	recommended.
	
	Currently, the only supported app verb is \c B_OPEN.
	
	\param type The MIME type of interest
	\param description Pointer to a pre-allocated string into which the preferred
	                   application's signature is copied. If the function fails, the
	                   contents of the string are undefined.
	\param verb \c The action of interest

	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No such preferred application exists
	- "error code": Failure
*/
status_t
MimeDatabase::GetPreferredApp(const char *type, char *signature, app_verb verb = B_OPEN) const
{
	// Since B_OPEN is the currently the only app_verb, it is essentially ignored
	ssize_t err = ReadMimeAttr(type, kPreferredAppAttr, signature, B_MIME_TYPE_LENGTH, kPreferredAppType);
	return err >= 0 ? B_OK : err ;
}

// GetAttrInfo
/*! \brief Fetches from the MIME database a BMessage describing the attributes
	typically associated with files of the given MIME type
	
	The attribute information is returned in a pre-allocated BMessage pointed to by
	the \c info parameter (note that the any prior contents of the message
	will be destroyed). Please see BMimeType::SetAttrInfo() for a description
	of the expected format of such a message.
		
	\param info Pointer to a pre-allocated BMessage into which information about
	            the MIME type's associated file attributes is stored.
	\return
	- \c B_OK: Success
	- "error code": Failure
*/
status_t
MimeDatabase::GetAttrInfo(const char *type, BMessage *info) const
{
	status_t err = ReadMimeAttrMessage(type, kAttrInfoAttr, info);
	if (!err) {
		info->what = 233;	// Don't know why, but that's what R5 does.
		err = info->AddString("type", type);
	}
	return err;	
}

// GetShortDescription
//!	Fetches the short description for the given MIME type.
/*!	The string pointed to by \c description must be long enough to
	hold the short description; a length of \c B_MIME_TYPE_LENGTH+1 is
	recommended.
	
	\param type The MIME type of interest
	\param description Pointer to a pre-allocated string into which the short
	                   description is copied. If the function fails, the contents
	                   of the string are undefined.

	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No short description exists for the given type
	- "error code": Failure
*/
status_t
MimeDatabase::GetShortDescription(const char *type, char *description) const
{
///	DBG(OUT("MimeDatabase::GetShortDescription()\n"));
	ssize_t err = ReadMimeAttr(type, kShortDescriptionAttr, description, B_MIME_TYPE_LENGTH, kShortDescriptionType);
	return err >= 0 ? B_OK : err ;
}

// GetLongDescription
//!	Fetches the long description for the given MIME type.
/*!	The string pointed to by \c description must be long enough to
	hold the long description; a length of \c B_MIME_TYPE_LENGTH+1 is
	recommended.
	
	\param type The MIME type of interest
	\param description Pointer to a pre-allocated string into which the long
	                   description is copied. If the function fails, the contents
	                   of the string are undefined.

	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No long description exists for the given type
	- "error code": Failure
*/
status_t
MimeDatabase::GetLongDescription(const char *type, char *description) const {
//	DBG(OUT("MimeDatabase::GetLongDescription()\n"));
	ssize_t err = ReadMimeAttr(type, kLongDescriptionAttr, description, B_MIME_TYPE_LENGTH, kLongDescriptionType);
	return err >= 0 ? B_OK : err ;
}

// GetFileExtensions
//! Fetches a BMessage describing the MIME type's associated filename extensions
/*! The list of extensions is returned in a pre-allocated BMessage pointed to by
	the \c extensions parameter (note that the any prior contents of the message
	will be destroyed). Please see BMimeType::GetFileExtensions() for a description
	of the message format.
	  
	\param extensions Pointer to a pre-allocated BMessage into which the MIME
	                  type's associated file extensions will be stored.
	\return
	- \c B_OK: Success
	- "error code": Failure
*/
status_t
MimeDatabase::GetFileExtensions(const char *type, BMessage *extensions) const
{
	status_t err = ReadMimeAttrMessage(type, kFileExtensionsAttr, extensions);
	if (!err) {
		extensions->what = 234;	// Don't know why, but that's what R5 does.
		err = extensions->AddString("type", type);
	}
	return err;	
}

//! Sets the icon for the given mime type
/*! This is the version I would have used if I could have gotten a BBitmap
	to the registrar somehow. Since R5::BBitmap::Instantiate is causing a
	violent crash, I've copied most of the icon	color conversion code into
	MimeDatabase::GetIconData() so BMimeType::SetIcon() can get at it.
	
	Once we have a sufficiently complete OBOS::BBitmap implementation, we
	ought to be able to use this version of SetIcon() again. At that point,
	I'll add some real documentation.
*/
status_t
MimeDatabase::SetIcon(const char *type, const void *data, size_t dataSize, icon_size which)
{
	const char *attr = (which == B_MINI_ICON) ? kMiniIconAttr : kLargeIconAttr;
	return SetIcon(type, attr, data, dataSize, which);
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
MimeDatabase::SetPreferredApp(const char *type, const char *signature, app_verb verb = B_OPEN)
{
	DBG(OUT("MimeDatabase::SetPreferredApp()\n"));	
	status_t err = (type && signature && strlen(signature) < B_MIME_TYPE_LENGTH) ? B_OK : B_BAD_VALUE;
	if (!err)
		err = WriteMimeAttr(type, kPreferredAppAttr, signature, strlen(signature)+1, kPreferredAppType);
	if (!err)
		err = SendMonitorUpdate(B_PREFERRED_APP_CHANGED, type, B_META_MIME_MODIFIED);
	return err;
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
MimeDatabase::SetAttrInfo(const char *type, const BMessage *info)
{
	DBG(OUT("MimeDatabase::SetAttrInfo()\n"));	
	status_t err = (type && info) ? B_OK : B_BAD_VALUE;
	if (!err)
		err = WriteMimeAttrMessage(type, kAttrInfoAttr, info);
	if (!err)
		err = SendMonitorUpdate(B_ATTR_INFO_CHANGED, type, B_META_MIME_MODIFIED);
	return err;
}

// SetFileExtensions
//! Sets the list of filename extensions associated with the MIME type
/*! The list of extensions is given in a pre-allocated BMessage pointed to by
	the \c extensions parameter. Please see BMimeType::SetFileExtensions()
	for a description of the expected message format.
	  
	\param extensions Pointer to a pre-allocated, properly formatted BMessage containing
	                  the new list of file extensions to associate with this MIME type.
	\return
	- \c B_OK: Success
	- "error code": Failure
*/
status_t
MimeDatabase::SetFileExtensions(const char *type, const BMessage *extensions)
{
	DBG(OUT("MimeDatabase::SetFileExtensions()\n"));	
	status_t err = (type && extensions) ? B_OK : B_BAD_VALUE;
	if (!err)
		err = WriteMimeAttrMessage(type, kFileExtensionsAttr, extensions);
	if (!err)
		err = SendMonitorUpdate(B_FILE_EXTENSIONS_CHANGED, type, B_META_MIME_MODIFIED);
	return err;
}

// SetShortDescription
/*!	\brief Sets the short description for the given MIME type
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\param decsription Pointer to a NULL-terminated string containing the new short description
*/
status_t
MimeDatabase::SetShortDescription(const char *type, const char *description)
{
	DBG(OUT("MimeDatabase::SetShortDescription()\n"));	
	status_t err = (type && description && strlen(description) < B_MIME_TYPE_LENGTH) ? B_OK : B_BAD_VALUE;
	if (!err)
		err = WriteMimeAttr(type, kShortDescriptionAttr, description, strlen(description)+1, kShortDescriptionType);
	if (!err)
		err = SendMonitorUpdate(B_SHORT_DESCRIPTION_CHANGED, type, B_META_MIME_MODIFIED);
	return err;
}

// SetLongDescription
/*!	\brief Sets the long description for the given MIME type
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\param decsription Pointer to a NULL-terminated string containing the new long description
*/
status_t
MimeDatabase::SetLongDescription(const char *type, const char *description)
{
	DBG(OUT("MimeDatabase::SetLongDescription()\n"));
	status_t err = (type && description && strlen(description) < B_MIME_TYPE_LENGTH) ? B_OK : B_BAD_VALUE;
	if (!err)
		err = WriteMimeAttr(type, kLongDescriptionAttr, description, strlen(description)+1, kLongDescriptionType);
	if (!err)
		err = SendMonitorUpdate(B_SHORT_DESCRIPTION_CHANGED, type, B_META_MIME_MODIFIED);
	return err;
}

// GetAppHint
//!	Fetches the application hint for the given MIME type.
/*!	The entry_ref pointed to by \c ref must be pre-allocated.
	
	\param type The MIME type of interest
	\param ref Pointer to a pre-allocated \c entry_ref struct into
	           which the location of the hint application is copied.

	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No app hint exists for the given type
	- "error code": Failure
*/
status_t
MimeDatabase::GetAppHint(const char *type, entry_ref *ref) const
{
	char path[B_MIME_TYPE_LENGTH+1];
	BEntry entry;
	ssize_t err = ref ? B_OK : B_BAD_VALUE;
	if (!err)
		err = ReadMimeAttr(type, kAppHintAttr, path, B_MIME_TYPE_LENGTH, kAppHintType);
	if (err >= 0)
		err = entry.SetTo(path);
	if (!err)
		err = entry.GetRef(ref);		
	return err;
}

// SetAppHint
/*!	\brief Sets the application hint for the given MIME type
	\param type Pointer to a NULL-terminated string containing the MIME type of interest
	\param decsription Pointer to an entry_ref containing the location of an application
	       that should be used when launching an application with this signature.
*/
status_t
MimeDatabase::SetAppHint(const char *type, const entry_ref *ref)
{
	DBG(OUT("MimeDatabase::SetAppHint()\n"));
	BPath path;
	status_t err = (type && ref) ? B_OK : B_BAD_VALUE;
	if (!err)
		err = path.SetTo(ref);
	if (!err)	
		err = WriteMimeAttr(type, kAppHintAttr, path.Path(), strlen(path.Path())+1, kAppHintType);
	if (!err)
		err = SendMonitorUpdate(B_APP_HINT_CHANGED, type, B_META_MIME_MODIFIED);
	return err;
}

// GetIconForType
/*! \brief Fetches the large or mini icon used by an application of this type for files of the
	given type.
	
	The type of the \c BMimeType object is not required to actually be a subtype of
	\c "application/"; that is the intended use however, and calling \c GetIconForType()
	on a non-application type will likely return \c B_ENTRY_NOT_FOUND.
	
	The icon is copied into the \c BBitmap pointed to by \c icon. The bitmap must
	be the proper size: \c 32x32 for the large icon, \c 16x16 for the mini icon.	
	
	\param type The MIME type
	\param type Pointer to a pre-allocated string containing the MIME type whose
	            custom icon you wish to fetch. If NULL, works just like GetIcon().
	\param icon Pointer to a pre-allocated \c BBitmap of proper size and colorspace into
				which the icon is copied.
	\param icon_size Value that specifies which icon to return. Currently \c B_LARGE_ICON
					 and \c B_MINI_ICON are supported.
	\return
	- \c B_OK: Success
	- \c B_ENTRY_NOT_FOUND: No icon of the given size exists for the given type
	- "error code": Failure	

*/
status_t
MimeDatabase::GetIconForType(const char *type, const char *fileType, BBitmap *icon,
	  						   icon_size which) const
{
	std::string attr = ((which == B_MINI_ICON)
	                       ? kMiniIconAttrPrefix
	                         : kLargeIconAttrPrefix)
	                       + ToLower(fileType);
	return GetIcon(type, attr.c_str(), icon, which);
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
MimeDatabase::SetIconForType(const char *type, const char *fileType, const void *data,
							   size_t dataSize, icon_size which)
{
	std::string attr = ((which == B_MINI_ICON)
	                       ? kMiniIconAttrPrefix
	                         : kLargeIconAttrPrefix)
	                       + ToLower(fileType);
	return SetIcon(type, attr.c_str(), data, dataSize, which);
}

// StartWatching
//!	Subscribes the given BMessenger to the MIME monitor service
/*!	Notification messages will be sent with a \c BMessage::what value
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
MimeDatabase::StartWatching(BMessenger target)
{
	DBG(OUT("MimeDatabase::StartWatching()\n"));
	status_t err = target.IsValid() ? B_OK : B_BAD_VALUE;
	if (!err) 
		fMonitorSet.insert(target);
	return err;	
}

// StartWatching
//!	Unsubscribes the given BMessenger from the MIME monitor service
/*! \param target The \c BMessenger to unsubscribe 
*/
status_t
MimeDatabase::StopWatching(BMessenger target)
{
	DBG(OUT("MimeDatabase::StopWatching()\n"));
	status_t err = fMonitorSet.find(target) != fMonitorSet.end() ? B_OK : B_ENTRY_NOT_FOUND;
	if (!err)
		fMonitorSet.erase(target);
	return err;	
}

status_t
MimeDatabase::DeleteAppHint(const char *type)
{
	status_t err = DeleteAttribute(type, kAppHintAttr);
	if (!err)
		err = SendMonitorUpdate(B_APP_HINT_CHANGED, type, B_META_MIME_DELETED);
	return err;
}

status_t
MimeDatabase::DeleteAttrInfo(const char *type)
{
	status_t err = DeleteAttribute(type, kAttrInfoAttr);
	if (!err)
		err = SendMonitorUpdate(B_ATTR_INFO_CHANGED, type, B_META_MIME_DELETED);
	return err;
}

status_t
MimeDatabase::DeleteShortDescription(const char *type)
{
	status_t err = DeleteAttribute(type, kShortDescriptionAttr);
	if (!err)
		err = SendMonitorUpdate(B_SHORT_DESCRIPTION_CHANGED, type, B_META_MIME_DELETED);
	return err;
}

status_t
MimeDatabase::DeleteLongDescription(const char *type)
{
	status_t err = DeleteAttribute(type, kLongDescriptionAttr);
	if (!err)
		err = SendMonitorUpdate(B_LONG_DESCRIPTION_CHANGED, type, B_META_MIME_DELETED);
	return err;
}

status_t
MimeDatabase::DeleteFileExtensions(const char *type)
{
	status_t err = DeleteAttribute(type, kFileExtensionsAttr);
	if (!err)
		err = SendMonitorUpdate(B_FILE_EXTENSIONS_CHANGED, type, B_META_MIME_DELETED);
	return err;
}

status_t
MimeDatabase::DeleteIcon(const char *type, icon_size which)
{
	const char *attr = which == B_MINI_ICON ? kMiniIconAttr : kLargeIconAttr;
	status_t err = DeleteAttribute(type, attr);
	if (!err)
		err = SendMonitorUpdate(B_ICON_CHANGED, type, which, B_META_MIME_DELETED);
	return err;
}
	
status_t
MimeDatabase::DeleteIconForType(const char *type, const char *fileType, icon_size which)
{
	std::string attr;
	status_t err = fileType ? B_OK : B_BAD_VALUE;
	if (!err) {
		attr = (which == B_MINI_ICON ? kMiniIconAttrPrefix : kLargeIconAttrPrefix) + ToLower(fileType);
		err = DeleteAttribute(type, attr.c_str());
	}
	if (!err)
		err = SendMonitorUpdate(B_ICON_FOR_TYPE_CHANGED, type, fileType,
		                          which == B_LARGE_ICON, B_META_MIME_DELETED);
	return err;
}

status_t
MimeDatabase::DeletePreferredApp(const char *type, app_verb verb = B_OPEN)
{
	status_t err;
	switch (verb) {
		case B_OPEN:
			err = DeleteAttribute(type, kPreferredAppAttr);
			break;
				
		default:
			err = B_BAD_VALUE;
			break;
	}
	/*! \todo The R5 monitor makes no note of which app_verb value was updated. If
		additional app_verb values besides \c B_OPEN are someday added, the format
		of the MIME monitor messages will need to be augmented.
	*/
	if (!err)
		err = SendMonitorUpdate(B_PREFERRED_APP_CHANGED, type, B_META_MIME_DELETED);
}

status_t
MimeDatabase::DeleteSnifferRule(const char *type)
{
	status_t err = DeleteAttribute(type, kSnifferRuleAttr);
	if (!err)
		err = SendMonitorUpdate(B_SNIFFER_RULE_CHANGED, type, B_META_MIME_DELETED);
	return err;
}

// GetIconData
/*! \brief Returns properly formatted raw bitmap data, ready to be shipped off to the hacked
	up 4-parameter version of MimeDatabase::SetIcon()
	
	This function exists as something of a hack until an OBOS::BBitmap implementation is
	available. It takes the given bitmap, converts it to the B_CMAP8 color space if necessary
	and able, and returns said bitmap data in a newly allocated array pointed to by the
	pointer that's pointed to by \c data. The length of the array is stored in the integer
	pointed to by \c dataSize. The array is allocated with \c new[], and it's your responsibility
	to \c delete[] it when you're finished.
	
*/
status_t
MimeDatabase::GetIconData(const BBitmap *icon, icon_size which, void **data, int32 *dataSize)
{
	ssize_t err = (icon && data && dataSize && icon->InitCheck() == B_OK) ? B_OK : B_BAD_VALUE;

	BRect bounds;	
	BBitmap *icon8 = NULL;
	void *srcData = NULL;
	bool otherColorSpace = false;

	// Figure out what kind of data we *should* have
	if (!err) {
		switch (which) {
			case B_MINI_ICON:
				bounds.Set(0, 0, 15, 15);
				break;
			case B_LARGE_ICON:
				bounds.Set(0, 0, 31, 31);
				break;
			default:
				err = B_BAD_VALUE;
				break;
		}
	}
	// Check the icon
	if (!err)
		err = (icon->Bounds() == bounds) ? B_OK : B_BAD_VALUE;
	// Convert to B_CMAP8 if necessary
	if (!err) {
		otherColorSpace = (icon->ColorSpace() != B_CMAP8);
		if (otherColorSpace) {
			icon8 = new BBitmap(bounds, B_CMAP8);
			if (!icon8)
				err = B_NO_MEMORY;
			if (!err) {
				switch (icon->ColorSpace()) {
					case B_RGB32:
					{
						// Set each pixel individually, since SetBits() for B_RGB32 takes
						// 24-bit rgb pixel data...
						char *bgra = (char*)icon->Bits();
						for (uint32 i = 0; i*4+3 < icon->BitsLength(); bgra += 4, i++) {
							char rgb[3];
							rgb[0] = bgra[2];	// red
							rgb[1] = bgra[1];	// green
							rgb[2] = bgra[0];	// blue
							icon8->SetBits(rgb, 3, i, B_RGB32);
						}
						break;
					}
					case B_GRAY1:
					{
						icon8->SetBits(icon->Bits(), icon->BitsLength(), 0, B_GRAY1);
						break;
					}
					default:
						err = B_BAD_VALUE;
				}
				if (!err)
					err = icon8->InitCheck();	// I don't think this is actually useful, even if SetBits() fails...
			}
			if (!err) {
				srcData = icon8->Bits();
				*dataSize = icon8->BitsLength();
			}
		} else {
			srcData = icon->Bits();
			*dataSize = icon->BitsLength();
		}		
	}
	// Alloc a new data buffer
	if (!err) {
		*data = new char[*dataSize];
		if (!*data)
			err = B_NO_MEMORY;
	}
	// Copy the data into it.
	if (!err)
		memcpy(*data, srcData, *dataSize);	
	if (otherColorSpace)
		delete icon8;
	return err;	
}

// DeleteAttribute
//! Deletes the given attribute for the given type
/*!
	\param type The mime type
	\param attr The attribute name
	\return
    - B_OK: success
    - B_ENTRY_NOT_FOUND: no such type or attribute
    - "error code": failure
*/
status_t
MimeDatabase::DeleteAttribute(const char *type, const char *attr)
{
	status_t err = type ? B_OK : B_BAD_VALUE;
	BNode node;
	if (!err)
		err = OpenType(type, &node);
	if (!err)
		err = node.RemoveAttr(attr);
	return err;
}

// ReadMimeAttr
/*! \brief Reads up to \c len bytes of the given data from the given attribute
	       for the given MIME type.
	
	If no entry for the given type exists in the database, the function fails,
	and the contents of \c data are undefined.
	       
	\param type The MIME type
	\param attr The attribute name
	\param data Pointer to a memory buffer into which the data should be copied
	\param len The maximum number of bytes to read
	\param datatype The expected data type
	\return If successful, the number of bytes read is returned, otherwise, an
	        error code is returned.
*/
ssize_t
MimeDatabase::ReadMimeAttr(const char *type, const char *attr, void *data,
	size_t len, type_code datatype) const
{
	BNode node;
	ssize_t err = (type && attr && data ? B_OK : B_BAD_VALUE);
	if (!err)
		err = OpenType(type, &node);	
	if (!err) 
		err = node.ReadAttr(attr, datatype, 0, data, len);
	return err;
}

// ReadMimeAttrMessage
/*! \brief Reads a flattened BMessage from the given attribute of the given
	MIME type.
	
	If no entry for the given type exists in the database, or if the data
	stored in the attribute is not a flattened BMessage, the function fails
	and the contents of \c msg are undefined.
	       
	\param type The MIME type
	\param attr The attribute name
	\param data Pointer to a pre-allocated BMessage into which the attribute
			    data is unflattened.
*/
status_t
MimeDatabase::ReadMimeAttrMessage(const char *type, const char *attr, BMessage *msg) const
{
	BNode node;
	attr_info info;
	char *buffer = NULL;
	ssize_t err = (type && attr && msg ? B_OK : B_BAD_VALUE);
	if (!err)
		err = OpenType(type, &node);
	if (!err)
		err = node.GetAttrInfo(attr, &info);
	if (!err)
		err = info.type == B_MESSAGE_TYPE ? B_OK : B_BAD_VALUE;
	if (!err) {		
		buffer = new char[info.size];
		if (!buffer)
			err = B_NO_MEMORY;
	}
	if (!err) 
		err = node.ReadAttr(attr, B_MESSAGE_TYPE, 0, buffer, info.size);
	if (err >= 0)
		err = err == info.size ? B_OK : B_FILE_ERROR;
	if (!err)
		err = msg->Unflatten(buffer);
	delete [] buffer;
	return err;
}

// WriteMimeAttr
/*! \brief Writes \c len bytes of the given data to the given attribute
	       for the given MIME type.
	
	If no entry for the given type exists in the database, it is created.
	       
	\param type The MIME type
	\param attr The attribute name
	\param data Pointer to the data to write
	\param len The number of bytes to write
	\param datatype The data type of the given data
*/
status_t
MimeDatabase::WriteMimeAttr(const char *type, const char *attr, const void *data,
	size_t len, type_code datatype)
{
	BNode node;
	status_t err = (type && attr && data ? B_OK : B_BAD_VALUE);
	if (!err)
		err = OpenOrCreateType(type, &node);	
	if (!err) {
		ssize_t bytes = node.WriteAttr(attr, datatype, 0, data, len);
		if (bytes < B_OK)
			err = bytes;
		else
			err = (bytes != len ? B_FILE_ERROR : B_OK);
	}
	return err;
}

// WriteMimeAttr
/*! \brief Flattens the given \c BMessage and writes it to the given attribute
	       of the given MIME type.
	
	If no entry for the given type exists in the database, it is created.
	       
	\param type The MIME type
	\param attr The attribute name
	\param msg The BMessage to flatten and write
*/
status_t
MimeDatabase::WriteMimeAttrMessage(const char *type, const char *attr, const BMessage *msg)
{
	BNode node;
	BMallocIO data;
	ssize_t bytes;
	ssize_t err = (type && attr && msg ? B_OK : B_BAD_VALUE);
	if (!err)
		err = data.SetSize(msg->FlattenedSize());
	if (!err)
		err = msg->Flatten(&data, &bytes);
	if (!err)
		err = bytes == msg->FlattenedSize() ? B_OK : B_ERROR;
	if (!err)
		err = OpenOrCreateType(type, &node);	
	if (!err) 
		err = node.WriteAttr(attr, B_MESSAGE_TYPE, 0, data.Buffer(), data.BufferLength());
	if (err >= 0)
		err = err == data.BufferLength() ? B_OK : B_FILE_ERROR;
	return err;
}

// OpenType
/*! \brief Opens a BNode on the given type, failing if the type has no
           corresponding file in the database.
	\param type The MIME type to open
	\param result Pointer to a pre-allocated BNode into which
	              is opened on the given MIME type
*/
status_t
MimeDatabase::OpenType(const char *type, BNode *result) const
{
	status_t err = (type && result ? B_OK : B_BAD_VALUE);
	if (!err) 
		err = result->SetTo(TypeToFilename(type).c_str());		 
	return err;
}

// OpenOrCreateType
/*! \brief Opens a BNode on the given type, creating a node of the
	       appropriate flavor if necessary.
	
	All MIME types are converted to lowercase for use in the filesystem.
	\param type The MIME type to open
	\param result Pointer to a pre-allocated BNode into which
	              is opened on the given MIME type
*/
status_t
MimeDatabase::OpenOrCreateType(const char *type, BNode *result)
{
	std::string filename;
	std::string typeLower = ToLower(type);
	status_t err = (type && result ? B_OK : B_BAD_VALUE);
	if (!err) {
		filename = TypeToFilename(type);
		err = result->SetTo(filename.c_str());		 
	}
	if (err == B_ENTRY_NOT_FOUND) {
		// Figure out what type of node we need to create
		// + Supertype == directory
		// + Non-supertype == file
		int32 pos = typeLower.find_first_of('/');
		if (pos == std::string::npos) {
			// Supertype == directory				
			BDirectory parent(fDatabaseDir.c_str());
			err = parent.InitCheck();
			if (!err)
				err = parent.CreateDirectory(typeLower.c_str(), NULL);
		} else {
			// Non-supertype == file
			std::string super(typeLower, 0, pos);
			std::string sub(typeLower, pos+1);
			BDirectory parent((fDatabaseDir + "/" + super).c_str());
			err = parent.InitCheck();
			if (!err)
				err = parent.CreateFile(sub.c_str(), NULL);
		}
		// Now try opening again
		err = result->SetTo(filename.c_str());
	}
	return err;	
}

// TypeToFilename
//! Converts the given MIME type to an absolute path in the MIME database.
inline
std::string
MimeDatabase::TypeToFilename(const char *type) const
{
	return fDatabaseDir + "/" + ToLower(type);
}

// SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have
	subscribed to the MIME Monitor service
	\param type The MIME type that was updated
	\param which Bitmask describing which attribute was updated
	\param extraType The MIME type to which the change is applies
	\param largeIcon \true if the the large icon was updated, \false if the
		   small icon was updated
*/
status_t
MimeDatabase::SendMonitorUpdate(int32 which, const char *type, const char *extraType, bool largeIcon, int32 action) {
	BMessage msg(B_META_MIME_CHANGED);
	status_t err;
		
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
		err = SendMonitorUpdate(msg);
	return err;
}

// SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have
	subscribed to the MIME Monitor service
	\param type The MIME type that was updated
	\param which Bitmask describing which attribute was updated
	\param extraType The MIME type to which the change is applies
*/
status_t
MimeDatabase::SendMonitorUpdate(int32 which, const char *type, const char *extraType, int32 action) {
	BMessage msg(B_META_MIME_CHANGED);
	status_t err;
		
	err = msg.AddInt32("be:which", which);
	if (!err)
		err = msg.AddString("be:type", type);
	if (!err)
		err = msg.AddString("be:extra_type", extraType);
	if (!err)
		err = msg.AddInt32("be:action", action);
	if (!err)
		err = SendMonitorUpdate(msg);
	return err;
}

// SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have
	subscribed to the MIME Monitor service
	\param type The MIME type that was updated
	\param which Bitmask describing which attribute was updated
	\param largeIcon \true if the the large icon was updated, \false if the
		   small icon was updated
*/
status_t
MimeDatabase::SendMonitorUpdate(int32 which, const char *type, bool largeIcon, int32 action) {
	BMessage msg(B_META_MIME_CHANGED);
	status_t err;
		
	err = msg.AddInt32("be:which", which);
	if (!err)
		err = msg.AddString("be:type", type);
	if (!err)
		err = msg.AddBool("be:large_icon", largeIcon);
	if (!err)
		err = msg.AddInt32("be:action", action);
	if (!err)
		err = SendMonitorUpdate(msg);
	return err;
}

// SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have
	subscribed to the MIME Monitor service
	\param type The MIME type that was updated
	\param which Bitmask describing which attribute was updated
*/
status_t
MimeDatabase::SendMonitorUpdate(int32 which, const char *type, int32 action) {
	BMessage msg(B_META_MIME_CHANGED);
	status_t err;
		
	err = msg.AddInt32("be:which", which);
	if (!err)
		err = msg.AddString("be:type", type);
	if (!err)
		err = msg.AddInt32("be:action", action);
	if (!err)
		err = SendMonitorUpdate(msg);
	return err;
}

// SendMonitorUpdate
/*! \brief Sends an update notification to all BMessengers that have subscribed to
	the MIME Monitor service
	\param BMessage A preformatted MIME monitor message to be sent to all subscribers
*/
status_t
MimeDatabase::SendMonitorUpdate(BMessage &msg) {
	DBG(OUT("MimeDatabase::SendMonitorUpdate(BMessage&)\n"));
	std::set<BMessenger>::const_iterator i;
	for (i = fMonitorSet.begin(); i != fMonitorSet.end(); i++) {
		status_t err = (*i).SendMessage(&msg, (BHandler*)NULL);
		if (err)
			DBG(OUT("MimeDatabase::SendMonitorUpdate(BMessage&): BMessenger::SendMessage failed, 0x%x\n", err));
	}
	DBG(OUT("MimeDatabase::SendMonitorUpdate(BMessage&) done\n"));
	return B_OK;
}

// GetIconForAttr
//! Fetches the icon stored in the given attribute 
/*! The bitmap pointed to by \c icon must be of the proper size (\c 32x32
	for \c B_LARGE_ICON, \c 16x16 for \c B_MINI_ICON).
	
	\param type The mime type
	\param attr The attribute in which the icon is stored
	\param icon Pointer to a pre-allocated bitmap of proper dimensions and color depth
	\param size The size icon you're expecting (\c B_LARGE_ICON or \c B_MINI_ICON)
*/
status_t
MimeDatabase::GetIcon(const char *type, const char *attr, BBitmap *icon, icon_size which) const
{
	attr_info info;
	ssize_t err;
	BNode node;
	uint32 attrType;
	size_t attrSize;
	BRect bounds;
	char *buffer;

	err = type && icon ? B_OK : B_BAD_VALUE;
	
	// Figure out what kind of data we *should* find
	if (!err) {
		switch (which) {
			case B_MINI_ICON:
				bounds.Set(0, 0, 15, 15);
				attrType = kMiniIconType;
				attrSize = 16 * 16;
				break;
			case B_LARGE_ICON:
				bounds.Set(0, 0, 31, 31);
				attrType = kLargeIconType;
				attrSize = 32 * 32;
				break;
			default:
				err = B_BAD_VALUE;
				break;
		}
	}
	// Check the icon and attribute to see if they match
	if (!err)
		err = (icon->InitCheck() == B_OK && icon->Bounds() == bounds) ? B_OK : B_BAD_VALUE;
	if (!err) 
		err = OpenType(type, &node);
	if (!err) 
		err = node.GetAttrInfo(attr, &info);
	if (!err)
		err = (attrType == info.type && attrSize == info.size) ? B_OK : B_BAD_VALUE;
	// read the attribute
	if (!err) {
		bool otherColorSpace = (icon->ColorSpace() != B_CMAP8);
		char *buffer = NULL;
		ssize_t read;
		if (otherColorSpace) {
			// other color space than stored in attribute
			buffer = new(nothrow) char[attrSize];
			if (!buffer)
				err = B_NO_MEMORY;
			if (!err) 
				err = node.ReadAttr(attr, attrType, 0, buffer, attrSize);			
		} else {
			// same color space, just read direct
			err = node.ReadAttr(attr, attrType, 0, icon->Bits(), attrSize);
		}
		if (err >= 0)
			err = (err == attrSize) ? B_OK : B_FILE_ERROR;
		if (otherColorSpace) {
			if (!err) {
				icon->SetBits(buffer, attrSize, 0, B_CMAP8);
				err = icon->InitCheck();
			}
			delete[] buffer;
		}
	}

	return err;		
}

//! Sets the icon stored in the given attribute
/*! The bitmap data pointed to by \c data must be of the proper size (\c 32x32
	for \c B_LARGE_ICON, \c 16x16 for \c B_MINI_ICON) and the proper color
	space (B_CMAP8).
	
	\param type The mime type
	\param attr The attribute in which the icon is stored
	\param data Pointer to an array of bitmap data of proper dimensions and color depth
	\param dataSize The length of the array pointed to by \c data
	\param size The size icon you're expecting (\c B_LARGE_ICON or \c B_MINI_ICON)
*/
status_t
MimeDatabase::SetIcon(const char *type, const char *attr, const void *data, size_t dataSize, icon_size which)
{
	ssize_t err = (type && data) ? B_OK : B_BAD_VALUE;

	int32 attrType;
	int32 attrSize;
	
	// Figure out what kind of data we *should* have
	if (!err) {
		switch (which) {
			case B_MINI_ICON:
				attrType = kMiniIconType;
				attrSize = 16 * 16;
				break;
			case B_LARGE_ICON:
				attrType = kLargeIconType;
				attrSize = 32 * 32;
				break;
			default:
				err = B_BAD_VALUE;
				break;
		}
	}
	
	// Double check the data we've been given
	if (!err)
		err = (dataSize == attrSize) ? B_OK : B_BAD_VALUE;

	// Write the icon data
	BNode node;
	if (!err)
		err = OpenOrCreateType(type, &node);
	if (!err)
		err = node.WriteAttr(attr, attrType, 0, data, attrSize);
	if (err >= 0)
		err = (err == attrSize) ? B_OK : B_FILE_ERROR;
	return err;			
}

} // namespace BPrivate

