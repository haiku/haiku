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

#define DIGIT "/dev/misc/digit"

int main(int argc, char **argv)
{
	int rc = 0, fd, i;
	char buffer[128];
	size_t len = 128;
			
	printf("Digit Test!!!\n");
	printf("\nDigit was one of the example device drivers that Be\n"
	       "provided, so this test basically justs runs a simple test\n"
	       "to make sure it works with OpenBeOS.\n"
	       "\nNB This is a simple test :)\n\n");

	fd = open(DIGIT, O_RDONLY, 0);
	if (fd < 0) {
		printf("Failed at first hurdle :( Couldn't open the device!\n");
		return -1;
	}
	
	rc = read(fd, buffer, len);
	printf("Read on device gave %d\n", rc);
	for (i = 0; i < rc; i++) {
		printf("%02x ", buffer[i]);
		if ((i & 15) == 15)
			printf("\n");
	}
	printf("\n");
		
	close(fd);
	
	printf("closed device.\n");

	return 0;
}
