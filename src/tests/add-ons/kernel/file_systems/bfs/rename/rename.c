#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


int
main(int argc,char **argv)
{
	int file;

	// clean up (may not be necessary)
	remove("__directory/1");
	remove("__directory");
	remove("__file");

	file = open("__file", O_CREAT | O_TRUNC | O_WRONLY);
	if (file < 0)
		return -1;
	close(file);

	if (chmod("__file", 0644) != 0)
		printf("chmod fails: %s\n", strerror(errno));

	if (mkdir("__directory", 0755) != 0)
		return -1;

	// create a file in that directory
	file = open("__directory/1", O_CREAT | O_WRONLY);
	close(file);

	errno = 0;

	puts("rename test: Overwrite a directory with files in it");
	printf("  %s\n",rename("__file", "__directory") ? "Could not rename file!" : "Rename succeeded.");
	printf("  errno = %d (%s)\n", errno, strerror(errno));

	remove("__directory/1");
	errno = 0;

	// rename the file and try to remove the directory
	puts("rename test: Overwrite an empty directory");
	printf("  %s\n", rename("__file","__directory") ? "Could not rename file!" : "Rename succeeded.");
	printf("  errno = %d (%s)\n", errno, strerror(errno));

	remove("__directory");
	remove("__file");

	return 0;
}
