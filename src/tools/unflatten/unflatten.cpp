/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <File.h>
#include <Message.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	BFile file;
	BMessage msg;
	if (argc != 2) {
		fprintf(stderr, "You need to specify a filename on the command line.\n");
		return 1;
	}
	if (B_OK != file.SetTo(argv[1], O_RDONLY)) {
		fprintf(stderr, "File \"%s\" not found.\n", argv[1]);
		return 1;
	}
	if (B_OK != msg.Unflatten(&file)) {
		fprintf(stderr, "Unflatten failed.\n");
		return 1;
	}
	msg.PrintToStream();
	return 0;
}
