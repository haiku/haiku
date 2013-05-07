/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef DATABASE_DIRECTORY_H
#define DATABASE_DIRECTORY_H


#include <MergedDirectory.h>


namespace BPrivate {
namespace Storage {
namespace Mime {


class DatabaseLocation;


class DatabaseDirectory : public BMergedDirectory {
public:
								DatabaseDirectory();
	virtual						~DatabaseDirectory();

			status_t			Init(DatabaseLocation* databaseLocation,
									const char* superType = NULL);

protected:
	virtual	bool				ShallPreferFirstEntry(const entry_ref& entry1,
									int32 index1, const entry_ref& entry2,
									int32 index2);

private:
			bool				_IsValidMimeTypeEntry(const entry_ref& entry);
};


} // namespace Mime
} // namespace Storage
} // namespace BPrivate


#endif	// DATABASE_DIRECTORY_H
