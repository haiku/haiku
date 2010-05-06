/*
 * Copyright 2002-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "CreateAppMetaMimeThread.h"

#include <stdio.h>
#include <stdlib.h>

#include <AppFileInfo.h>
#include <Bitmap.h>
#include <fs_attr.h>
#include <MimeType.h>
#include <Node.h>
#include <Path.h>
#include <String.h>

#include <mime/database_support.h>

#include "Database.h"


namespace BPrivate {
namespace Storage {
namespace Mime {


CreateAppMetaMimeThread::CreateAppMetaMimeThread(const char *name,
	int32 priority, Database *database, BMessenger managerMessenger,
	const entry_ref *root, bool recursive, int32 force, BMessage *replyee)
	: MimeUpdateThread(name, priority, database, managerMessenger, root,
		recursive, force, replyee)
{
}


status_t
CreateAppMetaMimeThread::DoMimeUpdate(const entry_ref* ref, bool* _entryIsDir)
{
	if (ref == NULL)
		return B_BAD_VALUE;

	BNode typeNode;

	BFile file;
	status_t status = file.SetTo(ref, B_READ_ONLY | O_NOTRAVERSE);
	if (status < B_OK)
		return status;

	bool isDir = file.IsDirectory();
	if (_entryIsDir != NULL)
		*_entryIsDir = isDir;

	if (isDir || !file.IsFile())
		return B_OK;

	BAppFileInfo appInfo(&file);
	status = appInfo.InitCheck();
	if (status < B_OK)
		return status;

	// Read the app sig (which consequently keeps us from updating
	// non-applications, since we get an error if the file has no
	// app sig)
	BString signature;
	status = file.ReadAttrString("BEOS:APP_SIG", &signature);
	if (status < B_OK)
		return B_BAD_TYPE;

	// Init our various objects

	BMimeType mime;
	status = mime.SetTo(signature.String());
	if (status < B_OK)
		return status;

	InstallNotificationDeferrer _(fDatabase, signature.String());

	if (!mime.IsInstalled())
		mime.Install();

	BString path = "/";
	path.Append(signature);
	path.ToLower();
		// Signatures and MIME types are case insensitive, but we want to
		// preserve the case wherever possible
	path.Prepend(get_database_directory().c_str());

	status = typeNode.SetTo(path.String());
	if (status < B_OK)
		return status;

	// Preferred App
	attr_info info;
	if (status == B_OK && (fForce || typeNode.GetAttrInfo(kPreferredAppAttr, &info) != B_OK))
		status = mime.SetPreferredApp(signature.String());

	// Short Description (name of the application)
	if (status == B_OK && (fForce || typeNode.GetAttrInfo(kShortDescriptionAttr, &info) != B_OK))
		status = mime.SetShortDescription(ref->name);

	// App Hint
	if (status == B_OK && (fForce || typeNode.GetAttrInfo(kAppHintAttr, &info) != B_OK))
		status = mime.SetAppHint(ref);

	// Vector Icon
	if (status == B_OK && (fForce || typeNode.GetAttrInfo(kIconAttr, &info) != B_OK)) {
		uint8* data = NULL;
		size_t size = 0;
		if (appInfo.GetIcon(&data, &size) == B_OK) {
			status = mime.SetIcon(data, size);
			free(data);
		}
	}
	// Mini Icon
	BBitmap miniIcon(BRect(0, 0, 15, 15), B_BITMAP_NO_SERVER_LINK, B_CMAP8);
	if (status == B_OK && (fForce || typeNode.GetAttrInfo(kMiniIconAttr, &info) != B_OK)) {
		if (appInfo.GetIcon(&miniIcon, B_MINI_ICON) == B_OK)
			status = mime.SetIcon(&miniIcon, B_MINI_ICON);
	}
	// Large Icon
	BBitmap largeIcon(BRect(0, 0, 31, 31), B_BITMAP_NO_SERVER_LINK, B_CMAP8);
	if (status == B_OK && (fForce || typeNode.GetAttrInfo(kLargeIconAttr, &info) != B_OK)) {
		if (appInfo.GetIcon(&largeIcon, B_LARGE_ICON) == B_OK)
			status = mime.SetIcon(&largeIcon, B_LARGE_ICON);
	}

	// Supported Types
	bool setSupportedTypes = false;
	BMessage supportedTypes;
	if (status == B_OK && (fForce || typeNode.GetAttrInfo(kSupportedTypesAttr, &info) != B_OK)) {
		if (appInfo.GetSupportedTypes(&supportedTypes) == B_OK)
			setSupportedTypes = true;
	}

	// defer notifications for supported types
	const char* type;
	for (int32 i = 0; supportedTypes.FindString("types", i, &type) == B_OK; i++)
		fDatabase->DeferInstallNotification(type);

	// set supported types
	if (setSupportedTypes)
		status = mime.SetSupportedTypes(&supportedTypes);

	// Icons for supported types
	for (int32 i = 0; supportedTypes.FindString("types", i, &type) == B_OK; i++) {
		// vector icon
		uint8* data = NULL;
		size_t size = 0;
		if (status == B_OK && appInfo.GetIconForType(type, &data, &size) == B_OK) {
			status = mime.SetIconForType(type, data, size);
			free(data);
		}
		// mini icon
		if (status == B_OK && appInfo.GetIconForType(type, &miniIcon, B_MINI_ICON) == B_OK)
			status = mime.SetIconForType(type, &miniIcon, B_MINI_ICON);
		// large icon
		if (status == B_OK && appInfo.GetIconForType(type, &largeIcon, B_LARGE_ICON) == B_OK)
			status = mime.SetIconForType(type, &largeIcon, B_LARGE_ICON);
	}

	// undefer notifications for supported types
	for (int32 i = 0; supportedTypes.FindString("types", i, &type) == B_OK; i++)
		fDatabase->UndeferInstallNotification(type);

	return status;
}

}	// namespace Mime
}	// namespace Storage
}	// namespace BPrivate

