/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _MIME_APP_META_MIME_CREATOR_H
#define _MIME_APP_META_MIME_CREATOR_H


#include <SupportDefs.h>

#include <MergedDirectory.h>


struct entry_ref;


namespace BPrivate {
namespace Storage {
namespace Mime {


class Database;


class AppMetaMimeCreator {
public:
			class DatabaseLocker;

public:
								AppMetaMimeCreator(Database* database,
									DatabaseLocker* databaseLocker,
						   			int32 force);
								~AppMetaMimeCreator();

			status_t			Do(const entry_ref& entry, bool* _entryIsDir);

private:
			Database*			fDatabase;
			DatabaseLocker*		fDatabaseLocker;
			int32				fForce;

};


class AppMetaMimeCreator::DatabaseLocker {
public:
	virtual						~DatabaseLocker();

	virtual	bool				Lock() = 0;
	virtual	void				Unlock() = 0;
};


} // namespace Mime
} // namespace Storage
} // namespace BPrivate


#endif	// _MIME_APP_META_MIME_CREATOR_H
