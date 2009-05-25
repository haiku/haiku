/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fs_attr.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Node.h>


#define ATTRIBUTE_FILE_MAGIC	'attr'
#define ATTRIBUTE_DIR_NAME		"_HAIKU"
#define COPY_BUFFER_SIZE		128 * 1024


struct attribute_file {
	uint32		magic; // 'attr'
	uint32		entry_count;
	uint8		entries[1];
} _PACKED;


struct attribute_entry {
	type_code	type;
	uint32		size;
	uint8		name_length; // including 0 byte
	char		name[1]; // 0 terminated, followed by data
} _PACKED;


void
recurse_directory(BDirectory &directory, uint8 *copyBuffer)
{
	BNode node;
	entry_ref ref;
	BDirectory attributeDir;
	bool attributeDirCreated = false;
	char nameBuffer[B_FILE_NAME_LENGTH];
	directory.Rewind();

	while (directory.GetNextRef(&ref) == B_OK) {
		if (strcmp(ref.name, ATTRIBUTE_DIR_NAME) == 0)
			continue;

		if (node.SetTo(&ref) != B_OK) {
			printf("failed to set node to ref \"%s\"\n", ref.name);
			continue;
		}

		node.RewindAttrs();
		BFile attributeFile;
		uint32 attributeCount = 0;
		while (node.GetNextAttrName(nameBuffer) == B_OK) {
			attr_info info;
			if (node.GetAttrInfo(nameBuffer, &info) != B_OK) {
				printf("failed to get attr info of \"%s\" on file \"%s\"\n",
					nameBuffer, ref.name);
				continue;
			}

			if (attributeCount == 0) {
				if (!attributeDirCreated) {
					directory.CreateDirectory(ATTRIBUTE_DIR_NAME, NULL);
					if (!directory.Contains(ATTRIBUTE_DIR_NAME,
						B_DIRECTORY_NODE)) {
						printf("attribute store directory not available\n");
						return;
					}

					attributeDir.SetTo(&directory, ATTRIBUTE_DIR_NAME);
					attributeDirCreated = true;
				}

				attributeDir.CreateFile(ref.name, NULL);
				if (attributeFile.SetTo(&attributeDir, ref.name,
					B_WRITE_ONLY | B_ERASE_FILE) != B_OK) {
					printf("cannot open attribute file for writing\n");
					break;
				}

				attributeFile.Seek(sizeof(attribute_file) - 1, SEEK_SET);
			}

			attribute_entry entry;
			entry.type = info.type;
			entry.size = info.size;
			entry.name_length = strlen(nameBuffer) + 1;
			attributeFile.Write(&entry, sizeof(attribute_entry) - 1);
			attributeFile.Write(nameBuffer, entry.name_length);

			off_t offset = 0;
			while (info.size > 0) {
				size_t copySize = min_c(info.size, COPY_BUFFER_SIZE);
				if (node.ReadAttr(nameBuffer, info.type, offset, copyBuffer,
					copySize) < B_OK) {
					printf("error reading attribute \"%s\" of file \"%s\"\n",
						nameBuffer, ref.name);
					return;
				}

				attributeFile.Write(copyBuffer, copySize);
				info.size -= COPY_BUFFER_SIZE;
				offset += COPY_BUFFER_SIZE;
			}

			attributeCount++;
		}

		if (attributeCount > 0) {
			attribute_file file;
			file.magic = ATTRIBUTE_FILE_MAGIC;
			file.entry_count = attributeCount;
			attributeFile.WriteAt(0, &file, sizeof(attribute_file) - 1);
		}

		if (node.IsDirectory()) {
			BDirectory subDirectory(&ref);
			recurse_directory(subDirectory, copyBuffer);
		}
	}
}


int
main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("usage: %s <root directory>\n", argv[0]);
		return 1;
	}

	uint8 *copyBuffer = (uint8 *)malloc(COPY_BUFFER_SIZE);
	if (copyBuffer == NULL) {
		printf("cannot allocate copy buffer\n");
		return 2;
	}

	BDirectory root(argv[1]);
	recurse_directory(root, copyBuffer);

	free(copyBuffer);
	return 0;
}
