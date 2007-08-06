#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

size_t confstr(int name, char *buf, size_t len);
char string[512];

int main()
{
	int i;
	size_t size;
	for (i=0; i<INT_MAX; i++) {
		size = confstr(i, NULL, (size_t) 0);
		if (errno != B_BAD_VALUE)
			printf("%ld confstr %ld %s\n", i, size, strerror(errno));
		if (size != 0) {
			size = confstr(i, string, sizeof(string));
			printf("%ld value %s\n", i, string);
		}
	}
}

