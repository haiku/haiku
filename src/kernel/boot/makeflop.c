/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

void usage(char **argv)
{
	printf("usage: %s bootblock payload outfile\n", argv[0]);
}

int main(int argc, char *argv[])
{
	struct stat st;
	int err;
	unsigned int blocks;
	unsigned char bootsector[1024];
	unsigned char buf[512];
	size_t read_size;
	int infd;
	int outfd;

	if(argc < 4) {
		printf("insufficient args\n");
		usage(argv);
		return -1;
	}

	err = stat(argv[2], &st);
	if(err < 0) {
		printf("error stating file '%s'\n", argv[2]);
		return -1;
	}

	outfd = open(argv[3], O_BINARY|O_WRONLY|O_CREAT|O_TRUNC, 0666);
	if(outfd < 0) {
		printf("error: cannot open output file '%s'\n", argv[3]);
		return -1;
	}

	// first read the bootblock
	infd = open(argv[1], O_BINARY|O_RDONLY);
	if(infd < 0) {
		printf("error: cannot open bootblock file '%s'\n", argv[1]);
		return -1;
	}
	if(read(infd, bootsector, sizeof(bootsector)) < sizeof(bootsector)
	  || lseek(infd, 0, SEEK_END) != sizeof(bootsector)) {
		printf ("error: size of bootblock file '%s' must match %d bytes.\n", argv[1], sizeof(bootsector));
		return -1;
	}
	close(infd);

	// patch the size of the output into bytes 3 & 4 of the bootblock
	blocks = st.st_size / 512;
	blocks++;
	bootsector[2] = (blocks & 0x00ff);
	bootsector[3] = (blocks & 0xff00) >> 8;

	write(outfd, bootsector, sizeof(bootsector));

	infd = open(argv[2], O_BINARY|O_RDONLY);
	if(infd < 0) {
		printf("error: cannot open input file '%s'\n", argv[1]);
		return -1;
	}

	while((read_size = read(infd, buf, sizeof(buf))) > 0) {
		write(outfd, buf, read_size);
	}

	close(outfd);
	close(infd);

	return 0;
}

