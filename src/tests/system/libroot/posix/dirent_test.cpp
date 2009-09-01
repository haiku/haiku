/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <dirent.h>
#include <stdio.h>


int
main(int argc, char** argv)
{
	DIR* dir = opendir(".");

	while (true) {
		dirent* dirent = readdir(dir);
		if (dirent == NULL)
			break;

		printf("Entry: dev %ld, ino %Ld, name \"%s\"\n", dirent->d_dev,
			dirent->d_ino, dirent->d_name);
		//printf("  left: %u, next: %d\n", dir->entries_left, dir->next_entry);
	}

	closedir(dir);
	return 0;
}
