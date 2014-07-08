/*
 * Copyright 2002-2014 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Rene Gollent, rene@gollent.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <mime/DatabaseLocation.h>

#include <stdlib.h>
#include <syslog.h>

#include <new>

#include <Bitmap.h>
#include <DataIO.h>
#include <Directory.h>
#include <File.h>
#include <fs_attr.h>
#include <IconUtils.h>
#include <Message.h>
#include <Node.h>

#include <AutoDeleter.h>
#include <mime/database_support.h>


namespace BPrivate {
namespace Storage {
namespace Mime {


DatabaseLocation::DatabaseLocation()
	:
	fDirectories()
{
}


DatabaseLocation::~DatabaseLocation()
{
}


bool
DatabaseLocation::AddDirectory(const BString& directory)
{
	return !directory.IsEmpty() && fDirectories.Add(directory);
}


/*!	Opens a BNode on the given type, failing if the type has no
	corresponding file in the database.

	\param type The MIME type to open.
	\param _node Node opened on the given MIME type.
*/
status_t
DatabaseLocation::OpenType(const char* type, BNode& _node) const
{
	if (type == NULL)
		return B_BAD_VALUE;

	int32 index;
	return _OpenType(type, _node, index);
}


/*!	Opens a BNode on the given type, creating a node of the
	appropriate flavor if requested (and necessary).

	All MIME types are converted to lowercase for use in the filesystem.

	\param type The MIME type to open.
	\param _node Node opened on the given MIME type.
	\param _didCreate If not \c NULL, the variable the pointer refers to is
	       set to \c true, if the node has been newly created, to \c false
	       otherwise.

	\return A status code.
*/
status_t
DatabaseLocation::OpenWritableType(const char* type, BNode& _node, bool create,
	bool* _didCreate) const
{
	if (_didCreate)
		*_didCreate = false;

	// See, if the type already exists.
	int32 index;
	status_t result = _OpenType(type, _node, index);
	if (result == B_OK) {
		if (index == 0)
			return B_OK;
		else if (!create)
			return B_ENTRY_NOT_FOUND;

		// The caller wants a editable node, but the node found is not in the
		// user's settings directory. Copy the node.
		BNode nodeToClone(_node);
		if (nodeToClone.InitCheck() != B_OK)
			return nodeToClone.InitCheck();

		result = _CopyTypeNode(nodeToClone, type, _node);
		if (result != B_OK) {
			_node.Unset();
			return result;
		}

		if (_didCreate != NULL)
			*_didCreate = true;

		return result;
	} else if (!create)
		return B_ENTRY_NOT_FOUND;

	// type doesn't exist yet -- create the respective node
	result = _CreateTypeNode(type, _node);
	if (result != B_OK)
		return result;

	// write the type attribute
	size_t toWrite = strlen(type) + 1;
	ssize_t bytesWritten = _node.WriteAttr(kTypeAttr, B_STRING_TYPE, 0, type,
		toWrite);
	if (bytesWritten < 0)
		result = bytesWritten;
	else if ((size_t)bytesWritten != toWrite)
		result = B_FILE_ERROR;

	if (result != B_OK) {
		_node.Unset();
		return result;
	}

	if (_didCreate != NULL)
		*_didCreate = true;
	return B_OK;
}


/*! Reads up to \c length bytes of the given data from the given attribute
	for the given MIME type.

	If no entry for the given type exists in the database, the function fails,
	and the contents of \c data are undefined.

	\param type The MIME type.
	\param attribute The attribute name.
	\param data Pointer to a memory buffer into which the data should be copied.
	\param length The maximum number of bytes to read.
	\param datatype The expected data type.

	\return If successful, the number of bytes read is returned, otherwise, an
	        error code is returned.
*/
ssize_t
DatabaseLocation::ReadAttribute(const char* type, const char* attribute,
	void* data, size_t length, type_code datatype) const
{
	if (type == NULL || attribute == NULL || data == NULL)
		return B_BAD_VALUE;

	BNode node;
	status_t result = OpenType(type, node);
	if (result != B_OK)
		return result;

	return node.ReadAttr(attribute, datatype, 0, data, length);
}


