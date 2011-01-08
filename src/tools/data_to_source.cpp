/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


void
write_string(int fd, const char* data)
{
	int len = strlen(data);
	if (len == 0)
		return;

	ssize_t written = write(fd, data, len);
	if (written < 0) {
		fprintf(stderr, "Error: Failed to write to output file: %s\n",
			strerror(errno));
		exit(1);
	}
}


int
main(int argc, const char* const* argv)
{
	if (argc != 5) {
		fprintf(stderr,
			"Usage: %s <data var name> <size var name> <input> <output>\n",
			argv[0]);
		exit(1);
	}
	const char* dataVarName = argv[1];
	const char* sizeVarName = argv[2];
	const char* inFileName = argv[3];
	const char* outFileName = argv[4];

	// open files
	int inFD = open(inFileName, O_RDONLY);
	if (inFD < 0) {
		fprintf(stderr, "Error: Failed to open input file \"%s\": %s\n",
			inFileName, strerror(errno));
		exit(1);
	}

	int outFD = open(outFileName, O_WRONLY | O_CREAT | O_TRUNC,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (outFD < 0) {
		fprintf(stderr, "Error: Failed to open output file \"%s\": %s\n",
			outFileName, strerror(errno));
		exit(1);
	}

	const int kCharsPerLine = 15;
	const int kBufferSize = 16 * 1024;
	unsigned char buffer[kBufferSize];
	char lineBuffer[128];

	sprintf(lineBuffer, "unsigned char %s[] = {\n", dataVarName);
	write_string(outFD, lineBuffer);

	off_t dataSize = 0;
	off_t offset = 0;
	char* lineBufferEnd = lineBuffer;
	*lineBufferEnd = '\0';
	while (true) {
		// read a buffer
		ssize_t bytesRead = read(inFD, buffer, kBufferSize);
		if (bytesRead < 0) {
			fprintf(stderr, "Error: Failed to read from input file: %s\n",
				strerror(errno));
			exit(1);
		}
		if (bytesRead == 0)
			break;

		dataSize += bytesRead;

		// write lines
		for (int i = 0; i < bytesRead; i++, offset++) {
			if (offset % kCharsPerLine == 0) {
				if (offset > 0) {
					// line is full -- flush it
					strcpy(lineBufferEnd, ",\n");
					write_string(outFD, lineBuffer);
					lineBufferEnd = lineBuffer;
					*lineBufferEnd = '\0';
				}

				sprintf(lineBufferEnd, "\t%u", (unsigned)buffer[i]);
			} else
				sprintf(lineBufferEnd, ", %u", (unsigned)buffer[i]);

			lineBufferEnd += strlen(lineBufferEnd);
		}
	}

	// flush the line buffer
	if (lineBufferEnd != lineBuffer) {
		strcpy(lineBufferEnd, ",\n");
		write_string(outFD, lineBuffer);
	}

	// close the braces and write the size variable
	sprintf(lineBuffer, "};\nlong long %s = %lldLL;\n", sizeVarName,
		dataSize);
	write_string(outFD, lineBuffer);

	close(inFD);
	close(outFD);

	return 0;
}
