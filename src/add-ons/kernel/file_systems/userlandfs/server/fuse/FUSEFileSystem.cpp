/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "FUSEFileSystem.h"

#include <stdlib.h>
#include <string.h>

#include <new>

#include "FUSEVolume.h"


class FUSEFileSystem::ArgumentVector {
private:
	enum { MAX_ARGUMENTS = 128 };

public:
	ArgumentVector()
		:
		fBuffer(NULL),
		fCount(0)
	{
	}

	~ArgumentVector()
	{
		free(fBuffer);
	}

	const char* const* Arguments() const
	{
		return fArguments;
	}

	int ArgumentCount() const
	{
		return fCount;
	}

	status_t Init(const char* firstElement, const char* arguments)
	{
		size_t firstElementSize = firstElement != NULL
			? strlen(firstElement) + 1 : 0;

		// allocate the buffer
		fBuffer = (char*)malloc(firstElementSize + strlen(arguments) + 1);
		if (fBuffer == NULL)
			return B_NO_MEMORY;

		fCount = 0;

		bool inArgument = false;
		int bufferIndex = 0;

		// push the first element, if given
		if (firstElement != NULL) {
			memcpy(fBuffer, firstElement, firstElementSize);
			fArguments[fCount++] = fBuffer;
			bufferIndex = firstElementSize;
		}

		// parse the given string
		for (; *arguments != '\0'; arguments++) {
			char c = *arguments;
			switch (c) {
				case ' ':
				case '\t':
				case '\r':
				case '\n':
					// white-space marks argument boundaries
					if (inArgument) {
						// terminate the current argument
						fBuffer[bufferIndex++] = '\0';
						inArgument = false;
					}
					break;
				case '\\':
					c = *++arguments;
					if (c == '\0')
						break;
					// fall through
				default:
					if (!inArgument) {
						// push a new argument
						if (fCount == MAX_ARGUMENTS)
							break;

						fArguments[fCount++] = fBuffer + bufferIndex;
						inArgument = true;
					}

					fBuffer[bufferIndex++] = c;
					break;
			}
		}

		// terminate the last argument
		if (inArgument)
			fBuffer[bufferIndex++] = '\0';

		// NULL terminate the argument array
		fArguments[fCount] = NULL;

		return B_OK;
	}

private:
	char*		fBuffer;
	const char*	fArguments[MAX_ARGUMENTS + 1];
	int			fCount;
};


FUSEFileSystem::FUSEFileSystem(const char* fsName,
	int (*mainFunction)(int, const char* const*))
	:
	FileSystem(fsName),
	fMainFunction(mainFunction),
	fInitThread(-1),
	fInitStatus(B_NO_INIT),
	fInitSemaphore(-1),
	fExitSemaphore(-1),
	fInitParameters(NULL),
	fFS(NULL)
{
}


FUSEFileSystem::~FUSEFileSystem()
{
	if (fInitSemaphore >= 0)
		delete_sem(fInitSemaphore);

	if (fExitSemaphore >= 0)
		delete_sem(fExitSemaphore);

	if (fInitThread >= 0)
		wait_for_thread(fInitThread, NULL);
}


status_t
FUSEFileSystem::CreateVolume(Volume** _volume, dev_t id)
{
printf("FUSEFileSystem::CreateVolume()\n");
	// Only one volume is possible
	if (!fVolumes.IsEmpty())
		RETURN_ERROR(B_BUSY);

	// create the volume
	FUSEVolume* volume = new(std::nothrow) FUSEVolume(this, id);
	if (volume == NULL)
		return B_NO_MEMORY;

	*_volume = volume;
	return B_OK;
}


status_t
FUSEFileSystem::DeleteVolume(Volume* volume)
{
	delete volume;
	return B_OK;
}


