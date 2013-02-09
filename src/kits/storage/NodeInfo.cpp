/*
 * Copyright 2002-2006, Haiku Inc.
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
static const char *kNITypeAttribute			= NI_BEOS ":TYPE";
static const char *kNIPreferredAppAttribute	= NI_BEOS ":PREF_APP";
static const char *kNIAppHintAttribute		= NI_BEOS ":PPATH";
static const char *kNIMiniIconAttribute		= NI_BEOS ":M:STD_ICON";
static const char *kNILargeIconAttribute	= NI_BEOS ":L:STD_ICON";
static const char *kNIIconAttribute			= NI_BEOS ":ICON";


BNodeInfo::BNodeInfo()
	:
	fNode(NULL),
	fCStatus(B_NO_INIT)
{
}


BNodeInfo::BNodeInfo(BNode *node)
	:
	fNode(NULL),
	fCStatus(B_NO_INIT)
{
	fCStatus = SetTo(node);
}


BNodeInfo::~BNodeInfo()
{
}


// Initializes the BNodeInfo to the supplied node.
status_t
BNodeInfo::SetTo(BNode *node)
{
	fNode = NULL;
	// check parameter
	fCStatus = (node && node->InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	if (fCStatus == B_OK)
		fNode = node;

	return fCStatus;
}


// Returns whether the object has been properly initialized.
status_t
BNodeInfo::InitCheck() const
{
	return fCStatus;
}


// Writes the MIME type of the node into type.
status_t
BNodeInfo::GetType(char *type) const
{
	// check parameter and initialization
	status_t error = (type ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// get the attribute info and check type and length of the attr contents
	attr_info attrInfo;
	if (error == B_OK)
		error = fNode->GetAttrInfo(kNITypeAttribute, &attrInfo);
	if (error == B_OK && attrInfo.type != B_MIME_STRING_TYPE)
		error = B_BAD_TYPE;
	if (error == B_OK && attrInfo.size > B_MIME_TYPE_LENGTH)
		error = B_BAD_DATA;

	// read the data
	if (error == B_OK) {
		ssize_t read = fNode->ReadAttr(kNITypeAttribute, attrInfo.type, 0,
									   type, attrInfo.size);
		if (read < 0)
			error = read;
		else if (read != attrInfo.size)
			error = B_ERROR;

		if (error == B_OK) {
			// attribute strings doesn't have to be null terminated
			type[min_c(attrInfo.size, B_MIME_TYPE_LENGTH - 1)] = '\0';
		}
	}

	return error;
}

// Sets the MIME type of the node. If type is NULL the BEOS:TYPE attribute is
// removed instead.
status_t
BNodeInfo::SetType(const char *type)
{
	// check parameter and initialization
	status_t error = B_OK;
	if (error == B_OK && type && strlen(type) >= B_MIME_TYPE_LENGTH)
		error = B_BAD_VALUE;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;

	// write/remove the attribute
	if (error == B_OK) {
		if (type) {
			size_t toWrite = strlen(type) + 1;
			ssize_t written = fNode->WriteAttr(kNITypeAttribute,
											   B_MIME_STRING_TYPE, 0, type,
											   toWrite);
			if (written < 0)
				error = written;
			else if (written != (ssize_t)toWrite)
				error = B_ERROR;
		} else
			error = fNode->RemoveAttr(kNITypeAttribute);
	}
	return error;
}


// Gets the icon of the node.
status_t
BNodeInfo::GetIcon(BBitmap *icon, icon_size k) const
{
	const char* iconAttribute = kNIIconAttribute;
	const char* miniIconAttribute = kNIMiniIconAttribute;
	const char* largeIconAttribute = kNILargeIconAttribute;

	return BIconUtils::GetIcon(fNode, iconAttribute, miniIconAttribute,
							   largeIconAttribute, k, icon);

//	status_t error = B_OK;
//	// set some icon size related variables
//	const char *attribute = NULL;
//	BRect bounds;
//	uint32 attrType = 0;
//	size_t attrSize = 0;
//	switch (k) {
//		case B_MINI_ICON:
//			attribute = kNIMiniIconAttribute;
//			bounds.Set(0, 0, 15, 15);
//			attrType = B_MINI_ICON_TYPE;
//			attrSize = 16 * 16;
//			break;
//		case B_LARGE_ICON:
//			attribute = kNILargeIconAttribute;
//			bounds.Set(0, 0, 31, 31);
//			attrType = B_LARGE_ICON_TYPE;
//			attrSize = 32 * 32;
//			break;
//		default:
//			error = B_BAD_VALUE;
//			break;
//	}
//
//	// check parameter and initialization
//	if (error == B_OK
//		&& (!icon || icon->InitCheck() != B_OK || icon->Bounds() != bounds)) {
//		error = B_BAD_VALUE;
//	}
//	if (error == B_OK && InitCheck() != B_OK)
//		error = B_NO_INIT;
//
//	// get the attribute info and check type and size of the attr contents
//	attr_info attrInfo;
//	if (error == B_OK)
//		error = fNode->GetAttrInfo(attribute, &attrInfo);
//	if (error == B_OK && attrInfo.type != attrType)
//		error = B_BAD_TYPE;
//	if (error == B_OK && attrInfo.size != attrSize)
//		error = B_BAD_DATA;
//
//	// read the attribute
//	if (error == B_OK) {
//		bool otherColorSpace = (icon->ColorSpace() != B_CMAP8);
//		char *buffer = NULL;
//		ssize_t read;
//		if (otherColorSpace) {
//			// other color space than stored in attribute
//			buffer = new(nothrow) char[attrSize];
//			if (!buffer)
//				error = B_NO_MEMORY;
//			if (error == B_OK) {
//				read = fNode->ReadAttr(attribute, attrType, 0, buffer,
//									   attrSize);
//			}
//		} else {
//			read = fNode->ReadAttr(attribute, attrType, 0, icon->Bits(), 
//								   attrSize);
//		}
//		if (error == B_OK) {
//			if (read < 0)
//				error = read;
//			else if (read != attrInfo.size)
//				error = B_ERROR;
//		}
//		if (otherColorSpace) {
//			// other color space than stored in attribute
//			if (error == B_OK) {
//				error = icon->ImportBits(buffer, attrSize, B_ANY_BYTES_PER_ROW,
//										 0, B_CMAP8);
//			}
//			delete[] buffer;
//		}
//	}
//	return error;
}


// Sets the icon of the node. If icon is NULL, the attribute is removed
// instead.
status_t
BNodeInfo::SetIcon(const BBitmap *icon, icon_size k)
{
	status_t error = B_OK;
	// set some icon size related variables
	const char *attribute = NULL;
	BRect bounds;
	uint32 attrType = 0;
	size_t attrSize = 0;
	switch (k) {
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
			error = B_BAD_VALUE;
			break;
	}

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
			ssize_t written = 0;
			if (otherColorSpace) {
				BBitmap bitmap(bounds, B_BITMAP_NO_SERVER_LINK, B_CMAP8);
				error = bitmap.InitCheck();
				if (error == B_OK)
					error = bitmap.ImportBits(icon);
				if (error == B_OK) {
					written = fNode->WriteAttr(attribute, attrType, 0,
											   bitmap.Bits(), attrSize);
				}
			} else {
				written = fNode->WriteAttr(attribute, attrType, 0,
										   icon->Bits(), attrSize);
			}
			if (error == B_OK) {
				if (written < 0)
					error = written;
				else if (written != (ssize_t)attrSize)
					error = B_ERROR;
			}
		} else	// no icon given => remove
			error = fNode->RemoveAttr(attribute);
	}
	return error;
}


// Gets the icon of the node.
status_t
BNodeInfo::GetIcon(uint8** data, size_t* size, type_code* type) const
{
	// check params
	if (!data || !size || !type)
		return B_BAD_VALUE;

	// check initialization
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	// get the attribute info and check type and size of the attr contents
	attr_info attrInfo;
	status_t ret = fNode->GetAttrInfo(kNIIconAttribute, &attrInfo);
	if (ret < B_OK)
		return ret;

	// chicken out on unrealisticly large attributes
	if (attrInfo.size > 128 * 1024)
		return B_ERROR;

	// fill the params
	*type = attrInfo.type;
	*size = attrInfo.size;
	*data = new (nothrow) uint8[*size];

	if (!*data)
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


// Sets the node icon of the node. If data is NULL of size is 0, the
// "BEOS:ICON" attribute is removed instead.
status_t
BNodeInfo::SetIcon(const uint8* data, size_t size)
{
	// check initialization
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	status_t error = B_OK;

	// write/remove the attribute
	if (data && size > 0) {
		ssize_t written = fNode->WriteAttr(kNIIconAttribute,
										   B_VECTOR_ICON_TYPE,
										   0, data, size);
		if (written < 0)
			error = (status_t)written;
		else if (written != (ssize_t)size)
			error = B_ERROR;
	} else {
		// no icon given => remove
		error = fNode->RemoveAttr(kNIIconAttribute);
	}

	return error;
}


// Gets the preferred application of the node.
status_t
BNodeInfo::GetPreferredApp(char *signature, app_verb verb) const
{
	// check parameter and initialization
	status_t error = (signature && verb == B_OPEN ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;

	// get the attribute info and check type and length of the attr contents
	attr_info attrInfo;
	if (error == B_OK)
		error = fNode->GetAttrInfo(kNIPreferredAppAttribute, &attrInfo);
	if (error == B_OK && attrInfo.type != B_MIME_STRING_TYPE)
		error = B_BAD_TYPE;
	if (error == B_OK && attrInfo.size > B_MIME_TYPE_LENGTH)
		error = B_BAD_DATA;

	// read the data
	if (error == B_OK) {
		ssize_t read = fNode->ReadAttr(kNIPreferredAppAttribute, attrInfo.type,
									   0, signature, attrInfo.size);
		if (read < 0)
			error = read;
		else if (read != attrInfo.size)
			error = B_ERROR;

		if (error == B_OK) {
			// attribute strings doesn't have to be null terminated
			signature[min_c(attrInfo.size, B_MIME_TYPE_LENGTH - 1)] = '\0';
		}
	}
	return error;
}


// Sets the preferred application of the node. If signature is NULL, the
// "BEOS:PREF_APP" attribute is removed instead.
status_t
BNodeInfo::SetPreferredApp(const char *signature, app_verb verb)
{
	// check parameters and initialization
	status_t error = (verb == B_OPEN ? B_OK : B_BAD_VALUE);
	if (error == B_OK && signature && strlen(signature) >= B_MIME_TYPE_LENGTH)
		error = B_BAD_VALUE;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;

	// write/remove the attribute
	if (error == B_OK) {
		if (signature) {
			size_t toWrite = strlen(signature) + 1;
			ssize_t written = fNode->WriteAttr(kNIPreferredAppAttribute,
											   B_MIME_STRING_TYPE, 0,
											   signature, toWrite);
			if (written < 0)
				error = written;
			else if (written != (ssize_t)toWrite)
				error = B_ERROR;
		} else
			error = fNode->RemoveAttr(kNIPreferredAppAttribute);
	}
	return error;
}


// Fills out ref with a pointer to a hint about what application will open
// this node.
status_t
BNodeInfo::GetAppHint(entry_ref *ref) const
{
	// check parameter and initialization
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;

	// get the attribute info and check type and length of the attr contents
	attr_info attrInfo;
	if (error == B_OK)
		error = fNode->GetAttrInfo(kNIAppHintAttribute, &attrInfo);
	// NOTE: The attribute type should be B_STRING_TYPE, but R5 uses
	// B_MIME_STRING_TYPE.
	if (error == B_OK && attrInfo.type != B_MIME_STRING_TYPE)
		error = B_BAD_TYPE;
	if (error == B_OK && attrInfo.size > B_PATH_NAME_LENGTH)
		error = B_BAD_DATA;

	// read the data
	if (error == B_OK) {
		char path[B_PATH_NAME_LENGTH];
		ssize_t read = fNode->ReadAttr(kNIAppHintAttribute, attrInfo.type, 0,
									   path, attrInfo.size);
		if (read < 0)
			error = read;
		else if (read != attrInfo.size)
			error = B_ERROR;
		// get the entry_ref for the path
		if (error == B_OK) {
			// attribute strings doesn't have to be null terminated
			path[min_c(attrInfo.size, B_PATH_NAME_LENGTH - 1)] = '\0';
			error = get_ref_for_path(path, ref);
		}
	}
	return error;
}


// Sets the app hint of the node. If ref is NULL, the "BEOS:PPATH" attribute
// is removed instead.
status_t
BNodeInfo::SetAppHint(const entry_ref *ref)
{
	// check parameter and initialization
	status_t error = B_OK;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;

	// write/remove the attribute
	if (error == B_OK) {
		if (ref) {
			BPath path;
			error = path.SetTo(ref);
			if (error == B_OK) {
				size_t toWrite = strlen(path.Path()) + 1;
				ssize_t written = fNode->WriteAttr(kNIAppHintAttribute,
												   B_MIME_STRING_TYPE, 0,
												   path.Path(), toWrite);
				if (written < 0)
					error = written;
				else if (written != (ssize_t)toWrite)
					error = B_ERROR;
			}
		} else
			error = fNode->RemoveAttr(kNIAppHintAttribute);
	}
	return error;
}


// Gets the icon displayed by Tracker for the icon.
status_t
BNodeInfo::GetTrackerIcon(BBitmap *icon, icon_size iconSize) const
{
	if (!icon)
		return B_BAD_VALUE;

	// set some icon size related variables
	BRect bounds;
	switch (iconSize) {
		case B_MINI_ICON:
			bounds.Set(0, 0, 15, 15);
			break;
		case B_LARGE_ICON:
			bounds.Set(0, 0, 31, 31);
			break;
		default:
//			error = B_BAD_VALUE;
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
	if (GetIcon(icon, iconSize) == B_OK)
		return B_OK;

	// If not successful, see if the node has a type available at all.
	// If no type is available, use one of the standard types.
	status_t error = B_OK;
	char mimeString[B_MIME_TYPE_LENGTH];
	if (GetType(mimeString) != B_OK) {
		// Get the icon from a mime type...
		BMimeType type;
	
		struct stat stat;
		error = fNode->GetStat(&stat);
		if (error == B_OK) {
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

		return type.GetIcon(icon, iconSize);
	} else {
		// We know the mimetype of the node.
		bool success = false;

		// Get the preferred application and ask the MIME database, if that
		// application has a special icon for the node's file type.
		char signature[B_MIME_TYPE_LENGTH];
		if (GetPreferredApp(signature) == B_OK) {
			BMimeType type(signature);
			success = type.GetIconForType(mimeString, icon, iconSize) == B_OK;
		}

		// ToDo: Confirm Tracker asks preferred app icons before asking
		// mime icons.

		BMimeType nodeType(mimeString);

		// Ask the MIME database for the preferred application for the node's
		// file type and whether this application has a special icon for the
		// type.
		if (!success && nodeType.GetPreferredApp(signature) == B_OK) {
			BMimeType type(signature);
			success = type.GetIconForType(mimeString, icon, iconSize) == B_OK;
		}

		// Ask the MIME database whether there is an icon for the node's file
		// type.
		if (!success)
			success = nodeType.GetIcon(icon, iconSize) == B_OK;

		// Get the super type if still no success.
		BMimeType superType;
		if (!success && nodeType.GetSupertype(&superType) == B_OK) {
			// Ask the MIME database for the preferred application for the
			// node's super type and whether this application has a special
			// icon for the type.
			if (superType.GetPreferredApp(signature) == B_OK) {
				BMimeType type(signature);
				success = type.GetIconForType(superType.Type(), icon,
					iconSize) == B_OK;
			}
			// Get the icon of the super type itself.
			if (!success)
				success = superType.GetIcon(icon, iconSize) == B_OK;
		}
		
		if (success)
			return B_OK;
	}

	return B_ERROR;
}


// Gets the icon displayed by Tracker for the node referred to by ref.
status_t
BNodeInfo::GetTrackerIcon(const entry_ref *ref, BBitmap *icon, icon_size iconSize)
{
	// check ref param
	status_t error = (ref ? B_OK : B_BAD_VALUE);

	// init a BNode
	BNode node;
	if (error == B_OK)
		error = node.SetTo(ref);

	// init a BNodeInfo
	BNodeInfo nodeInfo;
	if (error == B_OK)
		error = nodeInfo.SetTo(&node);

	// let the non-static GetTrackerIcon() do the dirty work
	if (error == B_OK)
		error = nodeInfo.GetTrackerIcon(icon, iconSize);
	return error;
}


// NOTE: The following method is provided for binary compatibility purposes
// (for example the program "Guido" depends on this.)
extern "C"
status_t
GetTrackerIcon__9BNodeInfoP9entry_refP7BBitmap9icon_size(
	BNodeInfo *nodeInfo, entry_ref* ref,
	BBitmap* bitmap, icon_size iconSize)
{
	// NOTE: nodeInfo is ignored - maybe that's wrong!
	return BNodeInfo::GetTrackerIcon(ref, bitmap, iconSize);
}


void 
BNodeInfo::_ReservedNodeInfo1()
{
}


void 
BNodeInfo::_ReservedNodeInfo2()
{
}


void 
BNodeInfo::_ReservedNodeInfo3()
{
}


// Assignment operator is declared private to prevent it from being created
// automatically by the compiler.
BNodeInfo &
BNodeInfo::operator=(const BNodeInfo &nodeInfo)
{
	return *this;
}


// Copy constructor is declared private to prevent it from being created
// automatically by the compiler.
BNodeInfo::BNodeInfo(const BNodeInfo &)
{
}


//	#pragma mark -


namespace BPrivate {

// Private method used by Tracker. This should be moved to the Tracker
// source.
extern bool
CheckNodeIconHintPrivate(const BNode *node, bool checkMiniIconOnly)
{
	attr_info info;
	if (node->GetAttrInfo(kNIMiniIconAttribute, &info) != B_OK && checkMiniIconOnly)
		return false;

	if (node->GetAttrInfo(kNILargeIconAttribute, &info) != B_OK)
		return false;

	return true;
}

}	// namespace BPrivate
