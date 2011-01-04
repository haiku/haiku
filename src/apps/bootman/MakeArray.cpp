#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>


int
main(int argc, char* argv[])
{
	const char* application = argv[0];
	if (argc != 3) {
		fprintf(stderr, "Usage:\n%s <variable name> <file>", application);
		return 1;
	}
	const char* variableName = argv[1];
	const char* fileName = argv[2];

	int fd = open(fileName, 0);
	if (fd < 0) {
		fprintf(stderr, "%s: Error opening file '%s'!\n", application,
			fileName);
		return 1;
	}

	const int kSize = 1024;
	unsigned char buffer[kSize];
	int size = read(fd, buffer, kSize);
	const int COLUMNS = 16;
	int column = COLUMNS - 1;
	bool first = true;
	printf("// THIS FILE WAS GENERATED WITH\n");
	printf("// %s %s %s\n\n", application, variableName, fileName);
	printf("static const uint8 %s[] = {", variableName);
	while (size > 0) {
		for (int i = 0; i < size; i ++) {
			if (first)
				first = false;
			else
				printf(", ");
			column ++;
			if (column == COLUMNS) {
				printf("\n\t");
				column = 0;
			}
			printf("0x%2.2x", (int)buffer[i]);
		}
		size = read(fd, buffer, kSize);
	}
	printf("\n};\n");

	close(fd);
}
