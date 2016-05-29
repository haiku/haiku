/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "DwarfManager.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "DwarfFile.h"
#include "DwarfFileLoadingState.h"


DwarfManager::DwarfManager(uint8 addressSize)
	:
	fAddressSize(addressSize),
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
DwarfManager::LoadFile(const char* fileName, DwarfFileLoadingState& _state)
{
	AutoLocker<DwarfManager> locker(this);

	DwarfFile* file = _state.dwarfFile;
	BReference<DwarfFile> fileReference;
	if (file == NULL) {
		file = new(std::nothrow) DwarfFile;
		if (file == NULL)
			return B_NO_MEMORY;
		fileReference.SetTo(file, true);
		_state.dwarfFile = file;
	} else
		fileReference.SetTo(file);

	status_t error;
	if (_state.externalInfoFileName.IsEmpty()) {
		error = file->StartLoading(fileName, _state.externalInfoFileName);
		if (error != B_OK) {
			// only preserve state in the failure case if an external
			// debug information reference was found, but the corresponding
			// file could not be located on disk.
			_state.state = _state.externalInfoFileName.IsEmpty()
				? DWARF_FILE_LOADING_STATE_FAILED
				: DWARF_FILE_LOADING_STATE_USER_INPUT_NEEDED;

			return error;
		}
	}

	error = file->Load(fAddressSize, _state.locatedExternalInfoPath);
	if (error != B_OK) {
		_state.state = DWARF_FILE_LOADING_STATE_FAILED;
		return error;
	}

	fFiles.Add(file);

	fileReference.Detach();
		// keep a reference for ourselves in the list.

	_state.state = DWARF_FILE_LOADING_STATE_SUCCEEDED;

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
