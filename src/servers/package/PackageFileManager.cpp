/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageFileManager.h"

#include <Locker.h>

#include <AutoLocker.h>

#include "DebugSupport.h"
#include "Package.h"


PackageFileManager::PackageFileManager(BLocker& lock)
	:
	fLock(lock),
	fFilesByEntryRef()
{
}


PackageFileManager::~PackageFileManager()
{
}


status_t
PackageFileManager::Init()
{
	return fFilesByEntryRef.Init();
}


status_t
PackageFileManager::GetPackageFile(const entry_ref& entryRef,
	PackageFile*& _file)
{
	AutoLocker<BLocker> locker(fLock);

	PackageFile* file = fFilesByEntryRef.Lookup(entryRef);
	if (file != NULL) {
		if (file->AcquireReference() > 0) {
			_file = file;
			return B_OK;
		}

		// File already full dereferenced. It is about to be deleted.
		fFilesByEntryRef.Remove(file);
	}

	file = new(std::nothrow) PackageFile;
	if (file == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	status_t error = file->Init(entryRef, this);
	if (error != B_OK) {
		delete file;
		return error;
	}

	fFilesByEntryRef.Insert(file);

	_file = file;
	return B_OK;
}


status_t
PackageFileManager::CreatePackage(const entry_ref& entryRef, Package*& _package)
{
	PackageFile* packageFile;
	status_t error = GetPackageFile(entryRef, packageFile);
	if (error != B_OK)
		RETURN_ERROR(error);
	BReference<PackageFile> packageFileReference(packageFile, true);

	_package = new(std::nothrow) Package(packageFile);
	RETURN_ERROR(_package != NULL ? B_OK : B_NO_MEMORY);
}


void
PackageFileManager::PackageFileMoved(PackageFile* file,
	const node_ref& newDirectory)
{
	if (newDirectory == file->DirectoryRef())
		return;

	AutoLocker<BLocker> locker(fLock);

	fFilesByEntryRef.Remove(file);
	file->SetDirectoryRef(newDirectory);
	fFilesByEntryRef.Insert(file);
}


void
PackageFileManager::RemovePackageFile(PackageFile* file)
{
	AutoLocker<BLocker> locker(fLock);

	fFilesByEntryRef.Remove(file);
}
