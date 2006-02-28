/*
 * Copyright 2002-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "mime/CreateAppMetaMimeThread.h"
#include "mime/database_support.h"

#include <AppFileInfo.h>
#include <Bitmap.h>
#include <fs_attr.h>
#include <MimeType.h>
#include <Node.h>
#include <Path.h>
#include <String.h>

#include <stdio.h>


namespace BPrivate {
namespace Storage {
namespace Mime {


CreateAppMetaMimeThread::CreateAppMetaMimeThread(const char *name,
	int32 priority, BMessenger managerMessenger, const entry_ref *root,
	bool recursive, int32 force, BMessage *replyee)
	: MimeUpdateThread(name, priority, managerMessenger, root, recursive, force,
		replyee)
{
}


status_t
CreateAppMetaMimeThread::DoMimeUpdate(const entry_ref* ref, bool* _entryIsDir)
{
	if (ref == NULL)
		return B_BAD_VALUE;

	BNode typeNode;

	BFile file;
	status_t status = file.SetTo(ref, B_READ_ONLY);
	if (status < B_OK)
		return status;

	BAppFileInfo appInfo(&file);
	status = appInfo.InitCheck();
	if (status < B_OK)
		return status;

	BPath path;
	status = path.SetTo(ref);
	if (status < B_OK)
		return status;

	// Read the app sig (which consequently keeps us from updating
	// non-applications, since we get an error if the file has no
	// app sig)
	BString signature;
	status = file.ReadAttrString("BEOS:APP_SIG", &signature);
	if (status < B_OK)
		return B_BAD_TYPE;

	signature.ToLower();
		// Signatures and MIME types are case insensitive

	// Init our various objects

	BMimeType mime;
	status = mime.SetTo(signature.String());
	if (status < B_OK)
		return status;

	if (!mime.IsInstalled())
		mime.Install();

	char metaMimePath[B_PATH_NAME_LENGTH];
	sprintf(metaMimePath, "%s/%s", kDatabaseDir.c_str(), signature.String());

	status = typeNode.SetTo(metaMimePath);
	if (status < B_OK)
		return status;

	// Preferred App
	attr_info info;
	if (status == B_OK && (fForce || typeNode.GetAttrInfo(kPreferredAppAttr, &info) != B_OK)) {
		status = mime.SetPreferredApp(signature.String());
	}
	// Short Description (name of the application)
	if (status == B_OK && (fForce || typeNode.GetAttrInfo(kShortDescriptionAttr, &info) != B_OK)) {
		status = mime.SetShortDescription(path.Leaf());
	}
	// App Hint
	if (status == B_OK && (fForce || typeNode.GetAttrInfo(kAppHintAttr, &info) != B_OK)) {
		status = mime.SetAppHint(ref);
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
		if (appInfo.GetIcon(&miniIcon, B_LARGE_ICON) == B_OK)
			status = mime.SetIcon(&miniIcon, B_LARGE_ICON);
	}

	// Supported Types
	BMessage supportedTypes;
	if (status == B_OK && (fForce || typeNode.GetAttrInfo(kSupportedTypesAttr, &info) != B_OK)) {
		if (appInfo.GetSupportedTypes(&supportedTypes) == B_OK)
			status = mime.SetSupportedTypes(&supportedTypes);
	}

	// Icons for supported types
	const char* type;
	for (int32 i = 0; supportedTypes.FindString("types", i, &type) == B_OK; i++) {
		// mini icon
		if (status == B_OK && appInfo.GetIconForType(type, &miniIcon, B_MINI_ICON) == B_OK)
			status = mime.SetIconForType(type, &miniIcon, B_MINI_ICON);
		// large icon
		if (status == B_OK && appInfo.GetIconForType(type, &largeIcon, B_LARGE_ICON) == B_OK)
			status = mime.SetIconForType(type, &largeIcon, B_LARGE_ICON);
	}

	if (status == B_OK && _entryIsDir != NULL)
		*_entryIsDir = file.IsDirectory();

	return status;
}

}	// namespace Mime
}	// namespace Storage
}	// namespace BPrivate

