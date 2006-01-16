//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file UpdateMimeInfoThread.h
	UpdateMimeInfoThread implementation
*/

#include "mime/UpdateMimeInfoThread.h"

#include <fs_attr.h>
#include <MimeType.h>
#include <mime/database_support.h>
#include <Node.h>

namespace BPrivate {
namespace Storage {
namespace Mime {

// constructor
//! Creates a new UpdateMimeInfoThread object
UpdateMimeInfoThread::UpdateMimeInfoThread(const char *name, int32 priority,
	BMessenger managerMessenger, const entry_ref *root, bool recursive, bool force, BMessage *replyee)
	: MimeUpdateThread(name, priority, managerMessenger, root, recursive, force, replyee)
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
// TODO: This implementation is incomplete.
// For application executables we also need to copy several things from the
// resources into attributes (basically that is supported by BAppFileInfo,
// which we can use here, BTW).
// Furthermore fForce shouldn't be a boolean. The do_mime_update() API
// functions takes an "int force", which can have three valid values, 0, 1,
// and 2. 0 is no-force, 1 and 2 correspond to the mimeset flags -f and -F,
// respectively.
	status_t err = entry ? B_OK : B_BAD_VALUE;
	bool doUpdate = true;
	BNode node;

	if (!err)
		err = node.SetTo(entry);
	if (!err && entryIsDir)
		*entryIsDir = node.IsDirectory();
	if (!err && !fForce) {
		// If not forced, only update if the entry has no file type attribute
		attr_info info;
		if (!err)
			doUpdate = node.GetAttrInfo(kFileTypeAttr, &info) == B_ENTRY_NOT_FOUND;
	}
	if (!err && doUpdate) {
		BMimeType type;
		err = BMimeType::GuessMimeType(entry, &type);
		if (!err)
			err = type.InitCheck();
		if (!err) {
			const char *typeStr = type.Type();
			ssize_t len = strlen(typeStr)+1;
			ssize_t bytes = node.WriteAttr(kFileTypeAttr, kFileTypeType, 0, typeStr, len);
			if (bytes < B_OK)
				err = bytes;
			else
				err = (bytes != len ? (status_t)B_FILE_ERROR : (status_t)B_OK);
		}			
	}
	return err;
}

}	// namespace Mime
}	// namespace Storage
}	// namespace BPrivate