/*!	Reads a flattened BMessage from the given attribute of the given
	MIME type.

	If no entry for the given type exists in the database, or if the data
	stored in the attribute is not a flattened BMessage, the function fails
	and the contents of \c msg are undefined.

	\param type The MIME type.
	\param attribute The attribute name.
	\param data Reference to a pre-allocated BMessage into which the attribute
	       data is unflattened.

	\return A status code.
*/
status_t
DatabaseLocation::ReadMessageAttribute(const char* type, const char* attribute,
	BMessage& _message) const
{
	if (type == NULL || attribute == NULL)
		return B_BAD_VALUE;

	BNode node;
	attr_info info;

	status_t result = OpenType(type, node);
	if (result != B_OK)
		return result;

	result = node.GetAttrInfo(attribute, &info);
	if (result != B_OK)
		return result;

	if (info.type != B_MESSAGE_TYPE)
		return B_BAD_VALUE;

	void* buffer = malloc(info.size);
	if (buffer == NULL)
		return B_NO_MEMORY;
	MemoryDeleter bufferDeleter(buffer);

	ssize_t bytesRead = node.ReadAttr(attribute, B_MESSAGE_TYPE, 0, buffer,
		info.size);
	if (bytesRead != info.size)
		return bytesRead < 0 ? (status_t)bytesRead : (status_t)B_FILE_ERROR;

	return _message.Unflatten((const char*)buffer);
}


/*!	Reads a BString from the given attribute of the given MIME type.

	If no entry for the given type exists in the database, the function fails
	and the contents of \c str are undefined.

	\param type The MIME type.
	\param attribute The attribute name.
	\param _string Reference to a pre-allocated BString into which the attribute
	       data stored.

	\return A status code.
*/
status_t
DatabaseLocation::ReadStringAttribute(const char* type, const char* attribute,
	BString& _string) const
{
	if (type == NULL || attribute == NULL)
		return B_BAD_VALUE;

	BNode node;
	status_t result = OpenType(type, node);
	if (result != B_OK)
		return result;

	return node.ReadAttrString(attribute, &_string);
}


/*!	Writes \c len bytes of the given data to the given attribute
	for the given MIME type.

	If no entry for the given type exists in the database, it is created.

	\param type The MIME type.
	\param attribute The attribute name.
	\param data Pointer to the data to write.
	\param length The number of bytes to write.
	\param datatype The data type of the given data.

	\return A status code.
*/
status_t
DatabaseLocation::WriteAttribute(const char* type, const char* attribute,
	const void* data, size_t length, type_code datatype, bool* _didCreate) const
{
	if (type == NULL || attribute == NULL || data == NULL)
		return B_BAD_VALUE;

	BNode node;
	status_t result = OpenWritableType(type, node, true, _didCreate);
	if (result != B_OK)
		return result;

	ssize_t bytesWritten = node.WriteAttr(attribute, datatype, 0, data, length);
	if (bytesWritten < 0)
		return bytesWritten;
	return bytesWritten == (ssize_t)length
		? (status_t)B_OK : (status_t)B_FILE_ERROR;
}


/*! Flattens the given \c BMessage and writes it to the given attribute
	of the given MIME type.

	If no entry for the given type exists in the database, it is created.

	\param type The MIME type.
	\param attribute The attribute name.
	\param message The BMessage to flatten and write.

	\return A status code.
*/
status_t
DatabaseLocation::WriteMessageAttribute(const char* type, const char* attribute,
	const BMessage& message, bool* _didCreate) const
{
	BMallocIO data;
	status_t result = data.SetSize(message.FlattenedSize());
	if (result != B_OK)
		return result;

	ssize_t bytes;
	result = message.Flatten(&data, &bytes);
	if (result != B_OK)
		return result;

	return WriteAttribute(type, attribute, data.Buffer(), data.BufferLength(),
		B_MESSAGE_TYPE, _didCreate);
}


