/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


bool gDone = false;


static int32
try_to_lock(void *_fd)
{
	int fd = (int)_fd;

	struct flock flock = {
		F_RDLCK,	// shared lock
		SEEK_SET,
		200,
		0,			// until the end of the file
		0
	};

	if (fcntl(fd, F_SETLK, &flock) == 0) {
		fprintf(stderr, "a. Could lock file!\n");
		return -1;
	} else
		puts("test a passed.");

	gDone = true;

	// wait for lock to become free

	if (fcntl(fd, F_SETLKW, &flock) == -1) {
		fprintf(stderr, "b. Could not lock file: %s\n", strerror(errno));
		return -1;
	} else
		puts("test b passed.");

	flock.l_type = F_UNLCK;

	if (fcntl(fd, F_SETLK, &flock) == -1) {
		fprintf(stderr, "c. Could not unlock file: %s\n", strerror(errno));
		return -1;
	} else
		puts("test c passed.");

	return 0;
}


int 
main(int argc, char **argv)
{
	int fd = open("/bin/sh", O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "could not open file: %s\n", strerror(errno));
		return -1;
	}

	struct flock flock = {
		F_WRLCK,	// exclusive lock
		SEEK_SET,	// lock whole file
		0,
		0,
		0
	};

	if (fcntl(fd, F_SETLK, &flock) == 0) {
		fprintf(stderr, "0. Could lock file exclusively without write access!\n");
		return -1;
	} else
		puts("test 0 passed.");

	flock.l_type = F_RDLCK;
		// shared locks should be allowed

	if (fcntl(fd, F_SETLK, &flock) == -1) {
		fprintf(stderr, "1. Could not lock file: %s\n", strerror(errno));
		return -1;
	} else
		puts("test 1 passed.");

	close(fd);

	fd = open("/bin/sh", O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "could not open file: %s\n", strerror(errno));
		return -1;
	}

	flock.l_type = F_WRLCK;
	flock.l_start = 100;
	flock.l_len = 42;

	if (fcntl(fd, F_SETLK, &flock) == -1) {
		fprintf(stderr, "2. Could not lock file: %s\n", strerror(errno));
		return -1;
	} else
		puts("test 2 passed.");

	flock.l_start = 200;

	if (fcntl(fd, F_SETLK, &flock) == -1) {
		fprintf(stderr, "3. Could not lock file: %s\n", strerror(errno));
		return -1;
	} else
		puts("test 3 passed.");

	flock.l_start = 80;

	if (fcntl(fd, F_SETLK, &flock) == 0) {
		fprintf(stderr, "4. Could lock file exclusively on locked region!\n");
		return -1;
	} else
		puts("test 4 passed.");

	flock.l_type = F_UNLCK;
	flock.l_start = 100;

	if (fcntl(fd, F_SETLK, &flock) == -1) {
		fprintf(stderr, "5. Could not unlock file: %s\n", strerror(errno));
		return -1;
	} else
		puts("test 5 passed.");

	flock.l_type = F_WRLCK;
	flock.l_start = 80;

	if (fcntl(fd, F_SETLK, &flock) == -1) {
		fprintf(stderr, "6. Could not lock file: %s\n", strerror(errno));
		return -1;
	} else
		puts("test 6 passed.");

	thread_id thread = spawn_thread(try_to_lock, "try", B_NORMAL_PRIORITY, (void *)fd);
	if (thread < B_OK) {
		fprintf(stderr, "Could not spawn thread: %s\n", strerror(thread));
		return -1;
	}
	resume_thread(thread);

	while (!gDone) {
		snooze(100000); // 0.1s
	}

	flock.l_type = F_UNLCK;
	flock.l_start = 200;

	if (fcntl(fd, F_SETLK, &flock) == -1) {
		fprintf(stderr, "7. Could not unlock file: %s\n", strerror(errno));
		return -1;
	} else
		puts("test 7 passed.");

	status_t returnCode;
	wait_for_thread(thread, &returnCode);

	close(fd);
	return 0;
}
