#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syscalls.h>
#include <ktypes.h>
#include <sys/resource.h>
#include <Errors.h>
#include <errno.h>
#include <OS.h>
#include <stdlib.h>
#include <fcntl.h>

#define RANDOM_DEVICE "/dev/urandom"
#define RANDOM_CHECKS  5
#define STR_LEN       16
int main(int argc, char **argv)
{
	int fd;
	char data[STR_LEN];
	int rv, i, j;
		
	printf("/dev/urandom test\n"
	       "=================\n\n");

	fd = open(RANDOM_DEVICE, O_RDONLY);
	if (fd < 0) {
		printf("Failed to open %s\n", RANDOM_DEVICE);
		return -1;
	}
	
	for (i=0; i < RANDOM_CHECKS; i++) {
		rv = read(fd, data, STR_LEN);
		if (rv < STR_LEN)
			break;
		printf ("%d: ", i);
		for (j=0; j < sizeof(data);j++) {
			printf(" %02x ", (uint8)data[j]);
		}
		printf("\n");
	}
	close(fd);

	return 0;
}
