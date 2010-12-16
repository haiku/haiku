/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "UserlandFS.h"

#include <KernelExport.h>

#include "Compatibility.h"
#include "Debug.h"
#include "FileSystem.h"
#include "FileSystemInitializer.h"
#include "KernelDebug.h"
#include "RequestPort.h"
#include "Requests.h"
#include "UserlandFSDefs.h"


UserlandFS* UserlandFS::sUserlandFS = NULL;

// constructor
UserlandFS::UserlandFS()
	:
	fFileSystems(NULL),
	fDebuggerCommandsAdded(false)
{
}

// destructor
UserlandFS::~UserlandFS()
{
PRINT(("UserlandFS::~UserlandFS()\n"))
	delete fFileSystems;
	if (fDebuggerCommandsAdded)
		KernelDebug::RemoveDebuggerCommands();
}

// InitUserlandFS
status_t
UserlandFS::InitUserlandFS(UserlandFS** _userlandFS)
{
	// create the singleton instance
	sUserlandFS = new(nothrow) UserlandFS;
	if (!sUserlandFS)
		return B_NO_MEMORY;

	// init the thing
	status_t error = sUserlandFS->_Init();
	if (error != B_OK) {
		delete sUserlandFS;
		sUserlandFS = NULL;
		return error;
	}

	*_userlandFS = sUserlandFS;

	return B_OK;
}

// UninitUserlandFS
void
UserlandFS::UninitUserlandFS()
{
	delete sUserlandFS;
	sUserlandFS = NULL;
}

// GetUserlandFS
UserlandFS*
UserlandFS::GetUserlandFS()
{
	return sUserlandFS;
}

// RegisterFileSystem
status_t
UserlandFS::RegisterFileSystem(const char* name, FileSystem** _fileSystem)
{
	// check parameters
	if (!name || !_fileSystem)
		return B_BAD_VALUE;

	// check, if we do already know this file system, and create it, if not
	FileSystemInitializer* fileSystemInitializer;
	{
		FileSystemLocker _(fFileSystems);
		fileSystemInitializer = fFileSystems->Get(name);
		if (fileSystemInitializer) {
			fileSystemInitializer->AcquireReference();
		} else {
			fileSystemInitializer = new(nothrow) FileSystemInitializer(name);
			if (!fileSystemInitializer)
				return B_NO_MEMORY;

			status_t error = fFileSystems->Put(name, fileSystemInitializer);
			if (error != B_OK) {
				delete fileSystemInitializer;
				return error;
			}
		}
	}

	// prepare the file system
	status_t error = fileSystemInitializer->Access();
	if (error != B_OK) {
		_UnregisterFileSystem(name);
		return error;
	}

	*_fileSystem = fileSystemInitializer->GetFileSystem();
	return error;
}

// UnregisterFileSystem
status_t
UserlandFS::UnregisterFileSystem(FileSystem* fileSystem)
{
	if (!fileSystem)
		return B_BAD_VALUE;

	return _UnregisterFileSystem(fileSystem->GetName());
}

// CountFileSystems
int32
UserlandFS::CountFileSystems() const
{
	return fFileSystems->Size();
}

// _Init
status_t
UserlandFS::_Init()
{
	// add debugger commands
	KernelDebug::AddDebuggerCommands();
	fDebuggerCommandsAdded = true;

	// create file system map
	fFileSystems = new(nothrow) FileSystemMap;
	if (!fFileSystems)
		RETURN_ERROR(B_NO_MEMORY);
	status_t error = fFileSystems->InitCheck();
	if (error != B_OK)
		RETURN_ERROR(error);

	return B_OK;
}

// _UnregisterFileSystem
status_t
UserlandFS::_UnregisterFileSystem(const char* name)
{
	if (!name)
		return B_BAD_VALUE;

	// find the FS and decrement its reference counter
	FileSystemInitializer* fileSystemInitializer = NULL;
	bool deleteFS = false;
	{
		FileSystemLocker _(fFileSystems);
		fileSystemInitializer = fFileSystems->Get(name);
		if (!fileSystemInitializer)
			return B_BAD_VALUE;

		deleteFS = fileSystemInitializer->ReleaseReference();
		if (deleteFS)
			fFileSystems->Remove(name);
	}

	// delete the FS, if the last reference has been removed
	if (deleteFS)
		delete fileSystemInitializer;
	return B_OK;
}
