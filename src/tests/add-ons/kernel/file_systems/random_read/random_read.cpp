/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

/*!	Tests if the read functions of a file system work correctly by reading
	arbitrary bytes at arbitrary positions of a file previously created.

	The idea is to be able to calculate the contents on each position
	of the file. It currently only counts numbers, beginning from zero,
	which is probably not a really good test.
	But if there is a bug, it would be very likely to show up after some
	thousand (or even million) runs.

	Works only on little-endian processors, such as x86 (or else the partial
	read numbers wouldn't be correctly compared).

	Use the --help option to see how it's used.
*/

#include <File.h>
#include <StorageDefs.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define FILE_NAME		"RANDOM_READ_TEST_FILE"
#define FILE_SIZE		(1024 * 1024)
#define BUFFER_SIZE		(64 * 1024)
#define NUMBER_OF_LOOPS	1000
#define MAX_FAULTS		10

// currently only works with 4 byte values!
typedef uint32 test_t;


void
createFile(const char *name, size_t size)
{
	BFile file;
	status_t status = file.SetTo(name,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status < B_OK) {
		fprintf(stderr, "Could not create test file: %s!\n", strerror(status));
		return;
	}

	test_t max = size / sizeof(test_t);
	for (uint32 i = 0; i < max; i++) {
		if (file.Write(&i, sizeof(test_t)) != sizeof(test_t)) {
			fprintf(stderr,"Could not create the whole test file!\n");
			break;
		}
	}
}


void
readTest(const char *name, int32 loops)
{
	BFile file;
	status_t status = file.SetTo(name, B_READ_ONLY);
	if (status < B_OK) {
		fprintf(stderr, "Could not open test file! Run \"randomread create "
			"[size]\" first.\n"
			"This will create a file named \"%s\" in the current directory.\n",
			name);
		return;
	}
	
	off_t size;
	if ((status = file.GetSize(&size)) < B_OK) {
		fprintf(stderr, "Could not get file size: %s!\n", strerror(status));
		return;
	}

	char *buffer = (char *)malloc(BUFFER_SIZE);
	if (buffer == NULL) {
		fprintf(stderr, "no memory to create read buffer.\n");
		return;
	}
	srand(time(NULL));
	int32 faults = 0;

	for (int32 i = 0; i < loops; i++) {
		off_t pos = rand() % size;
		// we are lazy tester, minimum read size is 4 bytes
		int32 bytes = (rand() % BUFFER_SIZE) + 4;
		off_t max = size - pos;
		if (max > bytes)
			max = bytes;
		else
			bytes = max;

		ssize_t bytesRead = file.ReadAt(pos, buffer, bytes);
		if (bytesRead < B_OK) {
			printf("  Could not read %ld bytes at offset %Ld: %s\n",
				bytes, pos, strerror(bytesRead));
		} else if (bytesRead != max) {
			printf("  Could only read %ld bytes instead of %ld at offset %Ld\n",
				bytesRead, bytes, pos);
		}

		// test contents

		off_t bufferPos = pos;		
		test_t num = bufferPos / sizeof(test_t);

		// check leading partial number
		int32 partial = bufferPos % sizeof(test_t);
		if (partial) {
			test_t read = *(test_t *)(buffer - partial);
			bool correct;
			switch (partial) {
				// byteorder little-endian
				case 1:
					correct = (num & 0xffffff00) == (read & 0xffffff00);
					break;
				case 2:
					correct = (num & 0xffff0000) == (read & 0xffff0000);
					break;
				case 3:
					correct = (num & 0xff000000) == (read & 0xff000000);
					break;
			}
			if (!correct) {
				printf("[%Ld,%ld] Bytes at %Ld don't match (partial begin = "
					"%ld, should be %08lx, is %08lx)!\n", pos, bytes,
					bufferPos, partial, num, read);
				faults++;
			}
			bufferPos += sizeof(test_t) - partial;
		}
		num++;
		test_t *numBuffer = (test_t *)(buffer + sizeof(test_t) - partial);

		// test full numbers
		for (; bufferPos < bytesRead - sizeof(test_t);
				bufferPos += sizeof(test_t)) {
			if (faults > MAX_FAULTS) {
				printf("maximum number of faults reached, bail out.\n");
				return;
			}
			if (num != *numBuffer) {
				printf("[%Ld,%ld] Bytes at %Ld don't match (should be %08lx, "
					"is %08lx)!\n", pos, bytes, bufferPos, num, *numBuffer);
				faults++;
			}
			num++;
			numBuffer++;
		}

		// test last partial number
		partial = bytesRead - bufferPos;
		if (partial > 0) {
			uint32 read = *numBuffer;
			bool correct;
			switch (partial) {
				// byteorder little-endian
				case 1:
					correct = (num & 0x00ffffff) == (read & 0x00ffffff);
					break;
				case 2:
					correct = (num & 0x0000ffff) == (read & 0x0000ffff);
					break;
				case 3:
					correct = (num & 0x000000ff) == (read & 0x000000ff);
					break;
			}
			if (!correct) {
				printf("[%Ld,%ld] Bytes at %Ld don't match (partial end = "
					"%ld, should be %08lx, is %08lx)!\n", pos, bytes,
					bufferPos, partial, num, read);
				faults++;
			}
		}
	}
}


int
main(int argc, char **argv)
{
	size_t size = FILE_SIZE;
	int32 loops = NUMBER_OF_LOOPS;
	bool create = false;

	if (argv[1]) {
		if (!strcmp(argv[1], "create")) {
			create = true;
			if (argv[2])
				size = atol(argv[2]);
		}
		else if (isdigit(*argv[1]))
			loops = atol(argv[1]);
		else {
			// get a nice filename of the program
			char *filename = strrchr(argv[0], '/')
				? strrchr(argv[0] , '/') + 1 : argv[0];

			printf("You can either create a test file or perform the test.\n"
				"  Create:\t%s create [filesize]\n"
				"  Test:  \t%s [loops]\n\n"
				"Default size = %d, loops = %d\n",
				filename, filename, FILE_SIZE, NUMBER_OF_LOOPS);

			return 0;
		}
	}

	if (size == 0) {
		fprintf(stderr, "%s: given file size too small, set to 1 MB.\n",
			argv[0]);
		size = FILE_SIZE;
	}

	if (create)
		createFile(FILE_NAME, size);
	else
		readTest(FILE_NAME, loops);

	return 0;
}