/*!	Deletes the given attribute for the given type

	\param type The mime type
	\param attribute The attribute name

	\return A status code, \c B_OK on success or an error code on failure.
	\retval B_OK Success.
	\retval B_ENTRY_NOT_FOUND No such type or attribute.
*/
status_t
DatabaseLocation::DeleteAttribute(const char* type, const char* attribute) const
{
	if (type == NULL || attribute == NULL)
		return B_BAD_VALUE;

	BNode node;
	status_t result = OpenWritableType(type, node, false);
	if (result != B_OK)
		return result;

	return node.RemoveAttr(attribute);
}


/*! Fetches the application hint for the given MIME type.

	The entry_ref pointed to by \c ref must be pre-allocated.

	\param type The MIME type of interest
	\param _ref Reference to a pre-allocated \c entry_ref struct into
	       which the location of the hint application is copied.

	\return A status code, \c B_OK on success or an error code on failure.
	\retval B_OK Success.
	\retval B_ENTRY_NOT_FOUND No app hint exists for the given type
*/
status_t
DatabaseLocation::GetAppHint(const char* type, entry_ref& _ref)
{
	if (type == NULL)
		return B_BAD_VALUE;

	char path[B_PATH_NAME_LENGTH];
	BEntry entry;
	ssize_t status = ReadAttribute(type, kAppHintAttr, path, B_PATH_NAME_LENGTH,
		kAppHintType);

	if (status >= B_OK)
		status = entry.SetTo(path);
	if (status == B_OK)
		status = entry.GetRef(&_ref);

	return status;
}


/*!	Fetches from the MIME database a BMessage describing the attributes
	typically associated with files of the given MIME type

	The attribute information is returned in a pre-allocated BMessage pointed to
	by the \c info parameter (note that the any prior contents of the message
	will be destroyed). Please see BMimeType::SetAttrInfo() for a description
	of the expected format of such a message.

	\param _info Reference to a pre-allocated BMessage into which information
	       about the MIME type's associated file attributes is stored.

	\return A status code, \c B_OK on success or an error code on failure.
*/
status_t
DatabaseLocation::GetAttributesInfo(const char* type, BMessage& _info)
{
	status_t result = ReadMessageAttribute(type, kAttrInfoAttr, _info);

	if (result == B_ENTRY_NOT_FOUND) {
		// return an empty message
		_info.MakeEmpty();
		result = B_OK;
	}

	if (result == B_OK) {
		_info.what = 233;
			// Don't know why, but that's what R5 does.
		result = _info.AddString("type", type);
	}

	return result;
}


/*!	Fetches the short description for the given MIME type.

	The string pointed to by \c description must be long enough to
	hold the short description; a length of \c B_MIME_TYPE_LENGTH is
	recommended.

	\param type The MIME type of interest
	\param description Pointer to a pre-allocated string into which the short
	       description is copied. If the function fails, the contents of the
	       string are undefined.

	\return A status code, \c B_OK on success or an error code on failure.
	\retval B_OK Success.
	\retval B_ENTRY_NOT_FOUND No short description exists for the given type.
*/
status_t
DatabaseLocation::GetShortDescription(const char* type, char* description)
{
	ssize_t result = ReadAttribute(type, kShortDescriptionAttr, description,
		B_MIME_TYPE_LENGTH, kShortDescriptionType);

	return result >= 0 ? B_OK : result;
}


/*!	Fetches the long description for the given MIME type.

	The string pointed to by \c description must be long enough to
	hold the long description; a length of \c B_MIME_TYPE_LENGTH is
	recommended.

	\param type The MIME type of interest
	\param description Pointer to a pre-allocated string into which the long
	       description is copied. If the function fails, the contents of the
	       string are undefined.

	\return A status code, \c B_OK on success or an error code on failure.
	\retval B_OK Success.
	\retval B_ENTRY_NOT_FOUND No long description exists for the given type
*/
status_t
DatabaseLocation::GetLongDescription(const char* type, char* description)
{
	ssize_t result = ReadAttribute(type, kLongDescriptionAttr, description,
		B_MIME_TYPE_LENGTH, kLongDescriptionType);

	return result >= 0 ? B_OK : result;
}


