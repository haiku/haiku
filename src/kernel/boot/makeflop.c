/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef O_BINARY
#	define O_BINARY 0
#endif


void
usage(char *progName)
{
	printf("usage: %s [-p #padding] bootblock payload outfile\n", progName);
}


int
main(int argc, char *argv[])
{
	struct stat st;
	unsigned int blocks;
	unsigned char bootsector[1024];
	unsigned char buf[512];
	size_t read_size;
	size_t written_bytes;
	int padding = 0;
	int infd;
	int outfd;
	signed char opt;
	int err;

	char *progName = argv[0];
	if (strrchr(progName,'/'))
		progName = strrchr(progName,'/') + 1;

	while ((opt = getopt(argc, argv, "p:")) != -1) {
		switch (opt) {
			case 'p':
				padding = atoi(optarg);
				if (padding < 0) {
					usage(progName);
					return -1;
				}
				break;
			default:
				usage(progName);
				return -1;
        }
	}
	argc -= optind - 1; 
	argv += optind - 1;

	if (argc < 4) {
		printf("insufficient args\n");
		usage(progName);
		return -1;
	}

	err = stat(argv[2], &st);
	if (err < 0) {
		printf("error stating file '%s'\n", argv[2]);
		return -1;
	}

	outfd = open(argv[3], O_BINARY|O_WRONLY|O_CREAT|O_TRUNC, 0666);
	if (outfd < 0) {
		printf("error: cannot open output file '%s'\n", argv[3]);
		return -1;
	}

	// first read the bootblock
	infd = open(argv[1], O_BINARY|O_RDONLY);
	if (infd < 0) {
		printf("error: cannot open bootblock file '%s'\n", argv[1]);
		return -1;
	}
	if (read(infd, bootsector, sizeof(bootsector)) < sizeof(bootsector)
		|| lseek(infd, 0, SEEK_END) != sizeof(bootsector)) {
		printf ("error: size of bootblock file '%s' must match %d bytes.\n", argv[1], sizeof(bootsector));
		return -1;
	}
	close(infd);

	// patch the size of the output into bytes 3 & 4 of the bootblock
	blocks = st.st_size / 512;
	if ((st.st_size % 512) != 0)
		blocks++;
	printf("size %d, blocks %d (size %d)\n", (unsigned long)st.st_size, blocks, blocks * 512);
	bootsector[2] = (blocks & 0x00ff);
	bootsector[3] = (blocks & 0xff00) >> 8;

	write(outfd, bootsector, sizeof(bootsector));
	written_bytes = sizeof(bootsector);

	infd = open(argv[2], O_BINARY|O_RDONLY);
	if (infd < 0) {
		printf("error: cannot open input file '%s'\n", argv[1]);
		return -1;
	}

	while ((read_size = read(infd, buf, sizeof(buf))) > 0) {
		write(outfd, buf, read_size);
		written_bytes += read_size;
	}

	if (padding) {
		if (written_bytes % padding) {
			size_t towrite = padding - written_bytes % padding;
			unsigned char *buf = malloc(towrite);

			memset(buf, 0, towrite);
			write(outfd, buf, towrite);
			written_bytes += towrite;

			printf("output file padded to %ld\n", (unsigned long)written_bytes);
		}
	}

	close(outfd);
	close(infd);

	return 0;
}

