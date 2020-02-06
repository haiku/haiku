/*
 * Copyright 2002-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <FindDirectory.h>
#include <OS.h>


static int
choose_file(const char *path)
{
	struct dirent *dirent;
	int count = 0;
	int chosen;

	DIR *dir = opendir(path);
	if (dir == NULL)
		return -1;

	// count directory entries

	while ((dirent = readdir(dir)) != NULL) {
		if (dirent->d_name[0] == '.')
			continue;

		count++;
	}

	if (count == 0) {
		closedir(dir);
		return -1;
	}
	// choose and open entry

	chosen = rand() % count;
	count = 0;
	rewinddir(dir);

	while ((dirent = readdir(dir)) != NULL) {
		if (dirent->d_name[0] == '.')
			continue;

		if (chosen <= count) {
			char name[PATH_MAX];
			int fd;

			// build full path
			strlcpy(name, path, sizeof(name));
			strlcat(name, "/", sizeof(name));
			strlcat(name, dirent->d_name, sizeof(name));

			fd = open(name, O_RDONLY);
			if (fd >= 0) {
				closedir(dir);
				return fd;
			}
		}
		count++;
	}

	closedir(dir);
	return -1;
}


int
main(int argc, char **argv)
{
	char path[PATH_MAX] = {'\0'};
	const char *file = path;
	int fd;
	char *buffer;
	unsigned start, i;
	unsigned count;
	struct stat stat;

	srand(system_time() % INT_MAX);

	if (argc > 1) {
		// if there are arguments, choose one randomly
		file = argv[1 + (rand() % (argc - 1))];
	} else if (find_directory(B_SYSTEM_DATA_DIRECTORY, -1, false, path,
			sizeof(path)) == B_OK) {
		strlcat(path, "/fortunes", sizeof(path));
	}

	fd = open(file, O_RDONLY, 0);
	if (fd < 0) {
		fprintf(stderr, "Couldn't open %s: %s\n", file, strerror(errno));
		return 1;
	}

	if (fstat(fd, &stat) < 0) {
		fprintf(stderr, "stat() failed: %s\n", strerror(errno));
		return 1;
	}

	if (S_ISDIR(stat.st_mode)) {
		close(fd);

		fd = choose_file(file);
		if (fd < 0) {
			fprintf(stderr, "Could not find any fortune file.\n");
			return 1;
		}

		if (fstat(fd, &stat) < 0) {
			fprintf(stderr, "stat() failed: %s\n", strerror(errno));
			return 1;
		}
	}

	buffer = malloc(stat.st_size + 1);
	if (buffer == NULL) {
		fprintf(stderr, "Not enough memory.\n");
		return 1;
	}

	if (read(fd, buffer, stat.st_size) < 0) {
		fprintf(stderr, "Could not read from fortune file: %s\n",
			strerror(errno));
		free(buffer);
		return -1;
	}

	buffer[stat.st_size] = '\0';
	close(fd);

	// count fortunes

	count = 0;
	for (i = 0; i < stat.st_size - 2; i++) {
		if (!strncmp(buffer + i, "\n%\n", 3)) {
			count++;
			i += 3;
		}
	}

	if (!count) {
		printf("Out of cookies...\n");
		free(buffer);
		return 0;
	}

	count = rand() % count;
	start = 0;

	// find beginning & end
	for (i = 0; i < stat.st_size - 2; i++) {
		if (!strncmp(buffer + i, "\n%\n", 3)) {
			if (count-- <= 0) {
				buffer[i] = '\0';
				break;
			}

			i += 3;
			start = i;
		}
	}

	puts(buffer + start);
	free(buffer);
	return 0;
}
