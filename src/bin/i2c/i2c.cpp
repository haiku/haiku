/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT license.
 */


#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Drivers.h>

#include <AutoDeleter.h>

#include "i2c.h"


static struct option const kLongOptions[] = {
	{"help", no_argument, 0, 'h'},
	{NULL}
};


extern const char *__progname;
static const char *kProgramName = __progname;


void
usage(int returnValue)
{
	fprintf(stderr, "Usage: %s <path-to-i2c-bus-device>\n", kProgramName);
	exit(returnValue);
}


static int
scan_bus(const char *path)
{
	int err = EXIT_SUCCESS;
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: Could not access path: %s\n", kProgramName,
			strerror(errno));
		return EXIT_FAILURE;
	}

	setbuf(stdout, NULL);
	printf("Scanning I2C bus: %s\n", path);
	FileDescriptorCloser closer(fd);

	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	for (int i = 0; i < 128; i+=16) {
		printf("%02x: ", i);
		for (int j = 0; j < 16; j++) {
			uint16 addr = i + j;
			uint8 cmd = 0;
			uint8 data = 0;
			i2c_ioctl_exec exec;
			exec.addr = addr;
			exec.op = I2C_OP_READ_STOP;
			exec.cmdBuffer = &cmd;
			exec.cmdLength = sizeof(cmd);
			exec.buffer = &data;
			exec.bufferLength = sizeof(data);
			if (ioctl(fd, I2CEXEC, &exec, sizeof(exec)) == 0)
				printf("%02x ", addr);
			else
				printf("-- ");
		}
		printf("\n");
	}

	close(fd);

	return err;
}


int
main(int argc, char** argv)
{
	int c;
	while ((c = getopt_long(argc, argv, "h", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'h':
				usage(0);
				break;
			default:
				usage(1);
				break;
		}
	}

	if (argc - optind < 1)
		usage(1);
	const char* path = argv[optind++];

	exit(scan_bus(path));
}
