/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <map>

#include <Locker.h>
#include <Message.h>
#include <String.h>

#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>


class LocatableEntry;
class LocatableFile;
class SourceFile;
class TeamFileManagerSettings;


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
			status_t			SourceEntryLocated(const BString& path,
									const BString& locatedPath);

			status_t			LoadSourceFile(LocatableFile* file,
									SourceFile*& _sourceFile);
										// returns a reference

			status_t			LoadLocationMappings(TeamFileManagerSettings*
									settings);
			status_t			SaveLocationMappings(TeamFileManagerSettings*
									settings);

private:
			struct EntryPath;
			struct EntryHashDefinition;
			class Domain;
			struct SourceFileEntry;
			struct SourceFileHashDefinition;

			typedef BOpenHashTable<EntryHashDefinition> LocatableEntryTable;
			typedef DoublyLinkedList<LocatableEntry> DeadEntryList;
			typedef BOpenHashTable<SourceFileHashDefinition> SourceFileTable;
			typedef std::map<BString, BString> LocatedFileMap;

			friend struct SourceFileEntry;
				// for gcc 2

private:
			SourceFileEntry*	_LookupSourceFile(const BString& path);
			void				_SourceFileUnused(SourceFileEntry* entry);
			bool				_LocateFileIfMapped(const BString& sourcePath,
									LocatableFile* file);

private:
			BLocker				fLock;
			Domain*				fTargetDomain;
			Domain*				fSourceDomain;
			SourceFileTable*	fSourceFiles;

			LocatedFileMap		fSourceLocationMappings;
};



#endif	// FILE_MANAGER_H
