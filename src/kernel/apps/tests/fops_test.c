#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syscalls.h>
#include <ktypes.h>
#include <sys/resource.h>
#include <Errors.h>
#include <errno.h>
#include <OS.h>
#include <stdlib.h>

#define TEST_FILE "/boot/etc/fortunes"
#define RD_LINES 5
#define NEW_FORTUNE "The path has been shown - we are the ones who must walk it"


static void
read_file(FILE *f)
{
	char buffer[2048];
	int lines = 0;

	while ((fgets(buffer, sizeof(buffer), f)) != NULL) {
		size_t bytes = strlen(buffer);

		if (buffer[0] == '#')
			continue;

		if (buffer[bytes - 1] != '\n')
			buffer[bytes - 1] = '\n';

		fprintf(stdout, "%d: %s", ++lines, buffer);
	}
}


int
main(int argc, char **argv)
{
	FILE *f;
	
	f = fopen(TEST_FILE, "rw");
	if (!f) {
		printf("Failed to open %s :( [%s]\n", TEST_FILE,
                       strerror(errno));
		return -1;
	}

	read_file(f);
	fputs("#@#\n", f);
	fputs(NEW_FORTUNE, f);

	fprintf(stdout, "File added to.\n");
			
	fclose(f);
	printf("File opened and closed\n");

	f = fopen(TEST_FILE, "r");
	if (!f) {
		printf("Failed to open %s :( [%s]\n", TEST_FILE,
                       strerror(errno));
		return -1;
	}
	
	read_file(f);
	fclose(f);	
	printf("File opened and closed\n");
	
	return 0;
}
