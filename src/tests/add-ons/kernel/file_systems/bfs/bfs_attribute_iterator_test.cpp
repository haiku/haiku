/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <Entry.h>
#include <File.h>
#include <fs_attr.h>
#include <TypeConstants.h>


const char* kTempFile = "/tmp/bfs_attribute_iterator_test";

bool gVerbose;


bool
is_marker(const char* name)
{
	return strstr(name, ":marker") != NULL;
}


int32
attribute_index(const char* name)
{
	int32 number;
	if (sscanf(name, "test:%ld", &number) == 1)
		return number;

	return 0;
}


void
add_attributes(BFile& file, int32 start, int32 count)
{
	for (int32 index = start; index < start + count; index++) {
		char name[B_ATTR_NAME_LENGTH];
		snprintf(name, sizeof(name), "test:%ld", index);
		file.WriteAttr(name, B_INT32_TYPE, 0, &index, sizeof(int32));
	}
}


void
remove_attributes(BFile& file, int32 start, int32 count)
{
	for (int32 index = start; index < start + count; index++) {
		char name[B_ATTR_NAME_LENGTH];
		snprintf(name, sizeof(name), "test:%ld", index);
		file.RemoveAttr(name);
	}
}


void
add_marker_attribute(BFile& file, int32 index)
{
	char name[B_ATTR_NAME_LENGTH];
	snprintf(name, sizeof(name), "test:%ld:marker", index);
	file.WriteAttr(name, B_INT32_TYPE, 0, &index, sizeof(int32));
}


void
remove_marker_attribute(BFile& file, int32 index)
{
	char name[B_ATTR_NAME_LENGTH];
	snprintf(name, sizeof(name), "test:%ld:marker", index);
	file.RemoveAttr(name);
}


void
test_remove_attributes(BFile& file, int32 start, int32 count, int32 removeAt,
	int32 removeIndex1, int32 removeIndex2)
{
	int fd = file.Dup();
	if (fd < 0)
		return;

	int32 index = 0;
	bool seen[4096] = {0};

	if (gVerbose)
		printf("test removeAt %ld\n", removeAt);

	DIR* dir = fs_fopen_attr_dir(fd);
	while (struct dirent* entry = fs_read_attr_dir(dir)) {
		if (gVerbose)
			printf("  %ld. %s\n", index, entry->d_name);
		if (index == removeAt) {
			if (removeIndex1 > 0)
				remove_marker_attribute(file, removeIndex1);
			if (removeIndex2 > 0)
				remove_marker_attribute(file, removeIndex2);
		}
		index++;

		if (is_marker(entry->d_name))
			continue;

		int32 attributeIndex = attribute_index(entry->d_name);
		if (attributeIndex > 0) {
			if (seen[attributeIndex]) {
				printf("attribute index %ld already listed!\n", attributeIndex);
				exit(1);
			} else
				seen[attributeIndex] = true;
		}
	}

	fs_close_attr_dir(dir);
	close(fd);

	for (int32 i = start; i < start + count; i++) {
		if (!seen[i]) {
			printf("attribute index %ld not listed, saw only %ld/%ld!\n", i,
				index, count);
			exit(1);
		}
	}
}


void
test_add_attributes(BFile& file, int32 start, int32 count, int32 addAt,
	int32 addIndex1, int32 addIndex2)
{
	int fd = file.Dup();
	if (fd < 0)
		return;

	int32 index = 0;
	bool seen[4096] = {0};

	if (gVerbose)
		printf("test addAt %ld\n", addAt);

	DIR* dir = fs_fopen_attr_dir(fd);
	while (struct dirent* entry = fs_read_attr_dir(dir)) {
		if (gVerbose)
			printf("  %ld. %s\n", index, entry->d_name);
		if (index == addAt) {
			if (addIndex1 > 0)
				add_marker_attribute(file, addIndex1);
			if (addIndex2 > 0)
				add_marker_attribute(file, addIndex2);
		}
		index++;

		if (is_marker(entry->d_name))
			continue;

		int32 attributeIndex = attribute_index(entry->d_name);
		if (attributeIndex > 0) {
			if (seen[attributeIndex]) {
				printf("attribute index %ld already listed!\n", attributeIndex);
				exit(1);
			} else
				seen[attributeIndex] = true;
		}
	}

	fs_close_attr_dir(dir);
	close(fd);

	for (int32 i = start; i < start + count; i++) {
		if (!seen[i]) {
			printf("attribute index %ld not listed, saw only %ld/%ld!\n", i,
				index, count);
			exit(1);
		}
	}
}


