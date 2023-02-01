#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int
main(int argc, char** argv)
{
	const char* filename = "temp.fifo";

	if (mkfifo("temp.fifo", S_IRWXU) != 0) {
		perror("mkfifo error");
		return 1;
	}

	int rfd = open(filename, O_RDONLY | O_NONBLOCK);
	if (rfd < 0) {
		perror("open() error");
		return 1;
	}

	DIR* dir = opendir(filename);
	if (dir != NULL) {
		perror("opendir succeeded");
		return 1;
	}

	dir = fdopendir(rfd);
	if (dir != NULL) {
		perror("fdopendir succeeded");
		return 1;
	}

	unlink(filename);
	return 0;
}
