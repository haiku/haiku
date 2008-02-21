#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int
main()
{
	pid_t parentPID = getpid();
	printf("parent pid: %d\n", parentPID);
	printf("parent pgid: %d\n", getpgrp());

	pid_t childPID = fork();
	if (childPID < 0) {
		fprintf(stderr, "fork() failed: %s\n", strerror(errno));
		exit(1);
	} else if (childPID == 0) {
		// child
		childPID = getpid();
		printf("child  pid: %d, pgid: %d\n", childPID, getpgrp());

		printf("child  setpgid(0, 0)\n");
		if (setpgid(0, 0) < 0) {
			fprintf(stderr, "child: first setpgid() failed: %s\n", strerror(errno));
			exit(1);
		}
//		printf("child  setsid()\n");
//		if (setsid() < 0) {
//			fprintf(stderr, "child: setsid() failed: %s\n", strerror(errno));
//			exit(1);
//		}

		printf("child  pgid: %d\n", getpgrp());

		pid_t grandChildPID = fork();
		if (grandChildPID < 0) {
			fprintf(stderr, "fork() 2 failed: %s\n", strerror(errno));
			exit(1);
		} else if (grandChildPID == 0) {
			// grand child
			grandChildPID = getpid();
			printf("gchild pid: %d, pgid: %d\n", grandChildPID, getpgrp());
			sleep(2);
			printf("gchild pid: %d, pgid: %d\n", grandChildPID, getpgrp());
		} else {
			// child
			sleep(1);

			printf("child  setpgid(0, %d)\n", parentPID);
			if (setpgid(0, parentPID) < 0) {
//			printf("child  setpgid(0, 0)\n");
//			if (setpgid(0, 0) < 0) {
				fprintf(stderr, "child: second setpgid() failed: %s\n", strerror(errno));
				exit(1);
			}

			printf("child  pgid: %d\n", getpgrp());
		}

	} else {
		// parent
		sleep(3);
	}

	return 0;
}
