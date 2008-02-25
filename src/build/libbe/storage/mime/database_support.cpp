//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file database_support.cpp
	Private mime database functions and constants
*/

#include <AppMisc.h>
#include <DataIO.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Node.h>
#include <Path.h>
#include <storage_support.h>
#include <TypeConstants.h>

#include <fs_attr.h>	// For struct attr_info
#include <new>			// For new(nothrow)
#include <stdio.h>
#include <string>

#include "mime/database_support.h"

//#define DBG(x) x
#define DBG(x)
#define OUT printf

namespace BPrivate {
namespace Storage {
namespace Mime {

#if 0

static const char *get_user_settings_dir(BPath &path);

static BPath sSettingsDirPath;
static const std::string sSettingsDir = get_user_settings_dir(sSettingsDirPath);

static const char *sHaikuDBDirName	= "beos_mime";
	// when running natively under Haiku
static const char *sBeOSDBDirName	= "obos_mime";
	// when running under BeOS
const std::string kDatabaseDir = sSettingsDir + "/"
	+ (is_running_on_haiku() ? sHaikuDBDirName : sBeOSDBDirName);
const std::string kApplicationDatabaseDir		= kDatabaseDir + "/application";

#endif	// 0

#define ATTR_PREFIX "META:"
#define MINI_ICON_ATTR_PREFIX ATTR_PREFIX "M:"
#define LARGE_ICON_ATTR_PREFIX ATTR_PREFIX "L:"

const char *kMiniIconAttrPrefix		= MINI_ICON_ATTR_PREFIX;
const char *kLargeIconAttrPrefix	= LARGE_ICON_ATTR_PREFIX; 

// attribute names
const char *kFileTypeAttr			= "BEOS:TYPE";
const char *kTypeAttr				= ATTR_PREFIX "TYPE";
const char *kAppHintAttr			= ATTR_PREFIX "PPATH";
const char *kAttrInfoAttr			= ATTR_PREFIX "ATTR_INFO";
const char *kShortDescriptionAttr	= ATTR_PREFIX "S:DESC";
const char *kLongDescriptionAttr	= ATTR_PREFIX "L:DESC";
const char *kFileExtensionsAttr		= ATTR_PREFIX "EXTENS";
const char *kMiniIconAttr			= MINI_ICON_ATTR_PREFIX "STD_ICON";
const char *kLargeIconAttr			= LARGE_ICON_ATTR_PREFIX "STD_ICON";
const char *kPreferredAppAttr		= ATTR_PREFIX "PREF_APP";
const char *kSnifferRuleAttr		= ATTR_PREFIX "SNIFF_RULE";
const char *kSupportedTypesAttr		= ATTR_PREFIX "FILE_TYPES";

// attribute data types (as used in the R5 database)
const int32 kFileTypeType			= 'MIMS';	// B_MIME_STRING_TYPE
const int32 kTypeType				= B_STRING_TYPE;
const int32 kAppHintType			= 'MPTH';
const int32 kAttrInfoType			= B_MESSAGE_TYPE;
const int32 kShortDescriptionType	= 'MSDC';
const int32 kLongDescriptionType	= 'MLDC';
const int32 kFileExtensionsType		= B_MESSAGE_TYPE;
const int32 kMiniIconType			= B_MINI_ICON_TYPE;
const int32 kLargeIconType			= B_LARGE_ICON_TYPE;
const int32 kPreferredAppType		= 'MSIG';
const int32 kSnifferRuleType		= B_STRING_TYPE;
const int32 kSupportedTypesType		= B_MESSAGE_TYPE;

// Message fields
const char *kApplicationsField				= "applications";
const char *kExtensionsField				= "extensions";
const char *kSupertypesField				= "super_types";
const char *kSupportingAppsSubCountField	= "be:sub";
const char *kSupportingAppsSuperCountField	= "be:super";
const char *kTypesField						= "types";

// Mime types
const char *kGenericFileType	= "application/octet-stream";
const char *kDirectoryType		= "application/x-vnd.Be-directory";
const char *kSymlinkType		= "application/x-vnd.Be-symlink";
const char *kMetaMimeType		= "application/x-vnd.Be-meta-mime";

// Error codes
const status_t kMimeGuessFailureError	= B_ERRORS_END+1;

#if 0

// get_settings_dir
/*!	\brief Sets the supplied BPath to the user settings directory and returns
		   it as C string.
	\param path BPath to be set to the user settings path.
	\return the user settings path as C string (\code path.Path() \endcode).
*/
static
const char*
get_user_settings_dir(BPath &path)
{
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		path.SetTo("/boot/home/config/settings");
	return path.Path();
}

// type_to_filename
//! Converts the given MIME type to an absolute path in the MIME database.
std::string
type_to_filename(const char *type)
{
	return kDatabaseDir + "/" + BPrivate::Storage::to_lower(type);
}

// open_type
/*! \brief Opens a BNode on the given type, failing if the type has no
           corresponding file in the database.
	\param type The MIME type to open
	\param result Pointer to a pre-allocated BNode into which
	              is opened on the given MIME type
*/
status_t
open_type(const char *type, BNode *result)
{
	status_t err = (type && result ? B_OK : B_BAD_VALUE);
	if (!err) 
		err = result->SetTo(type_to_filename(type).c_str());		 
	return err;
}

// open_or_create_type
/*! \brief Opens a BNode on the given type, creating a node of the
	       appropriate flavor if necessary.
	
	All MIME types are converted to lowercase for use in the filesystem.
	\param type The MIME type to open
	\param result Pointer to a pre-allocated BNode into which
	              is opened on the given MIME type
*/
status_t
open_or_create_type(const char *type, BNode *result, bool *didCreate)
{
	if (didCreate)
		*didCreate = false;
	std::string filename;
	std::string typeLower = BPrivate::Storage::to_lower(type);
	status_t err = (type && result ? B_OK : B_BAD_VALUE);
	if (!err) {
		filename = type_to_filename(type);
		err = result->SetTo(filename.c_str());		 
	}
	if (err == B_ENTRY_NOT_FOUND) {
		// Figure out what type of node we need to create
		// + Supertype == directory
		// + Non-supertype == file
		uint32 pos = typeLower.find_first_of('/');
		if (pos == std::string::npos) {
			// Supertype == directory				
			BDirectory parent(kDatabaseDir.c_str());
			err = parent.InitCheck();
			if (!err)
				err = parent.CreateDirectory(typeLower.c_str(), NULL);
		} else {
			// Non-supertype == file
			std::string super(typeLower, 0, pos);
			std::string sub(typeLower, pos+1);
			BDirectory parent((kDatabaseDir + "/" + super).c_str());
			err = parent.InitCheck();
			if (!err)
				err = parent.CreateFile(sub.c_str(), NULL);
		}
		// Now try opening again
		err = result->SetTo(filename.c_str());
		if (!err && didCreate)
			*didCreate = true;
	}
	return err;	
}

// read_mime_attr
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
read_mime_attr(const char *type, const char *attr, void *data,
	size_t len, type_code datatype)
{
	BNode node;
	ssize_t err = (type && attr && data ? B_OK : B_BAD_VALUE);
	if (!err)
		err = open_type(type, &node);	
	if (!err) 
		err = node.ReadAttr(attr, datatype, 0, data, len);
	return err;
}

// read_mime_attr_message
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
read_mime_attr_message(const char *type, const char *attr, BMessage *msg)
{
	BNode node;
	attr_info info;
	char *buffer = NULL;
	ssize_t err = (type && attr && msg ? B_OK : B_BAD_VALUE);
	if (!err)
		err = open_type(type, &node);
	if (!err)
		err = node.GetAttrInfo(attr, &info);
	if (!err)
		err = info.type == B_MESSAGE_TYPE ? B_OK : B_BAD_VALUE;
	if (!err) {		
		buffer = new(std::nothrow) char[info.size];
		if (!buffer)
			err = B_NO_MEMORY;
	}
	if (!err) 
		err = node.ReadAttr(attr, B_MESSAGE_TYPE, 0, buffer, info.size);
	if (err >= 0)
		err = err == info.size ? (status_t)B_OK : (status_t)B_FILE_ERROR;
	if (!err)
		err = msg->Unflatten(buffer);
	delete [] buffer;
	return err;
}

// read_mime_attr_string
/*! \brief Reads a BString from the given attribute of the given
	MIME type.
	
	If no entry for the given type exists in the database, the function fails
	and the contents of \c str are undefined.
	       
	\param type The MIME type
	\param attr The attribute name
	\param str Pointer to a pre-allocated BString into which the attribute
			   data stored.
*/
status_t
read_mime_attr_string(const char *type, const char *attr, BString *str)
{
	BNode node;
	status_t err = (type && attr && str ? B_OK : B_BAD_VALUE);
	if (!err)
		err = open_type(type, &node);	
	if (!err) 
		err = node.ReadAttrString(attr, str);
	return err;
}

// write_mime_attr
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
write_mime_attr(const char *type, const char *attr, const void *data,
	size_t len, type_code datatype, bool *didCreate)
{
	BNode node;
	status_t err = (type && attr && data ? B_OK : B_BAD_VALUE);
	if (!err)
		err = open_or_create_type(type, &node, didCreate);	
	if (!err) {
		ssize_t bytes = node.WriteAttr(attr, datatype, 0, data, len);
		if (bytes < B_OK)
			err = bytes;
		else
			err = (bytes != (ssize_t)len ? (status_t)B_FILE_ERROR : (status_t)B_OK);
	}
	return err;
}

// write_mime_attr_message
/*! \brief Flattens the given \c BMessage and writes it to the given attribute
	       of the given MIME type.
	
	If no entry for the given type exists in the database, it is created.
	       
	\param type The MIME type
	\param attr The attribute name
	\param msg The BMessage to flatten and write
*/
status_t
write_mime_attr_message(const char *type, const char *attr, const BMessage *msg, bool *didCreate)
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
		err = open_or_create_type(type, &node, didCreate);	
	if (!err) 
		err = node.WriteAttr(attr, B_MESSAGE_TYPE, 0, data.Buffer(), data.BufferLength());
	if (err >= 0)
		err = err == (ssize_t)data.BufferLength() ? (ssize_t)B_OK : (ssize_t)B_FILE_ERROR;
	return err;
}

// delete_attribute
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
delete_attribute(const char *type, const char *attr)
{
	status_t err = type ? B_OK : B_BAD_VALUE;
	BNode node;
	if (!err)
		err = open_type(type, &node);
	if (!err)
		err = node.RemoveAttr(attr);
	return err;
}

#endif	// 0


} // namespace Mime
} // namespace Storage
} // namespace BPrivate

