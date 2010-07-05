#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// for R5:
//#define pread(fd, buf, count, pos) read_pos(fd, pos, buf, count)

int blockSize = 1024;
int group = 64;
int progress = 1;
int verbose = 0;
FILE *outputFile = NULL;

int scan_device(const char *dev, off_t startBlock, off_t endBlock)
{
	char *buffer = NULL;
	size_t len = group * blockSize;
	off_t block = startBlock;
	off_t at = block * blockSize;
	int i;
	int fd;

	buffer = (char *)malloc(len);
	if (buffer == NULL)
		return 1;


	fd = open(dev, O_RDONLY);
	if (fd < 0) {
		perror("open");
		goto err1;
	}
	// Check size

	if (verbose)
		fprintf(stderr, "Scanning '%s', %d * %d bytes at once\n",
			dev, group, blockSize);

	if (progress)
		fprintf(stderr, "\n");

	for (; block <= endBlock; block += group, at += len) {
		int got;
		if (progress)
			fprintf(stderr, "checking block %Ld\x1b[1A\n", block);
		got = pread(fd, buffer, len, at);
		if (got == len)
			continue;
		if (got >= 0) {
			if (verbose)
				fprintf(stderr, "block %Ld (offset %Ld): got %d < %zd\n",
					block, at, got, len);
			break;
		}
		if (got < 0) {
			fprintf(stderr, "block %Ld: error: %s\n", block, strerror(errno));
			/*
			if (errno != EIO && errno != ENXIO) {
				perror("pread");
				goto err2;
			}
			*/
		}
		// try each block separately
		for (i = 0; i < group; i++) {
			got = pread(fd, buffer, blockSize, at + blockSize * i);
			if (got == blockSize)
				continue;
			if (got < blockSize) {
				if (got < 0)
					fprintf(stderr, "block %Ld: error: %s\n", block + i, strerror(errno));
				else
					fprintf(stderr, "block %Ld: read %d bytes\n", block + i, got);
				fprintf(outputFile, "%Ld\n", block + i);
				fflush(stdout);
			}
		}
	}
	close(fd);
	free(buffer);
	return 0;

	close(fd);
err1:
	free(buffer);
	return 1;
}

int usage(int ret)
{
	fprintf(stderr, "badblocks [-sv] [-b block-size] [-c block-at-once] "
		"/dev/disk/... [end-block [start-block]]\n");
	return ret;
}

int main(int argc, char **argv)
{
	int ch;
	off_t startBlock = 0LL;
	off_t endBlock = INT64_MAX;
	outputFile = stdout;
	if (argc < 2)
		return usage(1);

	while ((ch = getopt(argc, argv, "b:c:hsv?")) != -1) {
		switch (ch) {
		case 'b':
			blockSize = atoi(optarg);
			//fprintf(stderr, "bs %d\n", blockSize);
			break;
		case 'c':
			group = atoi(optarg);
			//fprintf(stderr, "g %d\n", group);
			break;
		case 's':
			progress = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
		case '?':
			return usage(0);
		default:
			return usage(1);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 2)
		startBlock = atoll(argv[2]);
	if (argc > 1)
		endBlock = atoll(argv[1]);

	return scan_device(argv[0], startBlock, endBlock);
}
