#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static const char* kRandomDevice = "/dev/urandom";


int
main()
{
	int fd = open(kRandomDevice, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error: Failed to open \"%s\": %s", kRandomDevice,
			strerror(errno));
		exit(1);
	}

	uint8_t buffer[16];
	ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
	if (bytesRead < 0) {
		fprintf(stderr, "Error: Failed to read from random device: %s",
			strerror(errno));
		exit(1);
	}

	printf("Read %d bytes from random device: ", (int)bytesRead);
	for (int i = 0; i < bytesRead; i++)
		printf("%02x", buffer[i]);
	printf("\n");

	return 0;
}

