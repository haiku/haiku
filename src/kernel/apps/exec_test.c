/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <OS.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


void
print_process_info(const char *text)
{
	puts(text);

	printf("\tsession_id = %ld\n", getsid(0));
	printf("\tgroup_id = %ld\n", getpgid(0));
	printf("\tprocess_id = %ld\n", getpid());
	printf("\tparent_id = %ld\n", getppid());
}


int
main(int argc, char **argv)
{
	printf("exec_test: this is thread %ld\n", find_thread(NULL));

	if (argc < 2) {
		print_process_info("before exec():");
		puts("going to execl()...");

		execl("/bin/exec_test", "/bin/exec_test", "argument 1", NULL);
		printf("execl() returned: %s\n", strerror(errno));
	} else {
		int i;

		print_process_info("after exec():");
		puts("got arguments:");
		for (i = 0; i < argc; i++)
			printf("%d: \"%s\"\n", i, argv[i]);

		setsid();
		print_process_info("after setsid():");
	}

	return 0;
}

