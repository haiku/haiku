#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


int
main()
{
	// 1. Create a file by creating a symlink to a nonexistent file,
	// then opening it with O_CREAT specified.
	unlink("link4");
	unlink("file4");
	int ret = symlink("file4", "link4");
	printf("symlink -> %d\n", ret);
	ret = open("link4", O_RDWR | O_TRUNC | O_CREAT, 0666);
	if (ret < 0) {
		perror("open");
		return ret;
	} else {
		printf("ret -> %d\n", ret);
	}

	// 2. Trying to create a file through a symlink to a path in
	// a nonexistent directory should fail with ENOENT.
	const char *linkname = "link4";
	struct stat statbuf;
	unlink(linkname);
	assert(symlink ("/nonexistent/test8237/24715863701440", linkname) >= 0);
	assert(stat(linkname, &statbuf) < 0);

	ret = open(linkname, O_RDWR | O_TRUNC | O_CREAT, 0666);

	if (ret >= 0)
		printf("ret = %d\n", ret);
	else if (errno == ENOENT)
		printf("ret = %d, errno == ENOENT\n", ret);
	else
		printf("ret = %d, errno == %s\n", ret, strerror (errno));

	assert(ret < 0);
	assert(errno == ENOENT);

	printf("OK\n");
	return 0;
}
