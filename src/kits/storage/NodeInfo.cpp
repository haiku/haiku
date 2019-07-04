/*
 * Copyright 2002-2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <NodeInfo.h>

#include <new>
#include <string.h>

#include <MimeTypes.h>
#include <Bitmap.h>
#include <Entry.h>
#include <IconUtils.h>
#include <Node.h>
#include <Path.h>
#include <Rect.h>

#include <fs_attr.h>
#include <fs_info.h>


using namespace std;


// attribute names
#define NI_BEOS "BEOS"
static const char* kNITypeAttribute			= NI_BEOS ":TYPE";
static const char* kNIPreferredAppAttribute	= NI_BEOS ":PREF_APP";
static const char* kNIAppHintAttribute		= NI_BEOS ":PPATH";
static const char* kNIMiniIconAttribute		= NI_BEOS ":M:STD_ICON";
static const char* kNILargeIconAttribute	= NI_BEOS ":L:STD_ICON";
static const char* kNIIconAttribute			= NI_BEOS ":ICON";


//	#pragma mark - BNodeInfo


BNodeInfo::BNodeInfo()
	:
	fNode(NULL),
	fCStatus(B_NO_INIT)
{
}


BNodeInfo::BNodeInfo(BNode* node)
	:
	fNode(NULL),
	fCStatus(B_NO_INIT)
{
	fCStatus = SetTo(node);
}


BNodeInfo::~BNodeInfo()
{
}


status_t
BNodeInfo::SetTo(BNode* node)
{
	fNode = NULL;
	// check parameter
	fCStatus = (node && node->InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	if (fCStatus == B_OK)
		fNode = node;

	return fCStatus;
}


status_t
BNodeInfo::InitCheck() const
{
	return fCStatus;
}


status_t
BNodeInfo::GetType(char* type) const
{
	// check parameter and initialization
	status_t result = (type ? B_OK : B_BAD_VALUE);
	if (result == B_OK && InitCheck() != B_OK)
		result = B_NO_INIT;

	// get the attribute info and check type and length of the attr contents
	attr_info attrInfo;
	if (result == B_OK)
		result = fNode->GetAttrInfo(kNITypeAttribute, &attrInfo);

	if (result == B_OK && attrInfo.type != B_MIME_STRING_TYPE)
		result = B_BAD_TYPE;

	if (result == B_OK && attrInfo.size > B_MIME_TYPE_LENGTH)
		result = B_BAD_DATA;

	// read the data
	if (result == B_OK) {
		ssize_t read = fNode->ReadAttr(kNITypeAttribute, attrInfo.type, 0,
									   type, attrInfo.size);
		if (read < 0)
			result = read;
		else if (read != attrInfo.size)
			result = B_ERROR;

		if (result == B_OK) {
			// attribute strings doesn't have to be null terminated
			type[min_c(attrInfo.size, B_MIME_TYPE_LENGTH - 1)] = '\0';
		}
	}

	return result;
}


status_t
BNodeInfo::SetType(const char* type)
{
	// check parameter and initialization
	status_t result = B_OK;
	if (result == B_OK && type && strlen(type) >= B_MIME_TYPE_LENGTH)
		result = B_BAD_VALUE;

	if (result == B_OK && InitCheck() != B_OK)
		result = B_NO_INIT;

	// write/remove the attribute
	if (result == B_OK) {
		if (type != NULL) {
			size_t toWrite = strlen(type) + 1;
			ssize_t written = fNode->WriteAttr(kNITypeAttribute,
				B_MIME_STRING_TYPE, 0, type, toWrite);
			if (written < 0)
				result = written;
			else if (written != (ssize_t)toWrite)
				result = B_ERROR;
		} else
			result = fNode->RemoveAttr(kNITypeAttribute);
	}

	return result;
}


status_t
BNodeInfo::GetIcon(BBitmap* icon, icon_size which) const
{
	const char* iconAttribute = kNIIconAttribute;
	const char* miniIconAttribute = kNIMiniIconAttribute;
	const char* largeIconAttribute = kNILargeIconAttribute;

	return BIconUtils::GetIcon(fNode, iconAttribute, miniIconAttribute,
		largeIconAttribute, which, icon);
}


status_t
BNodeInfo::SetIcon(const BBitmap* icon, icon_size which)
{
	status_t result = B_OK;

	// set some icon size related variables
	const char* attribute = NULL;
	BRect bounds;
	uint32 attrType = 0;
	size_t attrSize = 0;

	switch (which) {
		case B_MINI_ICON:
			attribute = kNIMiniIconAttribute;
			bounds.Set(0, 0, 15, 15);
			attrType = B_MINI_ICON_TYPE;
			attrSize = 16 * 16;
			break;

		case B_LARGE_ICON:
			attribute = kNILargeIconAttribute;
			bounds.Set(0, 0, 31, 31);
			attrType = B_LARGE_ICON_TYPE;
			attrSize = 32 * 32;
			break;

		default:
			result = B_BAD_VALUE;
			break;
	}

	// check parameter and initialization
	if (result == B_OK && icon != NULL
		&& (icon->InitCheck() != B_OK || icon->Bounds() != bounds)) {
		result = B_BAD_VALUE;
	}
	if (result == B_OK && InitCheck() != B_OK)
		result = B_NO_INIT;

	// write/remove the attribute
	if (result == B_OK) {
		if (icon != NULL) {
			bool otherColorSpace = (icon->ColorSpace() != B_CMAP8);
			ssize_t written = 0;
			if (otherColorSpace) {
				BBitmap bitmap(bounds, B_BITMAP_NO_SERVER_LINK, B_CMAP8);
				result = bitmap.InitCheck();
				if (result == B_OK)
					result = bitmap.ImportBits(icon);

				if (result == B_OK) {
					written = fNode->WriteAttr(attribute, attrType, 0,
						bitmap.Bits(), attrSize);
				}
			} else {
				written = fNode->WriteAttr(attribute, attrType, 0,
					icon->Bits(), attrSize);
			}
			if (result == B_OK) {
				if (written < 0)
					result = written;
				else if (written != (ssize_t)attrSize)
					result = B_ERROR;
			}
		} else {
			// no icon given => remove
			result = fNode->RemoveAttr(attribute);
		}
	}

	return result;
}


status_t
BNodeInfo::GetIcon(uint8** data, size_t* size, type_code* type) const
{
	// check params
	if (data == NULL || size == NULL || type == NULL)
		return B_BAD_VALUE;

	// check initialization
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	// get the attribute info and check type and size of the attr contents
	attr_info attrInfo;
	status_t result = fNode->GetAttrInfo(kNIIconAttribute, &attrInfo);
	if (result != B_OK)
		return result;

	// chicken out on unrealisticly large attributes
	if (attrInfo.size > 128 * 1024)
		return B_ERROR;

	// fill the params
	*type = attrInfo.type;
	*size = attrInfo.size;
	*data = new (nothrow) uint8[*size];
	if (*data == NULL)
		return B_NO_MEMORY;

	// featch the data
	ssize_t read = fNode->ReadAttr(kNIIconAttribute, *type, 0, *data, *size);
	if (read != attrInfo.size) {
		delete[] *data;
		*data = NULL;
		return B_ERROR;
	}

	return B_OK;
}


status_t
BNodeInfo::SetIcon(const uint8* data, size_t size)
{
	// check initialization
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	status_t result = B_OK;

	// write/remove the attribute
	if (data && size > 0) {
		ssize_t written = fNode->WriteAttr(kNIIconAttribute,
			B_VECTOR_ICON_TYPE, 0, data, size);
		if (written < 0)
			result = (status_t)written;
		else if (written != (ssize_t)size)
			result = B_ERROR;
	} else {
		// no icon given => remove
		result = fNode->RemoveAttr(kNIIconAttribute);
	}

	return result;
}


status_t
BNodeInfo::GetPreferredApp(char* signature, app_verb verb) const
{
	// check parameter and initialization
	status_t result = (signature && verb == B_OPEN ? B_OK : B_BAD_VALUE);
	if (result == B_OK && InitCheck() != B_OK)
		result = B_NO_INIT;

	// get the attribute info and check type and length of the attr contents
	attr_info attrInfo;
	if (result == B_OK)
		result = fNode->GetAttrInfo(kNIPreferredAppAttribute, &attrInfo);

	if (result == B_OK && attrInfo.type != B_MIME_STRING_TYPE)
		result = B_BAD_TYPE;

	if (result == B_OK && attrInfo.size > B_MIME_TYPE_LENGTH)
		result = B_BAD_DATA;

	// read the data
	if (result == B_OK) {
		ssize_t read = fNode->ReadAttr(kNIPreferredAppAttribute, attrInfo.type,
			0, signature, attrInfo.size);
		if (read < 0)
			result = read;
		else if (read != attrInfo.size)
			result = B_ERROR;

		if (result == B_OK) {
			// attribute strings doesn't have to be null terminated
			signature[min_c(attrInfo.size, B_MIME_TYPE_LENGTH - 1)] = '\0';
		}
	}

	return result;
}


status_t
BNodeInfo::SetPreferredApp(const char* signature, app_verb verb)
{
	// check parameters and initialization
	status_t result = (verb == B_OPEN ? B_OK : B_BAD_VALUE);
	if (result == B_OK && signature && strlen(signature) >= B_MIME_TYPE_LENGTH)
		result = B_BAD_VALUE;

	if (result == B_OK && InitCheck() != B_OK)
		result = B_NO_INIT;

	// write/remove the attribute
	if (result == B_OK) {
		if (signature) {
			size_t toWrite = strlen(signature) + 1;
			ssize_t written = fNode->WriteAttr(kNIPreferredAppAttribute,
				B_MIME_STRING_TYPE, 0, signature, toWrite);
			if (written < 0)
				result = written;
			else if (written != (ssize_t)toWrite)
				result = B_ERROR;
		} else
			result = fNode->RemoveAttr(kNIPreferredAppAttribute);
	}

	return result;
}


status_t
BNodeInfo::GetAppHint(entry_ref* ref) const
{
	// check parameter and initialization
	status_t result = (ref ? B_OK : B_BAD_VALUE);
	if (result == B_OK && InitCheck() != B_OK)
		result = B_NO_INIT;

	// get the attribute info and check type and length of the attr contents
	attr_info attrInfo;
	if (result == B_OK)
		result = fNode->GetAttrInfo(kNIAppHintAttribute, &attrInfo);

	// NOTE: The attribute type should be B_STRING_TYPE, but R5 uses
	// B_MIME_STRING_TYPE.
	if (result == B_OK && attrInfo.type != B_MIME_STRING_TYPE)
		result = B_BAD_TYPE;

	if (result == B_OK && attrInfo.size > B_PATH_NAME_LENGTH)
		result = B_BAD_DATA;

	// read the data
	if (result == B_OK) {
		char path[B_PATH_NAME_LENGTH];
		ssize_t read = fNode->ReadAttr(kNIAppHintAttribute, attrInfo.type, 0,
			path, attrInfo.size);
		if (read < 0)
			result = read;
		else if (read != attrInfo.size)
			result = B_ERROR;

		// get the entry_ref for the path
		if (result == B_OK) {
			// attribute strings doesn't have to be null terminated
			path[min_c(attrInfo.size, B_PATH_NAME_LENGTH - 1)] = '\0';
			result = get_ref_for_path(path, ref);
		}
	}

	return result;
}


status_t
BNodeInfo::SetAppHint(const entry_ref* ref)
{
	// check parameter and initialization
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	status_t result = B_OK;
	if (ref != NULL) {
		// write/remove the attribute
		BPath path;
		result = path.SetTo(ref);
		if (result == B_OK) {
			size_t toWrite = strlen(path.Path()) + 1;
			ssize_t written = fNode->WriteAttr(kNIAppHintAttribute,
				B_MIME_STRING_TYPE, 0, path.Path(), toWrite);
			if (written < 0)
				result = written;
			else if (written != (ssize_t)toWrite)
				result = B_ERROR;
		}
	} else
		result = fNode->RemoveAttr(kNIAppHintAttribute);

	return result;
}


status_t
BNodeInfo::GetTrackerIcon(BBitmap* icon, icon_size which) const
{
	if (icon == NULL)
		return B_BAD_VALUE;

	// set some icon size related variables
	BRect bounds;
	switch (which) {
		case B_MINI_ICON:
			bounds.Set(0, 0, 15, 15);
			break;

		case B_LARGE_ICON:
			bounds.Set(0, 0, 31, 31);
			break;

		default:
//			result = B_BAD_VALUE;
			// NOTE: added to be less strict and support scaled icons
			bounds = icon->Bounds();
			break;
	}

	// check parameters and initialization
	if (icon->InitCheck() != B_OK || icon->Bounds() != bounds)
		return B_BAD_VALUE;

	if (InitCheck() != B_OK)
		return B_NO_INIT;

	// Ask GetIcon() first.
	if (GetIcon(icon, which) == B_OK)
		return B_OK;

	// If not successful, see if the node has a type available at all.
	// If no type is available, use one of the standard types.
	status_t result = B_OK;
	char mimeString[B_MIME_TYPE_LENGTH];
	if (GetType(mimeString) != B_OK) {
		// Get the icon from a mime type...
		BMimeType type;

		struct stat stat;
		result = fNode->GetStat(&stat);
		if (result == B_OK) {
			// no type available -- get the icon for the appropriate type
			// (file/dir/etc.)
			if (S_ISREG(stat.st_mode)) {
				// is it an application (executable) or just a regular file?
				if ((stat.st_mode & S_IXUSR) != 0)
					type.SetTo(B_APP_MIME_TYPE);
				else
					type.SetTo(B_FILE_MIME_TYPE);
			} else if (S_ISDIR(stat.st_mode)) {
				// it's either a volume or just a standard directory
				fs_info info;
				if (fs_stat_dev(stat.st_dev, &info) == 0
					&& stat.st_ino == info.root) {
					type.SetTo(B_VOLUME_MIME_TYPE);
				} else
					type.SetTo(B_DIRECTORY_MIME_TYPE);
			} else if (S_ISLNK(stat.st_mode))
				type.SetTo(B_SYMLINK_MIME_TYPE);
		} else {
			// GetStat() failed. Return the icon for
			// "application/octet-stream" from the MIME database.
			type.SetTo(B_FILE_MIME_TYPE);
		}

		return type.GetIcon(icon, which);
	} else {
		// We know the mimetype of the node.
		bool success = false;

		// Get the preferred application and ask the MIME database, if that
		// application has a special icon for the node's file type.
		char signature[B_MIME_TYPE_LENGTH];
		if (GetPreferredApp(signature) == B_OK) {
			BMimeType type(signature);
			success = type.GetIconForType(mimeString, icon, which) == B_OK;
		}

		// ToDo: Confirm Tracker asks preferred app icons before asking
		// mime icons.

		BMimeType nodeType(mimeString);

		// Ask the MIME database for the preferred application for the node's
		// file type and whether this application has a special icon for the
		// type.
		if (!success && nodeType.GetPreferredApp(signature) == B_OK) {
			BMimeType type(signature);
			success = type.GetIconForType(mimeString, icon, which) == B_OK;
		}

		// Ask the MIME database whether there is an icon for the node's file
		// type.
		if (!success)
			success = nodeType.GetIcon(icon, which) == B_OK;

		// Get the super type if still no success.
		BMimeType superType;
		if (!success && nodeType.GetSupertype(&superType) == B_OK) {
			// Ask the MIME database for the preferred application for the
			// node's super type and whether this application has a special
			// icon for the type.
			if (superType.GetPreferredApp(signature) == B_OK) {
				BMimeType type(signature);
				success = type.GetIconForType(superType.Type(), icon,
					which) == B_OK;
			}
			// Get the icon of the super type itself.
			if (!success)
				success = superType.GetIcon(icon, which) == B_OK;
		}

		if (success)
			return B_OK;
	}

	return B_ERROR;
}


status_t
BNodeInfo::GetTrackerIcon(const entry_ref* ref, BBitmap* icon, icon_size which)
{
	// check ref param
	status_t result = (ref ? B_OK : B_BAD_VALUE);

	// init a BNode
	BNode node;
	if (result == B_OK)
		result = node.SetTo(ref);

	// init a BNodeInfo
	BNodeInfo nodeInfo;
	if (result == B_OK)
		result = nodeInfo.SetTo(&node);

	// let the non-static GetTrackerIcon() do the dirty work
	if (result == B_OK)
		result = nodeInfo.GetTrackerIcon(icon, which);

	return result;
}


/*!	This method is provided for binary compatibility purposes
	(for example the program "Guido" depends on it.)
*/
extern "C"
status_t
GetTrackerIcon__9BNodeInfoP9entry_refP7BBitmap9icon_size(
	BNodeInfo* nodeInfo, entry_ref* ref,
	BBitmap* bitmap, icon_size iconSize)
{
	// NOTE: nodeInfo is ignored - maybe that's wrong!
	return BNodeInfo::GetTrackerIcon(ref, bitmap, iconSize);
}


void BNodeInfo::_ReservedNodeInfo1() {}
void BNodeInfo::_ReservedNodeInfo2() {}
void BNodeInfo::_ReservedNodeInfo3() {}


#ifdef _BEOS_R5_COMPATIBLE_
/*!	Assignment operator is declared private to prevent it from being created
	automatically by the compiler.
*/
BNodeInfo&
BNodeInfo::operator=(const BNodeInfo &nodeInfo)
{
	return *this;
}


/*!	Copy constructor is declared private to prevent it from being created
	automatically by the compiler.
*/
BNodeInfo::BNodeInfo(const BNodeInfo &)
{
}
#endif
