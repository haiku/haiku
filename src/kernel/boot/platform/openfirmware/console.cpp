/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <SupportDefs.h>
#include <string.h>
#include <util/kernel_cpp.h>

#include "Handle.h"
#include "console.h"
#include "openfirmware.h"


class ConsoleHandle : public Handle {
	public:
		ConsoleHandle();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);
};

static ConsoleHandle sInput, sOutput;
FILE *stdin, *stdout, *stderr;


ConsoleHandle::ConsoleHandle()
	: Handle()
{
}


ssize_t
ConsoleHandle::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	// don't seek in character devices
	return of_read(fHandle, buffer, bufferSize);
}


ssize_t
ConsoleHandle::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	// don't seek in character devices
	return of_write(fHandle, buffer, bufferSize);
}


//	#pragma mark -


status_t
console_init(void)
{
	int input, output;
	if (of_getprop(gChosen, "stdin", &input, sizeof(int)) == OF_FAILED)
		return B_ERROR;
	if (of_getprop(gChosen, "stdout", &output, sizeof(int)) == OF_FAILED)
		return B_ERROR;

	sInput.SetHandle(input);
	sOutput.SetHandle(output);

	// now that we're initialized, enable stdio functionality
	stdin = (FILE *)&sInput;
	stdout = stderr = (FILE *)&sOutput;

	return B_OK;
}


