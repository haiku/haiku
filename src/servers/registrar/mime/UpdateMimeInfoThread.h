//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file UpdateMimeInfoThread.h
	UpdateMimeInfoThread interface declaration 
*/

#ifndef _MIME_UPDATE_MIME_INFO_THREAD_H
#define _MIME_UPDATE_MIME_INFO_THREAD_H


#include <mime/MimeInfoUpdater.h>

#include "MimeUpdateThread.h"


namespace BPrivate {
namespace Storage {
namespace Mime {


class UpdateMimeInfoThread : public MimeUpdateThread {
public:
								UpdateMimeInfoThread(const char* name,
									int32 priority, Database* database,
									MimeEntryProcessor::DatabaseLocker*
										databaseLocker,
									BMessenger managerMessenger,
									const entry_ref* root, bool recursive,
									int32 force, BMessage* replyee);

	virtual	status_t			DoMimeUpdate(const entry_ref* entry,
									bool* _entryIsDir);

private:
			MimeInfoUpdater		fUpdater;
};


}	// namespace Mime
}	// namespace Storage
}	// namespace BPrivate

#endif	// _MIME_UPDATE_MIME_INFO_THREAD_H
