#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

// for R5:
//#define pread(fd, buf, count, pos) read_pos(fd, pos, buf, count)

int blockSize = 1024;
int group = 64;
int progress = 1;
int verbose = 0;

int scan_device(const char *dev)
{
	char *buffer = NULL;
	size_t len = group * blockSize;
	off_t at = 0LL;
	off_t block = 0L;
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

	if (progress)
		fprintf(stderr, "\n");

	for (; ; block += group, at += len) {
		int got;
		if (progress)
			fprintf(stderr, "\033[Achecking %Ld\n", block);
		got = pread(fd, buffer, len, at);
		if (got == len)
			continue;
		if (got >= 0) {
			if (verbose)
				fprintf(stderr, "at %Ld got %d < %d\n", at, got, len);
			break;
		}
		if (got < 0) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			if (errno != EIO && errno != ENXIO) {
				perror("pread");
				goto err2;
			}
		}
		// try each block separately
		for (i = 0; i < group; i++) {
			got = pread(fd, buffer, blockSize, at + blockSize * i);
			if (got == blockSize)
				continue;
			if (got < 0) {
				fprintf(stderr, "error: %s\n", strerror(errno));
				printf("%Ld\n", block);
				fflush(stdout);
			}
		}
	}
	close(fd);
	free(buffer);
	return 0;

err2:
	close(fd);
err1:
	free(buffer);
	return 1;
}

int usage(int ret)
{
	fprintf(stderr, "badblocks [-sv] [-b block-size] [-c block-at-once] /dev/disk/...\n");
	return ret;
}

int main(int argc, char **argv)
{
	int ch;
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

	for (; argc > 0; argc--, argv++) {
		scan_device(argv[0]);
	}
	return 0;
}
