/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
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
struct node_ref;


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
								Entry(const node_ref& directoryRef,
									const char* path = NULL);
								~Entry();

			status_t			GetPath(BPath& buffer, const char*& _path)
									const;
			BString				Path() const;

			status_t			GetPathOrName(BString& _path) const;
									// Tries to return some kind of string
									// representation. Useful only for debugging
									// and error reporting.
			BString				PathOrName() const;

private:
			const BDirectory*	fDirectory;
			const char*			fPath;
			const BEntry*		fEntry;
			const entry_ref*	fEntryRef;
			const node_ref*		fDirectoryRef;
};


} // namespace BPrivate


#endif	// _ENTRY_OPERATION_ENGINE_BASE_H
