/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <resource.h>
#include <Errors.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define FORTUNES "/boot/etc/fortunes"

int main(int argc, char **argv)
{
	int rc = 0, fd[3];
	int cnt, i;
	int on = 1;
	
	printf("File Test!!!\n");

	for (i=0;i<3;i++) {
		printf("%d : ", i + 1);
		fd[i] = open(FORTUNES, O_RDONLY, 0);
		printf("fd %d\n", fd[i]);
	}
	
	printf("closing the 3 fd's\n");
	for (i=0;i<3;i++) {
		close(fd[i]);
	}

	printf("\nNow we have no open fd's so next socket should be 3\n");
	fd[0] = open(FORTUNES, O_RDONLY, 0);
	printf("new fd %d\n", fd[0]);
	
	printf("Trying ioctl to set non-blocking: ");
	rc = ioctl(fd[0], FIONBIO, &on, sizeof(on));
	if (rc == EINVAL) {
		printf("OK\n");
	} else {
		printf("failed\n");
		printf("error was %s\n", strerror(errno));
	}
	
	close(fd[0]);
	
	return 0;
}
