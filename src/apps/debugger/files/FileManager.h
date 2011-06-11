/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <Locker.h>
#include <String.h>

#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>


class LocatableEntry;
class LocatableFile;
class SourceFile;


class FileManager {
public:
								FileManager();
								~FileManager();

			status_t			Init(bool targetIsLocal);

			bool				Lock()		{ return fLock.Lock(); }
			void				Unlock()	{ fLock.Unlock(); }

			LocatableFile*		GetTargetFile(const BString& directory,
									const BString& relativePath);
										// returns a reference
			LocatableFile*		GetTargetFile(const BString& path);
										// returns a reference
			void				TargetEntryLocated(const BString& path,
									const BString& locatedPath);

			LocatableFile*		GetSourceFile(const BString& directory,
									const BString& relativePath);
										// returns a reference
			LocatableFile*		GetSourceFile(const BString& path);
										// returns a reference
			void				SourceEntryLocated(const BString& path,
									const BString& locatedPath);

			status_t			LoadSourceFile(LocatableFile* file,
									SourceFile*& _sourceFile);
										// returns a reference

private:
			struct EntryPath;
			struct EntryHashDefinition;
			class Domain;
			struct SourceFileEntry;
			struct SourceFileHashDefinition;

			typedef BOpenHashTable<EntryHashDefinition> LocatableEntryTable;
			typedef DoublyLinkedList<LocatableEntry> DeadEntryList;
			typedef BOpenHashTable<SourceFileHashDefinition> SourceFileTable;

			friend struct SourceFileEntry;
				// for gcc 2

private:
			SourceFileEntry*	_LookupSourceFile(const BString& path);
			void				_SourceFileUnused(SourceFileEntry* entry);

private:
			BLocker				fLock;
			Domain*				fTargetDomain;
			Domain*				fSourceDomain;
			SourceFileTable*	fSourceFiles;
};



#endif	// FILE_MANAGER_H
