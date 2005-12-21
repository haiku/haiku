/* 
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <syscalls.h>
#include <generic_syscall.h>

#include <file_cache.h>

#include <stdio.h>
#include <string.h>


extern const char *__progname;


void
usage()
{
	fprintf(stderr, "usage: %s [clear | unset | set <module-name>]\n", __progname);
	exit(0);
}


int
main(int argc, char **argv)
{
	uint32 version = 0;
	status_t status = _kern_generic_syscall(CACHE_SYSCALLS, B_SYSCALL_INFO, &version, sizeof(version));
	if (status != B_OK) {
		fprintf(stderr, "%s: The cache syscalls are not available on this system.\n", __progname);
		return 1;
	}

	if (argc < 2)
		usage();

	if (!strcmp(argv[1], "clear")) {
		status = _kern_generic_syscall(CACHE_SYSCALLS, CACHE_CLEAR, NULL, 0);
		if (status != B_OK)
			fprintf(stderr, "%s: clearing the cache failed: %s\n", __progname, strerror(status));
	} else if (!strcmp(argv[1], "unset")) {
		status = _kern_generic_syscall(CACHE_SYSCALLS, CACHE_SET_MODULE, NULL, 0);
		if (status != B_OK)
			fprintf(stderr, "%s: unsetting the cache module failed: %s\n", __progname, strerror(status));
	} else if (!strcmp(argv[1], "set") && argc > 2) {
		status = _kern_generic_syscall(CACHE_SYSCALLS, CACHE_SET_MODULE, argv[2], strlen(argv[2]));
		if (status != B_OK)
			fprintf(stderr, "%s: setting the module failed: %s\n", __progname, strerror(status));
	} else
		usage();

	return 0;
}

