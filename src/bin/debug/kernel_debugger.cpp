/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>


#include <syscalls.h>


extern const char* __progname;

static const char* const kUsage =
	"Usage: %s [ <message> ]\n"
	"Enters the kernel debugger with the optional message.\n";


int
main(int argc, const char* const* argv)
{
	const char* message = "User command requested kernel debugger.";

	if (argc > 1) {
		if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
			printf(kUsage, __progname);
			return 0;
		}

		message = argv[1];
	}

	status_t error = _kern_kernel_debugger(message);
	if (error != B_OK) {
		fprintf(stderr, "Error: Entering the kernel debugger failed: %s\n",
			strerror(error));
		return 1;
	}

	return 0;
}
