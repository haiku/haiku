/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _ENTRY_OPERATION_ENGINE_BASE_H
#define _ENTRY_OPERATION_ENGINE_BASE_H


#include <String.h>


class BDirectory;
class BEntry;
class BPath;

struct entry_ref;


namespace BPrivate {


class BEntryOperationEngineBase {
public:
			class Entry;
};


class BEntryOperationEngineBase::Entry {
public:
								Entry(const char* path);
								Entry(const BDirectory& directory,
									const char* path = NULL);
								Entry(const BEntry& entry);
								Entry(const entry_ref& entryRef);
								~Entry();

			status_t			GetPath(BPath& buffer, const char*& _path)
									const;
			BString				Path() const;

private:
			const BDirectory*	fDirectory;
			const char*			fPath;
			const BEntry*		fEntry;
			const entry_ref*	fEntryRef;
};


} // namespace BPrivate


#endif	// _ENTRY_OPERATION_ENGINE_BASE_H
