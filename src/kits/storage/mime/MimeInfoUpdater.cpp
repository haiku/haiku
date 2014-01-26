/*
 * Copyright 2002-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Rene Gollent, rene@gollent.com.
 *		Michael Lotz, mmlr@mlotz.ch
 *		Jonas Sundström, jonas@kirilla.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <mime/MimeInfoUpdater.h>

#include <stdlib.h>

#include <AppFileInfo.h>
#include <Bitmap.h>
#include <File.h>
#include <fs_attr.h>
#include <MimeType.h>
#include <String.h>

#include <AutoLocker.h>
#include <mime/Database.h>
#include <mime/database_support.h>


static const char *kAppFlagsAttribute = "BEOS:APP_FLAGS";


static status_t
update_icon(BAppFileInfo &appFileInfoRead, BAppFileInfo &appFileInfoWrite,
	const char *type, BBitmap &icon, icon_size iconSize)
{
	status_t err = appFileInfoRead.GetIconForType(type, &icon, iconSize);
	if (err == B_OK)
		err = appFileInfoWrite.SetIconForType(type, &icon, iconSize, false);
	else if (err == B_ENTRY_NOT_FOUND)
		err = appFileInfoWrite.SetIconForType(type, NULL, iconSize, false);
	return err;
}


static status_t
update_icon(BAppFileInfo &appFileInfoRead, BAppFileInfo &appFileInfoWrite,
	const char *type)
{
	uint8* data = NULL;
	size_t size = 0;

	status_t err = appFileInfoRead.GetIconForType(type, &data, &size);
	if (err == B_OK)
		err = appFileInfoWrite.SetIconForType(type, data, size, false);
	else if (err == B_ENTRY_NOT_FOUND)
		err = appFileInfoWrite.SetIconForType(type, NULL, size, false);

	free(data);

	return err;
}


static bool
is_shared_object_mime_type(const BString &type)
{
	return type.ICompare(B_APP_MIME_TYPE) == 0;
}


namespace BPrivate {
namespace Storage {
namespace Mime {


MimeInfoUpdater::MimeInfoUpdater(Database* database,
	DatabaseLocker* databaseLocker, int32 force)
	:
	MimeEntryProcessor(database, databaseLocker, force)
{
}


MimeInfoUpdater::~MimeInfoUpdater()
{
}


status_t
MimeInfoUpdater::Do(const entry_ref& entry, bool* _entryIsDir)
{
	bool updateType = false;
	bool updateAppInfo = false;
	BNode node;

	status_t err = node.SetTo(&entry);
	if (!err && _entryIsDir)
		*_entryIsDir = node.IsDirectory();
	if (!err) {
		// If not forced, only update if the entry has no file type attribute
		attr_info info;
		if (fForce == B_UPDATE_MIME_INFO_FORCE_UPDATE_ALL
			|| node.GetAttrInfo(kFileTypeAttr, &info) == B_ENTRY_NOT_FOUND) {
			updateType = true;
		}
		updateAppInfo = (updateType
			|| fForce == B_UPDATE_MIME_INFO_FORCE_KEEP_TYPE);
	}

	// guess the MIME type
	BString type;
	if (!err && (updateType || updateAppInfo)) {
		AutoLocker<DatabaseLocker> databaseLocker(fDatabaseLocker);
		err = fDatabase->GuessMimeType(&entry, &type);
	}

	// update the MIME type
	if (!err && updateType) {
		ssize_t len = type.Length() + 1;
		ssize_t bytes = node.WriteAttr(kFileTypeAttr, kFileTypeType, 0, type,
			len);
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
		&& (err = file.SetTo(&entry, B_READ_WRITE)) == B_OK
		&& (err = appFileInfoRead.SetTo(&file)) == B_OK
		&& (err = appFileInfoWrite.SetTo(&file)) == B_OK) {

		// we read from resources and write to attributes
		appFileInfoRead.SetInfoLocation(B_USE_RESOURCES);
		appFileInfoWrite.SetInfoLocation(B_USE_ATTRIBUTES);

		// signature
		char signature[B_MIME_TYPE_LENGTH];
		err = appFileInfoRead.GetSignature(signature);
		if (err == B_OK)
			err = appFileInfoWrite.SetSignature(signature);
		else if (err == B_ENTRY_NOT_FOUND)
			err = appFileInfoWrite.SetSignature(NULL);
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
		} else if (err == B_ENTRY_NOT_FOUND) {
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
			err = appFileInfoWrite.SetSupportedTypes(&supportedTypes, false,
				false);
			hasSupportedTypes = true;
		} else if (err == B_ENTRY_NOT_FOUND)
			err = appFileInfoWrite.SetSupportedTypes(NULL, false, false);
		if (err != B_OK)
			return err;

		// vector icon
		err = update_icon(appFileInfoRead, appFileInfoWrite, NULL);
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
			else if (err == B_ENTRY_NOT_FOUND)
				err = appFileInfoWrite.SetVersionInfo(NULL, kind);
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
				err = update_icon(appFileInfoRead, appFileInfoWrite,
					supportedType);
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


} // namespace Mime
} // namespace Storage
} // namespace BPrivate
