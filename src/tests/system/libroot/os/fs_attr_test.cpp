/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <fs_attr.h>
#include <TypeConstants.h>


const char* kTestFileName = "/tmp/fs_attr_test";


void
test_read(int fd, const char* attribute, type_code type, const char* data,
	size_t length)
{
	attr_info info;
	if (fs_stat_attr(fd, attribute, &info) != 0) {
		fprintf(stderr, "Could not stat attribute \"%s\": %s\n", attribute,
			strerror(errno));
		exit(1);
	}

	if (info.size != length) {
		fprintf(stderr, "Length does not match for \"%s\": should be %ld, is "
			"%lld\n", data, length, info.size);
		exit(1);
	}

	if (info.type != type) {
		fprintf(stderr, "Type does not match B_RAW_TYPE!\n");
		exit(1);
	}

	char buffer[4096];
	ssize_t bytesRead = fs_read_attr(fd, attribute, B_RAW_TYPE, 0, buffer,
		length);
	if (bytesRead != (ssize_t)length) {
		fprintf(stderr, "Bytes read does not match: should be %ld, is %ld\n",
			length, bytesRead);
		exit(1);
	}

	if (memcmp(data, buffer, length)) {
		fprintf(stderr, "Bytes do not match: should be \"%s\", is \"%s\"\n",
			data, buffer);
		exit(1);
	}
}


int
main(int argc, char** argv)
{
	int fd = open(kTestFileName, O_CREAT | O_TRUNC | O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Creating test file \"%s\" failed: %s\n", kTestFileName,
			strerror(errno));
		return 1;
	}

	// Test the old BeOS API

	fs_write_attr(fd, "TEST", B_STRING_TYPE, 0, "Hello BeOS", 11);
	test_read(fd, "TEST", B_STRING_TYPE, "Hello BeOS", 11);

	// TODO: this shows a bug in BFS; the index is not updated correctly
	fs_write_attr(fd, "TEST", B_STRING_TYPE, 6, "Haiku", 6);
	test_read(fd, "TEST", B_STRING_TYPE, "Hello Haiku", 12);

	fs_write_attr(fd, "TESTraw", B_RAW_TYPE, 16, "Haiku", 6);
	test_read(fd, "TESTraw", B_RAW_TYPE, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0Haiku",
		22);

	fs_write_attr(fd, "TESTraw", B_RAW_TYPE, 0, "Haiku", 6);
	test_read(fd, "TESTraw", B_RAW_TYPE, "Haiku", 6);

	char buffer[4096];
	memset(buffer, 0, sizeof(buffer));
	strcpy(buffer, "Hello");
	fs_write_attr(fd, "TESTswitch", B_RAW_TYPE, 0, buffer,
		strlen(buffer) + 1);
	test_read(fd, "TESTswitch", B_RAW_TYPE, buffer, strlen(buffer) + 1);

	strcpy(buffer + 4000, "Haiku");
	fs_write_attr(fd, "TESTswitch", B_RAW_TYPE, 0, buffer, 4006);
	test_read(fd, "TESTswitch", B_RAW_TYPE, buffer, 4006);

	return 0;
}

