//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file CreateAppMetaMimeThread.h
	CreateAppMetaMimeThread interface declaration
*/

#ifndef _CREATE_APP_META_MIME_THREAD_H
#define _CREATE_APP_META_MIME_THREAD_H


#include <mime/AppMetaMimeCreator.h>

#include "MimeUpdateThread.h"


namespace BPrivate {
namespace Storage {
namespace Mime {


class CreateAppMetaMimeThread : public MimeUpdateThread {
public:
								CreateAppMetaMimeThread(const char* name,
									int32 priority, Database* database,
									MimeEntryProcessor::DatabaseLocker*
										databaseLocker,
									BMessenger managerMessenger,
									const entry_ref* root, bool recursive,
									int32 force, BMessage* replyee);

	virtual	status_t			DoMimeUpdate(const entry_ref* entry,
									bool* _entryIsDir);

private:
			AppMetaMimeCreator	fCreator;
};


}	// namespace Mime
}	// namespace Storage
}	// namespace BPrivate

#endif	// _CREATE_APP_META_MIME_THREAD_H
