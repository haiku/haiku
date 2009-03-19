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
	fMainFunction(mainFunction)
{
}


FUSEFileSystem::~FUSEFileSystem()
{
}


status_t
FUSEFileSystem::CreateVolume(Volume** _volume, dev_t id)
{
printf("FUSEFileSystem::CreateVolume()\n");
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
	// parse the parameters
	ArgumentVector args;
	status_t error = args.Init(GetName(), parameters);
	if (error != B_OK)
		RETURN_ERROR(error);

	// call main
	fMainFunction(args.ArgumentCount(), args.Arguments());

// TODO: Check whether everything went fine!

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
