/*
 * Copyright 2002-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */

/*!
	\file database_support.cpp
	Private mime database functions and constants
*/

#include <AppMisc.h>
#include <DataIO.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Entry.h>
#include <Message.h>
#include <Node.h>
#include <Path.h>
#include <storage_support.h>
#include <StringList.h>
#include <TypeConstants.h>

#include <fs_attr.h>	// For struct attr_info
#include <new>			// For new(nothrow)
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <AutoDeleter.h>

#include "mime/database_support.h"

//#define DBG(x) x
#define DBG(x)
#define OUT printf


namespace BPrivate {
namespace Storage {
namespace Mime {


#define ATTR_PREFIX "META:"
#define MINI_ICON_ATTR_PREFIX ATTR_PREFIX "M:"
#define LARGE_ICON_ATTR_PREFIX ATTR_PREFIX "L:"

const char *kMiniIconAttrPrefix		= MINI_ICON_ATTR_PREFIX;
const char *kLargeIconAttrPrefix	= LARGE_ICON_ATTR_PREFIX;
const char *kIconAttrPrefix			= ATTR_PREFIX;

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
const char *kIconAttr				= ATTR_PREFIX "ICON";
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
const int32 kIconType				= B_VECTOR_ICON_TYPE;
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

static const directory_which kBaseDirectoryConstants[] = {
	B_USER_SETTINGS_DIRECTORY,
	B_USER_NONPACKAGED_DATA_DIRECTORY,
	B_USER_DATA_DIRECTORY,
	B_COMMON_NONPACKAGED_DATA_DIRECTORY,
	B_COMMON_DATA_DIRECTORY,
	B_SYSTEM_DATA_DIRECTORY
};

static pthread_once_t sDatabaseDirectoryInitOnce = PTHREAD_ONCE_INIT;
static BStringList sDatabaseDirectories;


static void
init_database_directories()
{
	for (size_t i = 0;
		i < sizeof(kBaseDirectoryConstants)
			/ sizeof(kBaseDirectoryConstants[0]); i++) {
		BString directoryPath;
		BPath path;
		if (find_directory(kBaseDirectoryConstants[i], &path) == B_OK)
			directoryPath = path.Path();
		else if (i == 0)
			directoryPath = "/boot/home/config/settings";
		else
			continue;

		directoryPath += "/mime_db";
		sDatabaseDirectories.Add(directoryPath);
	}
}


const BStringList&
get_database_directories()
{
	pthread_once(&sDatabaseDirectoryInitOnce, &init_database_directories);
	return sDatabaseDirectories;
}


BString
get_writable_database_directory()
{
	return get_database_directories().StringAt(0);
}


static BString
type_to_filename(const char* type, int32 index)
{
	BString path = get_database_directories().StringAt(index);
	return path << '/' << BPrivate::Storage::to_lower(type).c_str();
}


/*! Converts the given MIME type to an absolute path in the user writeable MIME
	database directory.
*/
BString
type_to_writable_filename(const char* type)
{
	return type_to_filename(type, 0);
}


static status_t
open_type(const char* type, BNode* _node, int32& _index)
{
	const BStringList& directories = get_database_directories();
	int32 count = directories.CountStrings();
	for (int32 i = 0; i < count; i++) {
		status_t error = _node->SetTo(type_to_filename(type, i));
		attr_info attrInfo;
		if (error == B_OK && _node->GetAttrInfo(kTypeAttr, &attrInfo) == B_OK) {
			_index = i;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


static status_t
create_type_node(const char* type, BNode& _result)
{
	const char* slash = strchr(type, '/');
	BString superTypeName;
	if (slash != NULL)
		superTypeName.SetTo(type, slash - type);
	else
		superTypeName = type;
	superTypeName.ToLower();

	// open/create the directory for the supertype
	BDirectory parent(get_writable_database_directory());
	status_t error = parent.InitCheck();
	if (error != B_OK)
		return error;

	BDirectory superTypeDirectory;
	if (BEntry(&parent, superTypeName).Exists())
		error = superTypeDirectory.SetTo(&parent, superTypeName);
	else
		error = parent.CreateDirectory(superTypeName, &superTypeDirectory);
	if (error != B_OK)
		return error;

	// create the subtype
	BFile subTypeFile;
	if (slash != NULL) {
		error = superTypeDirectory.CreateFile(BString(slash + 1).ToLower(),
			&subTypeFile);
		if (error != B_OK)
			return error;
	}

	// assign the result
	if (slash != NULL)
		_result = subTypeFile;
	else
		_result = superTypeDirectory;
	return _result.InitCheck();
}


static status_t
copy_type_node(BNode& source, const char* type, BNode& _target)
{
	status_t error = create_type_node(type, _target);
	if (error != B_OK)
		return error;

	// copy the attributes
	MemoryDeleter bufferDeleter;
	size_t bufferSize = 0;

	source.RewindAttrs();
	char attribute[B_ATTR_NAME_LENGTH];
	while (source.GetNextAttrName(attribute) == B_OK) {
		attr_info info;
		error = source.GetAttrInfo(attribute, &info);
		if (error != B_OK) {
			syslog(LOG_ERR, "Failed to get info for attribute \"%s\" of MIME "
				"type \"%s\": %s", attribute, type, strerror(error));
			continue;
		}

		// resize our buffer, if necessary
		if (info.size > bufferSize) {
			bufferDeleter.SetTo(malloc(info.size));
			if (bufferDeleter.Get() == NULL)
				return B_NO_MEMORY;
			bufferSize = info.size;
		}

		ssize_t bytesRead = source.ReadAttr(attribute, info.type, 0,
			bufferDeleter.Get(), info.size);
		if (bytesRead != info.size) {
			syslog(LOG_ERR, "Failed to read attribute \"%s\" of MIME "
				"type \"%s\": %s", attribute, type,
		  		bytesRead < 0 ? strerror(bytesRead) : "short read");
			continue;
		}

		ssize_t bytesWritten = _target.WriteAttr(attribute, info.type, 0,
			bufferDeleter.Get(), info.size);
		if (bytesWritten < 0) {
			syslog(LOG_ERR, "Failed to write attribute \"%s\" of MIME "
				"type \"%s\": %s", attribute, type,
		  		bytesWritten < 0 ? strerror(bytesWritten) : "short write");
			continue;
		}
	}

	return B_OK;
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
	if (type == NULL || result == NULL)
		return B_BAD_VALUE;

	int32 index;
	return open_type(type, result, index);
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

	// See, if the type already exists.
	int32 index;
	status_t error = open_type(type, result, index);
	if (error == B_OK) {
		if (index == 0)
			return B_OK;

		// The caller wants a editable node, but the node found is not in the
		// user's settings directory. Copy the node.
		BNode nodeToClone(*result);
		if (nodeToClone.InitCheck() != B_OK)
			return nodeToClone.InitCheck();

		error = copy_type_node(nodeToClone, type, *result);
		if (error != B_OK) {
			result->Unset();
			return error;
		}

		if (didCreate != NULL)
			*didCreate = true;
		return error;
	}

	// type doesn't exist yet -- create the respective node
	error = create_type_node(type, *result);
	if (error != B_OK)
		return error;

	// write the type attribute
	error = result->WriteAttr(kTypeAttr, B_STRING_TYPE, 0, type,
		strlen(type) + 1);
	if (error != B_OK) {
		result->Unset();
		return error;
	}

	if (didCreate != NULL)
		*didCreate = true;
	return B_OK;
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



} // namespace Mime
} // namespace Storage
} // namespace BPrivate

