#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


int
main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "usage: symlink <target> <link-name>\n");
		return -1;
	}
	if (symlink(argv[2], argv[1]) < 0) {
		fprintf(stderr, "symlink failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

