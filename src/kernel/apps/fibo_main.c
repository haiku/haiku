/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscalls.h>


static
void
usage(char const *app)
{
	printf("usage: %s [-s] ###\n", app);
	_kern_exit(-1);
}


int
main(int argc, char *argv[])
{
	int num = 0;
	int silent = 0;
	int result;

	switch (argc) {
		case 2:
			num = atoi(argv[1]);
			break;
		case 3:
			if (strcmp(argv[1], "-s") == 0) {
				num = atoi(argv[2]);
				silent = 1;
			} else {
				usage(argv[0]);
			}
			break;
		default:
			usage(argv[0]);
			break;
	}

	if (num < 2) {
		result = 1;
	} else {
		team_id pid;
		int retcode;
		char buffer[64];
		char *aaargv[] = { "/boot/bin/fibo", "-s", buffer, NULL };
		int  aaargc = 3;

		sprintf(buffer, "%d", num-1);
		pid = _kern_create_team(aaargv[0], aaargv[0], aaargv, aaargc, NULL, 0, 5);
		_kern_wait_for_team(pid, &retcode);
		result = retcode;

		sprintf(buffer, "%d", num-2);
		pid = _kern_create_team(aaargv[0], aaargv[0], aaargv, aaargc, NULL, 0, 5);
		_kern_wait_for_team(pid, &retcode);
		result += retcode;
	}

	if (silent) {
		return result;
	} else {
		printf("%d\n", result);
		return 0;
	}
}
