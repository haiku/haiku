#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syscalls.h>
#include <ktypes.h>
#include <resource.h>
#include <Errors.h>
#include <OS.h>
#include <stdlib.h>

#define TEST_FILE "/boot/etc/fortunes"
#define RD_LINES 5
#define NEW_FORTUNE "The path has been shown - we are the ones who must walk it"


static void read_file(FILE *f)
{
	int lines = 0;
	char *buffer, *tmp;
	size_t bytes = 0;
	
	while ((buffer = fgetln(f, &bytes)) != NULL) {
		if (*buffer == '#')
			continue;
		tmp = (char*) malloc(bytes + 1);
		if (!tmp)
			break;
		memcpy(tmp, buffer, bytes);
		if (tmp[bytes - 1] != '\n')
			tmp[bytes - 1] = '\n';
		fprintf(stdout, "%d: %s", ++lines, tmp);
	}
}

int main(int argc, char **argv)
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
