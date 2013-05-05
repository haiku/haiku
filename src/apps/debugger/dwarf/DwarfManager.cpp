/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DwarfManager.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "DwarfFile.h"


DwarfManager::DwarfManager()
	:
	fLock("dwarf manager")
{
}


DwarfManager::~DwarfManager()
{
}


status_t
DwarfManager::Init()
{
	return fLock.InitCheck();
}


status_t
DwarfManager::LoadFile(const char* fileName, DwarfFile*& _file)
{
	AutoLocker<DwarfManager> locker(this);

	DwarfFile* file = new(std::nothrow) DwarfFile;
	if (file == NULL)
		return B_NO_MEMORY;

	BReference<DwarfFile> fileReference(file, true);
	status_t error = file->Load(fileName);
	if (error != B_OK) {
		return error;
	}

	fFiles.Add(file);
	fileReference.Detach();
		// we keep the initial reference for ourselves

	file->AcquireReference();
	_file = file;
	return B_OK;
}


status_t
DwarfManager::FinishLoading()
{
	AutoLocker<DwarfManager> locker(this);

	for (FileList::Iterator it = fFiles.GetIterator();
			DwarfFile* file = it.Next();) {
		status_t error = file->FinishLoading();
		if (error != B_OK)
			return error;
	}

	return B_OK;
}
