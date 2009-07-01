/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <Locker.h>
#include <String.h>

#include <util/OpenHashTable.h>


class LocatableFile;


class FileManager {
public:
								FileManager();
								~FileManager();

			status_t			Init(bool targetIsLocal);

			bool				Lock()		{ return fLock.Lock(); }
			void				Unlock()	{ fLock.Unlock(); }

			LocatableFile*		GetTargetFile(const BString& directory,
									const BString& name);
										// returns a reference
			LocatableFile*		GetTargetFile(const BString& path);
										// returns a reference
			void				TargetEntryLocated(const BString& path,
									const BString& locatedPath);

			LocatableFile*		GetSourceFile(const BString& directory,
									const BString& name);
										// returns a reference
			LocatableFile*		GetSourceFile(const BString& path);
										// returns a reference
			void				SourceEntryLocated(const BString& path,
									const BString& locatedPath);

private:
			struct EntryPath;
			struct EntryHashDefinition;
			class Domain;

			typedef OpenHashTable<EntryHashDefinition> LocatableEntryTable;

private:
			BLocker				fLock;
			Domain*				fTargetDomain;
			Domain*				fSourceDomain;
};



#endif	// FILE_MANAGER_H