/*!	Fetches a BMessage describing the MIME type's associated filename
	extensions.

	The list of extensions is returned in a pre-allocated BMessage pointed to
	by the \c extensions parameter (note that the any prior contents of the
	message will be destroyed). Please see BMimeType::GetFileExtensions() for
	a description of the message format.

	\param extensions Reference to a pre-allocated BMessage into which the MIME
	       type's associated file extensions will be stored.

	\return A status code, \c B_OK on success or an error code on failure.
*/
status_t
DatabaseLocation::GetFileExtensions(const char* type, BMessage& _extensions)
{
	status_t result = ReadMessageAttribute(type, kFileExtensionsAttr, _extensions);
	if (result == B_ENTRY_NOT_FOUND) {
		// return an empty message
		_extensions.MakeEmpty();
		result = B_OK;
	}

	if (result == B_OK) {
		_extensions.what = 234;	// Don't know why, but that's what R5 does.
		result = _extensions.AddString("type", type);
	}

	return result;
}


/*!	Fetches the icon of given size associated with the given MIME type.

	The bitmap pointed to by \c icon must be of the proper size (\c 32x32
	for \c B_LARGE_ICON, \c 16x16 for \c B_MINI_ICON) and color depth
	(\c B_CMAP8).

	\param type The mime type
	\param icon Reference to a pre-allocated bitmap of proper dimensions and
	       color depth
	\param size The size icon you're interested in (\c B_LARGE_ICON or
	       \c B_MINI_ICON)

	\return A status code.
*/
status_t
DatabaseLocation::GetIcon(const char* type, BBitmap& _icon, icon_size size)
{
	return GetIconForType(type, NULL, _icon, size);
}


/*!	Fetches the vector icon associated with the given MIME type.

	\param type The mime type
	\param _data Reference via which the allocated icon data is returned. You
	       need to free the buffer once you're done with it.
	\param _size Reference via which the size of the icon data is returned.

	\return A status code.
*/
status_t
DatabaseLocation::GetIcon(const char* type, uint8*& _data, size_t& _size)
{
	return GetIconForType(type, NULL, _data, _size);
}


/*!	Fetches the large or mini icon used by an application of this type
	for files of the given type.

	The type of the \c BMimeType object is not required to actually be a subtype
	of \c "application/"; that is the intended use however, and calling
	\c GetIconForType() on a non-application type will likely return
	\c B_ENTRY_NOT_FOUND.

	The icon is copied into the \c BBitmap pointed to by \c icon. The bitmap
	must be the proper size: \c 32x32 for the large icon, \c 16x16 for the mini
	icon.

	\param type The MIME type
	\param fileType Pointer to a pre-allocated string containing the MIME type
	       whose custom icon you wish to fetch. If NULL, works just like
	       GetIcon().
	\param icon Reference to a pre-allocated \c BBitmap of proper size and
	       colorspace into which the icon is copied.
	\param icon_size Value that specifies which icon to return. Currently
	       \c B_LARGE_ICON and \c B_MINI_ICON are supported.

	\return A status code, \c B_OK on success or an error code on failure.
	\retval B_OK Success.
	\retval B_ENTRY_NOT_FOUND No icon of the given size exists for the given type
*/
status_t
DatabaseLocation::GetIconForType(const char* type, const char* fileType,
	BBitmap& _icon, icon_size which)
{
	if (type == NULL)
		return B_BAD_VALUE;

	// open the node for the given type
	BNode node;
	status_t result = OpenType(type, node);
	if (result != B_OK)
		return result;

	// construct our attribute name
	BString vectorIconAttrName;
	BString smallIconAttrName;
	BString largeIconAttrName;

	if (fileType != NULL) {
		BString lowerCaseFileType(fileType);
		lowerCaseFileType.ToLower();

		vectorIconAttrName << kIconAttrPrefix << lowerCaseFileType;
		smallIconAttrName << kMiniIconAttrPrefix << lowerCaseFileType;
		largeIconAttrName << kLargeIconAttrPrefix << lowerCaseFileType;
	} else {
		vectorIconAttrName = kIconAttr;
		smallIconAttrName = kMiniIconAttr;
		largeIconAttrName = kLargeIconAttr;
	}

	return BIconUtils::GetIcon(&node, vectorIconAttrName, smallIconAttrName,
		largeIconAttrName, which, &_icon);
}


