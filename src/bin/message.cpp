/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */

#include <File.h>
#include <Message.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int
main(int argc, char *argv[])
{
	if (argc < 2 || argc > 3) {
		printf("usage: %s <flattened message file> [index]\n", argv[0]);
		return 1;
	}

	BFile input(argv[1], B_READ_ONLY);
	if (!input.IsReadable()) {
		printf("cannot open \"%s\" for reading\n", argv[1]);
		return 2;
	}

	off_t fileSize;
	status_t result;
	if ((result = input.GetSize(&fileSize)) != B_OK) {
		printf("cannot determine size of file \"%s\"\n", argv[1]);
		return 3;
	}

	int index = argc > 2 ? atoi(argv[2]) : 0;

	for (int i = 1; input.Position() < fileSize; ++i) {
		BMessage message;
		result = message.Unflatten(&input);
		if (result != B_OK) {
			printf("failed to unflatten message: %s\n", strerror(result));
			return 4;
		}
		if (index == 0 || i == index)
			message.PrintToStream();
	}

	return 0;
}
