#include <sys/uio.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int 
main()
{
	int fd = open("testfile", O_CREAT | O_RDWR, 0666);

	if (fd < 0) {
		printf("file open error %s\n", strerror(errno));
		return 1;
	}

	int dummy;
	int ret;
	iovec vec1 = { &dummy, sizeof(dummy) };
	ret = writev(fd, &vec1, 0x80000001);

	if (ret < 0) {
		printf("vec 1 write error %s\n", strerror(errno));
	}

	iovec vec2 = { (void *)0x80100000, 0x1000 };
	ret = writev(fd, &vec2, 1);

	if (ret < 0) {
		printf("vec 2 write error %s\n", strerror(errno));
	}

	iovec vec3 = { 0, 1 };
	ret = writev(fd, &vec3, 0xfff);

	if (ret < 0) {
		printf("vec 3 write error %s\n", strerror(errno));
	}

	close(fd);

	return 0;
}
