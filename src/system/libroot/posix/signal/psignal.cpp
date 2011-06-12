/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include <stdio.h>
#include <string.h>


void
psignal(int signal, const char* message)
{
	if (message != NULL && message[0] != '\0')
		fprintf(stderr, "%s: %s\n", message, strsignal(signal));
	else
		fprintf(stderr, "%s\n", strsignal(signal));
}