/*!	Fetches the vector icon used by an application of this type for files
	of the given type.

	The type of the \c BMimeType object is not required to actually be a subtype
	of \c "application/"; that is the intended use however, and calling
	\c GetIconForType() on a non-application type will likely return
	\c B_ENTRY_NOT_FOUND.

	The icon data is allocated and returned in \a _data.

	\param type The MIME type
	\param fileType Reference to a pre-allocated string containing the MIME type
	       whose custom icon you wish to fetch. If NULL, works just like
	       GetIcon().
	\param _data Reference via which the icon data is returned on success.
	\param _size Reference via which the size of the icon data is returned.

	\return A status code, \c B_OK on success or another code on failure.
	\retval B_OK Success.
	\retval B_ENTRY_NOT_FOUND No vector icon existed for the given type.
*/
status_t
DatabaseLocation::GetIconForType(const char* type, const char* fileType,
	uint8*& _data, size_t& _size)
{
	if (type == NULL)
		return B_BAD_VALUE;

	// open the node for the given type
	BNode node;
	status_t result = OpenType(type, node);
	if (result != B_OK)
		return result;

	// construct our attribute name
	BString iconAttrName;

	if (fileType != NULL)
		iconAttrName << kIconAttrPrefix << BString(fileType).ToLower();
	else
		iconAttrName = kIconAttr;

	// get info about attribute for that name
	attr_info info;
	if (result == B_OK)
		result = node.GetAttrInfo(iconAttrName, &info);

	// validate attribute type
	if (result == B_OK)
		result = (info.type == B_VECTOR_ICON_TYPE) ? B_OK : B_BAD_VALUE;

	// allocate a buffer and read the attribute data into it
	if (result == B_OK) {
		uint8* buffer = new(std::nothrow) uint8[info.size];
		if (buffer == NULL)
			result = B_NO_MEMORY;

		ssize_t bytesRead = -1;
		if (result == B_OK) {
			bytesRead = node.ReadAttr(iconAttrName, B_VECTOR_ICON_TYPE, 0, buffer,
				info.size);
		}

		if (bytesRead >= 0)
			result = bytesRead == info.size ? B_OK : B_FILE_ERROR;

		if (result == B_OK) {
			// success, set data pointer and size
			_data = buffer;
			_size = info.size;
		} else
			delete[] buffer;
	}

	return result;
}


/*!	Fetches signature of the MIME type's preferred application for the
	given action.

	The string pointed to by \c signature must be long enough to
	hold the short description; a length of \c B_MIME_TYPE_LENGTH is
	recommended.

	Currently, the only supported app verb is \c B_OPEN.

	\param type The MIME type of interest
	\param description Pointer to a pre-allocated string into which the
	       preferred application's signature is copied. If the function fails,
	       the contents of the string are undefined.
	\param verb \c The action of interest

	\return A status code, \c B_OK on success or another code on failure.
	\retval B_OK Success.
	\retval B_ENTRY_NOT_FOUND No such preferred application exists
*/
status_t
DatabaseLocation::GetPreferredApp(const char* type, char* signature,
	app_verb verb)
{
	// Since B_OPEN is the currently the only app_verb, it is essentially
	// ignored
	ssize_t result = ReadAttribute(type, kPreferredAppAttr, signature,
		B_MIME_TYPE_LENGTH, kPreferredAppType);

	return result >= 0 ? B_OK : result;
}


