/*
 * Copyright 2005-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "external_commands.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>


static FILE*
get_input()
{
	static FILE* sInput = fdopen(3, "r");
	return sInput;
}


static FILE*
get_output()
{
	static FILE* sOutput = fdopen(4, "w");
	return sOutput;
}


bool
FSShell::get_external_command(char* buffer, int size)
{
	// get the input stream
	FILE* in = get_input();
	if (in == NULL) {
		fprintf(stderr, "Error: Failed to open command input: %s\n",
			strerror(errno));
		return false;
	}

	while (true) {
		// read a command line
		if (fgets(buffer, size, in) != NULL)
			return true;

		// when interrupted, try again
		if (errno != EINTR)
			return false;
	}
}


void
FSShell::reply_to_external_command(int result)
{
	// get the output stream
	FILE* out = get_output();
	if (out == NULL) {
		fprintf(stderr, "Error: Failed to open command output: %s\n",
			strerror(errno));
		return;
	}

	if (fprintf(out, "%d\n", result) < 0 || fflush(out) == EOF) {
		fprintf(stderr, "Error: Failed to write command reply to output reply: "
			"%s\n", strerror(errno));
	}
}


void
FSShell::external_command_cleanup()
{
	// The file will be closed automatically when the team exits.
}
