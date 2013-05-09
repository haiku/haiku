/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <mime/AppMetaMimeCreator.h>

#include <Directory.h>
#include <Entry.h>


namespace BPrivate {
namespace Storage {
namespace Mime {


// #pragma mark - DatabaseLocker


MimeEntryProcessor::DatabaseLocker::~DatabaseLocker()
{
}


// #pragma mark - MimeEntryProcessor


MimeEntryProcessor::MimeEntryProcessor(Database* database,
	DatabaseLocker* databaseLocker, int32 force)
	:
	fDatabase(database),
	fDatabaseLocker(databaseLocker),
	fForce(force)
{
}


MimeEntryProcessor::~MimeEntryProcessor()
{
}


status_t
MimeEntryProcessor::DoRecursively(const entry_ref& entry)
{
	bool entryIsDir = false;
	status_t error = Do(entry, &entryIsDir);
	if (error != B_OK)
		return error;

	if (entryIsDir) {
		BDirectory directory;
		error = directory.SetTo(&entry);
		if (error != B_OK)
			return error;

		entry_ref childEntry;
		while (directory.GetNextRef(&childEntry) == B_OK)
			DoRecursively(childEntry);
	}

	return B_OK;
}


} // namespace Mime
} // namespace Storage
} // namespace BPrivate
