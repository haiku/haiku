//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file CreateAppMetaMimeThread.h
	CreateAppMetaMimeThread implementation
*/

#include "mime/CreateAppMetaMimeThread.h"

#include <Bitmap.h>
#include <fs_attr.h>
#include <MimeType.h>
#include <mime/database_support.h>
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
CreateAppMetaMimeThread::DoMimeUpdate(const entry_ref *entry, bool *entryIsDir)
{
	status_t err = entry ? B_OK : B_BAD_VALUE;
	BNode node;
	BString sig;
	BMimeType mime;
	BPath path;
	attr_info info;
	BBitmap miniIcon(BRect(0,0,15,15), B_BITMAP_NO_SERVER_LINK, B_CMAP8);
	BNode typeNode;

	if (!err)
		err = node.SetTo(entry);
	if (!err && entryIsDir)
		*entryIsDir = node.IsDirectory();
	// Read the app sig (which consequently keeps us from updating
	// non-applications, since we get an error if the file has no
	// app sig)
	if (!err) 
		err = node.ReadAttrString("BEOS:APP_SIG", &sig);
	if (!err) {
		// Init our various objects
		err = mime.SetTo(sig.String());
		if (!err && !mime.IsInstalled())
			mime.Install();
		if (!err) {
			char metaMimePath[B_PATH_NAME_LENGTH];
			sprintf(metaMimePath, "%s/%s", kDatabaseDir.c_str(), sig.String());
			err = typeNode.SetTo(metaMimePath);
		}
		if (!err) 
			err = path.SetTo(entry);
			
		// Preferred App
		if (!err && (fForce || typeNode.GetAttrInfo(kPreferredAppAttr, &info) == B_ENTRY_NOT_FOUND)) {
			err = mime.SetPreferredApp(sig.String());
		}
		// Short Description
		if (!err && (fForce || typeNode.GetAttrInfo(kShortDescriptionAttr, &info) == B_ENTRY_NOT_FOUND)) {
			err = mime.SetShortDescription(path.Leaf());
		}
		// App Hint
		if (!err && (fForce || typeNode.GetAttrInfo(kAppHintAttr, &info) == B_ENTRY_NOT_FOUND)) {
			err = mime.SetAppHint(entry);
		}
		// Mini icon
		if (!err && (fForce || typeNode.GetAttrInfo(kMiniIconAttr, &info) == B_ENTRY_NOT_FOUND)) {
			err = node.GetAttrInfo("BEOS:M:STD_ICON", &info);
			if (!err)
				err = info.size == 16*16 ? B_OK : B_BAD_VALUE;
			if (!err) {
				ssize_t bytes = node.ReadAttr("BEOS:M:STD_ICON", kMiniIconType, 0, miniIcon.Bits(), 16*16);
				err = bytes == 16*16 ? (status_t)B_OK : (status_t)B_FILE_ERROR;
			}
			if (!err)
				err = mime.SetIcon(&miniIcon, B_MINI_ICON);
		}
	}
	return err;
}

}	// namespace Mime
}	// namespace Storage
}	// namespace BPrivate

