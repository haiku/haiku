/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <mime/AppMetaMimeCreator.h>


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


} // namespace Mime
} // namespace Storage
} // namespace BPrivate
