/*
 * Copyright 2002-2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include "CreateAppMetaMimeThread.h"


namespace BPrivate {
namespace Storage {
namespace Mime {


CreateAppMetaMimeThread::CreateAppMetaMimeThread(const char* name,
	int32 priority, Database* database,
	MimeEntryProcessor::DatabaseLocker* databaseLocker,
	BMessenger managerMessenger, const entry_ref* root, bool recursive,
	int32 force, BMessage* replyee)
	:
	MimeUpdateThread(name, priority, database, managerMessenger, root,
		recursive, force, replyee),
	fCreator(database, databaseLocker, force)
{
}


status_t
CreateAppMetaMimeThread::DoMimeUpdate(const entry_ref* ref, bool* _entryIsDir)
{
	if (ref == NULL)
		return B_BAD_VALUE;

	return fCreator.Do(*ref, _entryIsDir);
}


}	// namespace Mime
}	// namespace Storage
}	// namespace BPrivate

