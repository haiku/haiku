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

namespace BPrivate {
namespace Storage {
namespace Mime {

CreateAppMetaMimeThread::CreateAppMetaMimeThread(const char *name, int32 priority,
	BMessenger managerMessenger, const entry_ref *root, bool recursive, bool force, BMessage *replyee)
	: MimeUpdateThread(name, priority, managerMessenger, root, recursive, force, replyee)
{
}

status_t
CreateAppMetaMimeThread::DoMimeUpdate(const entry_ref *entry, bool *entryIsDir)
{
	status_t err = entry ? B_OK : B_BAD_VALUE;
	bool doUpdate = false;
	BNode node;
	BString sig;

	if (!err)
		err = node.SetTo(entry);
	// Read the app sig (which consequently keeps us from updating non-applications)
	if (!err) 
		err = node.ReadAttrString("BEOS:APP_SIG", &sig);
	if (!err && !fForce) {
		// If not forced, only update if the entry has no file type attribute
//		attr_info info;
//		if (!err)
//			doUpdate = node.GetAttrInfo(kFileTypeAttr, &info) == B_ENTRY_NOT_FOUND;
	}
	if (!err && doUpdate) {
		BMimeType mime;
		BPath path;
		attr_info info;
		BBitmap miniIcon(BRect(0,0,15,15), B_CMAP8);
		
		// Init our various objects
		err = mime.SetTo(sig.String());
		if (!err)
			err = path.SetTo(entry);
			
		// Preferred App
		if (!err)
			err = mime.SetPreferredApp(sig.String());
		// Short Description
		if (!err)
			err = mime.SetShortDescription(path.Leaf());
		// App Hint
		if (!err)
			err = mime.SetAppHint(entry);
		// Mini icon
		if (!err)
			err = node.GetAttrInfo("BEOS:M:STD_ICON", &info);
		if (!err)
			err = info.size == 16*16 ? B_OK : B_BAD_VALUE;
		if (!err) {
			ssize_t bytes = node.ReadAttr("BEOS:M:STD_ICON", kMiniIconType, 0, miniIcon.Bits(), 16*16);
			err = bytes == 16*16 ? B_OK : B_FILE_ERROR;
		}
		if (!err)
			err = mime.SetIcon(&miniIcon, B_MINI_ICON);		
	}
	if (!err && entryIsDir)
		*entryIsDir = node.IsDirectory();
	return err;
}

}	// namespace Mime
}	// namespace Storage
}	// namespace BPrivate

