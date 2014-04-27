/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_FILE_MANAGER_H
#define PACKAGE_FILE_MANAGER_H


#include "PackageFile.h"


class BLocker;

class Package;


class PackageFileManager {
public:
								PackageFileManager(BLocker& lock);
								~PackageFileManager();

			status_t			Init();

			status_t			GetPackageFile(const entry_ref& entryRef,
									PackageFile*& _file);
									// returns a reference
			status_t			CreatePackage(const entry_ref& entryRef,
									Package*& _package);

			void				PackageFileMoved(PackageFile* file,
									const node_ref& newDirectory);

			// conceptually private
			void				RemovePackageFile(PackageFile* file);

private:
			typedef PackageFileEntryRefHashTable EntryRefTable;

private:
			BLocker&			fLock;
			EntryRefTable		fFilesByEntryRef;
};


#endif	// PACKAGE_FILE_MANAGER_H
