/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <OS.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


int
main(int argc, char **argv)
{
	printf("exec_test: this is thread %ld\n", find_thread(NULL));

	if (argc < 2) {
		puts("going to execl()...");
		execl("/bin/exec_test", "/bin/exec_test", "argument 1", NULL);
		printf("execl() returned: %s\n", strerror(errno));
	} else {
		int i;
		puts("got arguments:");
		for (i = 0; i < argc; i++)
			printf("%d: \"%s\"\n", i, argv[i]);
	}

	return 0;
}