status_t
FUSEFileSystem::InitClientFS(const char* parameters)
{
	// create the semaphores we need
	fInitSemaphore = create_sem(0, "FUSE init sem");
	if (fInitSemaphore < 0)
		RETURN_ERROR(fInitSemaphore);

	fExitSemaphore = create_sem(0, "FUSE exit sem");
	if (fExitSemaphore < 0)
		RETURN_ERROR(fExitSemaphore);

	fInitStatus = 1;
	fInitParameters = parameters;

	// Start the initialization thread -- it will call main() and won't return
	// until unmounting.
	fInitThread = spawn_thread(&_InitializationThreadEntry,
		"FUSE init", B_NORMAL_PRIORITY, this);
	if (fInitThread < 0)
		RETURN_ERROR(fInitThread);

	resume_thread(fInitThread);

	// wait for the initialization to finish
	while (acquire_sem(fInitSemaphore) == B_INTERRUPTED) {
	}

	fInitSemaphore = -1;

	if (fInitStatus > 0)
		RETURN_ERROR(B_ERROR);
	if (fInitStatus != B_OK)
		RETURN_ERROR(fInitStatus);

	// initialization went fine
	return B_OK;
}


void
FUSEFileSystem::ExitClientFS(status_t status)
{
	// set the exit status and notify the initialization thread
	fExitStatus = status;
	if (fExitSemaphore >= 0)
		delete_sem(fExitSemaphore);
}


status_t
FUSEFileSystem::FinishInitClientFS(const fuse_operations* ops, size_t opSize,
	void* userData)
{
	fExitStatus = B_ERROR;

	// do the initialization
	status_t error = _InitClientFS(ops, opSize, userData);

	// notify the mount thread
	fInitStatus = error;
	delete_sem(fInitSemaphore);

	// loop until unmounting
	while (acquire_sem(fExitSemaphore) == B_INTERRUPTED) {
	}

	fExitSemaphore = -1;

	if (error == B_OK)
		fuse_fs_destroy(fFS);

	return error;
}


/*static*/ status_t
FUSEFileSystem::_InitializationThreadEntry(void* data)
{
	return ((FUSEFileSystem*)data)->_InitializationThread();
}


status_t
FUSEFileSystem::_InitializationThread()
{
	// parse the parameters
	ArgumentVector args;
	status_t error = args.Init(GetName(), fInitParameters);
	if (error != B_OK) {
		fInitStatus = error;
		delete_sem(fInitSemaphore);
		return B_OK;
	}

	// call main -- should not return until unmounting
	fMainFunction(args.ArgumentCount(), args.Arguments());
printf("FUSEFileSystem::_InitializationThread(): main() returned!\n");

	if (fInitStatus > 0 && fInitSemaphore >= 0) {
		// something went wrong early -- main() returned without calling
		// fuse_main()
		fInitStatus = B_ERROR;
		delete_sem(fInitSemaphore);
	}

	return B_OK;
}


status_t
FUSEFileSystem::_InitClientFS(const fuse_operations* ops, size_t opSize,
	void* userData)
{
	// create a fuse_fs object
	fFS = fuse_fs_new(ops, opSize, userData);
	if (fFS == NULL)
		return B_ERROR;

	// init connection info
	fConnectionInfo.proto_major = 0;
	fConnectionInfo.proto_minor = 0;
	fConnectionInfo.async_read = false;
	fConnectionInfo.max_write = 64 * 1024;
	fConnectionInfo.max_readahead = 64 * 1024;

	fuse_fs_init(fFS, &fConnectionInfo);

	return B_OK;
}


// #pragma mark - bootstrapping


status_t
userlandfs_create_file_system(const char* fsName, image_id image,
	FileSystem** _fileSystem)
{
printf("userlandfs_create_file_system()\n");
	// look up the main() function of the add-on
	int (*mainFunction)(int argc, const char* const* argv);
	status_t error = get_image_symbol(image, "main", B_SYMBOL_TYPE_TEXT,
		(void**)&mainFunction);
	if (error != B_OK)
		return error;
printf("userlandfs_create_file_system(): found main: %p\n", mainFunction);

	// create the file system
	FUSEFileSystem* fileSystem = new(std::nothrow) FUSEFileSystem(fsName,
		mainFunction);
	if (fileSystem == NULL)
		return B_NO_MEMORY;

	*_fileSystem = fileSystem;
	return B_OK;
}
