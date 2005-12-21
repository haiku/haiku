/*
 * Copyright (c) 2004, François Revol.
 * Distributed under the terms of the MIT license.
 */


#include <FindDirectory.h>
#include <fs_info.h>

#include <stdio.h>


int
main(int argc, char **argv)
{
	dev_t dev = -1;
	
	if (argc > 1)
		dev = dev_for_path(argv[1]);

	for (int32 i = B_DESKTOP_DIRECTORY; i < B_UTILITIES_DIRECTORY; i++) {
		char buffer[B_PATH_NAME_LENGTH];
		status_t err = find_directory((directory_which)i, dev, false, buffer, B_PATH_NAME_LENGTH);
		if (err)
			continue;

		printf("dir[%04ld] = '%s'\n", i, buffer);
	}
	return 0;
}

