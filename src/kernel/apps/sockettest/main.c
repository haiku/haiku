/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <resource.h>
#include <Errors.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

int main(int argc, char **argv)
{
	int rc = 0, fd[3];
	int cnt, i;
	int on = 1;
	
	printf("Socket Test!!!\n");
	printf("This test doesn't do much but test that we can create a socket descriptor\n\n");

	for (i=0;i<3;i++) {
		printf("%d : ", i + 1);
		fd[i] = socket(PF_INET, SOCK_DGRAM, 0);
		printf("socket is fd %d\n", fd[i]);
	
		if (fd[i] > 0) {
			rc = read(fd[i], NULL, 0);
		
			printf("    read on fd %d gave %d (expecting 0)\n", fd[i], rc);
		}
	}
	
	printf("closing the 3 fd's\n");
	for (i=0;i<3;i++) {
		close(fd[i]);
	}

	printf("\nNow we have no open fd's so next socket should be 3\n");
	fd[0] = socket(PF_INET, SOCK_DGRAM, 0);
	printf("new socket = fd %d\n", fd[0]);
	
	printf("trying ioctl to set non blocking: ");
	rc = ioctl(fd[0], FIONBIO, &on, sizeof(on));
	if (rc == 0) {
		printf("OK\n");
	} else {
		printf("failed\n");
		printf("Error was %s\n", strerror(errno));
	}

	close(fd[0]);
		
	return 0;
}