/*!	Fetches the sniffer rule for the given MIME type.
	\param type The MIME type of interest
	\param _result Pointer to a pre-allocated BString into which the type's
	       sniffer rule is copied.

	\return A status code, \c B_OK on success or another code on failure.
	\retval B_OK Success.
	\retval B_ENTRY_NOT_FOUND No such preferred application exists.
*/
status_t
DatabaseLocation::GetSnifferRule(const char* type, BString& _result)
{
	return ReadStringAttribute(type, kSnifferRuleAttr, _result);
}


status_t
DatabaseLocation::GetSupportedTypes(const char* type, BMessage& _types)
{
	status_t result = ReadMessageAttribute(type, kSupportedTypesAttr, _types);
	if (result == B_ENTRY_NOT_FOUND) {
		// return an empty message
		_types.MakeEmpty();
		result = B_OK;
	}
	if (result == B_OK) {
		_types.what = 0;
		result = _types.AddString("type", type);
	}

	return result;
}


//! Checks if the given MIME type is present in the database
bool
DatabaseLocation::IsInstalled(const char* type)
{
	BNode node;
	return OpenType(type, node) == B_OK;
}


BString
DatabaseLocation::_TypeToFilename(const char* type, int32 index) const
{
	BString path = fDirectories.StringAt(index);
	return path << '/' << BString(type).ToLower();
}


status_t
DatabaseLocation::_OpenType(const char* type, BNode& _node, int32& _index) const
{
	int32 count = fDirectories.CountStrings();
	for (int32 i = 0; i < count; i++) {
		status_t result = _node.SetTo(_TypeToFilename(type, i));
		attr_info attrInfo;
		if (result == B_OK && _node.GetAttrInfo(kTypeAttr, &attrInfo) == B_OK) {
			_index = i;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
DatabaseLocation::_CreateTypeNode(const char* type, BNode& _node) const
{
	const char* slash = strchr(type, '/');
	BString superTypeName;
	if (slash != NULL)
		superTypeName.SetTo(type, slash - type);
	else
		superTypeName = type;
	superTypeName.ToLower();

	// open/create the directory for the supertype
	BDirectory parent(WritableDirectory());
	status_t result = parent.InitCheck();
	if (result != B_OK)
		return result;

	BDirectory superTypeDirectory;
	if (BEntry(&parent, superTypeName).Exists())
		result = superTypeDirectory.SetTo(&parent, superTypeName);
	else
		result = parent.CreateDirectory(superTypeName, &superTypeDirectory);

	if (result != B_OK)
		return result;

	// create the subtype
	BFile subTypeFile;
	if (slash != NULL) {
		result = superTypeDirectory.CreateFile(BString(slash + 1).ToLower(),
			&subTypeFile);
		if (result != B_OK)
			return result;
	}

	// assign the result
	if (slash != NULL)
		_node = subTypeFile;
	else
		_node = superTypeDirectory;
	return _node.InitCheck();
}


status_t
DatabaseLocation::_CopyTypeNode(BNode& source, const char* type, BNode& _target)
	const
{
	status_t result = _CreateTypeNode(type, _target);
	if (result != B_OK)
		return result;

	// copy the attributes
	MemoryDeleter bufferDeleter;
	size_t bufferSize = 0;

	source.RewindAttrs();
	char attribute[B_ATTR_NAME_LENGTH];
	while (source.GetNextAttrName(attribute) == B_OK) {
		attr_info info;
		result = source.GetAttrInfo(attribute, &info);
		if (result != B_OK) {
			syslog(LOG_ERR, "Failed to get info for attribute \"%s\" of MIME "
				"type \"%s\": %s", attribute, type, strerror(result));
			continue;
		}

		// resize our buffer, if necessary
		if (info.size > (off_t)bufferSize) {
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


} // namespace Mime
} // namespace Storage
} // namespace BPrivate
