#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#define BS 256

char buff[BS];

int main(int argc, char **argv)
{
	off_t at = 0LL;
	off_t block = 0L;
	int fd;
	if (argc < 2) {
		printf("badblocks /dev/disk/...\n");
		return 1;
	}
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}
	for (; ; block++, at+=BS) {
		if (lseek(fd, at, SEEK_SET) < 0) {
			perror("lseek");
			return 1;
		}
		if (read(fd, buff, BS) < BS) {
			if (errno != EIO && errno != ENXIO) {
				perror("read");
				return 1;
			}
			printf("%Ld\n", block);
			fflush(stdout);
		}
	}
	close(fd);
	return 0;
}
