/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>


int
main(int argc, char** argv)
{
	dirent *buf, *dirent;
	DIR* dir = opendir(".");

	printf("first pass:\n");
	while (true) {
		dirent = readdir(dir);
		if (dirent == NULL)
			break;

		printf("Entry: dev %ld, ino %Ld, name \"%s\"\n", dirent->d_dev,
			dirent->d_ino, dirent->d_name);
		//printf("  left: %u, next: %d\n", dir->entries_left, dir->next_entry);
	}

	rewinddir(dir);
	printf("second pass:\n");
	while (true) {
		dirent = readdir(dir);
		if (dirent == NULL)
			break;

		printf("Entry: dev %ld, ino %Ld, name \"%s\"\n", dirent->d_dev,
			dirent->d_ino, dirent->d_name);
		//printf("  left: %u, next: %d\n", dir->entries_left, dir->next_entry);
	}


	closedir(dir);

	dirent = NULL;
	buf = (struct dirent*)malloc(sizeof(struct dirent) + NAME_MAX);

	dir = opendir(".");

	printf("first pass:\n");
	while (true) {
		if (readdir_r(dir, buf, &dirent) != 0 || dirent == NULL)
			break;

		printf("Entry: dev %ld, ino %Ld, name \"%s\"\n", dirent->d_dev,
			dirent->d_ino, dirent->d_name);
		//printf("  left: %u, next: %d\n", dir->entries_left, dir->next_entry);
	}

	rewinddir(dir);
	printf("second pass:\n");
	while (true) {
		if (readdir_r(dir, buf, &dirent) != 0 || dirent == NULL)
			break;

		printf("Entry: dev %ld, ino %Ld, name \"%s\"\n", dirent->d_dev,
			dirent->d_ino, dirent->d_name);
		//printf("  left: %u, next: %d\n", dir->entries_left, dir->next_entry);
	}

	closedir(dir);

	return 0;
}
