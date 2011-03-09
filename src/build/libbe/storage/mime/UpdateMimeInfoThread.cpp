/*
 * Copyright 2002-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */

/*!
	\file UpdateMimeInfoThread.h
	UpdateMimeInfoThread implementation
*/

#include "mime/UpdateMimeInfoThread.h"

#include <AppFileInfo.h>
#include <Bitmap.h>
#include <fs_attr.h>
#include <Message.h>
#include <Messenger.h>
#include <mime/database_support.h>
#include <Node.h>
#include <Resources.h>
#include <String.h>

#if !defined(__BEOS__) || defined(__HAIKU__)
#	include <MimeType.h>
#else
#	define B_VECTOR_ICON_TYPE 'VICN'
#endif

#ifndef B_UPDATE_MIME_INFO_FORCE_UPDATE_ALL
#	define B_UPDATE_MIME_INFO_FORCE_UPDATE_ALL 2
#endif
#ifndef B_UPDATE_MIME_INFO_FORCE_KEEP_TYPE
#	define B_UPDATE_MIME_INFO_FORCE_KEEP_TYPE 1
#endif
#ifndef B_BITMAP_NO_SERVER_LINK
#	define B_BITMAP_NO_SERVER_LINK 0
#endif

namespace BPrivate {
namespace Storage {
namespace Mime {

static const char *kAppFlagsAttribute = "BEOS:APP_FLAGS";


static status_t
update_icon(BAppFileInfo &appFileInfoRead, BAppFileInfo &appFileInfoWrite,
	const char *type, BBitmap &icon, icon_size iconSize)
{
	status_t err = appFileInfoRead.GetIconForType(type, &icon, iconSize);
	if (err == B_OK)
		err = appFileInfoWrite.SetIconForType(type, &icon, iconSize);
	else if (err == B_ENTRY_NOT_FOUND || err == B_NAME_NOT_FOUND) {
		err = appFileInfoWrite.SetIconForType(type, NULL, iconSize);
#if defined(__BEOS__) && !defined(__HAIKU__)
		// gives an error if the attribute didn't exist yet...
		err = B_OK;
#endif
	}

	return err;
}


/*!
	This updates the vector icon of the file from its resources.
	Instead of putting this functionality into BAppFileInfo (as done in Haiku),
	we're doing it here, so that the mimeset executable that runs under BeOS
	can make use of it as well.
*/
static status_t
update_vector_icon(BFile& file, const char *type)
{
	// try to read icon from resources
	BResources resources(&file);

	BString name("BEOS:");

	// check type param
	if (type) {
		if (BMimeType::IsValid(type))
			name += type;
		else
			return B_BAD_VALUE;
	} else
		name += "ICON";

	size_t size;
	const void* data = resources.LoadResource(B_VECTOR_ICON_TYPE, name.String(), &size);
	if (data == NULL) {
		// remove attribute; the resources don't have an icon
		file.RemoveAttr(name.String());
		return B_OK;
	}

	// update icon

	ssize_t written = file.WriteAttr(name.String(), B_VECTOR_ICON_TYPE, 0, data, size);
	if (written < B_OK)
		return written;
	if ((size_t)written < size) {
		file.RemoveAttr(name.String());
		return B_ERROR;
	}
	return B_OK;
}


#if defined(__BEOS__) || !defined(__HAIKU__)
// BMimeType::GuessMimeType() doesn't seem to work under BeOS
status_t
guess_mime_type(const void *_buffer, int32 length, BMimeType *type)
{
	const uint8 *buffer = (const uint8*)_buffer;
	if (!buffer || !type)
		return B_BAD_VALUE;

	// we only know ELF files
	if (length >= 4 && buffer[0] == 0x7f && buffer[1] == 'E' && buffer[2] == 'L'
		&&  buffer[3] == 'F') {
		return type->SetType(B_ELF_APP_MIME_TYPE);
	}

	return type->SetType(B_FILE_MIME_TYPE);
}


status_t
guess_mime_type(const entry_ref *ref, BMimeType *type)
{
	if (!ref || !type)
		return B_BAD_VALUE;

	// get BEntry
	BEntry entry;
	status_t error = entry.SetTo(ref);
	if (error != B_OK)
		return error;

	// does entry exist?
	if (!entry.Exists())
		return B_ENTRY_NOT_FOUND;

	// check entry type
	if (entry.IsDirectory())
		return type->SetType("application/x-vnd.be-directory");
	if (entry.IsSymLink())
		return type->SetType("application/x-vnd.be-symlink");
	if (!entry.IsFile())
		return B_ERROR;

	// we have a file, read the first 4 bytes
	BFile file;
	char buffer[4];
	if (file.SetTo(ref, B_READ_ONLY) == B_OK
		&& file.Read(buffer, 4) == 4) {
		return guess_mime_type(buffer, 4, type);
	}

	// we couldn't open or read the file
	return type->SetType(B_FILE_MIME_TYPE);
}
#endif


static bool
is_shared_object_mime_type(BMimeType &type)
{
	return (type == B_APP_MIME_TYPE);
}


//	#pragma mark -


//! Creates a new UpdateMimeInfoThread object
UpdateMimeInfoThread::UpdateMimeInfoThread(const char *name, int32 priority,
	BMessenger managerMessenger, const entry_ref *root, bool recursive,
	int32 force, BMessage *replyee)
	: MimeUpdateThread(name, priority, managerMessenger, root, recursive, force,
		replyee)
{
}

// DoMimeUpdate
/*! \brief Performs an update_mime_info() update on the given entry

	If the entry has no \c BEOS:TYPE attribute, or if \c fForce is true, the
	entry is sniffed and its \c BEOS:TYPE attribute is set accordingly.
*/
status_t
UpdateMimeInfoThread::DoMimeUpdate(const entry_ref *entry, bool *entryIsDir)
{
	if (entry == NULL)
		return B_BAD_VALUE;

	bool updateType = false;
	bool updateAppInfo = false;

	BNode node;
	status_t err = node.SetTo(entry);
	if (!err && entryIsDir)
		*entryIsDir = node.IsDirectory();
	if (!err) {
		// If not forced, only update if the entry has no file type attribute
		attr_info info;
		if (fForce == B_UPDATE_MIME_INFO_FORCE_UPDATE_ALL
			|| node.GetAttrInfo(kFileTypeAttr, &info) == B_ENTRY_NOT_FOUND)
			updateType = true;

		updateAppInfo = updateType
			|| fForce == B_UPDATE_MIME_INFO_FORCE_KEEP_TYPE;
	}

	// guess the MIME type
	BMimeType type;
	if (!err && (updateType || updateAppInfo)) {
		err = BMimeType::GuessMimeType(entry, &type);
#if defined(__BEOS__) && !defined(__HAIKU__)
		// GuessMimeType() doesn't seem to work correctly under BeOS
		if (err)
			err = guess_mime_type(entry, &type);
#endif
		if (!err)
			err = type.InitCheck();
	}

	// update the MIME type
	if (!err && updateType) {
		const char *typeStr = type.Type();
		ssize_t len = strlen(typeStr) + 1;
		ssize_t bytes = node.WriteAttr(kFileTypeAttr, kFileTypeType, 0,
			typeStr, len);
		if (bytes < B_OK)
			err = bytes;
		else
			err = (bytes != len ? (status_t)B_FILE_ERROR : (status_t)B_OK);
	}

	// update the app file info attributes, if this is a shared object
	BFile file;
	BAppFileInfo appFileInfoRead;
	BAppFileInfo appFileInfoWrite;
	if (!err && updateAppInfo && node.IsFile()
		&& is_shared_object_mime_type(type)
		&& file.SetTo(entry, B_READ_WRITE) == B_OK
		&& appFileInfoRead.SetTo(&file) == B_OK
		&& appFileInfoWrite.SetTo(&file) == B_OK) {
		// we read from resources and write to attributes
		appFileInfoRead.SetInfoLocation(B_USE_RESOURCES);
		appFileInfoWrite.SetInfoLocation(B_USE_ATTRIBUTES);

		// signature
		char signature[B_MIME_TYPE_LENGTH];
		err = appFileInfoRead.GetSignature(signature);
		if (err == B_OK)
			err = appFileInfoWrite.SetSignature(signature);
		else if (err == B_ENTRY_NOT_FOUND || err == B_NAME_NOT_FOUND || err == B_BAD_VALUE) {
			// BeOS returns B_BAD_VALUE on shared libraries
			err = appFileInfoWrite.SetSignature(NULL);
#if defined(__BEOS__) && !defined(__HAIKU__)
			err = B_OK;
#endif
		}
		if (err != B_OK)
			return err;

		// catalog entry
		char catalogEntry[B_MIME_TYPE_LENGTH * 3];
		err = appFileInfoRead.GetCatalogEntry(catalogEntry);
		if (err == B_OK)
			err = appFileInfoWrite.SetCatalogEntry(catalogEntry);
		else if (err == B_ENTRY_NOT_FOUND)
			err = appFileInfoWrite.SetCatalogEntry(NULL);
		if (err != B_OK)
			return err;

		// app flags
		uint32 appFlags;
		err = appFileInfoRead.GetAppFlags(&appFlags);
		if (err == B_OK) {
			err = appFileInfoWrite.SetAppFlags(appFlags);
		} else if (err == B_ENTRY_NOT_FOUND || err == B_NAME_NOT_FOUND || err == B_BAD_VALUE) {
			file.RemoveAttr(kAppFlagsAttribute);
			err = B_OK;
		}
		if (err != B_OK)
			return err;

		// supported types
		BMessage supportedTypes;
		bool hasSupportedTypes = false;
		err = appFileInfoRead.GetSupportedTypes(&supportedTypes);
		if (err == B_OK) {
			err = appFileInfoWrite.SetSupportedTypes(&supportedTypes);
			hasSupportedTypes = true;
		} else if (err == B_ENTRY_NOT_FOUND || err == B_NAME_NOT_FOUND || err == B_BAD_VALUE) {
#if defined(__BEOS__) && !defined(__HAIKU__)
			file.RemoveAttr(kSupportedTypesAttr);
			err = B_OK;
#else
			err = appFileInfoWrite.SetSupportedTypes(NULL);
#endif
		}
		if (err != B_OK)
			return err;

		// vector icon
		err = update_vector_icon(file, NULL);
		if (err != B_OK)
			return err;

		// small icon
		BBitmap smallIcon(BRect(0, 0, 15, 15), B_BITMAP_NO_SERVER_LINK,
			B_CMAP8);
		if (smallIcon.InitCheck() != B_OK)
			return smallIcon.InitCheck();
		err = update_icon(appFileInfoRead, appFileInfoWrite, NULL, smallIcon,
			B_MINI_ICON);
		if (err != B_OK)
			return err;

		// large icon
		BBitmap largeIcon(BRect(0, 0, 31, 31), B_BITMAP_NO_SERVER_LINK,
			B_CMAP8);
		if (largeIcon.InitCheck() != B_OK)
			return largeIcon.InitCheck();
		err = update_icon(appFileInfoRead, appFileInfoWrite, NULL, largeIcon,
			B_LARGE_ICON);
		if (err != B_OK)
			return err;

		// version infos
		const version_kind versionKinds[]
			= {B_APP_VERSION_KIND, B_SYSTEM_VERSION_KIND};
		for (int i = 0; i < 2; i++) {
			version_kind kind = versionKinds[i];
			version_info versionInfo;
			err = appFileInfoRead.GetVersionInfo(&versionInfo, kind);
			if (err == B_OK)
				err = appFileInfoWrite.SetVersionInfo(&versionInfo, kind);
			else if (err == B_ENTRY_NOT_FOUND || err == B_NAME_NOT_FOUND || err == B_BAD_VALUE) {
#if !defined(HAIKU_HOST_PLATFORM_DANO) && !defined(HAIKU_HOST_PLATFORM_BEOS) && !defined(HAIKU_HOST_PLATFORM_BONE)
				// BeOS crashes when calling SetVersionInfo() with a NULL pointer
				err = appFileInfoWrite.SetVersionInfo(NULL, kind);
#else
				err = B_OK;
#endif
			}
			if (err != B_OK)
				return err;
		}

		// icons for supported types
		if (hasSupportedTypes) {
			const char *supportedType;
			for (int32 i = 0;
					supportedTypes.FindString("types", i, &supportedType) == B_OK;
				 	i++) {
				// vector icon
				err = update_vector_icon(file, supportedType);
				if (err != B_OK)
					return err;

				// small icon
				err = update_icon(appFileInfoRead, appFileInfoWrite,
					supportedType, smallIcon, B_MINI_ICON);
				if (err != B_OK)
					return err;

				// large icon
				err = update_icon(appFileInfoRead, appFileInfoWrite,
					supportedType, largeIcon, B_LARGE_ICON);
				if (err != B_OK)
					return err;
			}
		}
	}

	return err;
}

}	// namespace Mime
}	// namespace Storage
}	// namespace BPrivate
