/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


extern const char *__progname;

const char* kTemporaryFile = "/tmp/axels-lock-test";


const char*
type_name(int type)
{
	return type == F_RDLCK ? "shared" : type == F_WRLCK
		? "exclusive" : "remove";
}


int
do_lock(int fd, int type, off_t start, off_t length)
{
	printf("%s lock %Ld:%Ld\n", type_name(type), start, length);

	struct flock flock;
	flock.l_type = type;
	flock.l_whence = SEEK_SET;
	flock.l_start = start;
	flock.l_len = length;
	if (fcntl(fd, F_SETLK, &flock) != 0) {
		fprintf(stderr, "ERROR: %s lock %Ld:%Ld failed: %s\n", type_name(type),
			start, length, strerror(errno));
		return -1;
	}

	return 0;
}


int
shared_lock(int fd, off_t start, off_t length)
{
	return do_lock(fd, F_RDLCK, start, length);
}


int
exclusive_lock(int fd, off_t start, off_t length)
{
	return do_lock(fd, F_WRLCK, start, length);
}


int
remove_lock(int fd, off_t start, off_t length)
{
	return do_lock(fd, F_UNLCK, start, length);
}


void
wait_for_enter()
{
	puts("wait for <enter>...");
	char buffer[64];
	fgets(buffer, sizeof(buffer), stdin);
}


void
usage()
{
	fprintf(stderr, "usage: %s [shared|exclusive|unlock <start> <length>] "
		"[wait] [...]\n", __progname);
	exit(1);
}


bool
is_command(const char* op, const char* command1, const char* command2)
{
	int length = strlen(op);
	if (length == 0)
		return false;

	return command1 != NULL && !strncmp(op, command1, length)
		|| command2 != NULL && !strncmp(op, command2, length);
}


int
main(int argc, char** argv)
{
	int fd = open(kTemporaryFile, O_CREAT | O_RDWR, 0644);
	if (fd < 0) {
		fprintf(stderr, "Could not create lock file: %s\n", strerror(errno));
		return 1;
	}

	while (argc > 1) {
		const char* op = argv[1];
		if (is_command(op, "wait", NULL)) {
			wait_for_enter();
			argv++;
			argc--;
			continue;
		}
		if (argc < 3)
			usage();

		off_t start = strtoll(argv[2], NULL, 0);
		off_t length = strtoll(argv[3], NULL, 0);
		int type = 0;
		if (is_command(op, "read", "shared"))
			type = F_RDLCK;
		else if (is_command(op, "write", "exclusive"))
			type = F_WRLCK;
		else if (is_command(op, "unlock", "remove"))
			type = F_UNLCK;
		else
			usage();

		do_lock(fd, type, start, length);
		argc -= 3;
		argv += 3;
	}

	close(fd);
	return 0;
}

