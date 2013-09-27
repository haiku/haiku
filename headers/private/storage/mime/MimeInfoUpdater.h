/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _MIME_MIME_INFO_UPDATER_H
#define _MIME_MIME_INFO_UPDATER_H


#include <mime/MimeEntryProcessor.h>


namespace BPrivate {
namespace Storage {
namespace Mime {


class MimeInfoUpdater : public MimeEntryProcessor {
public:
								MimeInfoUpdater(Database* database,
									DatabaseLocker* databaseLocker,
						   			int32 force);
	virtual						~MimeInfoUpdater();

	virtual	status_t			Do(const entry_ref& entry, bool* _entryIsDir);
};


} // namespace Mime
} // namespace Storage
} // namespace BPrivate


#endif	// _MIME_MIME_INFO_UPDATER_H