int
main(int argc, char** argv)
{
	if (argc > 1 && !strcmp(argv[1], "-v"))
		gVerbose = true;

	BFile file;
	status_t status = file.SetTo(kTempFile, B_CREATE_FILE | B_READ_WRITE);
	if (status != B_OK) {
		fprintf(stderr, "Could not create temporary file: %s\n",
			strerror(status));
		return 1;
	}

	puts("--------- Remove Tests ----------");

	// remove test, many attributes

	puts("Test 1...");

	for (int32 count = 5; count <= 100; count += 5) {
		add_attributes(file, 1, count);
		add_marker_attribute(file, count);
		add_attributes(file, count + 1, count);

		test_remove_attributes(file, 1, count,
			count & 1 ? count - 1 : count + 1, count, -1);

		remove_attributes(file, 1, count * 2);
	}

	// remove test, all positions

	puts("Test 2...");

	for (int32 i = 1; i < 100; i++) {
		add_attributes(file, 1, 50);
		add_marker_attribute(file, 51);
		add_attributes(file, 51, 50);

		test_remove_attributes(file, 1, 100, i, 51, -1);

		remove_attributes(file, 1, 100);
	}

	// remove test, all positions, remove many

	puts("Test 3...");

	for (int32 i = 1; i < 100; i++) {
		add_attributes(file, 1, 33);
		add_marker_attribute(file, 33);
		add_attributes(file, 34, 34);
		add_marker_attribute(file, 67);
		add_attributes(file, 68, 33);

		test_remove_attributes(file, 1, 100, i, 33, 67);

		remove_attributes(file, 1, 100);
	}

	puts("--------- Add Tests ----------");

	// add test, many attributes

	puts("Test 4...");

	for (int32 count = 10; count <= 200; count += 10) {
		add_attributes(file, 1, count);

		int32 half = count / 2;

		test_add_attributes(file, 1, count,
			half & 1 ? half - 1 : half + 1, half - 2, half + 2);

		remove_attributes(file, 1, count);
	}

	// add test, all iterator positions

	puts("Test 5...");

	for (int32 i = 1; i < 100; i++) {
		add_attributes(file, 1, 100);

		test_add_attributes(file, 1, 100, i, 50, -1);

		remove_attributes(file, 1, 100);
	}

	// add test, all attribute positions

	puts("Test 6...");

	for (int32 i = 1; i < 100; i++) {
		add_attributes(file, 1, 100);

		test_add_attributes(file, 1, 100, 50, i, -1);

		remove_attributes(file, 1, 100);
	}

	// add test, many attributes

	puts("Test 7...");

	for (int32 i = 1; i < 100; i++) {
		add_attributes(file, 1, 100);

		test_add_attributes(file, 1, 100, i, 33, 67);

		remove_attributes(file, 1, 100);
	}

	// add test, many attributes

	puts("Test 8...");

	for (int32 i = 1; i < 100; i++) {
		add_attributes(file, 1, 100);

		test_add_attributes(file, 1, 100, 50, i - 1, i + 1);

		remove_attributes(file, 1, 100);
	}

	BEntry entry;
	if (entry.SetTo(kTempFile) == B_OK)
		entry.Remove();

	return 0;
}
