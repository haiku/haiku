/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
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
ConsoleHandle::ReadAt(void */*cookie*/, off_t /*pos*/, void *buffer, size_t bufferSize)
{
	// don't seek in character devices
	return of_read(fHandle, buffer, bufferSize);
}


ssize_t
ConsoleHandle::WriteAt(void */*cookie*/, off_t /*pos*/, const void *buffer, size_t bufferSize)
{
	const char *string = (const char *)buffer;

	// be nice to our audience and replace single "\n" with "\r\n"

	while (bufferSize > 0) {
		bool newLine = false;
		size_t length = 0;

		for (; length < bufferSize; length++) {
			if (string[length] == '\r' && length + 1 < bufferSize) {
				length += 2;
			} else if (string[length] == '\n') {
				newLine = true;
				break;
			}
		}

		if (length > bufferSize)
			length = bufferSize;

		if (length > 0) {
			of_write(fHandle, string, length);
			string += length;
			bufferSize -= length;
		}

		if (newLine) {
			of_write(fHandle, "\r\n", 2);
			string++;
			bufferSize--;
		}
	}

	return string - (char *)buffer;
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


