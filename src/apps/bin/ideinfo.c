#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <SupportDefs.h>
#include <BeBuild.h>

/* why isn't that documented ? */
#define IDE_GET_INFO 0x2710
#define IDE_GET_STATUS 0x2711

/* in uint16 */
#define IDE_INFO_LEN 0x100

int main(int argc, char **argv)
{
	int fd;
	uint16 buffer[IDE_INFO_LEN];
	if (argc < 2) {
		fprintf(stderr, "use: %s devicename\n", argv[0]);
		return 1;
	}
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "could not open %s, %s\n", argv[1], strerror(errno));
		return 2;
	}
	if (ioctl(fd, IDE_GET_INFO, buffer, IDE_INFO_LEN*sizeof(uint16)) < 0) {
		fprintf(stderr, "could not get ide info for %s\n", argv[1]);
		return 3;
	}
	{
		int f;
		f = open("/tmp/ideinfo.dump", O_WRONLY|O_CREAT, 0666);
		write(f, buffer, IDE_INFO_LEN*sizeof(uint16));
	}
	close(fd);
	return 0;
}
