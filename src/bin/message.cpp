/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */

#include <File.h>
#include <Message.h>

#include <stdio.h>
#include <string.h>


int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("usage: %s <flattened message file>\n", argv[0]);
		return 1;
	}

	BFile input(argv[1], B_READ_ONLY);
	if (!input.IsReadable()) {
		printf("cannot open \"%s\" for reading\n", argv[1]);
		return 2;
	}

	BMessage message;
	status_t result = message.Unflatten(&input);
	if (result != B_OK) {
		printf("failed to unflatten message: %s\n", strerror(result));
		return 3;
	}

	message.PrintToStream();
	return 0;
}
